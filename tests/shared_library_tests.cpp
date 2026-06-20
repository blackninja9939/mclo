#include <catch2/catch_test_macros.hpp>

#include "mclo/platform/shared_library.hpp"

#include <filesystem>
#include <system_error>
#include <unordered_set>

namespace
{
	// TEST_MODULE_PATH is injected by CMake as the absolute path of the test_module shared library.
	const std::filesystem::path module_path{ TEST_MODULE_PATH };

	using add_signature = int( int, int );
}

TEST_CASE( "default constructed shared_library, query state, is empty", "[shared_library]" )
{
	// Given a default constructed shared library
	const mclo::shared_library library;

	// Then it owns no module
	CHECK_FALSE( library.is_loaded() );
	CHECK_FALSE( static_cast<bool>( library ) );
	CHECK( library.native_handle() == nullptr );
	CHECK( library.use_count() == 0 );
	CHECK( library.path().empty() );
}

TEST_CASE( "valid module path, construct shared_library, loads module", "[shared_library]" )
{
	// Given a shared library constructed from a valid module path
	const mclo::shared_library library{ module_path };

	// Then it owns the loaded module
	CHECK( library.is_loaded() );
	CHECK( static_cast<bool>( library ) );
	CHECK( library.native_handle() != nullptr );
	CHECK( library.use_count() == 1 );
	CHECK( library.path() == module_path );
}

TEST_CASE( "missing module path, construct shared_library, throws system_error", "[shared_library]" )
{
	// Given a path that does not refer to a real module
	const std::filesystem::path missing = module_path.parent_path() / "does_not_exist.dll";

	// When constructing from it, Then it throws
	CHECK_THROWS_AS( mclo::shared_library{ missing }, std::system_error );
}

TEST_CASE( "missing module path, load, returns error", "[shared_library]" )
{
	// Given an empty shared library
	mclo::shared_library library;
	const std::filesystem::path missing = module_path.parent_path() / "does_not_exist.dll";

	// When loading a missing module
	const auto result = library.load( missing );

	// Then it reports failure and is left empty
	CHECK_FALSE( result.has_value() );
	CHECK_FALSE( library.is_loaded() );
}

TEST_CASE( "loaded shared_library", "[shared_library]" )
{
	// Given a loaded shared library
	mclo::shared_library library{ module_path };

	SECTION( "get exported function, returns callable pointer" )
	{
		const auto function = library.get<add_signature>( "mclo_test_add" );

		REQUIRE( function.has_value() );
		CHECK( ( *function )( 2, 3 ) == 5 );
	}

	SECTION( "get exported variable, returns pointer to value" )
	{
		const auto value = library.get<int>( "mclo_test_value" );

		REQUIRE( value.has_value() );
		CHECK( **value == 42 );
	}

	SECTION( "get missing symbol, returns error" )
	{
		const auto missing = library.get<add_signature>( "does_not_exist" );

		CHECK_FALSE( missing.has_value() );
	}

	SECTION( "has existing symbol, returns true" )
	{
		CHECK( library.has( "mclo_test_add" ) );
		CHECK( library.has( "mclo_test_value" ) );
	}

	SECTION( "has missing symbol, returns false" )
	{
		CHECK_FALSE( library.has( "does_not_exist" ) );
	}

	SECTION( "unload, becomes empty" )
	{
		library.unload();

		CHECK_FALSE( library.is_loaded() );
		CHECK( library.native_handle() == nullptr );
	}
}

TEST_CASE( "shared_symbol from function, outlives source library, stays callable", "[shared_library]" )
{
	// Given a shared_symbol obtained from a temporary library handle
	mclo::shared_symbol<add_signature> function;
	{
		const mclo::shared_library library{ module_path };
		auto result = library.get_shared<add_signature>( "mclo_test_add" );
		REQUIRE( result.has_value() );
		function = *result;
	}

	// Then it keeps the module loaded and remains callable after the source handle is gone
	REQUIRE( static_cast<bool>( function ) );
	CHECK( function( 4, 5 ) == 9 );
	CHECK( function.library().is_loaded() );
}

TEST_CASE( "shared_symbol from variable, dereferences exported value", "[shared_library]" )
{
	// Given a shared_symbol to an exported variable
	const mclo::shared_library library{ module_path };
	const auto value = library.get_shared<int>( "mclo_test_value" );

	// Then it behaves like a pointer to the variable
	REQUIRE( value.has_value() );
	CHECK( **value == 42 );
	CHECK( value->library().is_loaded() );
}

TEST_CASE( "get_shared missing symbol, returns error", "[shared_library]" )
{
	// Given a loaded shared library
	const mclo::shared_library library{ module_path };

	// When requesting a missing symbol
	const auto missing = library.get_shared<add_signature>( "does_not_exist" );

	// Then it reports failure
	CHECK_FALSE( missing.has_value() );
}

TEST_CASE( "loaded shared_library, copy, shares ownership", "[shared_library]" )
{
	// Given a loaded shared library
	mclo::shared_library library{ module_path };

	// When copied
	mclo::shared_library copy = library;

	// Then both share ownership of the same module
	CHECK( copy.is_loaded() );
	CHECK( copy.native_handle() == library.native_handle() );
	CHECK( copy == library );
	CHECK( library.use_count() == 2 );
	CHECK( copy.use_count() == 2 );

	// And unloading one leaves the other valid
	library.unload();

	CHECK_FALSE( library.is_loaded() );
	CHECK( copy.is_loaded() );
	CHECK( copy.use_count() == 1 );
}

TEST_CASE( "two shared_library handles to same module, hash, are equal", "[shared_library]" )
{
	// Given two handles sharing the same module
	const mclo::shared_library library{ module_path };
	const mclo::shared_library copy = library;

	// Then they hash equally and can be stored in an unordered_set
	const std::hash<mclo::shared_library> hasher;
	CHECK( hasher( library ) == hasher( copy ) );

	const std::unordered_set<mclo::shared_library> set{ library, copy };
	CHECK( set.size() == 1 );
}

TEST_CASE( "two shared_library handles, swap, exchanges modules", "[shared_library]" )
{
	// Given a loaded and an empty shared library
	mclo::shared_library loaded{ module_path };
	mclo::shared_library empty;

	// When swapped
	swap( loaded, empty );

	// Then ownership moves across
	CHECK_FALSE( loaded.is_loaded() );
	CHECK( empty.is_loaded() );
	CHECK( empty.path() == module_path );
}
