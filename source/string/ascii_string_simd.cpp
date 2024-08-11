#include "mclo/string/detail/ascii_string_simd.hpp"

namespace
{
	using mclo::detail::char_batch;

	template <bool Upper>
	char_batch convert_case_simd( const char* ptr ) noexcept
	{
		//// SSE has no unsigned comparisons, so we must do the unsigned trick by shifting the chars
		//// into -128 to -128 + ('a' - 'z' )

		static constexpr char start_char = Upper ? 'a' : 'A';
		static constexpr char end_char = Upper ? 'z' : 'Z';

		const char_batch value = char_batch::load_unaligned( ptr );
		const char_batch shifted = value - char_batch( start_char - 128 );

		// 0 = lower case, -1 = anything else
		const char_batch nomodify = xsimd::bitwise_cast( shifted > char_batch( -128 + end_char - start_char ) );

		const char_batch flip = xsimd::bitwise_andnot( char_batch( 0x20 ), nomodify );

		//// just mask the XOR-mask so elements are XORed with 0 instead of 0x20
		//// XOR's identity value is 0, same as addition's
		return xsimd::bitwise_xor( value, flip );
	}
}

namespace mclo::detail
{
	char_batch to_upper_simd( const char* ptr ) noexcept
	{
		return convert_case_simd<true>( ptr );
	}

	char_batch to_lower_simd( const char* ptr ) noexcept
	{
		return convert_case_simd<false>( ptr );
	}
}
