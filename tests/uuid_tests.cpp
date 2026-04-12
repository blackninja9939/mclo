#include <catch2/catch_test_macros.hpp>

#include <mclo/utility/array.hpp>
#include <mclo/utility/uuid.hpp>

#include "assert_macros.hpp"
#include "consteval_check.hpp"

#include <random>

namespace
{
	constexpr std::array<std::byte, 16> known_bytes = {
		std::byte{ 0x12 },
		std::byte{ 0x34 },
		std::byte{ 0x56 },
		std::byte{ 0x78 },
		std::byte{ 0x9A },
		std::byte{ 0xBC },
		std::byte{ 0xDE },
		std::byte{ 0xF0 },
		std::byte{ 0x12 },
		std::byte{ 0x34 },
		std::byte{ 0x56 },
		std::byte{ 0x78 },
		std::byte{ 0x9A },
		std::byte{ 0xBC },
		std::byte{ 0xDE },
		std::byte{ 0xF0 },
	};
}

TEST_CASE( "default constructed uuid, all bytes are zero", "[uuid]" )
{
	constexpr mclo::uuid id;
	constexpr mclo::uuid zero_uuid{ mclo::broadcast_array<16>( std::byte{ 0 } ) };

	STATIC_CHECK( id == zero_uuid );
	for ( const auto byte : id.bytes )
	{
		CHECK( byte == std::byte{ 0 } );
	}
}

TEST_CASE( "uuid from bytes, to_string, returns expected format", "[uuid]" )
{
	static constexpr mclo::uuid id( known_bytes );

	const std::string str = id.to_string();

	CHECK( str == "12345678-9abc-def0-1234-56789abcdef0" );
}

TEST_CASE( "uuid from lowercase string, equals uuid from bytes", "[uuid]" )
{
	static constexpr mclo::uuid from_str( "12345678-9abc-def0-1234-56789abcdef0" );
	static constexpr mclo::uuid from_bytes( known_bytes );

	CONSTEVAL_CHECK( from_str == from_bytes );
}

TEST_CASE( "uuid from uppercase string, equals uuid from bytes", "[uuid]" )
{
	static constexpr mclo::uuid from_str( "12345678-9ABC-DEF0-1234-56789ABCDEF0" );
	static constexpr mclo::uuid from_bytes( known_bytes );

	CONSTEVAL_CHECK( from_str == from_bytes );
}

TEST_CASE( "uuid from string round-trip, to_string matches original", "[uuid]" )
{
	static constexpr mclo::uuid id( "550e8400-e29b-41d4-a716-446655440000" );

	const std::string str = id.to_string();

	CHECK( str == "550e8400-e29b-41d4-a716-446655440000" );
}

TEST_CASE( "uuid from invalid length string, asserts", "[uuid]" )
{
	CHECK_ASSERTS( mclo::uuid( "too-short" ), "Invalid UUID string" );
	CHECK_ASSERTS( mclo::uuid( "550e8400-e29b-41d4-a716-4466554400001234" ), "Invalid UUID string" );
}

TEST_CASE( "uuid from string with missing hyphens, asserts", "[uuid]" )
{
	CHECK_ASSERTS( mclo::uuid( "550e8400xe29b-41d4-a716-446655440000" ), "Invalid UUID string" );
}

TEST_CASE( "uuid from string with invalid hex character, asserts", "[uuid]" )
{
	CHECK_ASSERTS( mclo::uuid( "550g8400-e29b-41d4-a716-446655440000" ), "Invalid UUID string" );
}

TEST_CASE( "two different uuids, comparison, ordered correctly", "[uuid]" )
{
	static constexpr mclo::uuid lower( "00000000-0000-0000-0000-000000000001" );
	static constexpr mclo::uuid higher( "00000000-0000-0000-0000-000000000002" );

	CONSTEVAL_CHECK( lower < higher );
	CONSTEVAL_CHECK( lower <= higher );
	CONSTEVAL_CHECK( higher > lower );
	CONSTEVAL_CHECK( higher >= lower );
	CONSTEVAL_CHECK( lower != higher );
	CONSTEVAL_CHECK_FALSE( lower == higher );
}

TEST_CASE( "uuid, generate, produces different uuids", "[uuid]" )
{
	const mclo::uuid id1 = mclo::uuid::generate();
	const mclo::uuid id2 = mclo::uuid::generate();

	CHECK( id1 != id2 );
}

TEST_CASE( "uuid, generate, sets version 4 and RFC 4122 variant", "[uuid]" )
{
	const mclo::uuid id = mclo::uuid::generate();

	const auto version_byte = std::to_integer<unsigned>( id.bytes[ 6 ] );
	CHECK( ( version_byte >> 4 ) == 0x4 );

	const auto variant_byte = std::to_integer<unsigned>( id.bytes[ 8 ] );
	CHECK( ( variant_byte >> 6 ) == 0b10 );
}

TEST_CASE( "uuid, generate with custom engine, sets version 4 and RFC 4122 variant", "[uuid]" )
{
	std::mt19937 generator( 42 );
	const mclo::uuid id = mclo::uuid::generate( generator );

	const auto version_byte = std::to_integer<unsigned>( id.bytes[ 6 ] );
	CHECK( ( version_byte >> 4 ) == 0x4 );

	const auto variant_byte = std::to_integer<unsigned>( id.bytes[ 8 ] );
	CHECK( ( variant_byte >> 6 ) == 0b10 );
}

TEST_CASE( "uuid, generate with same seed, produces same uuids", "[uuid]" )
{
	std::mt19937 gen1( 12345 );
	std::mt19937 gen2( 12345 );

	const mclo::uuid id1 = mclo::uuid::generate( gen1 );
	const mclo::uuid id2 = mclo::uuid::generate( gen2 );

	CHECK( id1 == id2 );
}

TEST_CASE( "uuid, generate with different seeds, produces different uuids", "[uuid]" )
{
	std::mt19937 gen1( 12345 );
	std::mt19937 gen2( 67890 );

	const mclo::uuid id1 = mclo::uuid::generate( gen1 );
	const mclo::uuid id2 = mclo::uuid::generate( gen2 );

	CHECK( id1 != id2 );
}

TEST_CASE( "uuid, generate then to_string then from string, round-trips", "[uuid]" )
{
	std::mt19937 generator( 99 );
	const mclo::uuid original = mclo::uuid::generate( generator );

	const std::string str = original.to_string();
	const mclo::uuid parsed( str );

	CHECK( parsed == original );
}

TEST_CASE( "uuid, try_parse with valid string, returns uuid", "[uuid]" )
{
	static constexpr auto result = mclo::uuid::try_parse( "550e8400-e29b-41d4-a716-446655440000" );

	STATIC_REQUIRE( result.has_value() );
	STATIC_CHECK( *result == mclo::uuid( "550e8400-e29b-41d4-a716-446655440000" ) );
}

TEST_CASE( "uuid, try_parse with invalid length, returns nullopt", "[uuid]" )
{
	CONSTEVAL_CHECK_FALSE( mclo::uuid::try_parse( "too-short" ).has_value() );
	CONSTEVAL_CHECK_FALSE( mclo::uuid::try_parse( "550e8400-e29b-41d4-a716-4466554400001234" ).has_value() );
}

TEST_CASE( "uuid, try_parse with missing hyphens, returns nullopt", "[uuid]" )
{
	CONSTEVAL_CHECK_FALSE( mclo::uuid::try_parse( "550e8400xe29b-41d4-a716-446655440000" ).has_value() );
}

TEST_CASE( "uuid, try_parse with invalid hex character, returns nullopt", "[uuid]" )
{
	CONSTEVAL_CHECK_FALSE( mclo::uuid::try_parse( "550g8400-e29b-41d4-a716-446655440000" ).has_value() );
}

TEST_CASE( "uuid, try_parse with empty string, returns nullopt", "[uuid]" )
{
	CONSTEVAL_CHECK_FALSE( mclo::uuid::try_parse( "" ).has_value() );
}
