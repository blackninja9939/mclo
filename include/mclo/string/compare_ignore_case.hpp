#pragma once

#include "mclo/preprocessor/platform.hpp"
#include "mclo/string/ascii_string_utils.hpp"
#include "mclo/string/string_view_type.hpp"

#include <string_view>

namespace mclo
{
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
			if ( std::is_constant_evaluated() )
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
}
