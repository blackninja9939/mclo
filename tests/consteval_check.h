#pragma once

#include <catch2/catch_test_macros.hpp>

#define CONSTEVAL_CHECK( EXPR )                                                                                        \
	STATIC_CHECK( EXPR );                                                                                              \
	CHECK( EXPR )

#define CONSTEVAL_CHECK_FALSE( EXPR )                                                                                  \
	STATIC_CHECK_FALSE( EXPR );                                                                                        \
	CHECK_FALSE( EXPR )
