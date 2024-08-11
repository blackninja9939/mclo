#pragma once

#include "mclo/string/ascii_string_utils.hpp"
#include "mclo/math.hpp"
#include "mclo/string/detail/ascii_string_simd.hpp"

namespace mclo::detail
{
	void to_upper_simd( char* first, char* const last ) noexcept
	{
		const char* const simd_last =
			first + mclo::round_down_to_multiple_of<std::size_t>( last - first, char_batch::size );
		for ( ; first != simd_last; first += char_batch::size )
		{
			const char_batch upper = to_upper_simd( first );
			upper.store_unaligned( first );
		}

		return to_upper_scalar( first, last );
	}

	void to_lower_simd( char* first, char* const last ) noexcept
	{
		const char* const simd_last =
			first + mclo::round_down_to_multiple_of<std::size_t>( last - first, char_batch::size );

		for ( ; first != simd_last; first += char_batch::size )
		{
			const char_batch lower = to_lower_simd( first );
			lower.store_unaligned( first );
		}

		return to_lower_scalar( first, last );
	}
}
