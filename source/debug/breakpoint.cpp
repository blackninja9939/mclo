#include "mclo/debug/breakpoint.hpp"

#include "mclo/debug/debugger_attached.hpp"

void mclo::breakpoint() noexcept
{
#ifdef _WIN32
	__debugbreak();
#elif defined( __clang__ )
	__builtin_debugtrap();
#elif defined( __GNUC__ )
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
