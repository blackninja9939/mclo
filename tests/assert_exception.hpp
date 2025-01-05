#pragma once

#include <exception>

namespace mclo
{
	struct test_assert_exception final : public std::exception
	{
		using std::exception::exception;
	};
}
