#include <catch2/catch_test_macros.hpp>

#include "mclo/container/type_id_set.hpp"

namespace
{
	struct test_type_1;
	struct test_type_2;
	struct test_type_3;

	void check_empty( const mclo::type_id_set<>& type_set )
	{
		CHECK( type_set.empty() );
		CHECK( type_set.size() == 0 );
		CHECK( type_set.max_size() > 0 );
		CHECK( type_set.begin() == type_set.end() );
		CHECK( type_set.cbegin() == type_set.cend() );
		CHECK_FALSE( type_set.contains<test_type_1>() );
		CHECK_FALSE( type_set.contains<test_type_2>() );
		CHECK_FALSE( type_set.contains<test_type_3>() );
		CHECK( type_set == type_set );
	}

	template <typename... Ts>
	void check_contains_types( const mclo::type_id_set<>& type_set )
	{
		CHECK( type_set.size() == sizeof...( Ts ) );
		CHECK( type_set.max_size() > 0 );
		CHECK( type_set.begin() != type_set.end() );
		CHECK( type_set.cbegin() != type_set.cend() );
		CHECK( type_set != mclo::type_id_set<>{} );
		CHECK( type_set == type_set );
		CHECK( type_set.contains<Ts...>() );

		for ( const mclo::meta::type_id_t type : type_set )
		{
			const bool valid = ( ( type == mclo::meta::type_id<Ts> ) || ... );
			CHECK( valid );
		}
	}
}

TEST_CASE( "default constructed type_id_set, is empty with no types present", "[type_id_set]" )
{
	mclo::type_id_set<> type_set;

	check_empty( type_set );
}

TEST_CASE( "empty type_id_set, insert type, only that type is present", "[type_id_set]" )
{
	mclo::type_id_set<> type_set;

	type_set.insert<test_type_1>();

	check_contains_types<test_type_1>( type_set );
}

TEST_CASE( "non-empty type set, insert different type, contains both", "[type_id_set]" )
{
	mclo::type_id_set<> type_set;
	type_set.insert<test_type_1>();

	type_set.insert<test_type_2>();

	check_contains_types<test_type_1, test_type_2>( type_set );
}

TEST_CASE( "non-empty type_id_set, insert same type, only that type is present", "[type_id_set]" )
{
	mclo::type_id_set<> type_set;
	type_set.insert<test_type_1>();

	type_set.insert<test_type_1>();

	check_contains_types<test_type_1>( type_set );
}

TEST_CASE( "non-empty type_id_set, erase present type, does not contain type", "[type_id_set]" )
{
	mclo::type_id_set<> type_set;
	type_set.insert<test_type_1>();
	type_set.insert<test_type_2>();
	type_set.insert<test_type_3>();

	type_set.erase<test_type_2>();

	check_contains_types<test_type_1, test_type_3>( type_set );
}

TEST_CASE( "non-empty type_id_set, erase not present type, no change", "[type_id_set]" )
{
	mclo::type_id_set<> type_set;
	type_set.insert<test_type_1>();
	type_set.insert<test_type_3>();

	type_set.erase<test_type_2>();

	check_contains_types<test_type_1, test_type_3>( type_set );
}

TEST_CASE( "type_id_set, clear, set is empty" )
{
	mclo::type_id_set<> type_set;
	type_set.insert<test_type_1>();
	type_set.insert<test_type_2>();
	type_set.insert<test_type_3>();

	type_set.clear();

	check_empty( type_set );
}

TEST_CASE( "type_id_set, member swap, sets are swapped" )
{
	mclo::type_id_set<> type_set_1;
	type_set_1.insert<test_type_1>();
	type_set_1.insert<test_type_2>();
	mclo::type_id_set<> type_set_2;
	type_set_2.insert<test_type_3>();

	type_set_1.swap( type_set_2 );

	check_contains_types<test_type_3>( type_set_1 );
	check_contains_types<test_type_1, test_type_2>( type_set_2 );
}

TEST_CASE( "type_id_set, free swap, sets are swapped" )
{
	mclo::type_id_set<> type_set_1;
	type_set_1.insert<test_type_1>();
	type_set_1.insert<test_type_2>();
	mclo::type_id_set<> type_set_2;
	type_set_2.insert<test_type_3>();

	swap( type_set_1, type_set_2 );

	check_contains_types<test_type_3>( type_set_1 );
	check_contains_types<test_type_1, test_type_2>( type_set_2 );
}

TEST_CASE( "type_id_set, copy construct, both contains same types", "[type_id_set]" )
{
	mclo::type_id_set<> source;
	source.insert<test_type_1>();
	source.insert<test_type_3>();

	mclo::type_id_set<> type_set( source );

	check_contains_types<test_type_1, test_type_3>( source );
	check_contains_types<test_type_1, test_type_3>( type_set );
}

TEST_CASE( "type_id_set, move construct, new contains same types and original empty", "[type_id_set]" )
{
	mclo::type_id_set<> source;
	source.insert<test_type_1>();
	source.insert<test_type_3>();

	mclo::type_id_set<> type_set( std::move( source ) );

	check_empty( source );
	check_contains_types<test_type_1, test_type_3>( type_set );
}

TEST_CASE( "type_id_set, copy assign, both contains same types", "[type_id_set]" )
{
	mclo::type_id_set<> source;
	source.insert<test_type_1>();
	source.insert<test_type_3>();
	mclo::type_id_set<> type_set;
	type_set.insert<test_type_2>();

	type_set = source;

	check_contains_types<test_type_1, test_type_3>( source );
	check_contains_types<test_type_1, test_type_3>( type_set );
}

TEST_CASE( "type_id_set, move assign, new contains same types and original empty", "[type_id_set]" )
{
	mclo::type_id_set<> source;
	source.insert<test_type_1>();
	source.insert<test_type_3>();
	mclo::type_id_set<> type_set;
	type_set.insert<test_type_2>();

	type_set = std::move( source );

	check_empty( source );
	check_contains_types<test_type_1, test_type_3>( type_set );
}
