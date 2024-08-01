#include <mclo/string_utils.hpp>

#include <mclo/bit.hpp>

#include <cwctype>

#include <immintrin.h>

namespace mclo
{
	void to_upper( std::wstring& string ) noexcept
	{
		for ( wchar_t& c : string )
		{
			c = std::towupper( c );
		}
	}

	void to_lower( std::wstring& string ) noexcept
	{
		for ( wchar_t& c : string )
		{
			c = std::towlower( c );
		}
	}
}

namespace
{
	[[nodiscard]] __m128i to_upper_simd( const char* ptr ) noexcept
	{
		// SSE has no unsigned comparisons, so we must do the unsigned trick by shifting the chars
		// into -128 to -128 + ('a' - 'z' )
		const __m128i value = _mm_loadu_epi8( ptr );
		const __m128i shifted = _mm_sub_epi8( value, _mm_set1_epi8( 'a' - 128 ) );

		// 0 = lower case, -1 = anything else
		const __m128i nomodify = _mm_cmpgt_epi8( shifted, _mm_set1_epi8( -128 + 'z' - 'a' ) );

		const __m128i flip = _mm_andnot_si128( nomodify, _mm_set1_epi8( 0x20 ) ); // 0x20:lcase    0:non-lcase

		// just mask the XOR-mask so elements are XORed with 0 instead of 0x20
		// XOR's identity value is 0, same as addition's
		return _mm_xor_si128( value, flip );
	}
}

int mclo::detail::compare_ignore_case_simd( const char* lhs, const char* rhs, std::size_t size ) noexcept
{
	static constexpr std::size_t simd_size = sizeof( __m128 ) / sizeof( char );

	while ( size >= simd_size )
	{
		const __m128i lhs_data = to_upper_simd( lhs );
		const __m128i rhs_data = to_upper_simd( rhs );
		lhs += simd_size;
		rhs += simd_size;
		size -= simd_size;

		const __m128i difference = _mm_sub_epi8( lhs_data, rhs_data );
		const __m128i result = _mm_cmpeq_epi8( difference, _mm_setzero_si128() );
		const int mask = _mm_movemask_epi8( result );
		const int first_not_equal = mclo::countr_one( mask );

		if ( first_not_equal != 16 )
		{
			const __m128i shuffled =
				_mm_shuffle_epi8( difference, _mm_set1_epi8( static_cast<char>( first_not_equal ) ) );
			return static_cast<signed char>( _mm_extract_epi8( shuffled, 0 ) );
		}
	}

	return detail::compare_ignore_case_scalar( lhs, rhs, size );
}
