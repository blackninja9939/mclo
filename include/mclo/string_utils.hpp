#pragma once

#include "algorithm.hpp"
#include "ascii_string_utils.hpp"
#include "fnva1.hpp"
#include "numeric.hpp"
#include "string_buffer.hpp"
#include "type_traits.hpp"

#include <array>
#include <charconv>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mclo
{
	template <typename String>
	struct string_character
	{
		using type = typename String::value_type;
	};

	template <typename CharT>
	struct string_character<const CharT*>
	{
		using type = CharT;
	};
	template <typename CharT, std::size_t N>
	struct string_character<const CharT[ N ]>
	{
		using type = CharT;
	};
	template <typename CharT, std::size_t N>
	struct string_character<CharT[ N ]>
	{
		using type = CharT;
	};

	template <typename String>
	using string_character_t = typename string_character<String>::type;

	template <typename String>
	using string_view_type_t = std::basic_string_view<string_character_t<String>>;

	template <typename T, typename... FromCharArgs>
	[[nodiscard]] std::optional<T> from_string( const std::string_view str, FromCharArgs&&... args ) noexcept
	{
		T value;
		const std::from_chars_result result =
			std::from_chars( str.data(), str.data() + str.size(), value, std::forward<FromCharArgs>( args )... );

		if ( result.ec == std::errc{} )
		{
			return value;
		}
		return {};
	}

	template <typename T, typename... ToCharArgs>
	[[nodiscard]] std::string_view to_string( char* const first,
											  char* const last,
											  const T value,
											  ToCharArgs&&... args ) noexcept
	{
		const auto [ ptr, ec ] = std::to_chars( first, last, value, std::forward<ToCharArgs>( args )... );

		if ( ec == std::errc{} )
		{
			return std::string_view( first, ptr - first );
		}
		return {};
	}

	template <std::size_t BufferSize, typename T, typename... ToCharArgs>
	[[nodiscard]] std::string_view to_string( std::array<char, BufferSize>& buffer,
											  const T value,
											  ToCharArgs&&... args ) noexcept
	{
		return to_string( buffer.data(), buffer.data() + BufferSize, value, std::forward<ToCharArgs>( args )... );
	}

	template <std::size_t BufferSize, typename T, typename... ToCharArgs>
	[[nodiscard]] std::string_view to_string( char ( &buffer )[ BufferSize ],
											  const T value,
											  ToCharArgs&&... args ) noexcept
	{
		return to_string( std::begin( buffer ), std::end( buffer ), value, std::forward<ToCharArgs>( args )... );
	}

	template <typename CharT>
	constexpr auto whitespace_characters_v = trandscode_ascii_literal<CharT>( " \n\f\t\r\v" );

	template <typename CharT>
	constexpr auto numeric_characters_v = trandscode_ascii_literal<CharT>( "0123456789" );

	template <typename CharT>
	constexpr auto uppercase_characters_v = trandscode_ascii_literal<CharT>( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );

	template <typename CharT>
	constexpr auto lowercase_characters_v = trandscode_ascii_literal<CharT>( "abcdefgihjklmnopqrstuvwxyz" );

	template <typename CharT>
	constexpr auto alphabet_characters_v =
		trandscode_ascii_literal<CharT>( "abcdefgihjklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" );

	template <typename CharT>
	constexpr auto alphanumeric_characters_v =
		trandscode_ascii_literal<CharT>( "abcdefgihjklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" );

	inline constexpr auto whitespace_characters = whitespace_characters_v<char>;
	inline constexpr auto numeric_characters = numeric_characters_v<char>;
	inline constexpr auto uppercase_characters = uppercase_characters_v<char>;
	inline constexpr auto lowercase_characters = lowercase_characters_v<char>;
	inline constexpr auto alphabet_characters = alphabet_characters_v<char>;
	inline constexpr auto alphanumeric_characters = alphanumeric_characters_v<char>;

	template <typename String>
	[[nodiscard]] constexpr string_view_type_t<String> trim_front(
		const String& string,
		const string_view_type_t<String> to_trim = whitespace_characters_v<string_character_t<String>> ) noexcept
	{
		const string_view_type_t<String> view{ string };
		const auto start = view.find_first_not_of( to_trim );
		if ( start == string_view_type_t<String>::npos )
		{
			return {};
		}
		return view.substr( start );
	}

	template <typename String>
	[[nodiscard]] constexpr string_view_type_t<String> trim_back(
		const String& string,
		const string_view_type_t<String> to_trim = whitespace_characters_v<string_character_t<String>> ) noexcept
	{
		const string_view_type_t<String> view{ string };
		const auto end = view.find_last_not_of( to_trim );
		if ( end == string_view_type_t<String>::npos )
		{
			return {};
		}
		return view.substr( 0, end + 1 );
	}

	template <typename String>
	[[nodiscard]] constexpr string_view_type_t<String> trim(
		const String& string,
		const string_view_type_t<String> to_trim = whitespace_characters_v<string_character_t<String>> ) noexcept
	{
		const string_view_type_t<String> view{ string };
		auto start = view.find_first_not_of( to_trim );
		if ( start == string_view_type_t<String>::npos )
		{
			return {};
		}

		const auto end = view.find_last_not_of( to_trim );
		if ( end == string_view_type_t<String>::npos )
		{
			return view.substr( start );
		}

		return view.substr( start, end + 1 - start );
	}

	namespace detail
	{
		[[nodiscard]] int compare_ignore_case_simd( const char* lhs, const char* rhs, std::size_t size ) noexcept;
		
		[[nodiscard]] constexpr int compare_ignore_case_scalar( const char* lhs,
																const char* rhs,
																std::size_t size ) noexcept
		{
			while ( size-- > 0 )
			{
				const char lhs_char = to_lower( *lhs++ );
				const char rhs_char = to_lower( *rhs++ );
				const int result = lhs_char - rhs_char;
				if ( result != 0 )
				{
					return result;
				}
			}
			return 0;
		}

		[[nodiscard]] constexpr int compare_ignore_case( const char* lhs, const char* rhs, std::size_t size ) noexcept
		{
			if ( mclo::is_constant_evaluated() )
			{
				return compare_ignore_case_scalar( lhs, rhs, size );
			}
			else
			{
				return compare_ignore_case_simd( lhs, rhs, size );
			}
		}
	}

	[[nodiscard]] constexpr int compare_ignore_case( const std::string_view lhs, const std::string_view rhs ) noexcept
	{
		const std::size_t lhs_size = lhs.size();
		const std::size_t rhs_size = rhs.size();

		const int result = detail::compare_ignore_case( lhs.data(), rhs.data(), std::min( lhs_size, rhs_size ) );
		if ( result != 0 )
		{
			return result;
		}

		if ( lhs_size < rhs_size )
		{
			return -1;
		}
		if ( lhs_size > rhs_size )
		{
			return 1;
		}
		return 0;
	}

	template <typename CharT>
	void replace_all( std::basic_string<CharT>& string,
					  const std::basic_string_view<type_identity_t<CharT>> find,
					  const std::basic_string_view<type_identity_t<CharT>> replace ) noexcept
	{
		const std::size_t find_size = find.size();
		const std::size_t replace_size = replace.size();
		std::size_t index = 0;
		for ( ;; )
		{
			index = string.find( find, index );
			if ( index == std::basic_string<CharT>::npos )
			{
				break;
			}
			string.replace( index, find_size, replace );
			index += replace_size;
		}
	}

	namespace detail
	{
		template <typename String, typename... Strings>
		[[nodiscard]] constexpr String join_string_views( const Strings&... strings )
		{
			const std::size_t size = ( std::size( strings ) + ... );
			String result;
			result.reserve( size );
			( result.append( std::data( strings ), std::size( strings ) ), ... );
			return result;
		}

		template <typename String, typename It, typename StringLength>
		[[nodiscard]] constexpr String reserve_string_for( It first, It last, StringLength string_length )
		{
			String result;

			using iterator_category = typename std::iterator_traits<It>::iterator_category;
			if constexpr ( std::is_base_of_v<std::forward_iterator_tag, iterator_category> )
			{
				const std::size_t size = mclo::accumulate(
					first, last, std::size_t( 0 ), [ &string_length ]( const std::size_t current, const auto& string ) {
						return current + string_length( string );
					} );

				result.reserve( size );
			}

			return result;
		}

		template <typename String, typename It, typename StringLength>
		[[nodiscard]] constexpr String join_string_iterators( It first, It last, StringLength string_length )
		{
			using value_type = typename std::iterator_traits<It>::value_type;
			using string_value_type = typename String::value_type;

			String result = reserve_string_for<String>( first, last, string_length );
			while ( first != last )
			{
				if constexpr ( std::is_same_v<value_type, string_value_type> )
				{
					result.push_back( *first++ );
				}
				else
				{
					result.append( *first++ );
				}
			}
			return result;
		}

		template <typename String, typename It>
		constexpr bool is_iterators_over_literals =
			std::is_same_v<typename std::iterator_traits<It>::value_type, const string_character_t<String>*>;

		inline constexpr std::size_t default_literal_size_buffer_max = 16;
	}

	template <typename String = std::string,
			  std::size_t literal_size_buffer_max = detail::default_literal_size_buffer_max,
			  typename It,
			  typename = std::enable_if_t<detail::is_iterators_over_literals<String, It>>>
	[[nodiscard]] constexpr String join_string( It first, It last )
	{
		using string_value_type = typename String::value_type;
		using traits_type = std::char_traits<string_value_type>;
		using iterator_category = typename std::iterator_traits<It>::iterator_category;

		if constexpr ( std::is_base_of_v<std::forward_iterator_tag, iterator_category> )
		{
			if ( std::distance( first, last ) > literal_size_buffer_max )
			{
				return detail::join_string_iterators<String>(
					first, last, []( const auto& string ) { return traits_type::length( string ); } );
			}

			std::array<std::basic_string_view<string_value_type>, literal_size_buffer_max> sizes{};
			auto sizes_it = sizes.begin();
			while ( first != last )
			{
				*sizes_it++ = *first++;
			}

			return detail::join_string_iterators<String>(
				sizes.begin(), sizes_it, []( const auto& string ) { return std::size( string ); } );
		}
		else
		{
			return detail::join_string_iterators<String>(
				first, last, []( const auto& string ) { return traits_type::length( string ); } );
		}
	}

	template <typename String = std::string,
			  typename It,
			  typename = std::enable_if_t<!detail::is_iterators_over_literals<String, It>>>
	[[nodiscard]] constexpr String join_string( It first, It last )
	{
		return detail::join_string_iterators<String>(
			first, last, []( const auto& string ) { return std::size( string ); } );
	}

	template <
		typename String = std::string,
		std::size_t literal_size_buffer_max = detail::default_literal_size_buffer_max,
		typename Container,
		typename = std::enable_if_t<detail::is_iterators_over_literals<String, typename Container::const_iterator>>>
	[[nodiscard]] constexpr String join_string( const Container& strings )
	{
		return join_string<String, literal_size_buffer_max>( std::begin( strings ), std::end( strings ) );
	}

	template <
		typename String = std::string,
		typename Container,
		typename = std::enable_if_t<!detail::is_iterators_over_literals<String, typename Container::const_iterator>>>
	[[nodiscard]] constexpr String join_string( const Container& strings )
	{
		return join_string<String>( std::begin( strings ), std::end( strings ) );
	}

	template <typename String = std::string, typename... Strings>
	[[nodiscard]] constexpr String join_string( const Strings&... strings )
	{
		return detail::join_string_views<String>( std::string_view( strings )... );
	}

	template <typename String>
	[[nodiscard]] constexpr std::size_t string_hash( const String& string ) noexcept
	{
		// Necessary so literals do not include null terminator and so char*'s calculate their length
		const string_view_type_t<String> view{ string };
		return mclo::fnv1a( view.data(), view.size() );
	}

	template <typename String>
	[[nodiscard]] constexpr std::size_t string_hash_ignore_case( const String& string ) noexcept
	{
		const string_view_type_t<String> view{ string };
		return mclo::fnv1a( view.data(), view.size(), static_cast<char ( * )( char ) noexcept>( to_lower ) );
	}

	struct string_hash_t
	{
		template <typename String>
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const String& string )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::string_hash( string );
		}
	};

	struct string_hash_ignore_case_t
	{
		template <typename String>
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const String& string )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::string_hash_ignore_case( string );
		}
	};

	namespace detail
	{
		template <typename BaseOp>
		struct string_compare_ignore_case_t
		{
			[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()(
				const std::string_view lhs, const std::string_view rhs ) MCLO_CONST_CALL_OPERATOR noexcept
			{
				return BaseOp{}( mclo::compare_ignore_case( lhs, rhs ), 0 );
			}
		};
	}

	using string_equal_to_ignore_case = detail::string_compare_ignore_case_t<std::equal_to<>>;
	using string_not_equal_to_ignore_case = detail::string_compare_ignore_case_t<std::not_equal_to<>>;
	using string_less_ignore_case = detail::string_compare_ignore_case_t<std::less<>>;
	using string_greater_ignore_case = detail::string_compare_ignore_case_t<std::greater<>>;
	using string_less_equal_ignore_case = detail::string_compare_ignore_case_t<std::less_equal<>>;
	using string_greater_equal_ignore_case = detail::string_compare_ignore_case_t<std::greater_equal<>>;

	void to_upper( std::wstring& string ) noexcept;
	void to_lower( std::wstring& string ) noexcept;
}
