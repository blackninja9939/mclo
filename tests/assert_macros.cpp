#pragma once

#include "assert_macros.hpp"

#include <fmt/format.h>
#include <string_view>

mclo::assert_exception_matcher::assert_exception_matcher( std::string message )
	: message( std::move( message ) )
{
}

bool mclo::assert_exception_matcher::match( const test_assert_exception& in ) const
{
	const std::string_view in_message( in.what() );
	return in_message.find( message ) != std::string_view::npos;
}

std::string mclo::assert_exception_matcher::describe() const
{
	return fmt::format( "assert message contains '{}'", message );
}
