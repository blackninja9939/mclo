#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"
#include "mclo/memory/allocate_from_tuple.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cinttypes>
#include <climits>
#include <cstddef>

namespace mclo
{
	namespace detail
	{
		template <typename T>
		constexpr bool is_default_zero_initialized()
		{
			using byte_array = std::array<std::byte, sizeof( T )>;
			constexpr byte_array bytes = std::bit_cast<byte_array>( T() );
			return std::all_of( bytes.begin(), bytes.end(), []( const std::byte b ) { return b == std::byte( 0 ); } );
		}

		template <typename T>
		constexpr bool is_zero_initializeable_v =
			std::is_trivially_default_constructible_v<T> || is_default_zero_initialized<T>();

		// todo(mc): these upper bits depend on x86-64 vs ARM and 32 vs 64 bit
		// do we want to even support 32 bit code? I doubt it
		inline constexpr std::size_t platform_free_upper_bits = ( sizeof( void* ) * CHAR_BIT ) - 48;
	}

	template <typename T, typename Tag, std::size_t Alignment = alignof( T ), std::size_t FreeUpperBits = 0>
	class tagged_ptr
	{
	private:
		static constexpr std::size_t free_upper_bits = FreeUpperBits;
		static constexpr std::size_t free_lower_bits = std::bit_width( Alignment );
		static constexpr std::size_t total_free_bits = free_upper_bits + free_lower_bits;
		static constexpr std::size_t used_ptr_bits = ( sizeof( void* ) * CHAR_BIT ) - total_free_bits;

		static constexpr std::size_t tag_mask = ( std::uintptr_t( 1 ) << total_free_bits ) - 1;
		static constexpr std::size_t ptr_mask = ~tag_mask;

		static constexpr bool is_integer = std::is_integral_v<Tag> || std::is_enum_v<Tag>;

		using tag_storage_type = mclo::uint_least_t<sizeof( Tag ) * CHAR_BIT>;
		static_assert( std::is_convertible_v<tag_storage_type, std::uintptr_t>,
					   "Tag type is too large to fit in a uintptr_t" );

		[[nodiscard]] static constexpr std::uintptr_t pack_tag_unchecked( const Tag tag ) noexcept
		{
			if constexpr ( is_integer )
			{
				return static_cast<std::uintptr_t>( tag );
			}
			else
			{
				return std::bit_cast<tag_storage_type>( tag );
			}
		}

	public:
		using element_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using tag_type = Tag;

		static_assert( std::is_trivially_destructible_v<tag_type>, "Tag type must be trivially destructible" );
		static_assert( std::is_trivially_copyable_v<tag_type>, "Tag type must be trivially copyable" );
		static_assert( detail::is_zero_initializeable_v<tag_type>,
					   "Tag type must be able to be initialized with all bits zero" );

		[[nodiscard]] static constexpr bool can_store_tag( const tag_type tag ) noexcept
		{
			if constexpr ( ( sizeof( tag_type ) * CHAR_BIT ) <= total_free_bits )
			{
				return true;
			}
			else
			{
				return std::bit_width( pack_tag_unchecked( tag ) ) <= total_free_bits;
			}
		}

		constexpr tagged_ptr() noexcept = default;

		constexpr tagged_ptr( std::nullptr_t ) noexcept
		{
		}

		explicit tagged_ptr( pointer ptr ) noexcept
			: m_bits( pack_ptr( ptr ) )
		{
		}

		tagged_ptr( pointer ptr, const tag_type tag ) noexcept
			: m_bits( pack_ptr( ptr ) | pack_tag( tag ) )
		{
		}

		tagged_ptr& operator=( pointer ptr ) noexcept
		{
			set_ptr( ptr );
			return *this;
		}

		void set_ptr( pointer ptr ) noexcept
		{
			m_bits = pack_ptr( ptr ) | ( m_bits & tag_mask );
		}
		void clear_ptr() noexcept
		{
			m_bits &= tag_mask;
		}

		void set_tag( const tag_type tag ) noexcept
		{
			m_bits = pack_tag( tag ) | ( m_bits & ptr_mask );
		}
		void clear_tag() noexcept
		{
			m_bits &= ptr_mask;
		}

		void reset() noexcept
		{
			m_bits = 0;
		}
		void reset( pointer ptr ) noexcept
		{
			m_bits = pack_ptr( ptr );
		}
		void reset( pointer ptr, const tag_type tag ) noexcept
		{
			m_bits = pack_ptr( ptr ) | pack_tag( tag );
		}

		[[nodiscard]] pointer get() const noexcept
		{
			return unpack_ptr( m_bits );
		}
		[[nodiscard]] tag_type tag() const noexcept
		{
			return unpack_tag( m_bits );
		}

		[[nodiscard]] element_type& operator*() const noexcept
		{
			return *get();
		}
		[[nodiscard]] pointer operator->() const noexcept
		{
			return get();
		}

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return get() != nullptr;
		}

		[[nodiscard]] friend bool operator==( const tagged_ptr& lhs, const tagged_ptr& rhs ) noexcept
		{
			return lhs.m_bits == rhs.m_bits;
		}
		[[nodiscard]] friend bool operator==( const tagged_ptr& lhs, const const_pointer rhs ) noexcept
		{
			return lhs.get() == rhs;
		}

		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const tagged_ptr& ptr ) noexcept
		{
			hash_append( hasher, ptr.get() );
			hash_append( hasher, ptr.tag() );
		}

	private:
		[[nodiscard]] static std::uintptr_t pack_ptr( pointer ptr ) MCLO_NOEXCEPT_TESTS
		{
			const auto ptr_bits = reinterpret_cast<std::uintptr_t>( ptr );
			DEBUG_ASSERT( ( ptr_bits == 0 || std::bit_floor( ptr_bits ) >= free_lower_bits ),
						  "Ptr is too strictly aligned, it must be aligned to at least Alignment" );
			return ptr_bits << free_upper_bits;
		}
		[[nodiscard]] static pointer unpack_ptr( std::uintptr_t ptr ) noexcept
		{
			return reinterpret_cast<pointer>( ( ptr & ptr_mask ) >> free_upper_bits );
		}

		[[nodiscard]] static std::uintptr_t pack_tag( const tag_type tag ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( can_store_tag( tag ), "Tag using too many bits" );
			return pack_tag_unchecked( tag );
		}
		[[nodiscard]] static tag_type unpack_tag( std::uintptr_t tag ) noexcept
		{
			if constexpr ( is_integer )
			{
				return static_cast<tag_type>( tag & tag_mask );
			}
			else
			{
				return std::bit_cast<tag_type>( tag_storage_type( tag & tag_mask ) );
			}
		}

		// Layout is a T* shifted left by free_upper_bits, the lower free_lower_bits will
		// always be zero and implicitly written over as part of the data as well
		std::uintptr_t m_bits = 0;
	};

	template <typename T, typename U>
	tagged_ptr( T*, U ) -> tagged_ptr<T, U>;

	// This is a tagged_ptr that also uses the platform's free upper bits not just the lower alignment bits
	template <typename T, typename Tag, std::size_t Alignment = alignof( T )>
	using platform_tagged_ptr = tagged_ptr<T, Tag, Alignment, detail::platform_free_upper_bits>;

	template <typename T, typename Tag>
	class tagged_unique_ptr : public tagged_ptr<T, Tag, alignof( std::max_align_t )>
	{
		using base = tagged_ptr<T, Tag, alignof( std::max_align_t )>;

		void delete_ptr()
		{
			delete base::get();
		}

	public:
		using base::base;
		using pointer = typename base::pointer;
		using tag_type = typename base::tag_type;

		tagged_unique_ptr( const tagged_unique_ptr& other ) = delete;
		tagged_unique_ptr& operator=( const tagged_unique_ptr& other ) = delete;

		tagged_unique_ptr( tagged_unique_ptr&& other ) noexcept
			: base( other.release(), other.tag() )
		{
		}
		tagged_unique_ptr& operator=( tagged_unique_ptr&& other ) noexcept
		{
			reset( other.release(), other.tag() );
			return *this;
		}

		tagged_unique_ptr& operator=( pointer ptr ) noexcept
		{
			set_ptr( ptr );
			return *this;
		}

		[[nodiscard]] pointer release() noexcept
		{
			pointer ptr = base::get();
			base::clear_ptr();
			return ptr;
		}

		void set_ptr( pointer ptr ) noexcept
		{
			delete_ptr();
			base::set_ptr( ptr );
		}
		void clear_ptr() noexcept
		{
			delete_ptr();
			base::clear_ptr();
		}

		void reset() noexcept
		{
			delete_ptr();
			base::reset();
		}
		void reset( pointer ptr ) noexcept
		{
			delete_ptr();
			base::reset( ptr );
		}
		void reset( pointer ptr, const tag_type tag ) noexcept
		{
			delete_ptr( ptr );
			base::reset( tag );
		}

		~tagged_unique_ptr()
		{
			delete_ptr();
		}
	};

	template <typename T, typename Tag, typename... Args>
	[[nodiscard]] tagged_unique_ptr<T, Tag> make_tagged_unique( Args&&... args )
	{
		return tagged_unique_ptr<T, Tag>{ new T( std::forward<Args>( args )... ) };
	}

	namespace detail
	{
		template <typename T>
		struct [[nodiscard]] guard_deleter
		{
			explicit guard_deleter( T* ptr ) noexcept
				: ptr( ptr )
			{
			}
			~guard_deleter()
			{
				delete ptr;
			}
			void release() noexcept
			{
				ptr = nullptr;
			}
			T* ptr = nullptr;
		};
	}

	template <typename T, typename Tag, typename... TArgs, typename... TagArgs>
	[[nodiscard]] tagged_unique_ptr<T, Tag> make_tagged_unique( std::piecewise_construct_t,
																std::tuple<TArgs...> object_args,
																std::tuple<TagArgs...> tag_args )
	{
		// If Tag construction throws we ensure we delete the allocated object despite not yet having constructed
		// the owning tagged_unique_ptr
		detail::guard_deleter<T> guard{ mclo::allocate_from_tuple<T>( std::move( object_args ) ) };
		tagged_unique_ptr<T, Tag> tagged{ guard.ptr, std::make_from_tuple<Tag>( std::move( tag_args ) ) };
		guard.release();
		return tagged;
	}
}

template <typename T, typename Tag>
struct std::hash<mclo::tagged_ptr<T, Tag>> : mclo::hash<mclo::tagged_ptr<T, Tag>>
{
};
