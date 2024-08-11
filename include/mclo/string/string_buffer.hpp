#pragma once

#include "mclo/algorithm.hpp"

#include "ascii_string_utils.hpp"

#include <array>
#include <cassert>
#include <stdexcept>
#include <string_view>

#ifdef __cpp_lib_format
#include <format>
#endif

#ifdef __cpp_impl_three_way_comparison
#include <compare>
#endif

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

		constexpr basic_string_buffer( const size_type count, const value_type ch ) noexcept
		{
			resize( count, ch );
		}
		constexpr explicit basic_string_buffer( const size_type count ) noexcept
		{
			resize( count );
		}

		template <std::size_t OtherSize, typename = std::enable_if_t<( OtherSize <= Size )>>
		constexpr basic_string_buffer( const value_type ( &array )[ OtherSize ] ) noexcept
		{
			assign( array );
		}

		constexpr basic_string_buffer( const view_type& str ) noexcept
		{
			assign( str );
		}

		// Assign

		constexpr void assign( const size_type count, value_type ch ) noexcept
		{
			assert( count <= max_string_size );
			traits_assign( m_data.data(), count, ch );
			m_length = count;
			null_terminate_end();
		}

		template <std::size_t OtherSize, typename = std::enable_if_t<( OtherSize <= Size )>>
		constexpr void assign( const basic_string_buffer<CharT, OtherSize>& str ) noexcept
		{
			assign_with_null( str.data(), str.size() );
		}

		template <std::size_t OtherSize, typename = std::enable_if_t<( OtherSize <= Size )>>
		constexpr void assign( const value_type ( &str )[ OtherSize ] ) noexcept
		{
			assign_with_null( str, OtherSize - 1 );
		}

		constexpr void assign( const const_pointer str, const size_type size ) noexcept
		{
			assert( size <= max_string_size );
			traits_copy( m_data.data(), str, size );
			m_length = size;
			null_terminate_end();
		}

		constexpr void assign( const view_type& str ) noexcept
		{
			assign( str.data(), str.size() );
		}

		// Element access

		constexpr reference at( const size_type pos )
		{
			if ( pos >= m_length )
			{
				throw std::out_of_range( "string_buffer at out of range" );
			}
			return m_data[ pos ];
		}
		constexpr const_reference at( const size_type pos ) const
		{
			if ( pos >= m_length )
			{
				throw std::out_of_range( "string_buffer at out of range" );
			}
			return m_data[ pos ];
		}

		constexpr reference operator[]( const size_type pos ) noexcept
		{
			assert( pos < m_length );
			return m_data[ pos ];
		}
		constexpr const_reference operator[]( const size_type pos ) const noexcept
		{
			assert( pos < m_length );
			return m_data[ pos ];
		}

		constexpr reference front() noexcept
		{
			assert( m_length > 0 );
			return m_data.front();
		}
		constexpr const_reference front() const noexcept
		{
			assert( m_length > 0 );
			return m_data.front();
		}

		constexpr reference back() noexcept
		{
			assert( m_length > 0 );
			return m_data.back();
		}
		constexpr const_reference back() const noexcept
		{
			assert( m_length > 0 );
			return m_data.back();
		}

		constexpr pointer data() noexcept
		{
			return m_data.data();
		}
		constexpr const_pointer data() const noexcept
		{
			return m_data.data();
		}

		constexpr const_pointer c_str() const noexcept
		{
			return m_data.data();
		}

		constexpr operator view_type() const noexcept
		{
			return { m_data.data(), m_length };
		}

		// Iterators

		constexpr iterator begin() noexcept
		{
			return m_data.begin();
		}
		constexpr const_iterator begin() const noexcept
		{
			return m_data.begin();
		}
		constexpr const_iterator cbegin() const noexcept
		{
			return m_data.cbegin();
		}

		constexpr iterator end() noexcept
		{
			return begin() + m_length;
		}
		constexpr const_iterator end() const noexcept
		{
			return begin() + m_length;
		}
		constexpr const_iterator cend() const noexcept
		{
			return cbegin() + m_length;
		}

		constexpr reverse_iterator rbegin() noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		constexpr const_reverse_iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		constexpr const_reverse_iterator crbegin() const noexcept
		{
			return std::make_reverse_iterator( cend() );
		}

		constexpr reverse_iterator rend() noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		constexpr const_reverse_iterator rend() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		constexpr const_reverse_iterator rcend() const noexcept
		{
			return std::make_reverse_iterator( cbegin() );
		}

		// Size

		constexpr bool empty() const noexcept
		{
			return m_length == 0;
		}

		constexpr size_type size() const noexcept
		{
			return m_length;
		}
		constexpr size_type length() const noexcept
		{
			return m_length;
		}

		constexpr size_type max_size() const noexcept
		{
			return max_string_size;
		}

		constexpr size_type capacity() const noexcept
		{
			return max_string_size;
		}

		constexpr void reserve( [[maybe_unused]] const size_type size ) noexcept
		{
			assert( size <= max_string_size );
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

		constexpr void resize( const size_type count, const value_type ch ) noexcept
		{
			assert( count <= max_string_size );
			if ( count == m_length )
			{
				return;
			}

			if ( count > m_length )
			{
				mclo::fill_n( end(), count - m_length, ch );
			}

			// Want to set length and terminate for trimming and growing, trimming just ignores existing characters and
			// inserts null
			m_length = count;
			null_terminate_end();
		}

		constexpr void resize( const size_type count ) noexcept
		{
			assert( count <= max_string_size );
			if ( count > m_length )
			{
				// We know we're filling null terminator so do it as part of the fill_n itself so do not call
				// null_terminate_end
				mclo::fill_n( end(), count - m_length + 1, value_type() );
				m_length = count;
			}
			else if ( count < m_length )
			{
				m_length = count;
				null_terminate_end();
			}
		}

		constexpr void push_back( const value_type c ) noexcept
		{
			assert( m_length < max_string_size );
			traits_type::assign( m_data[ m_length++ ], c );
			null_terminate_end();
		}

		constexpr void pop_back() noexcept
		{
			assert( m_length > 0 );
			--m_length;
			null_terminate_end();
		}

		constexpr void append( const view_type& string ) noexcept
		{
			assert( string.size() + m_length <= max_string_size );
			traits_copy( m_data.data() + m_length, string.data(), string.size() );
			m_length += string.size();
			null_terminate_end();
		}

		// Search

		// find, rfind, find_first/last_(not)of

		// Operations

		// substr

		template <std::size_t OtherSize>
		constexpr int compare( const basic_string_buffer<CharT, OtherSize>& str ) const noexcept
		{
			return compare( view_type( str ) );
		}

		template <std::size_t OtherSize>
		constexpr int compare( const value_type ( &str )[ OtherSize ] ) const noexcept
		{
			return compare( view_type( str ) );
		}

		constexpr int compare( const view_type& str ) const noexcept
		{
			return view_type( *this ).compare( str );
		}

#ifdef __cpp_lib_starts_ends_with
		constexpr bool starts_with( const view_type str ) const noexcept
		{
			return view_type( *this ).starts_with( str );
		}
		constexpr bool starts_with( const value_type ch ) const noexcept
		{
			return view_type( *this ).starts_with( ch );
		}
		constexpr bool starts_with( const const_pointer str ) const noexcept
		{
			return view_type( *this ).starts_with( str );
		}

		constexpr bool ends_with( const view_type str ) const noexcept
		{
			return view_type( *this ).ends_with( str );
		}
		constexpr bool ends_with( const value_type ch ) const noexcept
		{
			return view_type( *this ).ends_with( ch );
		}
		constexpr bool ends_with( const const_pointer str ) const noexcept
		{
			return view_type( *this ).ends_with( str );
		}
#endif

#ifdef __cpp_lib_string_contains
		constexpr bool contains( const view_type str ) const noexcept
		{
			return view_type( *this ).contains( str );
		}
		constexpr bool contains( const value_type ch ) const noexcept
		{
			return view_type( *this ).contains( ch );
		}
		constexpr bool contains( const const_pointer str ) const noexcept
		{
			return view_type( *this ).contains( str );
		}
#endif

	private:
		static constexpr void traits_copy( pointer dest, const const_pointer src, const size_type count ) noexcept
		{
			if ( mclo::is_constant_evaluated() )
			{
				mclo::copy( src, src + count, dest );
			}
			else
			{
				traits_type::copy( dest, src, count );
			}
		}
		static constexpr void traits_assign( pointer ptr, const size_type count, const value_type ch ) noexcept
		{
			if ( mclo::is_constant_evaluated() )
			{
				mclo::fill_n( ptr, count, ch );
			}
			else
			{
				traits_type::assign( ptr, count, ch );
			}
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

	namespace detail
	{
		template <typename T>
		constexpr std::size_t string_size( const T& object ) noexcept
		{
			return object.size();
		}
		template <typename CharT, std::size_t N>
		constexpr std::size_t string_size( const CharT ( & )[ N ] ) noexcept
		{
			return N - 1;
		}

		template <typename String>
		constexpr bool is_string_buffer = false;

		template <typename CharT, std::size_t N>
		constexpr bool is_string_buffer<mclo::basic_string_buffer<CharT, N>> = true;
	}

#define IMPLEMENT_COMPARISONS( OPERATOR )                                                                              \
	template <typename CharT, std::size_t Size, typename String>                                                       \
	[[nodiscard]] constexpr auto operator OPERATOR( const basic_string_buffer<CharT, Size>& lhs,                       \
													const String& rhs ) noexcept                                       \
	{                                                                                                                  \
		using view_type = typename basic_string_buffer<CharT, Size>::view_type;                                        \
		return view_type( lhs ) OPERATOR view_type( rhs );                                                             \
	}

	// != synthesized in C++20

	IMPLEMENT_COMPARISONS( == )

#ifdef __cpp_impl_three_way_comparison
	namespace detail
	{
		template <typename T>
		struct string_comp_category
		{
			using type = std::weak_ordering;
		};

		template <typename T>
			requires requires { typename T::comparison_category; }
		struct string_comp_category<T>
		{
			using type = T::comparison_category;
		};

		template <typename T>
		using string_comp_category_t = typename string_comp_category<T>::type;
	}

	IMPLEMENT_COMPARISONS( <=> )

#else

#define IMPLEMENT_REVERSED_COMPARISONS( OPERATOR )                                                                     \
	template <typename CharT, std::size_t Size, typename String>                                                       \
	[[nodiscard]] constexpr bool operator OPERATOR( const String& lhs,                                                 \
													const basic_string_buffer<CharT, Size>& rhs ) noexcept             \
	{                                                                                                                  \
		using view_type = typename basic_string_buffer<CharT, Size>::view_type;                                        \
		return view_type( lhs ) OPERATOR view_type( rhs );                                                             \
	}

	IMPLEMENT_REVERSED_COMPARISONS( == );
	IMPLEMENT_COMPARISONS( != )
	IMPLEMENT_REVERSED_COMPARISONS( != );
	IMPLEMENT_COMPARISONS( < )
	IMPLEMENT_REVERSED_COMPARISONS( < );
	IMPLEMENT_COMPARISONS( > )
	IMPLEMENT_REVERSED_COMPARISONS( > )
	IMPLEMENT_COMPARISONS( <= )
	IMPLEMENT_REVERSED_COMPARISONS( <= )
	IMPLEMENT_COMPARISONS( >= )
	IMPLEMENT_REVERSED_COMPARISONS( >= )

#undef IMPLEMENT_REVERSED_COMPARISONS
#endif

#undef IMPLEMENT_COMPARISONS

	template <typename CharT, std::size_t N>
	basic_string_buffer( const CharT ( & )[ N ] ) -> basic_string_buffer<CharT, N>;

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

#if __cpp_nontype_template_args > 201911L
	inline namespace literals
	{
		inline namespace string_buffer_literals
		{
			template <string_buffer str>
			[[nodiscard]] constexpr auto operator""_sb() noexcept
			{
				return str;
			}
			template <wstring_buffer str>
			[[nodiscard]] constexpr auto operator""_sb() noexcept
			{
				return str;
			}
#ifdef __cpp_char8_t
			template <u8string_buffer str>
			[[nodiscard]] constexpr auto operator""_sb() noexcept
			{
				return str;
			}
#endif
			template <u16string_buffer str>
			[[nodiscard]] constexpr auto operator""_sb() noexcept
			{
				return str;
			}
			template <u32string_buffer str>
			[[nodiscard]] constexpr auto operator""_sb() noexcept
			{
				return str;
			}
		}
	}
#endif

	template <typename CharT, std::size_t N>
	[[nodiscard]] constexpr basic_string_buffer<CharT, N> trandscode_ascii_literal( const char ( &str )[ N ] ) noexcept
	{
		basic_string_buffer<CharT, N> buffer( N - 1 );
		for ( std::size_t i = 0; i < N - 1; ++i )
		{
			const char ch = str[ i ];
			assert( mclo::is_ascii( ch ) );
			buffer[ i ] = static_cast<CharT>( ch );
		}
		return buffer;
	}
}

namespace std
{
	template <typename CharT, std::size_t Size>
	struct hash<mclo::basic_string_buffer<CharT, Size>>
	{
		using buffer_type = mclo::basic_string_buffer<CharT, Size>;
		using view_type = typename buffer_type::view_type;

		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR std::size_t operator()( const buffer_type& str )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return std::hash<view_type>()( view_type( str ) );
		}
	};

#ifdef __cpp_lib_format
	template <typename CharT, std::size_t Size>
	struct formatter<mclo::basic_string_buffer<CharT, Size>>
		: formatter<typename mclo::basic_string_buffer<CharT, Size>::view_type>
	{
	};
#endif
}
