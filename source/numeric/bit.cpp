#include "mclo/numeric/bit.hpp"

#include "mclo/platform/compiler_detection.hpp"

#ifndef MCLO_COMPILER_MSVC
#include <cpuid.h>
#endif

namespace
{
	const bool has_bmi2_support = [] {
#ifdef MCLO_COMPILER_MSVC
		int regs[ 4 ];
		__cpuidex( regs, 7, 0 );
		return ( regs[ 1 ] & ( 1 << 8 ) ) != 0; // EBX bit 8
#else
		unsigned eax, ebx, ecx, edx;
		if ( __get_cpuid_count( 7, 0, &eax, &ebx, &ecx, &edx ) )
		{
			return ( ebx & ( 1 << 8 ) ) != 0;
		}
		return false;
#endif
	}();
}

bool mclo::detail::has_bmi2() noexcept
{
	return has_bmi2_support;
}
