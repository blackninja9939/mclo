#include "mclo/platform/api_export.hpp"

// A minimal shared library used by the shared_library unit tests. Symbols are exported with C linkage so they
// can be looked up by their plain, unmangled names.
extern "C"
{
	MCLO_API_EXPORT int mclo_test_add( const int lhs, const int rhs )
	{
		return lhs + rhs;
	}

	MCLO_API_EXPORT int mclo_test_value = 42;
}
