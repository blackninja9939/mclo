#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "consteval_check.h"

#include "mclo/container/dense_slot_map.hpp"
#include "mclo/meta/type_list.hpp"

#include <random>
#include <unordered_set>

using namespace Catch::Matchers;

template <typename T, std::size_t TotalBits, std::size_t GenerationBits>
struct Catch::StringMaker<mclo::slot_map_handle<T, TotalBits, GenerationBits>>
{
	using handle = mclo::slot_map_handle<T, TotalBits, GenerationBits>;
	using pair = std::pair<typename handle::representation_type, typename handle::representation_type>;

	static std::string convert( const mclo::slot_map_handle<T, TotalBits, GenerationBits> value )
	{
		return Catch::StringMaker<pair>::convert( pair( value.index, value.generation ) );
	}
};

using test_map = mclo::dense_slot_map<std::string>;

struct throwing_tester
{
	throwing_tester() = default;
	explicit throwing_tester( int val )
		: i( val )
	{
		if ( i == 5 )
		{
			throw std::runtime_error( "Error" );
		}
	}
	int i;
};

// Utility functions
template <typename T>
void check_empty( const T& map )
{
	CHECK( map.size() == 0 );
	CHECK( map.empty() );
	CHECK( map.begin() == map.end() );
	CHECK( map.cbegin() == map.cend() );
}

template <typename T>
void check_not_empty( const T& map )
{
	CHECK( map.size() > 0 );
	CHECK( !map.empty() );
	CHECK( map.begin() != map.end() );
}

TEST_CASE( "SlotMapHandle Default Construct Is Null", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle;
	CHECK( handle.is_null() );
}

TEST_CASE( "SlotMapHandle Construct With Values Not Null", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle{ 16, 8 };
	CHECK_FALSE( handle.is_null() );
	CHECK( handle.index == 16 );
	CHECK( handle.generation == 8 );
}

TEST_CASE( "SlotMapHandle Get Combined Is Expected", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle{ 16, 8 };
	CHECK( handle.get_combined() == ( ( 8u << ( 32u - 8u ) ) | 16u ) );
}

TEST_CASE( "SlotMapHandle Set Combined Expected Value Not Null", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle;
	handle.set_combined( ( 8u << ( 32u - 8u ) ) | 16u );
	CHECK_FALSE( handle.is_null() );
	CHECK( handle.index == 16 );
	CHECK( handle.generation == 8 );
}

TEST_CASE( "SlotMapHandle Same Values Compare Equal Are Equal", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle1{ 16, 8 };
	handle_type handle2{ 16, 8 };
	CHECK( handle1 == handle2 );
}

TEST_CASE( "SlotMapHandle Different Index Compare Equal Are Not Equal", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle1{ 16, 8 };
	handle_type handle2{ 17, 8 };
	CHECK( handle1 != handle2 );
}

TEST_CASE( "SlotMapHandle Different Generation Compare Equal Are Not Equal", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle1{ 16, 8 };
	handle_type handle2{ 16, 7 };
	CHECK( handle1 != handle2 );
}

TEST_CASE( "DenseSlotMap Default Construct Is Empty", "[slot_map]" )
{
	test_map map;
	check_empty( map );
	CHECK( map.slot_count() == 0 );
	CHECK( map.capacity() == 0 );
}

TEST_CASE( "DenseSlotMap Allocator Construct Is Empty With Expected Allocator", "[slot_map]" )
{
	typename test_map::allocator_type allocator;
	test_map map( allocator );
	check_empty( map );
	CHECK( map.slot_count() == 0 );
	CHECK( map.capacity() == 0 );
	CHECK( map.get_allocator() == allocator );
}

TEST_CASE( "DenseSlotMap Reserve Slot Constructor Is Empty With Reserved Slots", "[slot_map]" )
{
	test_map map( 5 );
	check_empty( map );
	CHECK( map.slot_count() == 5 );
}

TEST_CASE( "DenseSlotMap Null Handle Is Valid Is False", "[slot_map]" )
{
	test_map map;
	typename test_map::handle_type handle;
	CHECK_FALSE( map.is_valid( handle ) );
}

TEST_CASE( "DenseSlotMap Null Handle Lookup Is Nullptr", "[slot_map]" )
{
	test_map map;
	typename test_map::handle_type handle;
	CHECK( map.lookup( handle ) == nullptr );
}

TEST_CASE( "DenseSlotMap Insert LValue Value Is Inserted And Returns Valid Handle", "[slot_map]" )
{
	test_map map;
	const std::string value = "hello";
	auto handle = map.insert( value );
	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );
	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );
	CHECK( map.is_valid( handle ) );
	std::string* ptr = map.lookup( handle );
	REQUIRE( ptr != nullptr );
	CHECK( *ptr == value );
}

TEST_CASE( "DenseSlotMap Insert RValue Value Is Inserted And Returns Valid Handle", "[slot_map]" )
{
	test_map map;
	std::string value = "hello";
	auto handle = map.insert( std::move( value ) );
	CHECK( value.empty() );
	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );
	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );
	CHECK( map.is_valid( handle ) );
	std::string* ptr = map.lookup( handle );
	REQUIRE( ptr != nullptr );
	CHECK( *ptr == "hello" );
}

TEST_CASE( "DenseSlotMap Emplace Value Is Inserted And Returns Valid Handle", "[slot_map]" )
{
	test_map map;
	const std::string value( 6, 'a' );
	auto handle = map.emplace( 6, 'a' );
	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );
	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );
	CHECK( map.is_valid( handle ) );
	std::string* ptr = map.lookup( handle );
	REQUIRE( ptr != nullptr );
	CHECK( *ptr == value );
}

TEST_CASE( "DenseSlotMap Emplace And Get Value Is Inserted And Returns Valid Handle", "[slot_map]" )
{
	test_map map;
	const std::string value( 6, 'a' );
	auto [ object, handle ] = map.emplace_and_get( 6, 'a' );
	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );
	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );
	CHECK( map.is_valid( handle ) );
	std::string* ptr = map.lookup( handle );
	REQUIRE( ptr != nullptr );
	CHECK( *ptr == value );
	CHECK( &object == ptr );
	CHECK( object == *ptr );
}

TEST_CASE( "DenseSlotMap Repeated Insertion Uniqu Handles And Objects", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	std::unordered_set<typename decltype( map )::handle_type> handles;
	handles.reserve( 20 );

	for ( int i = 0; i < 20; ++i )
	{
		const auto handle = map.insert( i );
		handles.insert( handle );
	}

	CHECK( map.size() == 20u );
	CHECK( map.slot_count() == 20u );
	CHECK( map.capacity() >= 20u );
	CHECK( handles.size() == 20u );
	std::unordered_set<int*> ptrs;
	ptrs.reserve( 20 );
	for ( const auto handle : handles )
	{
		ptrs.insert( map.lookup( handle ) );
	}
	CHECK( ptrs.size() == 20u );
}

TEST_CASE( "DenseSlotMap Insert More Than Max Size Throws Length Error", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	using size_type = typename mclo::dense_slot_map<int>::size_type;
	const size_type max_size = map.max_size();
	map.reserve( max_size );
	for ( size_type i = 0; i < max_size; ++i )
	{
		( void )map.insert( i );
	}
	CHECK_THROWS_AS( map.insert( 0 ), std::length_error );
	CHECK_THROWS_AS( map.emplace( 0 ), std::length_error );
	CHECK_THROWS_AS( map.emplace_and_get( 0 ), std::length_error );
}

TEST_CASE( "DenseSlotMap Insert After Erase Same Index Higher Generation", "[slot_map]" )
{
	test_map map;
	auto handle = map.insert( "42" );
	map.erase( handle );
	handle = map.insert( "9939" );
	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );
	CHECK( handle.index == 0 );
	CHECK( handle.generation == 1 );
	CHECK( map.is_valid( handle ) );
	std::string* ptr = map.lookup( handle );
	REQUIRE( ptr != nullptr );
	CHECK( *ptr == "9939" );
}

TEST_CASE( "DenseSlotMap Insert Throwing Type Strong Exception Guarantee Held", "[slot_map]" )
{
	mclo::dense_slot_map<throwing_tester> map;
	auto success_handle = map.emplace( 0 );
	CHECK_THROWS_AS( map.emplace( 5 ), std::runtime_error );
	CHECK( map.size() == 1 );
	CHECK( map.slot_count() == 1 );
	CHECK( map.is_valid( success_handle ) );
	throwing_tester* ptr = map.lookup( success_handle );
	REQUIRE( ptr != nullptr );
	CHECK( ptr->i == 0 );
}

TEST_CASE( "DenseSlotMap Reserve Slots Increases Slots But Capacity Unchanged", "[slot_map]" )
{
	test_map map;
	map.reserve_slots( 2 );
	check_empty( map );
	CHECK( map.slot_count() == 2 );
	CHECK( map.capacity() == 0 );
}

TEST_CASE( "DenseSlotMap Reserve Increases Slots And Capacity", "[slot_map]" )
{
	test_map map;
	map.reserve( 2 );
	check_empty( map );
	CHECK( map.capacity() >= 2 );
	CHECK( map.slot_count() == 2 );
}

TEST_CASE( "DenseSlotMap Insert After Reserve No Reallocation", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	( void )map.reserve( 2 );
	( void )map.insert( 5 );
	( void )map.insert( 5 );
	CHECK( map.capacity() >= 2 );
	CHECK( map.slot_count() == 2 );
}

TEST_CASE( "DenseSlotMap Reserve Slots More Than Max Size Throws Length Error", "[slot_map]" )
{
	test_map map;
	CHECK_THROWS_AS( map.reserve_slots( map.max_size() + 1 ), std::length_error );
}

TEST_CASE( "DenseSlotMap Reserve More Than Max Size Throws Length Error", "[slot_map]" )
{
	test_map map;
	CHECK_THROWS_AS( map.reserve( map.max_size() + 1 ), std::length_error );
}

TEST_CASE( "DenseSlotMap Pop Valid Handle Engaged Optional", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	auto handle = map.insert( 42 );
	auto popped = map.pop( handle );
	CHECK_FALSE( map.is_valid( handle ) );
	CHECK( map.lookup( handle ) == nullptr );
	REQUIRE( popped.has_value() );
	CHECK( *popped == 42 );
}

TEST_CASE( "DenseSlotMap Pop Invalid Handle Empty Optional", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	auto handle = map.insert( 42 );
	map.erase( handle );
	auto popped = map.pop( handle );
	CHECK_FALSE( popped.has_value() );
}

TEST_CASE( "DenseSlotMap Clear Destroys Objects", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	auto handles = std::vector{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };
	map.clear();
	check_empty( map );
}

TEST_CASE( "DenseSlotMap Insert After Clear Old Handles Invalidated", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	auto initial_handles = std::vector{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };
	map.clear();
	auto new_handles = std::vector{ map.insert( 1 ), map.insert( 2 ), map.insert( 3 ) };
	for ( const auto handle : initial_handles )
	{
		CHECK_FALSE( map.is_valid( handle ) );
		CHECK( map.lookup( handle ) == nullptr );
	}
	int val = 1;
	for ( const auto handle : new_handles )
	{
		CHECK( map.is_valid( handle ) );
		const int* ptr = map.lookup( handle );
		REQUIRE( ptr != nullptr );
		CHECK( *ptr == val );
		++val;
	}
}

TEST_CASE( "DenseSlotMap Reset Destroys Objects", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	auto handles = std::vector{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };
	map.reset();
	check_empty( map );
}

TEST_CASE( "DenseSlotMap Member Swap Is Swapped", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	( void )map.insert( 42 );
	mclo::dense_slot_map<int> map2;
	( void )map2.insert( 99 );
	map.swap( map2 );
	CHECK( map.front() == 99 );
	CHECK( map2.front() == 42 );
}

TEST_CASE( "DenseSlotMap Free Function Swap Is Swapped", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	( void )map.insert( 42 );
	mclo::dense_slot_map<int> map2;
	( void )map2.insert( 99 );
	using std::swap;
	swap( map, map2 );
	CHECK( map.front() == 99 );
	CHECK( map2.front() == 42 );
}

TEST_CASE( "DenseSlotMap Iterate Expected Values", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	std::vector handles{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };
	constexpr std::array initialRange{ 42, 16, 99 };
	CHECK_THAT( map, UnorderedRangeEquals( initialRange ) );
	map.erase( handles.front() );
	handles.front() = map.insert( 202 );
	constexpr std::array modifiedRange{ 202, 16, 99 };
	CHECK_THAT( map, UnorderedRangeEquals( modifiedRange ) );
}

TEST_CASE( "DenseSlotMap Get Handle For End Iterator Null Handle", "[slot_map]" )
{
	test_map map;
	auto handle = map.get_handle( map.end() );
	CHECK( handle.is_null() );
}

TEST_CASE( "DenseSlotMap Get Handle For Valid Iterator Expected Handle", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	( void )map.insert( 5 );
	( void )map.insert( 11 );
	( void )map.insert( -20 );
	auto it = map.begin() + 1;
	auto handle = map.get_handle( it );
	CHECK_FALSE( handle.is_null() );
	CHECK( map.is_valid( handle ) );
	const int* ptr = map.lookup( handle );
	REQUIRE( ptr != nullptr );
	CHECK( ptr == &*it );
	CHECK( *ptr == *it );
}

TEST_CASE( "DenseSlotMap Erase If Erases Expected Handles", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	( void )map.insert( 32 );
	( void )map.insert( 5 );
	( void )map.insert( 5 );
	( void )map.insert( 11 );
	( void )map.insert( -20 );
	( void )map.insert( 5 );
	auto count = std::erase_if( map, []( const int value ) { return value == 5; } );
	CHECK( count == 3 );
	CHECK( map.size() == 3 );
	constexpr std::array expectedValues{ 32, 11, -20 };
	CHECK_THAT( map, UnorderedRangeEquals( expectedValues ) );
}

TEST_CASE( "DenseSlotMap Copy Construct Original Unmodified Copy Identical Values", "[slot_map]" )
{
	constexpr std::array expectedValues{ "42", "16", "99" };
	test_map map1;
	for ( const auto& value : expectedValues )
	{
		( void )map1.insert( value );
	}
	test_map map2 = map1;
	check_not_empty( map1 );
	check_not_empty( map2 );
	CHECK( map1.size() == map2.size() );
	CHECK_THAT( map1, UnorderedRangeEquals( expectedValues ) );
	CHECK_THAT( map2, UnorderedRangeEquals( expectedValues ) );
}

TEST_CASE( "DenseSlotMap Move Construct Original Empty Move Takes Values", "[slot_map]" )
{
	constexpr std::array expectedValues{ "42", "16", "99" };
	test_map map1;
	for ( const auto& value : expectedValues )
	{
		( void )map1.insert( value );
	}
	test_map map2 = std::move( map1 );
	check_empty( map1 );
	CHECK( map1.slot_count() == 0 );
	CHECK( map1.capacity() == 0 );
	check_not_empty( map2 );
	CHECK( map2.slot_count() == 3 );
	CHECK( map2.capacity() >= 3 );
	CHECK( map2.size() == 3 );
	CHECK_THAT( map2, UnorderedRangeEquals( expectedValues ) );
}

TEST_CASE( "DenseSlotMap Copy Assign Original Unmodified Copy Identical Values", "[slot_map]" )
{
	constexpr std::array expectedValues{ "42", "16", "99" };
	test_map map1;
	for ( const auto& value : expectedValues )
	{
		( void )map1.insert( value );
	}
	test_map map2;
	( void )map2.insert( "1" );
	map2 = map1;
	check_not_empty( map1 );
	check_not_empty( map2 );
	CHECK( map1.size() == map2.size() );
	CHECK_THAT( map1, UnorderedRangeEquals( expectedValues ) );
	CHECK_THAT( map2, UnorderedRangeEquals( expectedValues ) );
}

TEST_CASE( "DenseSlotMap Fuzz Testing Works Correctly", "[slot_map]" )
{
	test_map map;
	std::mt19937_64 rng{ 0 };
	std::uniform_int_distribution<std::size_t> dist{ 0u, 99 };
	std::unordered_set<typename test_map::handle_type> handles;
	for ( std::size_t index = 0; index < 5000; ++index )
	{
		auto random = dist( rng );
		if ( random < 25 && !map.empty() )
		{
			auto handle = *std::next( handles.begin(), random % handles.size() );
			handles.erase( handle );
			map.erase( handle );
		}
		else if ( random < 50 && !map.empty() )
		{
			auto it = map.begin() + ( random % map.size() );
			handles.erase( map.get_handle( it ) );
			map.erase( it );
		}
		else
		{
			handles.insert( map.insert( std::to_string( index ) ) );
		}
	}
}
