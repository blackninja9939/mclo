#include <catch2/catch_test_macros.hpp>

#include "consteval_check.hpp"

#include "mclo/random/seed_mixing.hpp"

#include <cstdint>

TEST_CASE( "avalanche_bits of zero is zero", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::avalanche_bits( 0 ) == 0 );
}

TEST_CASE( "avalanche_bits produces its known diffused output", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::avalanche_bits( UINT64_C( 11400714819324433052 ) ) == UINT64_C( 6457827717110365317 ) );
}

TEST_CASE( "avalanche_bits of distinct inputs are distinct", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::avalanche_bits( 1 ) != mclo::avalanche_bits( 2 ) );
	CONSTEVAL_CHECK( mclo::avalanche_bits( 1 ) != mclo::avalanche_bits( 0 ) );
	CONSTEVAL_CHECK( mclo::avalanche_bits( 100 ) != mclo::avalanche_bits( 101 ) );
}

TEST_CASE( "mix_seed of all-zero positions is non-zero", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::mix_seed( 0 ) != 0 );
	CONSTEVAL_CHECK( mclo::mix_seed( 0, 0 ) != 0 );
	CONSTEVAL_CHECK( mclo::mix_seed( 0, 0, 0 ) != 0 );
}

TEST_CASE( "mix_seed is deterministic", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::mix_seed( 7, 42, 99 ) == mclo::mix_seed( 7, 42, 99 ) );
}

TEST_CASE( "mix_seed is sensitive to position order", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::mix_seed( 1, 2 ) != mclo::mix_seed( 2, 1 ) );
	CONSTEVAL_CHECK( mclo::mix_seed( 3, 5, 7 ) != mclo::mix_seed( 7, 5, 3 ) );
}

TEST_CASE( "mix_seed decorrelates adjacent positions", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::mix_seed( 0, 0 ) != mclo::mix_seed( 0, 1 ) );
	CONSTEVAL_CHECK( mclo::mix_seed( 0, 0 ) != mclo::mix_seed( 1, 0 ) );
	CONSTEVAL_CHECK( mclo::mix_seed( 1, 0 ) != mclo::mix_seed( 0, 1 ) );
}

TEST_CASE( "mix_seed arity changes the seed", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::mix_seed( 5 ) != mclo::mix_seed( 5, 0 ) );
	CONSTEVAL_CHECK( mclo::mix_seed( 5, 0 ) != mclo::mix_seed( 5, 0, 0 ) );
}

TEST_CASE( "mix_seed accepts signed and mixed integral positions", "[random][seed_mixing]" )
{
	CONSTEVAL_CHECK( mclo::mix_seed( -1 ) == mclo::mix_seed( -1 ) );
	CONSTEVAL_CHECK( mclo::mix_seed( -1 ) != mclo::mix_seed( 1 ) );
	CONSTEVAL_CHECK( mclo::mix_seed( std::int32_t{ 2 }, std::uint64_t{ 3 } ) ==
					 mclo::mix_seed( std::int32_t{ 2 }, std::uint64_t{ 3 } ) );
}
