#pragma once

#include "assert_macros.hpp"

#include <format>

mclo::assert_exception_matcher::assert_exception_matcher( std::string message )
	: message( std::move( message ) )
{
}

bool mclo::assert_exception_matcher::match( const test_assert_exception& in ) const
{
	return message == in.what();
}

std::string mclo::assert_exception_matcher::describe() const
{
	return std::format( "exception message is '{}'", message );
}
