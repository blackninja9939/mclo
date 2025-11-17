#include "mclo/debug/breakpoint.hpp"

#include "mclo/debug/debugger_attached.hpp"
#include "mclo/platform/compiler_detection.hpp"

void mclo::breakpoint() noexcept
{
#ifdef MCLO_COMPILER_MSVC
	__debugbreak();
#elif defined( MCLO_COMPILER_CLANG )
	__builtin_debugtrap();
#elif defined( MCLO_COMPILER_GCC )
	asm( "int3" );
#endif
}

void mclo::breakpoint_if_debugging() noexcept
{
	if ( is_debugger_attached() )
	{
		breakpoint();
	}
}
