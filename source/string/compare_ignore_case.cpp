#include "mclo/string/compare_ignore_case.hpp"

#include "ascii_string_simd.hpp"

#include <bit>

int mclo::detail::compare_ignore_case_simd( const char* lhs, const char* rhs, std::size_t size ) noexcept
{
	while ( size >= char_batch::size )
	{
		const char_batch lhs_data = to_upper_simd( lhs );
		const char_batch rhs_data = to_upper_simd( rhs );

		const char_batch difference = lhs_data - rhs_data;
		const char_batch::batch_bool_type result = difference == 0;
		const auto mask = result.mask();
		const int first_not_equal = std::countr_one( mask );

		if ( first_not_equal != char_batch::size )
		{
			return difference.get( first_not_equal );
		}

		lhs += char_batch::size;
		rhs += char_batch::size;
		size -= char_batch::size;
	}

	return detail::compare_ignore_case_scalar( lhs, rhs, size );
}
