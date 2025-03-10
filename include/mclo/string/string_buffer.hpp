#pragma once

#include "ascii_string_utils.hpp"

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"

#include <array>
#include <compare>
#include <fmt/format.h>
#include <stdexcept>
#include <string_view>

namespace mclo
{
	template <typename CharT, std::size_t Size>
	class basic_string_buffer
	{
	public:
		using storage_type = std::array<CharT, Size>;
		using traits_type = std::char_traits<CharT>;
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

		static constexpr size_type npos = size_type( -1 );
		static constexpr size_type max_string_size = Size - 1;

		// Constructors

		constexpr basic_string_buffer() noexcept = default;

		constexpr basic_string_buffer( const size_type count, const value_type ch ) MCLO_NOEXCEPT_TESTS
		{
			resize( count, ch );
		}
		constexpr explicit basic_string_buffer( const size_type count ) MCLO_NOEXCEPT_TESTS
		{
			resize( count );
		}

		template <std::size_t OtherSize>
			requires( OtherSize <= Size )
		constexpr basic_string_buffer( const value_type ( &array )[ OtherSize ] ) noexcept
		{
			assign( array );
		}

		constexpr basic_string_buffer( const view_type& str ) MCLO_NOEXCEPT_TESTS
		{
			assign( str );
		}

		// Assign

		constexpr void assign( const size_type count, value_type ch ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( count <= max_string_size, "Count larger than max string size" );
			traits_assign( m_data.data(), count, ch );
			m_length = count;
			null_terminate_end();
		}

		template <std::size_t OtherSize>
			requires( OtherSize <= Size )
		constexpr void assign( const basic_string_buffer<CharT, OtherSize>& str ) noexcept
		{
			assign_with_null( str.data(), str.size() );
		}

		template <std::size_t OtherSize>
			requires( OtherSize <= Size )
		constexpr void assign( const value_type ( &str )[ OtherSize ] ) noexcept
		{
			assign_with_null( str, OtherSize - 1 );
		}

		constexpr void assign( const const_pointer str, const size_type size ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( size <= max_string_size, "Size larger than max string size" );
			traits_copy( m_data.data(), str, size );
			m_length = size;
			null_terminate_end();
		}

		constexpr void assign( const view_type& str ) MCLO_NOEXCEPT_TESTS
		{
			assign( str.data(), str.size() );
		}

		// Element access

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

		[[nodiscard]] constexpr reference operator[]( const size_type pos ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos < m_length, "Indexing out of range" );
			return m_data[ pos ];
		}
		[[nodiscard]] constexpr const_reference operator[]( const size_type pos ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos < m_length, "Indexing out of range" );
			return m_data[ pos ];
		}

		[[nodiscard]] constexpr reference front() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.front();
		}
		[[nodiscard]] constexpr const_reference front() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.front();
		}

		[[nodiscard]] constexpr reference back() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.back();
		}
		[[nodiscard]] constexpr const_reference back() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			return m_data.back();
		}

		[[nodiscard]] constexpr pointer data() noexcept
		{
			return m_data.data();
		}
		[[nodiscard]] constexpr const_pointer data() const noexcept
		{
			return m_data.data();
		}

		[[nodiscard]] constexpr const_pointer c_str() const noexcept
		{
			return m_data.data();
		}

		[[nodiscard]] constexpr operator view_type() const noexcept
		{
			return { m_data.data(), m_length };
		}

		// Iterators

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

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_length == 0;
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_length;
		}
		[[nodiscard]] constexpr size_type length() const noexcept
		{
			return m_length;
		}

		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return max_string_size;
		}

		[[nodiscard]] constexpr size_type capacity() const noexcept
		{
			return max_string_size;
		}

		constexpr void reserve( [[maybe_unused]] const size_type size ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( size <= max_string_size, "Size larger than max string size" );
		}

		constexpr void shrink_to_fit() noexcept
		{
		}

		// Modifiers

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

		constexpr void swap( basic_string_buffer& other ) noexcept
		{
			std::swap( m_data, other.m_data );
			std::swap( m_length, other.m_length );
		}

		friend constexpr void swap( basic_string_buffer& lhs, basic_string_buffer& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		constexpr void resize( const size_type count, const value_type ch ) MCLO_NOEXCEPT_TESTS
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

		constexpr void resize( const size_type count ) MCLO_NOEXCEPT_TESTS
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

		constexpr void push_back( const value_type c ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_length < max_string_size, "Count would be larger than max string size" );
			traits_type::assign( m_data[ m_length++ ], c );
			null_terminate_end();
		}

		constexpr void pop_back() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_length > 0, "Container is empty" );
			--m_length;
			null_terminate_end();
		}

		constexpr void append( const view_type& string ) MCLO_NOEXCEPT_TESTS
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

		template <std::size_t OtherSize>
		[[nodiscard]] constexpr int compare( const basic_string_buffer<CharT, OtherSize>& str ) const noexcept
		{
			return compare( view_type( str ) );
		}

		template <std::size_t OtherSize>
		[[nodiscard]] constexpr int compare( const value_type ( &str )[ OtherSize ] ) const noexcept
		{
			return compare( view_type( str ) );
		}

		[[nodiscard]] constexpr int compare( const view_type& str ) const noexcept
		{
			return view_type( *this ).compare( str );
		}

#ifdef __cpp_lib_starts_ends_with
		[[nodiscard]] constexpr bool starts_with( const view_type str ) const noexcept
		{
			return view_type( *this ).starts_with( str );
		}
		[[nodiscard]] constexpr bool starts_with( const value_type ch ) const noexcept
		{
			return view_type( *this ).starts_with( ch );
		}
		[[nodiscard]] constexpr bool starts_with( const const_pointer str ) const noexcept
		{
			return view_type( *this ).starts_with( str );
		}

		[[nodiscard]] constexpr bool ends_with( const view_type str ) const noexcept
		{
			return view_type( *this ).ends_with( str );
		}
		[[nodiscard]] constexpr bool ends_with( const value_type ch ) const noexcept
		{
			return view_type( *this ).ends_with( ch );
		}
		[[nodiscard]] constexpr bool ends_with( const const_pointer str ) const noexcept
		{
			return view_type( *this ).ends_with( str );
		}
#endif

#ifdef __cpp_lib_string_contains
		[[nodiscard]] constexpr bool contains( const view_type str ) const noexcept
		{
			return view_type( *this ).contains( str );
		}
		[[nodiscard]] constexpr bool contains( const value_type ch ) const noexcept
		{
			return view_type( *this ).contains( ch );
		}
		[[nodiscard]] constexpr bool contains( const const_pointer str ) const noexcept
		{
			return view_type( *this ).contains( str );
		}
#endif

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

	template <typename CharT, std::size_t Size, typename String>
	[[nodiscard]] constexpr bool operator==( const basic_string_buffer<CharT, Size>& lhs, const String& rhs ) noexcept
	{
		using view_type = typename basic_string_buffer<CharT, Size>::view_type;
		return view_type( lhs ) == view_type( rhs );
	}

	template <typename CharT, std::size_t Size, typename String>
	[[nodiscard]] constexpr auto operator<=>( const basic_string_buffer<CharT, Size>& lhs, const String& rhs ) noexcept
	{
		using view_type = typename basic_string_buffer<CharT, Size>::view_type;
		return view_type( lhs ) <=> view_type( rhs );
	}

	// Formatting
	template <typename CharT, std::size_t Size>
	auto format_as( const basic_string_buffer<CharT, Size>& str )
	{
		using view_type = typename basic_string_buffer<CharT, Size>::view_type;
		return view_type( str );
	}

	// Guides
	template <typename CharT, std::size_t N>
	basic_string_buffer( const CharT ( & )[ N ] ) -> basic_string_buffer<CharT, N>;

	// Aliases
	template <std::size_t Size>
	using string_buffer = basic_string_buffer<char, Size>;

	template <std::size_t Size>
	using wstring_buffer = basic_string_buffer<wchar_t, Size>;

#ifdef __cpp_char8_t
	template <std::size_t Size>
	using u8string_buffer = basic_string_buffer<char8_t, Size>;
#endif

	template <std::size_t Size>
	using u16string_buffer = basic_string_buffer<char16_t, Size>;

	template <std::size_t Size>
	using u32string_buffer = basic_string_buffer<char32_t, Size>;

	template <typename CharT, std::size_t N>
	[[nodiscard]] constexpr basic_string_buffer<CharT, N> trandscode_ascii_literal( const char ( &str )[ N ] ) noexcept
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
	struct hash<mclo::basic_string_buffer<CharT, Size>> : mclo::hash<mclo::basic_string_buffer<CharT, Size>>
	{
	};
}
