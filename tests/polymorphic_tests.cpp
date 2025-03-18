#include <catch2/catch_test_macros.hpp>

#include "mclo/memory/polymorphic.hpp"

#include <numeric>

namespace
{
	struct abstract_base
	{
		virtual ~abstract_base() = default;

		virtual int get_value() const = 0;
		virtual void set_value( const int i ) = 0;
	};

	struct non_abstract : abstract_base
	{
		int get_value() const override
		{
			return 42;
		}
		void set_value( const int ) override
		{
		}
	};

	struct concrete : abstract_base
	{
		int m_value;

		explicit concrete( const int value )
			: m_value( value )
		{
		}

		int get_value() const final
		{
			return m_value;
		}

		void set_value( const int i ) final
		{
			m_value = i;
		}
	};

	struct init_list : concrete
	{
		init_list( const std::initializer_list<int> list, const int i )
			: concrete( std::accumulate( list.begin(), list.end(), i ) )
		{
		}
	};

	struct destroy_tracker : non_abstract
	{
		explicit destroy_tracker( bool& destroyed )
			: m_destroyed( destroyed )
		{
		}

		bool& m_destroyed;

		~destroy_tracker()
		{
			m_destroyed = true;
		}
	};
}

TEST_CASE( "mclo::polymorphic default construction", "[polymorphic]" )
{
	const mclo::polymorphic<non_abstract> object;
	CHECK( object->get_value() == 42 );
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic value construction", "[polymorphic]" )
{
	const mclo::polymorphic<abstract_base> object( concrete( 11 ) );
	CHECK( object->get_value() == 11 );
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic in place construction", "[polymorphic]" )
{
	const mclo::polymorphic<abstract_base> object( std::in_place_type<concrete>, 5 );
	CHECK( object->get_value() == 5 );
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic in place construction with initializer list", "[polymorphic]" )
{
	const mclo::polymorphic<abstract_base> object( std::in_place_type<init_list>, { 3, 5, 6 }, 3 );
	CHECK( object->get_value() == 17 );
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic copy construction", "[polymorphic]" )
{
	mclo::polymorphic<abstract_base> object1{ concrete( 10 ) };

	mclo::polymorphic<abstract_base> object2 = object1;
	object1->set_value( 75 );

	CHECK( object1->get_value() == 75 );
	CHECK( object2->get_value() == 10 );
	CHECK_FALSE( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic move construction", "[polymorphic]" )
{
	mclo::polymorphic<abstract_base> object1{ concrete( 100 ) };
	mclo::polymorphic<abstract_base> object2 = std::move( object1 );

	CHECK( object2->get_value() == 100 );
	CHECK( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic copy assignment", "[polymorphic]" )
{
	mclo::polymorphic<abstract_base> object1{ concrete( 25 ) };
	mclo::polymorphic<abstract_base> object2{ non_abstract() };

	object2 = object1;
	object1->set_value( 50 );

	CHECK( object1->get_value() == 50 );
	CHECK( object2->get_value() == 25 );
	CHECK_FALSE( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic move assignment", "[polymorphic]" )
{
	mclo::polymorphic<abstract_base> object1{ concrete( 75 ) };
	mclo::polymorphic<abstract_base> object2{ non_abstract() };

	object2 = std::move( object1 );

	CHECK( object2->get_value() == 75 );
	CHECK( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::polymorphic swapping", "[polymorphic]" )
{
	mclo::polymorphic<abstract_base> object1{ concrete( 10 ) };
	mclo::polymorphic<abstract_base> object2{ non_abstract() };

	SECTION( "member swap" )
	{
		object1.swap( object2 );
		CHECK( object1->get_value() == 42 );
		CHECK( object2->get_value() == 10 );
	}
	SECTION( "free function swap" )
	{
		swap( object1, object2 );
		CHECK( object1->get_value() == 42 );
		CHECK( object2->get_value() == 10 );
	}
}

TEST_CASE( "mclo::polymorphic swapping with valueless", "[polymorphic]" )
{
	mclo::polymorphic<abstract_base> object1{ concrete( 30 ) };
	mclo::polymorphic<abstract_base> object2 = std::move( object1 );

	CHECK( object1.valueless_after_move() );

	std::swap( object1, object2 );

	CHECK( object2.valueless_after_move() );
	CHECK( object1->get_value() == 30 );
}

TEST_CASE( "mclo::polymorphic destruction", "[polymorphic]" )
{
	bool was_destroyed = false;

	{
		mclo::polymorphic<destroy_tracker> object{ destroy_tracker( was_destroyed ) };
	}

	CHECK( was_destroyed );
}

TEST_CASE( "mclo::polymorphic get_allocator", "[polymorphic]" )
{
	mclo::polymorphic<abstract_base> object{ concrete( 42 ) };

	CHECK( object.get_allocator() == std::allocator<abstract_base>{} );
}
