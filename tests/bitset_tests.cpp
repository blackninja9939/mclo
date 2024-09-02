#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/bitset.hpp"
#include "mclo/meta/type_list.hpp"

using namespace Catch::Matchers;

namespace
{
	constexpr std::size_t bitset_size = 34;

	template <std::unsigned_integral UnderlyingType>
	class dynamic_bitset_wrapper : public mclo::dynamic_bitset<UnderlyingType>
	{
	public:
		dynamic_bitset_wrapper()
			: mclo::dynamic_bitset<UnderlyingType>( bitset_size )
		{
		}
	};

	using test_types = mclo::meta::type_list<dynamic_bitset_wrapper<std::uint32_t>,
											 mclo::bitset<bitset_size, std::uint32_t>,
											 dynamic_bitset_wrapper<std::uint64_t>,
											 mclo::bitset<bitset_size, std::uint64_t>>;
}

TEMPLATE_LIST_TEST_CASE( "bitset default constructor", "[bitset]", test_types )
{
	const TestType set;
	CHECK( set.size() == bitset_size );
	CHECK_FALSE( set.all() );
	CHECK_FALSE( set.any() );
	CHECK( set.none() );
	CHECK( set.count() == 0 );
	CHECK( set.find_first_set() == TestType::npos );
	CHECK( set.find_first_unset() == 0 );

	bool any = false;
	set.for_each_set( [ &any ]( const std::size_t ) { any = true; } );

	for ( std::size_t index = 0; index < set.size(); ++index )
	{
		CHECK_FALSE( set.test( index ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "bitset set", "[bitset]", test_types )
{
	TestType set;

	set.set( 32 );
	CHECK( set.test( 32 ) );
	CHECK_FALSE( set.all() );
	CHECK( set.any() );
	CHECK_FALSE( set.none() );
	CHECK( set.count() == 1 );
	CHECK( set.find_first_set() == 32 );
	CHECK( set.find_first_unset() == 0 );

	set.for_each_set( []( const std::size_t index ) { CHECK( index == 32 ); } );

	for ( std::size_t index = 0; index < set.size(); ++index )
	{
		if ( index == 32 )
		{
			continue;
		}
		CHECK_FALSE( set.test( index ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "bitset find_first_set loop", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 );

	for ( std::size_t index = set.find_first_set(); index != TestType::npos; index = set.find_first_set( index + 1 ) )
	{
		CHECK( ( index == 4 || index == 32 ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "bitset find_first_unset loop", "[bitset]", test_types )
{
	TestType set;
	set.set().reset( 4 ).reset( 32 ).reset( 33 );

	std::size_t count = 0;
	for ( std::size_t index = set.find_first_unset(); index != TestType::npos;
		  index = set.find_first_unset( index + 1 ) )
	{
		CHECK( ( index == 4 || index == 32 || index == 33 ) );
		++count;
	}

	CHECK( count == 3 );
}
