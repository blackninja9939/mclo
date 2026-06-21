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

		inline constexpr std::size_t platform_free_upper_bits = ( sizeof( void* ) * CHAR_BIT ) - 48;
	}

	/// @brief A pointer that packs a small tag value into its otherwise-unused bits.
	/// @details Modern platforms leave the upper address bits unused and alignment guarantees leave the lower bits
	/// zero. @c tagged_ptr exploits both to store a @p Tag alongside a @c T* in a single pointer-sized word, with no
	/// extra storage. The number of available bits is derived from @p Alignment (lower bits) and the platform's
	/// address width (upper bits); @ref can_store_tag reports whether a given tag fits.
	/// @tparam T The pointee type.
	/// @tparam Tag The tag type stored in the spare bits. Must be trivially copyable, trivially destructible and
	/// zero-initializable.
	/// @tparam Alignment The alignment the stored pointer is guaranteed to satisfy, defaulting to @c alignof(T). A
	/// larger alignment frees more low bits for the tag.
	/// @warning The stored pointer must be aligned to at least @p Alignment, otherwise behaviour is undefined.
	template <typename T, typename Tag, std::size_t Alignment = alignof( T )>
	class tagged_ptr
	{
	public:
		/// @brief The pointee type.
		using element_type = T;
		/// @brief Pointer to the pointee type.
		using pointer = T*;
		/// @brief Const pointer to the pointee type.
		using const_pointer = const T*;
		/// @brief The tag type stored in the spare bits.
		using tag_type = Tag;
		/// @brief The unsigned integer type holding the packed pointer and tag.
		using packed_type = std::uintptr_t;

	private:
		static constexpr std::size_t free_upper_bits = detail::platform_free_upper_bits;
		static constexpr std::size_t free_lower_bits = std::bit_width( Alignment ) - 1;
		static constexpr std::size_t total_free_bits = free_upper_bits + free_lower_bits;
		static constexpr std::size_t used_ptr_bits = ( sizeof( void* ) * CHAR_BIT ) - total_free_bits;

		static constexpr std::size_t tag_mask = ( packed_type( 1 ) << total_free_bits ) - 1;
		static constexpr std::size_t ptr_mask = ~tag_mask;

		static constexpr bool is_integer = std::is_integral_v<Tag> || std::is_enum_v<Tag>;

		using tag_storage_type = mclo::uint_least_t<sizeof( Tag ) * CHAR_BIT>;
		static_assert( std::is_convertible_v<tag_storage_type, packed_type>,
					   "Tag type is too large to fit in a uintptr_t" );

		[[nodiscard]] static constexpr packed_type pack_tag_unchecked( const Tag tag ) noexcept
		{
			if constexpr ( is_integer )
			{
				return static_cast<packed_type>( tag );
			}
			else
			{
				return std::bit_cast<tag_storage_type>( tag );
			}
		}

	public:
		static_assert( std::is_trivially_destructible_v<tag_type>, "Tag type must be trivially destructible" );
		static_assert( std::is_trivially_copyable_v<tag_type>, "Tag type must be trivially copyable" );
		static_assert( detail::is_zero_initializeable_v<tag_type>,
					   "Tag type must be able to be initialized with all bits zero" );

		/// @brief Reports whether the given tag value fits in the available spare bits.
		/// @param tag The tag value to test.
		/// @return True if @p tag can be stored without losing information, false otherwise.
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

		/// @brief Constructs a null tagged pointer with a zero tag.
		constexpr tagged_ptr() noexcept = default;

		/// @brief Constructs a null tagged pointer with a zero tag.
		constexpr tagged_ptr( std::nullptr_t ) noexcept
		{
		}

		/// @brief Constructs from a pre-packed pointer-and-tag word, as returned by @ref packed.
		/// @param packed The packed representation.
		constexpr explicit tagged_ptr( packed_type packed ) noexcept
			: m_bits( packed )
		{
		}

		/// @brief Constructs from a pointer with a zero tag.
		/// @param ptr The pointer to store. Must be aligned to at least @p Alignment.
		explicit tagged_ptr( pointer ptr ) noexcept
			: m_bits( pack_ptr( ptr ) )
		{
		}

		/// @brief Constructs from a pointer and a tag.
		/// @param ptr The pointer to store. Must be aligned to at least @p Alignment.
		/// @param tag The tag to store. Must satisfy @ref can_store_tag.
		tagged_ptr( pointer ptr, const tag_type tag ) noexcept
			: m_bits( pack_ptr( ptr ) | pack_tag( tag ) )
		{
		}

		/// @brief Replaces the stored pointer, preserving the current tag.
		/// @param ptr The new pointer. Must be aligned to at least @p Alignment.
		tagged_ptr& operator=( pointer ptr ) noexcept
		{
			set_ptr( ptr );
			return *this;
		}

		/// @brief Returns the raw packed pointer-and-tag word.
		[[nodiscard]] constexpr packed_type packed() const noexcept
		{
			return m_bits;
		}

		/// @brief Replaces the stored pointer, preserving the current tag.
		/// @param ptr The new pointer. Must be aligned to at least @p Alignment.
		void set_ptr( pointer ptr ) noexcept
		{
			m_bits = pack_ptr( ptr ) | ( m_bits & tag_mask );
		}
		/// @brief Sets the stored pointer to null, preserving the current tag.
		void clear_ptr() noexcept
		{
			m_bits &= tag_mask;
		}

		/// @brief Replaces the stored tag, preserving the current pointer.
		/// @param tag The new tag. Must satisfy @ref can_store_tag.
		constexpr void set_tag( const tag_type tag ) noexcept
		{
			m_bits = pack_tag( tag ) | ( m_bits & ptr_mask );
		}
		/// @brief Resets the tag to zero, preserving the current pointer.
		constexpr void clear_tag() noexcept
		{
			m_bits &= ptr_mask;
		}

		/// @brief Resets to a null pointer and zero tag.
		void reset() noexcept
		{
			m_bits = 0;
		}
		/// @brief Resets to the given pointer with a zero tag.
		/// @param ptr The new pointer. Must be aligned to at least @p Alignment.
		void reset( pointer ptr ) noexcept
		{
			m_bits = pack_ptr( ptr );
		}
		/// @brief Resets to the given pointer and tag.
		/// @param ptr The new pointer. Must be aligned to at least @p Alignment.
		/// @param tag The new tag. Must satisfy @ref can_store_tag.
		void reset( pointer ptr, const tag_type tag ) noexcept
		{
			m_bits = pack_ptr( ptr ) | pack_tag( tag );
		}

		/// @brief Returns the stored pointer.
		[[nodiscard]] pointer get() const noexcept
		{
			return unpack_ptr( m_bits );
		}
		/// @brief Returns the stored tag.
		[[nodiscard]] constexpr tag_type tag() const noexcept
		{
			return unpack_tag( m_bits );
		}

		/// @brief Dereferences the stored pointer.
		[[nodiscard]] std::add_lvalue_reference_t<element_type> operator*() const noexcept
		{
			return *get();
		}
		/// @brief Accesses members of the pointee through the stored pointer.
		[[nodiscard]] pointer operator->() const noexcept
		{
			return get();
		}

		/// @brief Returns true if the stored pointer is non-null.
		[[nodiscard]] explicit operator bool() const noexcept
		{
			return get() != nullptr;
		}

		/// @brief Compares two tagged pointers for equality of both pointer and tag.
		[[nodiscard]] friend constexpr bool operator==( const tagged_ptr& lhs, const tagged_ptr& rhs ) noexcept
		{
			return lhs.m_bits == rhs.m_bits;
		}
		/// @brief Compares the stored pointer against a raw pointer, ignoring the tag.
		[[nodiscard]] friend constexpr bool operator==( const tagged_ptr& lhs, const const_pointer rhs ) noexcept
		{
			return lhs.get() == rhs;
		}

		/// @brief Swaps the contents with another tagged pointer.
		constexpr void swap( tagged_ptr& other ) noexcept
		{
			std::swap( m_bits, other.m_bits );
		}

		/// @brief Swaps two tagged pointers.
		friend constexpr void swap( tagged_ptr& lhs, tagged_ptr& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		/// @brief Hashes the tagged pointer by combining its pointer and tag.
		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const tagged_ptr& ptr ) noexcept
		{
			hash_append( hasher, ptr.get() );
			hash_append( hasher, ptr.tag() );
		}

	private:
		[[nodiscard]] static packed_type pack_ptr( pointer ptr ) MCLO_NOEXCEPT_TESTS
		{
			const auto ptr_bits = reinterpret_cast<packed_type>( ptr );
			DEBUG_ASSERT( ( ptr_bits == 0 || std::bit_floor( ptr_bits ) >= free_lower_bits ),
						  "Ptr is too strictly aligned, it must be aligned to at least Alignment" );
			return ptr_bits << free_upper_bits;
		}
		[[nodiscard]] static pointer unpack_ptr( packed_type ptr ) noexcept
		{
			return reinterpret_cast<pointer>( ( ptr & ptr_mask ) >> free_upper_bits );
		}

		[[nodiscard]] static constexpr packed_type pack_tag( const tag_type tag ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( can_store_tag( tag ), "Tag using too many bits" );
			return pack_tag_unchecked( tag );
		}
		[[nodiscard]] static constexpr tag_type unpack_tag( packed_type tag ) noexcept
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
		packed_type m_bits = 0;
	};

	template <typename T, typename U>
	tagged_ptr( T*, U ) -> tagged_ptr<T, U>;

	/// @brief An owning @ref tagged_ptr that deletes the pointee on destruction, like @c std::unique_ptr.
	/// @details Combines unique ownership with the spare-bit tag storage of @ref tagged_ptr. The pointer is deleted
	/// whenever it is replaced or the owner is destroyed. Because the deleter uses plain @c delete, the stored
	/// pointer is only assumed aligned to @c alignof(std::max_align_t), which determines the available tag bits.
	/// @tparam T The pointee type, deleted via @c delete on destruction.
	/// @tparam Tag The tag type stored in the spare bits.
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

		/// @brief Move constructor. Transfers ownership and tag, leaving @p other empty.
		tagged_unique_ptr( tagged_unique_ptr&& other ) noexcept
			: base( other.release(), other.tag() )
		{
		}
		/// @brief Move assignment. Deletes the current pointee then transfers ownership and tag from @p other.
		tagged_unique_ptr& operator=( tagged_unique_ptr&& other ) noexcept
		{
			reset( other.release(), other.tag() );
			return *this;
		}

		/// @brief Takes ownership of @p ptr, deleting the current pointee first.
		/// @param ptr The pointer to take ownership of.
		tagged_unique_ptr& operator=( pointer ptr ) noexcept
		{
			set_ptr( ptr );
			return *this;
		}

		/// @brief Releases ownership of the stored pointer without deleting it.
		/// @return The previously owned pointer; the caller is now responsible for deleting it.
		[[nodiscard]] pointer release() noexcept
		{
			pointer ptr = base::get();
			base::clear_ptr();
			return ptr;
		}

		/// @brief Deletes the current pointee and takes ownership of @p ptr, preserving the tag.
		/// @param ptr The pointer to take ownership of.
		void set_ptr( pointer ptr ) noexcept
		{
			delete_ptr();
			base::set_ptr( ptr );
		}
		/// @brief Deletes the current pointee and stores null, preserving the tag.
		void clear_ptr() noexcept
		{
			delete_ptr();
			base::clear_ptr();
		}

		/// @brief Deletes the current pointee and resets to null with a zero tag.
		void reset() noexcept
		{
			delete_ptr();
			base::reset();
		}
		/// @brief Deletes the current pointee and takes ownership of @p ptr with a zero tag.
		/// @param ptr The pointer to take ownership of.
		void reset( pointer ptr ) noexcept
		{
			delete_ptr();
			base::reset( ptr );
		}
		/// @brief Deletes the current pointee and takes ownership of @p ptr with the given tag.
		/// @param ptr The pointer to take ownership of.
		/// @param tag The tag to store.
		void reset( pointer ptr, const tag_type tag ) noexcept
		{
			delete_ptr();
			base::reset( ptr, tag );
		}

		/// @brief Destroys the owner, deleting the stored pointee.
		~tagged_unique_ptr()
		{
			delete_ptr();
		}
	};

	/// @brief Creates a @ref tagged_unique_ptr owning a newly constructed @p T with a default tag.
	/// @tparam T The type to allocate and construct.
	/// @tparam Tag The tag type.
	/// @tparam Args The constructor argument types.
	/// @param args The arguments forwarded to the constructor of @p T.
	/// @return A @ref tagged_unique_ptr owning the new object.
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
	/// @brief Creates a @ref tagged_unique_ptr, constructing the object and tag from separate argument tuples.
	/// @details Mirrors @c std::piecewise_construct: the object is built from @p object_args and the tag from
	/// @p tag_args. If constructing the tag throws, the already-allocated object is deleted before propagating.
	/// @tparam T The object type to allocate and construct.
	/// @tparam Tag The tag type to construct.
	/// @tparam TArgs The object constructor argument types.
	/// @tparam TagArgs The tag constructor argument types.
	/// @param object_args A tuple of arguments forwarded to the constructor of @p T.
	/// @param tag_args A tuple of arguments forwarded to the constructor of @p Tag.
	/// @return A @ref tagged_unique_ptr owning the new object with the constructed tag.
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
