#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/platform/arch_detection.hpp"
#include "mclo/platform/compiler_detection.hpp"

#include <atomic>
#include <bit>
#include <cstdint>
#include <type_traits>

/// @brief Defined to @c 1 when the platform provides a native 128-bit atomic compare exchange (x86-64 and ARM64),
/// and @c 0 otherwise.
#if defined( MCLO_ARCH_X86 ) || defined( MCLO_ARCH_ARM )
#define MCLO_HAS_ATOMIC128 1
#else
#define MCLO_HAS_ATOMIC128 0
#endif

// On x86-64 the 128-bit atomic lowers to the cmpxchg16b instruction which the build does not enable globally, so
// request it per-function to keep the operation inline rather than emitting a libatomic call. ARM64 lowers to
// ldaxp/stlxp (or casp with LSE) without any extra target feature.
#if defined( MCLO_COMPILER_GCC_COMPATIBLE ) && defined( MCLO_ARCH_X86 )
#define MCLO_ATOMIC128_TARGET [[gnu::target( "cx16" )]]
#else
#define MCLO_ATOMIC128_TARGET
#endif

#if MCLO_HAS_ATOMIC128 && defined( MCLO_COMPILER_MSVC )
#include <intrin.h>
#endif

namespace mclo
{
	namespace detail
	{
		struct alignas( 16 ) storage128
		{
			std::uint64_t m_low;
			std::uint64_t m_high;
		};
		static_assert( sizeof( storage128 ) == 16 );

#if defined( MCLO_COMPILER_GCC_COMPATIBLE )
		[[nodiscard]] constexpr int to_builtin_memory_order( const std::memory_order order ) noexcept
		{
			switch ( order )
			{
				case std::memory_order_relaxed:
					return __ATOMIC_RELAXED;
				case std::memory_order_consume:
					return __ATOMIC_CONSUME;
				case std::memory_order_acquire:
					return __ATOMIC_ACQUIRE;
				case std::memory_order_release:
					return __ATOMIC_RELEASE;
				case std::memory_order_acq_rel:
					return __ATOMIC_ACQ_REL;
				case std::memory_order_seq_cst:
					return __ATOMIC_SEQ_CST;
				default:
					UNREACHABLE( "Invalid memory order" );
			}
		}
#endif

#if defined( MCLO_COMPILER_MSVC )
		[[nodiscard]] constexpr std::memory_order strongest_memory_order( const std::memory_order success,
																		  const std::memory_order failure ) noexcept
		{
			constexpr auto is_acquire = []( const std::memory_order order ) {
				return order == std::memory_order_consume || order == std::memory_order_acquire ||
					   order == std::memory_order_acq_rel;
			};
			constexpr auto is_release = []( const std::memory_order order ) {
				return order == std::memory_order_release || order == std::memory_order_acq_rel;
			};

			if ( success == std::memory_order_seq_cst || failure == std::memory_order_seq_cst )
			{
				return std::memory_order_seq_cst;
			}
			const bool acquire = is_acquire( success ) || is_acquire( failure );
			const bool release = is_release( success );
			if ( acquire && release )
			{
				return std::memory_order_acq_rel;
			}
			if ( release )
			{
				return std::memory_order_release;
			}
			if ( acquire )
			{
				return std::memory_order_acquire;
			}
			return std::memory_order_relaxed;
		}

		// Returns non-zero on success. On failure @p expected is updated with the current value.
		[[nodiscard]] inline unsigned char atomic128_compare_exchange( storage128* const destination,
																	   const storage128 desired,
																	   storage128* const expected,
																	   const std::memory_order order ) noexcept
		{
			__int64* const target = reinterpret_cast<__int64*>( destination );
			__int64* const comparand = reinterpret_cast<__int64*>( expected );
			const __int64 high = static_cast<__int64>( desired.m_high );
			const __int64 low = static_cast<__int64>( desired.m_low );
#if defined( MCLO_ARCH_ARM )
			switch ( order )
			{
				case std::memory_order_relaxed:
					return _InterlockedCompareExchange128_nf( target, high, low, comparand );
				case std::memory_order_consume:
				case std::memory_order_acquire:
					return _InterlockedCompareExchange128_acq( target, high, low, comparand );
				case std::memory_order_release:
					return _InterlockedCompareExchange128_rel( target, high, low, comparand );
				case std::memory_order_acq_rel:
				case std::memory_order_seq_cst:
					return _InterlockedCompareExchange128( target, high, low, comparand );
				default:
					UNREACHABLE( "Invalid memory order for atomic compare exchange" );
			}
#else
			// x86-64 only provides the full barrier form, which is always at least as strong as requested.
			( void )order;
			return _InterlockedCompareExchange128( target, high, low, comparand );
#endif
		}
#endif
	}

	/// @brief A lock-free atomic over a 16 byte trivially copyable type using a 128-bit compare exchange.
	/// @details On x86-64 this lowers to @c cmpxchg16b (@c _InterlockedCompareExchange128 on MSVC, the @c __atomic
	/// builtins on GCC and Clang); on ARM64 it lowers to the 128-bit load/store exclusive pair. The full 128 bit width
	/// allows pairing a pointer with a 64 bit counter without risking ABA on the counter, which is the basis for the
	/// lock-free @c atomic_shared_ptr.
	/// @tparam T A trivially copyable type that is exactly 16 bytes wide.
	template <typename T>
	class atomic128
	{
		static_assert( MCLO_HAS_ATOMIC128,
					   "atomic128 requires a 128-bit compare exchange, only x86-64 and ARM64 are supported" );
		static_assert( std::is_trivially_copyable_v<T>, "atomic128 requires a trivially copyable type" );
		static_assert( sizeof( T ) == 16, "atomic128 requires a 16 byte type" );
		static_assert( alignof( T ) <= 16, "atomic128 requires an alignment of at most 16 bytes" );

	public:
		/// @brief The trivially copyable value type stored in the atomic.
		using value_type = T;

		/// @brief Always @c true, an @c atomic128 is unconditionally lock-free on supported platforms.
		static constexpr bool is_always_lock_free = true;

		/// @brief Constructs the atomic holding a value-initialised (all bytes zero) value.
		atomic128() noexcept = default;

		/// @brief Constructs the atomic holding @p desired.
		/// @param desired The initial value.
		constexpr atomic128( const T desired ) noexcept
			: m_storage( std::bit_cast<detail::storage128>( desired ) )
		{
		}

		atomic128( const atomic128& ) = delete;
		atomic128& operator=( const atomic128& ) = delete;

		/// @brief Checks whether operations on this atomic are lock-free.
		/// @return Always @c true.
		[[nodiscard]] bool is_lock_free() const noexcept
		{
			return true;
		}

		/// @brief Atomically loads the current value.
		/// @param order The memory order, must not be @c memory_order_release or @c memory_order_acq_rel.
		/// @return The value currently held.
		[[nodiscard]] T load( const std::memory_order order = std::memory_order_seq_cst ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( order != std::memory_order_release && order != std::memory_order_acq_rel,
						  "Invalid memory order for atomic load" );
			return std::bit_cast<T>( load_storage( order ) );
		}

		/// @brief Atomically replaces the current value with @p desired.
		/// @param desired The value to store.
		/// @param order The memory order, must not be @c memory_order_consume, @c memory_order_acquire or
		/// @c memory_order_acq_rel.
		void store( const T desired, const std::memory_order order = std::memory_order_seq_cst ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( order != std::memory_order_consume && order != std::memory_order_acquire &&
							  order != std::memory_order_acq_rel,
						  "Invalid memory order for atomic store" );
			store_storage( std::bit_cast<detail::storage128>( desired ), order );
		}

		/// @brief Atomically replaces the current value with @p desired and returns the previous value.
		/// @param desired The value to store.
		/// @param order The memory order for the read-modify-write operation.
		/// @return The value held before the exchange.
		T exchange( const T desired, const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			return std::bit_cast<T>( exchange_storage( std::bit_cast<detail::storage128>( desired ), order ) );
		}

		/// @brief Atomically compares the current value with @p expected and, if they are equal, replaces it with
		/// @p desired.
		/// @param expected The value expected to be found; on failure it is updated with the actual current value.
		/// @param desired The value to store if the comparison succeeds.
		/// @param success The memory order to use if the comparison succeeds.
		/// @param failure The memory order to use if the comparison fails, must not be stronger than @p success.
		/// @return @c true if the value was exchanged, @c false otherwise.
		bool compare_exchange_strong( T& expected,
									  const T desired,
									  const std::memory_order success,
									  const std::memory_order failure ) noexcept
		{
			detail::storage128 expected_storage = std::bit_cast<detail::storage128>( expected );
			const bool succeeded = compare_exchange_storage(
				expected_storage, std::bit_cast<detail::storage128>( desired ), success, failure );
			if ( !succeeded )
			{
				expected = std::bit_cast<T>( expected_storage );
			}
			return succeeded;
		}

		/// @brief Atomically compares the current value with @p expected and, if they are equal, replaces it with
		/// @p desired.
		/// @param expected The value expected to be found; on failure it is updated with the actual current value.
		/// @param desired The value to store if the comparison succeeds.
		/// @param order The memory order to use for both the success and failure cases.
		/// @return @c true if the value was exchanged, @c false otherwise.
		bool compare_exchange_strong( T& expected,
									  const T desired,
									  const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			return compare_exchange_strong( expected, desired, order, fail_order( order ) );
		}

		/// @brief Equivalent to @c compare_exchange_strong; a 128-bit compare exchange never fails spuriously.
		/// @param expected The value expected to be found; on failure it is updated with the actual current value.
		/// @param desired The value to store if the comparison succeeds.
		/// @param success The memory order to use if the comparison succeeds.
		/// @param failure The memory order to use if the comparison fails, must not be stronger than @p success.
		/// @return @c true if the value was exchanged, @c false otherwise.
		bool compare_exchange_weak( T& expected,
									const T desired,
									const std::memory_order success,
									const std::memory_order failure ) noexcept
		{
			return compare_exchange_strong( expected, desired, success, failure );
		}

		/// @brief Equivalent to @c compare_exchange_strong; a 128-bit compare exchange never fails spuriously.
		/// @param expected The value expected to be found; on failure it is updated with the actual current value.
		/// @param desired The value to store if the comparison succeeds.
		/// @param order The memory order to use for both the success and failure cases.
		/// @return @c true if the value was exchanged, @c false otherwise.
		bool compare_exchange_weak( T& expected,
									const T desired,
									const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			return compare_exchange_strong( expected, desired, order, fail_order( order ) );
		}

	private:
		[[nodiscard]] static constexpr std::memory_order fail_order( const std::memory_order order ) noexcept
		{
			switch ( order )
			{
				case std::memory_order_acq_rel:
					return std::memory_order_acquire;
				case std::memory_order_release:
					return std::memory_order_relaxed;
				default:
					return order;
			}
		}

		[[nodiscard]] detail::storage128* platform_storage() const noexcept
		{
			return const_cast<detail::storage128*>( &m_storage );
		}

		MCLO_ATOMIC128_TARGET [[nodiscard]] detail::storage128 load_storage(
			const std::memory_order order ) const noexcept
		{
#ifdef MCLO_COMPILER_MSVC
			detail::storage128 expected{ 0, 0 };
			// A compare against zero never modifies the storage, it just reads the current value into expected.
			( void )detail::atomic128_compare_exchange(
				platform_storage(), detail::storage128{ 0, 0 }, &expected, order );
			return expected;
#else
			return __atomic_load_n( platform_storage(), detail::to_builtin_memory_order( order ) );
#endif
		}

		MCLO_ATOMIC128_TARGET void store_storage( const detail::storage128 desired,
												  const std::memory_order order ) noexcept
		{
#ifdef MCLO_COMPILER_MSVC
			detail::storage128 expected = load_storage( std::memory_order_relaxed );
			while ( !compare_exchange_storage( expected, desired, order, std::memory_order_relaxed ) )
			{
			}
#else
			__atomic_store_n( platform_storage(), desired, detail::to_builtin_memory_order( order ) );
#endif
		}

		MCLO_ATOMIC128_TARGET [[nodiscard]] detail::storage128 exchange_storage(
			const detail::storage128 desired, const std::memory_order order ) noexcept
		{
#ifdef MCLO_COMPILER_MSVC
			detail::storage128 expected = load_storage( std::memory_order_relaxed );
			while ( !compare_exchange_storage( expected, desired, order, std::memory_order_relaxed ) )
			{
			}
			return expected;
#else
			return __atomic_exchange_n( platform_storage(), desired, detail::to_builtin_memory_order( order ) );
#endif
		}

		MCLO_ATOMIC128_TARGET bool compare_exchange_storage( detail::storage128& expected,
															 const detail::storage128 desired,
															 const std::memory_order success,
															 const std::memory_order failure ) noexcept
		{
#ifdef MCLO_COMPILER_MSVC
			return detail::atomic128_compare_exchange(
					   platform_storage(), desired, &expected, detail::strongest_memory_order( success, failure ) ) !=
				   0;
#else
			return __atomic_compare_exchange_n( platform_storage(),
												&expected,
												desired,
												false,
												detail::to_builtin_memory_order( success ),
												detail::to_builtin_memory_order( failure ) );
#endif
		}

		detail::storage128 m_storage{ 0, 0 };
	};
}
