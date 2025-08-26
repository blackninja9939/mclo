#include "mclo/numeric/bit.hpp"

#ifndef _MSC_VER
#include <cpuid.h>
#endif

namespace
{
	const bool has_bmi2_support = [] {
#ifdef _MSC_VER
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
