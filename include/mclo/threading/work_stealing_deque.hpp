#pragma once

#include <atomic>
#include <new>
#include <optional>
#include <type_traits>

#include <mclo/numeric/align.hpp>
#include <mclo/preprocessor/platform.hpp>

namespace mclo
{
	MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( 4324 ) // structure was padded due to alignment specifier
	/// @brief A lock-free work-stealing deque for managing tasks between multiple threads.
	/// @tparam T The type of elements stored in the deque. Must be trivially copyable and destructible.
	/// @details Based on the Chase-Lev work-stealing deque algorithm including the weak memory models fixes from
	/// Nhat Minh Le, Antoniu Pop, Albert Cohen, and Francesco Zappa Nardelli.
	template <typename T>
	class work_stealing_deque
	{
	public:
		static_assert( std::is_trivially_destructible_v<T>, "T must be trivially destructible" );
		static_assert( std::is_trivially_copyable_v<T>, "T must be trivially copyable" );

		/// @brief Construct a new work stealing deque object.
		/// @param capacity The initial capacity of the deque. Will be rounded up to the next power of two.
		explicit work_stealing_deque( const std::size_t capacity = 1024 )
			: m_storage( allocate_storage( capacity ) )
		{
			m_old_storages.reserve( 16 );
		}

		~work_stealing_deque()
		{
			deallocate_storage( m_storage.load( std::memory_order_relaxed ) );
		}

		/// @brief Get the current capacity of the deque.
		/// @warning This capacity is approximate as other threads may be modifying the deque concurrently.
		/// @return The approximate capacity of the deque.
		[[nodiscard]] std::size_t capacity() const
		{
			return m_storage.load( std::memory_order_acquire )->capacity();
		}

		/// @brief Get the current size of the deque.
		/// @warning This size is approximate as other threads may be modifying the deque concurrently.
		/// @return The approximate size of the deque.
		[[nodiscard]] std::size_t size() const
		{
			const std::int64_t bottom = m_bottom.load( std::memory_order_relaxed );
			const std::int64_t top = m_top.load( std::memory_order_relaxed );
			return ( bottom >= top ) ? static_cast<std::size_t>( bottom - top ) : 0;
		}

		/// @brief Check if the deque is empty.
		/// @warning This check is approximate as other threads may be modifying the deque concurrently.
		/// @return If the deque is potentially empty.
		[[nodiscard]] bool empty() const
		{
			const std::int64_t bottom = m_bottom.load( std::memory_order_relaxed );
			const std::int64_t top = m_top.load( std::memory_order_relaxed );
			return bottom <= top;
		}

		/// @brief Push a value onto the deque.
		/// @param value The value to push.
		/// @warning Can only be called by the thread owning the deque.
		/// @details Will resize the internal storage if full. Old storage is only deleted on destruction of the deque.
		void push( T value )
		{
			const std::int64_t bottom = m_bottom.load( std::memory_order_relaxed );
			const std::int64_t top = m_top.load( std::memory_order_acquire );
			ring_storage* storage = m_storage.load( std::memory_order_relaxed );

			// Full, need to resize
			if ( storage->capacity() - 1 < static_cast<std::size_t>( bottom - top ) )
			{
				m_old_storages.emplace_back( std::exchange( storage, storage->resize( bottom, top ) ) );
				m_storage.store( storage, std::memory_order_relaxed );
			}

			// Store value and publish new bottom
			storage->store( bottom, value );
			m_bottom.store( bottom + 1, std::memory_order_release );
		}

		/// @brief Pop a value from the deque.
		/// @warning Can only be called by the thread owning the deque.
		/// @return The popped value, or std::nullopt if the deque is empty.
		std::optional<T> pop()
		{
			const std::int64_t bottom = m_bottom.load( std::memory_order_relaxed ) - 1;
			ring_storage* storage = m_storage.load( std::memory_order_relaxed );

			// Claims bottom so cannot be stolen and synchronizes with stealing threads
			m_bottom.store( bottom, std::memory_order_relaxed );
			std::atomic_thread_fence( std::memory_order_seq_cst );

			std::int64_t top = m_top.load( std::memory_order_relaxed );

			std::optional<T> result;

			// Not empty
			if ( top <= bottom )
			{
				// Load the potential value
				result.emplace( storage->load( bottom ) );

				// If last item we can race with steal
				if ( top == bottom )
				{
					if ( !m_top.compare_exchange_strong(
							 top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) )
					{
						// We lost the race, clear out the result
						result.reset();
					}

					m_bottom.store( bottom + 1, std::memory_order_relaxed );
				}
			}
			else
			{
				// Was empty
				m_bottom.store( bottom + 1, std::memory_order_relaxed );
			}

			return result;
		}

		/// @brief Steal a value from the deque.
		/// @warning Can be called by any thread.
		/// @return The stolen value, or std::nullopt if the deque is empty.
		std::optional<T> steal()
		{
			std::int64_t top = m_top.load( std::memory_order_acquire );

			std::atomic_thread_fence( std::memory_order_seq_cst );

			const std::int64_t bottom = m_bottom.load( std::memory_order_acquire );

			std::optional<T> result;

			if ( top < bottom )
			{
				ring_storage* storage = m_storage.load( std::memory_order_consume );
				result.emplace( storage->load( top ) );

				// Can race with pop
				if ( !m_top.compare_exchange_strong(
						 top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) )
				{
					// We lost the race, clear out the result
					result.reset();
				}
			}

			return result;
		}

	private:
		/// @brief Internal ring storage for the deque.
		/// @details Allocated as one allocation including its mask followed by the data. Data is access by offsetting
		/// the this pointer.
		/// @warning Must be allocated with allocate_storage and deallocated with deallocate_storage.
		class ring_storage
		{
		public:
			explicit ring_storage( const std::size_t pow2_capacity )
				: m_mask( pow2_capacity - 1 )
			{
			}

			void store( const std::int64_t index, T value ) noexcept
			{
				data()[ index & m_mask ] = value;
			}

			T load( const std::int64_t index ) const noexcept
			{
				return data()[ index & m_mask ];
			}

			std::size_t capacity() const noexcept
			{
				return m_mask + 1;
			}

			[[nodiscard]] ring_storage* resize( const std::int64_t bottom, const std::int64_t top ) const
			{
				ring_storage* new_storage = allocate_storage( capacity() * 2 );
				for ( std::int64_t i = top; i != bottom; ++i )
				{
					new_storage->store( i, load( i ) );
				}
				return new_storage;
			}

		private:
			T* data() noexcept
			{
				return std::launder(
					reinterpret_cast<T*>( reinterpret_cast<std::byte*>( this ) + ring_storage_data_offset ) );
			}
			const T* data() const noexcept
			{
				return std::launder( reinterpret_cast<const T*>( reinterpret_cast<const std::byte*>( this ) +
																 ring_storage_data_offset ) );
			}

			std::size_t m_mask{ 0 };
		};

		static constexpr std::size_t ring_storage_alignment = std::max( alignof( ring_storage ), alignof( T ) );
		static constexpr std::size_t ring_storage_data_offset = align_up( sizeof( ring_storage ), alignof( T ) );

		[[nodiscard]] static ring_storage* allocate_storage( std::size_t capacity )
		{
			capacity = std::bit_ceil( capacity );

			const std::size_t memory_size = ring_storage_data_offset + ( sizeof( T ) * capacity );

			void* ptr = ::operator new( memory_size, std::align_val_t( ring_storage_alignment ) );
			return ::new ( ptr ) ring_storage( capacity );
		}

		static void deallocate_storage( ring_storage* storage ) noexcept
		{
			std::destroy_at( storage );
			::operator delete( storage, std::align_val_t( ring_storage_alignment ) );
		}

		struct deleter
		{
			void operator()( ring_storage* storage ) const noexcept
			{
				deallocate_storage( storage );
			}
		};

		alignas( std::hardware_destructive_interference_size ) std::atomic_int64_t m_top{ 0 };
		alignas( std::hardware_destructive_interference_size ) std::atomic_int64_t m_bottom{ 0 };
		alignas( std::hardware_destructive_interference_size ) std::atomic<ring_storage*> m_storage;
		std::vector<std::unique_ptr<ring_storage, deleter>> m_old_storages;
	};
	MCLO_MSVC_POP_WARNINGS
}
