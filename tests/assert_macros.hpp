#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "assert_exception.hpp"

#include <string>

namespace mclo
{
	class assert_exception_matcher final : public Catch::Matchers::MatcherBase<test_assert_exception>
	{
	public:
		assert_exception_matcher( std::string message );
		bool match( const test_assert_exception& in ) const override;
		std::string describe() const override;

	private:
		std::string message;
	};
}

// clang-format off
#define CHECK_ASSERTS( EXPRESSION, MESSAGE ) CHECK_THROWS_MATCHES( EXPRESSION, mclo::test_assert_exception, mclo::assert_exception_matcher( MESSAGE ) )
#define REQUIRE_ASSERTS( EXPRESSION, MESSAGE ) REQUIRE_THROWS_MATCHES( EXPRESSION, mclo::test_assert_exception, mclo::assert_exception_matcher( MESSAGE ) )
// clang-format on
