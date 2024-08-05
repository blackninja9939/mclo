#pragma once

#include <xsimd/xsimd.hpp>

namespace mclo::detail
{
	using char_batch = xsimd::batch<char>;

	[[nodiscard]] char_batch to_upper_simd( const char* ptr ) noexcept;
	[[nodiscard]] char_batch to_lower_simd( const char* ptr ) noexcept;
}
