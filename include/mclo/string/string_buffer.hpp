#pragma once

#include "ascii_string_utils.hpp"

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"

#include <array>
#include <compare>
#include <format>
#include <stdexcept>
#include <string_view>

namespace mclo
{
	/// @brief A fixed-capacity, null-terminated string stored entirely inline with no heap allocation.
	/// @details Holds up to @c Size-1 characters plus a null terminator in an inline @c std::array, providing a
	/// @c std::string -like interface without dynamic allocation. Exceeding the capacity is a precondition violation
	/// rather than a reallocation. Convertible to a @c std::basic_string_view and usable with @c std::format and
	/// @c std::hash.
	/// @tparam CharT The character type.
	/// @tparam Size The total inline buffer size; the maximum string length is @c Size-1.
	template <typename CharT, std::size_t Size>
	class basic_string_buffer
	{
	public:
		using storage_type = std::array<CharT, Size>;
		using traits_type = std::char_traits<CharT>;
		/// @brief The corresponding @c std::basic_string_view type.
		using view_type = std::basic_string_view<CharT, traits_type>;

		using value_type = typename storage_type::value_type;
		using size_type = typename storage_type::size_type;
		using difference_type = typename storage_type::difference_type;
		using reference = typename storage_type::reference;
		using const_reference = typename storage_type::const_reference;
		using pointer = typename storage_type::pointer;
		using const_pointer = typename storage_type::const_pointer;
		using iterator = typename storage_type::iterator;
		using const_iterator = typename storage_type::const_iterator;
		using reverse_iterator = typename storage_type::reverse_iterator;
		using const_reverse_iterator = typename storage_type::const_reverse_iterator;

		/// @brief Sentinel value returned by search operations to indicate "not found".
		static constexpr size_type npos = size_type( -1 );
		/// @brief The maximum number of characters the buffer can hold, equal to @c Size-1.
		static constexpr size_type max_string_size = Size - 1;

		// Constructors

		/// @brief Constructs an empty string.
		constexpr basic_string_buffer() noexcept = default;

		/// @brief Constructs a string of @p count copies of @p ch.
		/// @pre @p count must not exceed @ref max_string_size.
		constexpr basic_string_buffer( const size_type count, const value_type ch ) noexcept
		{
			resize( count, ch );
		}
		/// @brief Constructs a string of @p count default (null) characters.
		/// @pre @p count must not exceed @ref max_string_size.
		constexpr explicit basic_string_buffer( const size_type count ) noexcept
		{
			resize( count );
		}

		/// @brief Constructs from a character array literal.
		template <std::size_t OtherSize>
			requires( OtherSize <= Size )
		constexpr basic_string_buffer( const value_type ( &array )[ OtherSize ] ) noexcept
		{
			assign( array );
		}

		/// @brief Constructs from a string view.
		/// @pre The view's size must not exceed @ref max_string_size.
		constexpr basic_string_buffer( const view_type& str ) noexcept
		{
			assign( str );
		}

		// Assign

		/// @brief Replaces the contents with @p count copies of @p ch.
		/// @pre @p count must not exceed @ref max_string_size.
		constexpr void assign( const size_type count, value_type ch ) noexcept
		{
			DEBUG_ASSERT( count <= max_string_size, "Count larger than max string size" );
			traits_assign( m_data.data(), count, ch );
			m_length = count;
			null_terminate_end();
		}

		/// @brief Replaces the contents with a copy of another string buffer.
		template <std::size_t OtherSize>
			requires( OtherSize <= Size )
		constexpr void assign( const basic_string_buffer<CharT, OtherSize>& str ) noexcept
		{
			assign_with_null( str.data(), str.size() );
		}

		/// @brief Replaces the contents with a character array literal.
		template <std::size_t OtherSize>
			requires( OtherSize <= Size )
		constexpr void assign( const value_type ( &str )[ OtherSize ] ) noexcept
		{
			assign_with_null( str, OtherSize - 1 );
		}

		/// @brief Replaces the contents with @p size characters copied from @p str.
		/// @pre @p size must not exceed @ref max_string_size.
		constexpr void assign( const const_pointer str, const size_type size ) noexcept
		{
			DEBUG_ASSERT( size <= max_string_size, "Size larger than max string size" );
			traits_copy( m_data.data(), str, size );
			m_length = size;
			null_terminate_end();
		}

		/// @brief Replaces the contents with the characters of @p str.
		/// @pre The view's size must not exceed @ref max_string_size.
		constexpr void assign( const view_type& str ) noexcept
		{
			assign( str.data(), str.size() );
		}

		// Element access

		/// @brief Returns a reference to the character at @p pos with bounds checking.
		/// @throws std::out_of_range if @p pos is not less than @ref size().
		[[nodiscard]] constexpr reference at( const size_type pos )
		{
			if ( pos >= m_length )
			{
				throw std::out_of_range( "string_buffer at out of range" );
			}
			return m_data[ pos ];
		}
		[[nodiscard]] constexpr const_reference at( const size_type pos ) const
		{
			if ( pos >= m_length )
			{
				throw std::out_of_range( "string_buffer at out of range" );
			}
			return m_data[ pos ];
		}

		/// @brief Returns a reference to the character at @p pos.
		/// @pre @p pos must be less than @ref size().
		[[nodiscard]] constexpr reference operator[]( const size_type pos ) noexcept
		{
			DEBUG_ASSERT( pos < m_length, "Indexing out of range" );
			return m_data[ pos ];
		}
		[[nodiscard]] constexpr const_reference operator[]( const size_type pos ) const noexcept
		{
			DEBUG_ASSERT( pos < m_length, "Indexing out of range" );
			return m_data[ pos ];
		}

		/// @brief Returns a reference to the first character.
		/// @pre The string must not be empty.
		[[nodiscard]] constexpr reference front() noexcept
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.front();
		}
		[[nodiscard]] constexpr const_reference front() const noexcept
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.front();
		}

		/// @brief Returns a reference to the last character.
		/// @pre The string must not be empty.
		[[nodiscard]] constexpr reference back() noexcept
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.back();
		}
		[[nodiscard]] constexpr const_reference back() const noexcept
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.back();
		}

		/// @brief Returns a pointer to the null-terminated character buffer.
		[[nodiscard]] constexpr pointer data() noexcept
		{
			return m_data.data();
		}
		[[nodiscard]] constexpr const_pointer data() const noexcept
		{
			return m_data.data();
		}

		/// @brief Returns a pointer to the null-terminated character buffer.
		[[nodiscard]] constexpr const_pointer c_str() const noexcept
		{
			return m_data.data();
		}

		/// @brief Converts to a string view over the current contents.
		[[nodiscard]] constexpr operator view_type() const noexcept
		{
			return { m_data.data(), m_length };
		}

		// Iterators

		/// @brief Returns an iterator to the first character.
		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return m_data.begin();
		}
		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return m_data.begin();
		}
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return m_data.cbegin();
		}

		/// @brief Returns an iterator one past the last character.
		[[nodiscard]] constexpr iterator end() noexcept
		{
			return begin() + m_length;
		}
		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return begin() + m_length;
		}
		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return cbegin() + m_length;
		}

		/// @brief Returns a reverse iterator to the last character.
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return std::make_reverse_iterator( cend() );
		}

		/// @brief Returns a reverse iterator one past the first character.
		[[nodiscard]] constexpr reverse_iterator rend() noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] constexpr const_reverse_iterator rcend() const noexcept
		{
			return std::make_reverse_iterator( cbegin() );
		}

		// Size

		/// @brief Returns true if the string contains no characters.
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_length == 0;
		}

		/// @brief Returns the number of characters.
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_length;
		}
		/// @brief Returns the number of characters.
		[[nodiscard]] constexpr size_type length() const noexcept
		{
			return m_length;
		}

		/// @brief Returns the maximum number of characters the buffer can hold.
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return max_string_size;
		}

		/// @brief Returns the buffer capacity, which is fixed and equal to @ref max_size().
		[[nodiscard]] constexpr size_type capacity() const noexcept
		{
			return max_string_size;
		}

		/// @brief No-op provided for interface compatibility with @c std::string.
		/// @pre @p size must not exceed @ref max_string_size.
		constexpr void reserve( [[maybe_unused]] const size_type size ) noexcept
		{
			DEBUG_ASSERT( size <= max_string_size, "Size larger than max string size" );
		}

		/// @brief No-op provided for interface compatibility with @c std::string.
		constexpr void shrink_to_fit() noexcept
		{
		}

		// Modifiers

		/// @brief Removes all characters, leaving an empty string.
		constexpr void clear() noexcept
		{
			m_length = 0;
			null_terminate_end();
		}

		/*
		 * Must: append, operator+=
		 * Nice: insert, erase
		 * Low prio: replace
		 * Low prio but easy:  copy
		 */

		/// @brief Swaps the contents with @p other.
		constexpr void swap( basic_string_buffer& other ) noexcept
		{
			std::swap( m_data, other.m_data );
			std::swap( m_length, other.m_length );
		}

		/// @brief Swaps the contents of two string buffers.
		friend constexpr void swap( basic_string_buffer& lhs, basic_string_buffer& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		/// @brief Resizes to @p count characters, padding with @p ch when growing.
		/// @pre @p count must not exceed @ref max_string_size.
		constexpr void resize( const size_type count, const value_type ch ) noexcept
		{
			DEBUG_ASSERT( count <= max_string_size, "Count larger than max string size" );
			if ( count == m_length )
			{
				return;
			}

			if ( count > m_length )
			{
				std::fill_n( end(), count - m_length, ch );
			}

			// Want to set length and terminate for trimming and growing, trimming just ignores existing characters and
			// inserts null
			m_length = count;
			null_terminate_end();
		}

		/// @brief Resizes to @p count characters, padding with null characters when growing.
		/// @pre @p count must not exceed @ref max_string_size.
		constexpr void resize( const size_type count ) noexcept
		{
			DEBUG_ASSERT( count <= max_string_size, "Count larger than max string size" );
			if ( count > m_length )
			{
				// We know we're filling null terminator so do it as part of the fill_n itself so do not call
				// null_terminate_end
				std::fill_n( end(), count - m_length + 1, value_type() );
				m_length = count;
			}
			else if ( count < m_length )
			{
				m_length = count;
				null_terminate_end();
			}
		}

		/// @brief Appends a single character to the end.
		/// @pre The string must have room for one more character.
		constexpr void push_back( const value_type c ) noexcept
		{
			DEBUG_ASSERT( m_length < max_string_size, "Count would be larger than max string size" );
			traits_type::assign( m_data[ m_length++ ], c );
			null_terminate_end();
		}

		/// @brief Removes the last character.
		/// @pre The string must not be empty.
		constexpr void pop_back() noexcept
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			--m_length;
			null_terminate_end();
		}

		/// @brief Appends the characters of @p string to the end.
		/// @pre The combined length must not exceed @ref max_string_size.
		constexpr void append( const view_type& string ) noexcept
		{
			DEBUG_ASSERT( string.size() + m_length < max_string_size, "Count would be larger than max string size" );
			traits_copy( m_data.data() + m_length, string.data(), string.size() );
			m_length += string.size();
			null_terminate_end();
		}

		// Search

		// find, rfind, find_first/last_(not)of

		// Operations

		// substr

		/// @brief Lexicographically compares against another string buffer.
		/// @return A negative, zero, or positive value as with @c std::string::compare.
		template <std::size_t OtherSize>
		[[nodiscard]] constexpr int compare( const basic_string_buffer<CharT, OtherSize>& str ) const noexcept
		{
			return compare( view_type( str ) );
		}

		/// @brief Lexicographically compares against a character array literal.
		template <std::size_t OtherSize>
		[[nodiscard]] constexpr int compare( const value_type ( &str )[ OtherSize ] ) const noexcept
		{
			return compare( view_type( str ) );
		}

		/// @brief Lexicographically compares against a string view.
		[[nodiscard]] constexpr int compare( const view_type& str ) const noexcept
		{
			return view_type( *this ).compare( str );
		}

#ifdef __cpp_lib_starts_ends_with
		/// @brief Returns true if the string begins with @p str.
		[[nodiscard]] constexpr bool starts_with( const view_type str ) const noexcept
		{
			return view_type( *this ).starts_with( str );
		}
		/// @brief Returns true if the string begins with the character @p ch.
		[[nodiscard]] constexpr bool starts_with( const value_type ch ) const noexcept
		{
			return view_type( *this ).starts_with( ch );
		}
		/// @brief Returns true if the string begins with the null-terminated @p str.
		[[nodiscard]] constexpr bool starts_with( const const_pointer str ) const noexcept
		{
			return view_type( *this ).starts_with( str );
		}

		/// @brief Returns true if the string ends with @p str.
		[[nodiscard]] constexpr bool ends_with( const view_type str ) const noexcept
		{
			return view_type( *this ).ends_with( str );
		}
		/// @brief Returns true if the string ends with the character @p ch.
		[[nodiscard]] constexpr bool ends_with( const value_type ch ) const noexcept
		{
			return view_type( *this ).ends_with( ch );
		}
		/// @brief Returns true if the string ends with the null-terminated @p str.
		[[nodiscard]] constexpr bool ends_with( const const_pointer str ) const noexcept
		{
			return view_type( *this ).ends_with( str );
		}
#endif

#ifdef __cpp_lib_string_contains
		/// @brief Returns true if the string contains @p str as a substring.
		[[nodiscard]] constexpr bool contains( const view_type str ) const noexcept
		{
			return view_type( *this ).contains( str );
		}
		/// @brief Returns true if the string contains the character @p ch.
		[[nodiscard]] constexpr bool contains( const value_type ch ) const noexcept
		{
			return view_type( *this ).contains( ch );
		}
		/// @brief Returns true if the string contains the null-terminated @p str as a substring.
		[[nodiscard]] constexpr bool contains( const const_pointer str ) const noexcept
		{
			return view_type( *this ).contains( str );
		}
#endif

		/// @brief Hashing hook feeding the string's characters into @p hasher.
		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const basic_string_buffer& str ) noexcept
		{
			hash_append( hasher, view_type( str ) );
		}

	private:
		static constexpr void traits_copy( pointer dest, const const_pointer src, const size_type count ) noexcept
		{
			traits_type::copy( dest, src, count );
		}
		static constexpr void traits_assign( pointer ptr, const size_type count, const value_type ch ) noexcept
		{
			traits_type::assign( ptr, count, ch );
		}

		constexpr void assign_with_null( const const_pointer str, const size_type size ) noexcept
		{
			traits_copy( m_data.data(), str, size + 1 );
			m_length = size;
		}

		constexpr void null_terminate_end() noexcept
		{
			traits_type::assign( m_data[ m_length ], value_type() );
		}

		storage_type m_data{};
		size_type m_length = 0;
	};

	// Comparison operators

	/// @brief Compares a string buffer with any string-like value for equality.
	template <typename CharT, std::size_t Size, typename String>
	[[nodiscard]] constexpr bool operator==( const basic_string_buffer<CharT, Size>& lhs, const String& rhs ) noexcept
	{
		using view_type = typename basic_string_buffer<CharT, Size>::view_type;
		return view_type( lhs ) == view_type( rhs );
	}

	/// @brief Lexicographically orders a string buffer against any string-like value.
	template <typename CharT, std::size_t Size, typename String>
	[[nodiscard]] constexpr auto operator<=>( const basic_string_buffer<CharT, Size>& lhs, const String& rhs ) noexcept
	{
		using view_type = typename basic_string_buffer<CharT, Size>::view_type;
		return view_type( lhs ) <=> view_type( rhs );
	}

	// Guides
	template <typename CharT, std::size_t N>
	basic_string_buffer( const CharT ( & )[ N ] ) -> basic_string_buffer<CharT, N>;

	// Aliases
	/// @brief A @ref basic_string_buffer of @c char with capacity @c Size-1.
	template <std::size_t Size>
	using string_buffer = basic_string_buffer<char, Size>;

	/// @brief A @ref basic_string_buffer of @c wchar_t with capacity @c Size-1.
	template <std::size_t Size>
	using wstring_buffer = basic_string_buffer<wchar_t, Size>;

#ifdef __cpp_char8_t
	/// @brief A @ref basic_string_buffer of @c char8_t with capacity @c Size-1.
	template <std::size_t Size>
	using u8string_buffer = basic_string_buffer<char8_t, Size>;
#endif

	/// @brief A @ref basic_string_buffer of @c char16_t with capacity @c Size-1.
	template <std::size_t Size>
	using u16string_buffer = basic_string_buffer<char16_t, Size>;

	/// @brief A @ref basic_string_buffer of @c char32_t with capacity @c Size-1.
	template <std::size_t Size>
	using u32string_buffer = basic_string_buffer<char32_t, Size>;

	/// @brief Transcodes an ASCII string literal into a @ref basic_string_buffer of @p CharT.
	/// @details Widens each ASCII @c char to @p CharT; intended for turning ASCII literals into buffers of another
	/// character type at compile time.
	/// @tparam CharT The target character type.
	/// @param str The ASCII string literal to transcode.
	/// @return A buffer holding the transcoded characters.
	/// @pre Every character of @p str must be ASCII.
	template <typename CharT, std::size_t N>
	[[nodiscard]] constexpr basic_string_buffer<CharT, N> transcode_ascii_literal( const char ( &str )[ N ] ) noexcept
	{
		basic_string_buffer<CharT, N> buffer( N - 1 );
		for ( std::size_t i = 0; i < N - 1; ++i )
		{
			const char ch = str[ i ];
			DEBUG_ASSERT( mclo::is_ascii( ch ), "Character is not ascii", ch );
			buffer[ i ] = static_cast<CharT>( ch );
		}
		return buffer;
	}
}

namespace std
{
	template <typename CharT, std::size_t Size>
	struct formatter<mclo::basic_string_buffer<CharT, Size>>
		: formatter<typename mclo::basic_string_buffer<CharT, Size>::view_type>
	{
	};

	template <typename CharT, std::size_t Size>
	struct hash<mclo::basic_string_buffer<CharT, Size>> : mclo::hash<mclo::basic_string_buffer<CharT, Size>>
	{
	};
}
