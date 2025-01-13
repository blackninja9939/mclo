#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/memory/intrusive_ptr.hpp"
#include "mclo/memory/intrusive_ref_counter.hpp"
#include "mclo/meta/type_list.hpp"

namespace
{
	struct test_type : mclo::intrusive_ref_counter<test_type>
	{
		int m_data = 16;
	};
	struct deletion_tracker : mclo::intrusive_ref_counter<deletion_tracker>
	{
		deletion_tracker( bool& deleted ) noexcept
			: m_deleted( deleted )
		{
		}
		~deletion_tracker()
		{
			m_deleted = true;
		}
		bool& m_deleted;
	};

	template <typename T, typename U>
	void check_ptr_equals( const mclo::intrusive_ptr<T>& ptr, const U& rhs )
	{
		CHECK( ptr.get() == rhs );
		CHECK( ptr.operator->() == rhs );
		CHECK( ptr == rhs );
		const auto cmp = ptr <=> rhs;
		CHECK( std::is_eq( cmp ) );
	}
}

TEST_CASE( "intrusive_ptr with pointer destructor", "[new_type]" )
{
	bool deleted = false;
	const auto raw = new deletion_tracker( deleted );
	{
		mclo::intrusive_ptr<deletion_tracker> ptr( raw );
		{
			mclo::intrusive_ptr<deletion_tracker> copy( raw );
			CHECK_FALSE( deleted );
		}
		CHECK_FALSE( deleted );
	}
	CHECK( deleted );
}

TEST_CASE( "intrusive_ptr default is empty", "[new_type]" )
{
	const mclo::intrusive_ptr<test_type> ptr;
	check_ptr_equals( ptr, nullptr );
	CHECK_FALSE( ptr );
}

TEST_CASE( "intrusive_ptr construct nullptr is empty", "[new_type]" )
{
	const mclo::intrusive_ptr<test_type> ptr( nullptr );
	check_ptr_equals( ptr, static_cast<test_type*>( nullptr ) );
	CHECK_FALSE( ptr );
}

TEST_CASE( "intrusive_ptr with pointer not empty", "[new_type]" )
{
	const auto raw = new test_type();
	mclo::intrusive_ptr<test_type> ptr( raw );
	CHECK( raw->use_count() == 1 );
	check_ptr_equals( ptr, raw );
	REQUIRE( ptr );
	CHECK( std::addressof( *ptr ) == raw );
	CHECK( ptr->m_data == 16 );
	ptr->m_data = 4;
	CHECK( ptr->m_data == 4 );
	CHECK( raw->m_data == 4 );
}

TEST_CASE( "intrusive_ptr with pointer copy constructor adds reference", "[new_type]" )
{
	const auto raw = new test_type();
	mclo::intrusive_ptr<test_type> ptr( raw );
	mclo::intrusive_ptr<test_type> copy = ptr;
	CHECK( raw->use_count() == 2 );
}

TEST_CASE( "intrusive_ptr with pointer copy assignment replaces and adds reference", "[new_type]" )
{
	const auto raw = new test_type();
	const auto raw2 = new test_type();
	mclo::intrusive_ptr<test_type> ptr( raw );
	mclo::intrusive_ptr<test_type> copy( raw2 );
	copy = ptr;
	check_ptr_equals( copy, raw );
	CHECK( raw->use_count() == 2 );
}

TEST_CASE( "intrusive_ptr with pointer move constructor maintains reference clears source", "[new_type]" )
{
	const auto raw = new test_type();
	mclo::intrusive_ptr<test_type> ptr( raw );
	mclo::intrusive_ptr<test_type> copy = std::move( ptr );
	CHECK( raw->use_count() == 1 );
	CHECK_FALSE( ptr );
}

TEST_CASE( "intrusive_ptr with pointer copy move replaces and maintains reference clears source", "[new_type]" )
{
	const auto raw = new test_type();
	const auto raw2 = new test_type();
	mclo::intrusive_ptr<test_type> ptr( raw );
	mclo::intrusive_ptr<test_type> copy( raw2 );
	copy = std::move( ptr );
	check_ptr_equals( copy, raw );
	CHECK( raw->use_count() == 1 );
}

TEST_CASE( "intrusive_ptr with pointer detatch clears and maintains reference", "[new_type]" )
{
	bool deleted = false;
	const auto raw = new deletion_tracker( deleted );
	{
		mclo::intrusive_ptr<deletion_tracker> ptr( raw );
		ptr.detatch();
		CHECK_FALSE( deleted );
		CHECK_FALSE( ptr );
	}
	CHECK_FALSE( deleted );
	delete raw;
}

TEST_CASE( "intrusive_ptr with reset clears and decrements reference", "[new_type]" )
{
	bool deleted = false;
	const auto raw = new deletion_tracker( deleted );
	mclo::intrusive_ptr<deletion_tracker> ptr( raw );
	ptr.reset();
	CHECK_FALSE( ptr );
	CHECK( deleted );
}

TEST_CASE( "intrusive_ptr reset nullptr clears and decrements reference", "[new_type]" )
{
	bool deleted = false;
	const auto raw = new deletion_tracker( deleted );
	mclo::intrusive_ptr<deletion_tracker> ptr( raw );
	ptr.reset( nullptr );
	CHECK_FALSE( ptr );
	CHECK( deleted );
}

TEST_CASE( "intrusive_ptr assign nullptr clears and decrements reference", "[new_type]" )
{
	bool deleted = false;
	const auto raw = new deletion_tracker( deleted );
	mclo::intrusive_ptr<deletion_tracker> ptr( raw );
	ptr = nullptr;
	CHECK_FALSE( ptr );
	CHECK( deleted );
}

TEST_CASE( "intrusive_ptr reset other object replaces and decrements original reference", "[new_type]" )
{
	bool deleted = false;
	const auto raw = new deletion_tracker( deleted );

	bool deleted2 = false;
	const auto raw2 = new deletion_tracker( deleted );

	mclo::intrusive_ptr<deletion_tracker> ptr( raw );
	ptr.reset( raw2 );

	CHECK( deleted );
	CHECK_FALSE( deleted2 );
	check_ptr_equals( ptr, raw2 );
	REQUIRE( ptr );
	CHECK( ptr->use_count() == 1 );
}
