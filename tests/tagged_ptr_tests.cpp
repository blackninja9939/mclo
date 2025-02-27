#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/memory/tagged_ptr.hpp"
#include "mclo/meta/type_aliases.hpp"

#include <memory>

namespace
{
	struct tester
	{
		tester() = default;
		tester( int val )
			: value( val )
		{
		}

		operator int() const noexcept
		{
			return value;
		}

		int value = 0;
	};

	using test_types = mclo::meta::type_list<std::uint8_t, int, std::size_t, tester>;
}

TEMPLATE_LIST_TEST_CASE( "default tagged_ptr", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr;

	CHECK( ptr.tag() == 0 );
	CHECK( !ptr );
	CHECK( ptr.get() == nullptr );
	CHECK( ptr == nullptr );
	CHECK( ptr != owned.get() );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr from ptr", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get() );

	CHECK( ptr.tag() == 0 );

	REQUIRE( ptr );
	REQUIRE( ptr.get() == owned.get() );
	REQUIRE( ptr != nullptr );
	REQUIRE( ptr == owned.get() );
	CHECK( *ptr == 42 );

	*owned = 99;
	CHECK( *ptr == 99 );

	*ptr = 16;
	CHECK( *owned == 16 );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr from ptr and tag", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	CHECK( ptr.tag() == 3 );

	REQUIRE( ptr );
	REQUIRE( ptr.get() == owned.get() );
	REQUIRE( ptr != nullptr );
	REQUIRE( ptr == owned.get() );
	CHECK( *ptr == 42 );

	*owned = 99;
	CHECK( *ptr == 99 );
	CHECK( ptr.tag() == 3 );

	*ptr = 16;
	CHECK( *owned == 16 );
	CHECK( ptr.tag() == 3 );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr set_tag", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	ptr.set_tag( 1 );
	CHECK( ptr.tag() == 1 );
	REQUIRE( ptr );
	REQUIRE( ptr.get() == owned.get() );
	REQUIRE( ptr != nullptr );
	REQUIRE( ptr == owned.get() );
	CHECK( *ptr == 42 );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr clear_tag", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	ptr.clear_tag();
	CHECK( ptr.tag() == 0 );
	REQUIRE( ptr );
	REQUIRE( ptr.get() == owned.get() );
	REQUIRE( ptr != nullptr );
	REQUIRE( ptr == owned.get() );
	CHECK( *ptr == 42 );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr set_ptr", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	auto other_owned = std::make_unique<int>( 6002 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	ptr.set_ptr( other_owned.get() );
	CHECK( ptr.tag() == 3 );
	REQUIRE( ptr );
	REQUIRE( ptr.get() == other_owned.get() );
	REQUIRE( ptr != nullptr );
	REQUIRE( ptr == other_owned.get() );
	CHECK( *ptr == 6002 );
	CHECK( *owned == 42 );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr clear_ptr", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	ptr.clear_ptr();
	CHECK( ptr.tag() == 3 );
	CHECK( !ptr );
	CHECK( ptr.get() == nullptr );
	CHECK( ptr == nullptr );
	CHECK( ptr != owned.get() );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr reset", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	ptr.reset();
	CHECK( ptr.tag() == 0 );
	CHECK( !ptr );
	CHECK( ptr.get() == nullptr );
	CHECK( ptr == nullptr );
	CHECK( ptr != owned.get() );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr reset ptr", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	auto other_owned = std::make_unique<int>( 6002 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	ptr.reset( other_owned.get() );
	CHECK( ptr.tag() == 0 );
	REQUIRE( ptr );
	REQUIRE( ptr.get() == other_owned.get() );
	REQUIRE( ptr != nullptr );
	REQUIRE( ptr == other_owned.get() );
	CHECK( *ptr == 6002 );
	CHECK( *owned == 42 );
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr reset ptr and value", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	auto other_owned = std::make_unique<int>( 6002 );
	mclo::tagged_ptr<int, TestType> ptr( owned.get(), 3 );

	ptr.reset( other_owned.get(), 1 );
	CHECK( ptr.tag() == 1 );
	REQUIRE( ptr );
	REQUIRE( ptr.get() == other_owned.get() );
	REQUIRE( ptr != nullptr );
	REQUIRE( ptr == other_owned.get() );
	CHECK( *ptr == 6002 );
	CHECK( *owned == 42 );
}

TEMPLATE_LIST_TEST_CASE( "tagged_unique_ptr", "[tagged_ptr]", test_types )
{
	mclo::tagged_unique_ptr<const int, TestType> ptr = mclo::make_tagged_unique<const int, TestType>( 42 );

	CHECK( ptr.tag() == 0 );

	REQUIRE( ptr );
	REQUIRE( ptr != nullptr );
	CHECK( *ptr == 42 );

	const auto address = reinterpret_cast<std::uintptr_t>( ptr.get() );

	ptr.set_ptr( new int( 16 ) );

	CHECK( ptr.tag() == 0 );
	CHECK( *ptr == 16 );
	CHECK( address != reinterpret_cast<std::uintptr_t>( ptr.get() ) );
}

TEMPLATE_LIST_TEST_CASE( "tagged_unique_ptr piecewise", "[tagged_ptr]", test_types )
{
	mclo::tagged_unique_ptr ptr = mclo::make_tagged_unique<int, TestType>(
		std::piecewise_construct, std::forward_as_tuple( 42 ), std::forward_as_tuple( 6 ) );

	CHECK( ptr.tag() == 6 );

	REQUIRE( ptr );
	REQUIRE( ptr != nullptr );
	CHECK( *ptr == 42 );

	const auto address = reinterpret_cast<std::uintptr_t>( ptr.get() );

	ptr.set_ptr( new int( 16 ) );

	CHECK( ptr.tag() == 6 );
	CHECK( *ptr == 16 );
	CHECK( address != reinterpret_cast<std::uintptr_t>( ptr.get() ) );
}

TEMPLATE_LIST_TEST_CASE( "platform_tagged_ptr from ptr and tag", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::platform_tagged_ptr ptr( owned.get(), TestType( 16 ) );

	CHECK( ptr.tag() == 16 );
}

TEST_CASE( "tagged_unique_ptr piecewise throw", "[tagged_ptr]" )
{
	struct throwing_tester
	{
		throwing_tester() = default;
		throwing_tester( int val )
			: i( val )
		{
			if ( i == 5 )
			{
				throw std::runtime_error( "Error" );
			}
		}
		int i;
	};
	struct allocation_tracker
	{
		allocation_tracker( bool& tracker )
			: tracker( tracker )
		{
			tracker = true;
		}
		~allocation_tracker()
		{
			tracker = false;
		}
		bool& tracker;
	};

	bool allocation_active = false;
	try
	{
		const mclo::tagged_unique_ptr ptr = mclo::make_tagged_unique<const allocation_tracker, throwing_tester>(
			std::piecewise_construct, std::forward_as_tuple( allocation_active ), std::forward_as_tuple( 5 ) );
	}
	catch ( ... )
	{
		CHECK( !allocation_active );
	}
}

TEMPLATE_LIST_TEST_CASE( "tagged_ptr comparisons", "[tagged_ptr]", test_types )
{
	auto owned = std::make_unique<int>( 42 );
	mclo::tagged_ptr<int, TestType> ptr;

	CHECK( ptr == nullptr );
	CHECK( nullptr == ptr );
	CHECK( ptr == ptr );
	CHECK_FALSE( ptr != nullptr );
	CHECK_FALSE( nullptr != ptr );
	CHECK_FALSE( ptr != ptr );
}
