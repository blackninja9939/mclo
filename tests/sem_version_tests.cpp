#include <catch2/catch_test_macros.hpp>

#include "consteval_check.hpp"

#include "mclo/hash/hash.hpp"
#include "mclo/random/default_random_generator.hpp"
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
	const std::size_t hash = mclo::hash_object( test_version );
	CHECK( hash != 0 );
	CHECK( hash != test_version.major );
	CHECK( hash != test_version.minor );
	CHECK( hash != test_version.patch );
	CHECK( hash != test_version.major + test_version.minor + test_version.patch );
}

TEST_CASE( "sem_version to_string", "[sem_version]" )
{
	mclo::default_random_generator rng( 42 );
	for ( int i = 0; i < 100; ++i )
	{
		const auto major = static_cast<std::uint8_t>( rng.uniform( 0, 255 ) );
		const auto minor = static_cast<std::uint8_t>( rng.uniform( 0, 255 ) );
		const auto patch = static_cast<std::uint8_t>( rng.uniform( 0, 255 ) );
		const std::string version_str = mclo::sem_version{ major, minor, patch }.to_string();
		CHECK( version_str == std::to_string( major ) + "." + std::to_string( minor ) + "." + std::to_string( patch ) );
	}
}
