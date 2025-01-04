#include <catch2/catch_test_macros.hpp>

#include "consteval_check.h"

#include "mclo/hash/hash.hpp"
#include "mclo/utility/sem_version.hpp"

namespace
{
	constexpr mclo::sem_version test_version{ 1, 1, 0 };
}

TEST_CASE( "sem_version satisfies", "[sem_version]" )
{
	CONSTEVAL_CHECK( test_version.satisfies( { 1, 1, 0 } ) );
	CONSTEVAL_CHECK( test_version.satisfies( { 1, 0, 0 } ) );
	CONSTEVAL_CHECK( test_version.satisfies( { 1, 0, 1 } ) );
	CONSTEVAL_CHECK_FALSE( test_version.satisfies( { 1, 2, 0 } ) );
	CONSTEVAL_CHECK_FALSE( test_version.satisfies( { 2, 0, 0 } ) );
	CONSTEVAL_CHECK_FALSE( test_version.satisfies( { 0, 0, 0 } ) );
	CONSTEVAL_CHECK_FALSE( test_version.satisfies( { 0, 0, 1 } ) );
	CONSTEVAL_CHECK_FALSE( test_version.satisfies( { 0, 1, 0 } ) );
	CONSTEVAL_CHECK_FALSE( test_version.satisfies( { 2, 0, 1 } ) );
}

TEST_CASE( "sem_version comparison", "[sem_version]" )
{
	CONSTEVAL_CHECK( test_version == test_version );
	CONSTEVAL_CHECK( test_version <= test_version );
	CONSTEVAL_CHECK( test_version >= test_version );
	CONSTEVAL_CHECK_FALSE( test_version != test_version );
	CONSTEVAL_CHECK_FALSE( test_version < test_version );
	CONSTEVAL_CHECK_FALSE( test_version > test_version );
	CONSTEVAL_CHECK( test_version == ( mclo::sem_version{ 1, 1, 0 } ) );
	CONSTEVAL_CHECK( test_version <= ( mclo::sem_version{ 1, 1, 0 } ) );
	CONSTEVAL_CHECK( test_version >= ( mclo::sem_version{ 1, 1, 0 } ) );
	CONSTEVAL_CHECK_FALSE( test_version != ( mclo::sem_version{ 1, 1, 0 } ) );
	CONSTEVAL_CHECK_FALSE( test_version < ( mclo::sem_version{ 1, 1, 0 } ) );
	CONSTEVAL_CHECK_FALSE( test_version > ( mclo::sem_version{ 1, 1, 0 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 1, 0, 0 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 1, 0, 1 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 1, 2, 0 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 2, 0, 0 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 0, 0, 0 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 0, 0, 1 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 0, 1, 0 } ) );
	CONSTEVAL_CHECK( test_version != ( mclo::sem_version{ 2, 0, 1 } ) );
	CONSTEVAL_CHECK( test_version < ( mclo::sem_version{ 1, 1, 1 } ) );
	CONSTEVAL_CHECK( test_version < ( mclo::sem_version{ 1, 2, 0 } ) );
	CONSTEVAL_CHECK( test_version < ( mclo::sem_version{ 2, 0, 0 } ) );
}

TEST_CASE( "sem_version hash", "[sem_version]" )
{
	const std::size_t hash = mclo::hash( test_version );
	CHECK( hash != 0 );
	CHECK( hash != test_version.major );
	CHECK( hash != test_version.minor );
	CHECK( hash != test_version.patch );
	CHECK( hash != test_version.major + test_version.minor + test_version.patch );
}
