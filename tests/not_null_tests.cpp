#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "assert_macros.hpp"

#include "mclo/memory/not_null.hpp"
#include "mclo/meta/type_list.hpp"

#include <memory>

namespace
{
	using raw_pointer = mclo::not_null<int*>;

	static_assert( !std::is_default_constructible_v<raw_pointer> );
	static_assert( !std::is_constructible_v<raw_pointer, std::nullptr_t> );
	static_assert( !std::is_assignable_v<raw_pointer, std::nullptr_t> );
	static_assert( std::is_constructible_v<raw_pointer, int*> );

	struct raw_pointer_context
	{
		using pointer = mclo::not_null<int*>;

		static pointer null()
		{
			return static_cast<int*>( nullptr );
		}

		pointer ptr()
		{
			return &object;
		}

		int object{ 42 };
	};

	struct unqiue_pointer_context
	{
		using pointer = mclo::not_null<std::unique_ptr<int>>;

		static pointer null()
		{
			return std::unique_ptr<int>{};
		}

		pointer ptr()
		{
			return std::make_unique<int>( 42 );
		}
	};

	struct shared_pointer_context
	{
		using pointer = mclo::not_null<std::shared_ptr<int>>;

		static pointer null()
		{
			return std::shared_ptr<int>{};
		}
		pointer ptr()
		{
			return std::make_shared<int>( 42 );
		}
	};

	using test_types = mclo::meta::type_list<raw_pointer_context, unqiue_pointer_context, shared_pointer_context>;
}

TEMPLATE_LIST_TEST_CASE( "not_null same as get", "[not_null]", test_types )
{
	TestType context;
	const auto ptr = context.ptr();
	const auto& raw = ptr.get();
	REQUIRE( raw );
	CHECK( *raw == 42 );

	*ptr = 14;
	CHECK( *raw == 14 );

	CHECK( raw == ptr.operator->() );
	CHECK( std::to_address( raw ) == std::to_address( ptr ) );
}

TEMPLATE_LIST_TEST_CASE( "not_null swap", "[not_null]", test_types )
{
	TestType context1;
	auto ptr1 = context1.ptr();
	const auto raw1_original = std::to_address( ptr1 );

	TestType context2;
	auto ptr2 = context2.ptr();
	const auto raw2_original = std::to_address( ptr2 );

	ptr1.swap( ptr2 );

	CHECK( std::to_address( ptr1 ) == raw2_original );
	CHECK( std::to_address( ptr2 ) == raw1_original );
}

TEMPLATE_LIST_TEST_CASE( "not_null unsafe_release", "[not_null]", test_types )
{
	TestType context;
	auto ptr = context.ptr();
	const auto raw = std::to_address( ptr );

	const auto released = std::move( ptr ).unsafe_release();
	REQUIRE( released );

	CHECK( std::to_address( released ) == raw );
	CHECK_ASSERTS( ptr.get(), "Accessing moved from not_null pointer" );
	CHECK_ASSERTS( std::move( ptr ).unsafe_release(), "Releasing from moved from not_null pointer" );
}

TEMPLATE_LIST_TEST_CASE( "not_null from nullptr asserts", "[not_null]", test_types )
{
	TestType context;
	CHECK_ASSERTS( context.null(), "Constructing not_null with a null pointer" );
}

TEST_CASE( "not_null of void*", "[not_null]" )
{
	int i = 0;
	mclo::not_null<void*> ptr( &i );

	mclo::not_null<const void*> c_ptr( ptr );

	CHECK( ptr.get() == &i );
	CHECK( c_ptr.get() == &i );
	CHECK( ptr == c_ptr );
}
