#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include "assert_exception.hpp"
#include "mclo/debug/assert.hpp"

namespace
{
	[[noreturn]] void test_handler( const libassert::assertion_info& info )
	{
		const char* const message = info.message ? info.message->c_str() : nullptr;
		throw mclo::test_assert_exception( message );
	}
}

class assert_handler_listener : public Catch::EventListenerBase
{
public:
	using Catch::EventListenerBase::EventListenerBase;

	void testRunStarting( const Catch::TestRunInfo& ) override
	{
		libassert::set_failure_handler( test_handler );
	}
};

CATCH_REGISTER_LISTENER( assert_handler_listener )
