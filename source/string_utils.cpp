#include <mclo/string_utils.hpp>

#include <mclo/bit.hpp>

#include <cwctype>

#include <xsimd/xsimd.hpp>

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
	using char_batch = xsimd::batch<char>;

	[[nodiscard]] char_batch to_upper_simd( const char* ptr ) noexcept
	{
		//// SSE has no unsigned comparisons, so we must do the unsigned trick by shifting the chars
		//// into -128 to -128 + ('a' - 'z' )

		const char_batch value = char_batch::load_unaligned( ptr );
		const char_batch shifted = value - char_batch( 'a' - 128 );

		// 0 = lower case, -1 = anything else
		const char_batch nomodify = shifted > char_batch( -128 + 'z' - 'a' );

		const char_batch flip = xsimd::bitwise_andnot( char_batch( 0x20 ), nomodify );

		//// just mask the XOR-mask so elements are XORed with 0 instead of 0x20
		//// XOR's identity value is 0, same as addition's
		return xsimd::bitwise_xor( value, flip );
	}
}

int mclo::detail::compare_ignore_case_simd( const char* lhs, const char* rhs, std::size_t size ) noexcept
{
	while ( size >= char_batch::size )
	{
		const char_batch lhs_data = to_upper_simd( lhs );
		const char_batch rhs_data = to_upper_simd( rhs );
		lhs += char_batch::size;
		rhs += char_batch::size;
		size -= char_batch::size;

		const char_batch difference = lhs_data - rhs_data;
		const char_batch::batch_bool_type result = difference == 0;
		const auto mask = result.mask();
		const int first_not_equal = mclo::countr_one( mask );

		if ( first_not_equal != char_batch::size )
		{
			const char_batch shuffled =
				xsimd::swizzle( difference, char_batch( static_cast<char>( first_not_equal ) ) );
			return static_cast<signed char>( _mm_extract_epi8( shuffled, 0 ) );
		}
	}

	return detail::compare_ignore_case_scalar( lhs, rhs, size );
}
