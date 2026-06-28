#include "mclo/debug/assert.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace mclo::detail
{
	[[noreturn]] void assert_failed( const char* const expression,
									 const char* const file,
									 const unsigned line,
									 const char* const message,
									 const char* const formatted_args ) noexcept
	{
		assert( expression && file && line > 0 );

		std::fprintf( stderr, "Assertion failed: %s, file: %s, line: %u\n", expression, file, line );
		if ( message )
		{
			std::fprintf( stderr, "Message: %s\n", message );
		}
		if ( formatted_args )
		{
			std::fputs( formatted_args, stderr );
		}

		std::fflush( stderr );
		std::abort();
	}
}
