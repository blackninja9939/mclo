#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "consteval_check.h"

#include "mclo/dense_slot_map.hpp"
#include "mclo/paged_slot_map.hpp"
#include "mclo/meta/type_list.hpp"

#include <random>

namespace
{
	using test_paged = mclo::paged_slot_map<std::string, 2>;

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

	template <typename T>
	void check_empty( const T& map )
	{
		CHECK( map.size() == 0 );
		CHECK( map.empty() );
		CHECK( map.begin() == map.end() );
		CHECK( map.cbegin() == map.cend() );
		CHECK( map.rbegin() == map.rend() );
		CHECK( map.crbegin() == map.crend() );
	}

	template <typename T>
	void check_not_empty( const T& map )
	{
		CHECK_FALSE( map.size() == 0 );
		CHECK_FALSE( map.empty() );
		CHECK_FALSE( map.begin() == map.end() );
		CHECK_FALSE( map.cbegin() == map.cend() );
		CHECK_FALSE( map.rbegin() == map.rend() );
		CHECK_FALSE( map.crbegin() == map.crend() );
	}
}

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

TEST_CASE( "slot map handle", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;

	const auto index = GENERATE( take( 5, random( 0u, handle_type::max_index ) ) );
	const auto generation = GENERATE( take( 5, random( 0u, handle_type::max_generation ) ) );

	const handle_type handle{ index, generation };
	CHECK( handle.index == index );
	CHECK( handle.generation == generation );
	CHECK( handle.get_combined() == ( ( generation << ( 32 - 8 ) ) | index ) );
}

TEST_CASE( "slot map handle set_combined", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;
	handle_type handle{ 0, 0 };

	const auto index = GENERATE( take( 5, random( 0u, handle_type::max_index ) ) );
	const auto generation = GENERATE( take( 5, random( 0u, handle_type::max_generation ) ) );

	handle.set_combined( ( generation << ( 32 - 8 ) ) | index );

	CHECK( handle.index == index );
	CHECK( handle.generation == generation );
}

TEST_CASE( "slot map handle comparisons", "[slot_map]" )
{
	using handle_type = mclo::slot_map_handle<int, 32, 8>;

	const auto index = GENERATE( take( 5, random( 1u, handle_type::max_index - 1 ) ) );
	const auto generation = GENERATE( take( 5, random( 1u, handle_type::max_generation - 1 ) ) );

	const handle_type handle{ index, generation };

	SECTION( "Equality" )
	{
		CHECK( handle == handle );
		CHECK_FALSE( handle != handle );
		CHECK( handle <= handle );
		CHECK( handle >= handle );
		CHECK_FALSE( handle < handle );
		CHECK_FALSE( handle > handle );
	}
	SECTION( "Greater index" )
	{
		const handle_type handle_greater{ index + 1, generation };

		CHECK_FALSE( handle == handle_greater );
		CHECK( handle != handle_greater );
		CHECK( handle <= handle_greater );
		CHECK_FALSE( handle >= handle_greater );
		CHECK( handle < handle_greater );
		CHECK_FALSE( handle > handle_greater );
	}
	SECTION( "Greater generation" )
	{
		const handle_type handle_greater{ index, generation + 1 };

		CHECK_FALSE( handle == handle_greater );
		CHECK( handle != handle_greater );
		CHECK( handle <= handle_greater );
		CHECK_FALSE( handle >= handle_greater );
		CHECK( handle < handle_greater );
		CHECK_FALSE( handle > handle_greater );
	}
	SECTION( "Lower index" )
	{
		const handle_type handle_smaller{ index - 1, generation };

		CHECK_FALSE( handle == handle_smaller );
		CHECK( handle != handle_smaller );
		CHECK_FALSE( handle <= handle_smaller );
		CHECK( handle >= handle_smaller );
		CHECK_FALSE( handle < handle_smaller );
		CHECK( handle > handle_smaller );
	}
	SECTION( "Lower generation" )
	{
		const handle_type handle_smaller{ index, generation - 1 };

		CHECK_FALSE( handle == handle_smaller );
		CHECK( handle != handle_smaller );
		CHECK_FALSE( handle <= handle_smaller );
		CHECK( handle >= handle_smaller );
		CHECK_FALSE( handle < handle_smaller );
		CHECK( handle > handle_smaller );
	}
}

TEST_CASE( "dense_slot_map default constructor", "[slot_map]" )
{
	const mclo::dense_slot_map<int> map;
	check_empty( map );
	CHECK( map.slot_count() == 0 );
	CHECK( map.capacity() == 0 );
}

TEST_CASE( "dense_slot_map allocator constructor", "[slot_map]" )
{
	typename mclo::dense_slot_map<int>::allocator_type allocator;
	const mclo::dense_slot_map<int> map( allocator );
	check_empty( map );
	CHECK( map.slot_count() == 0 );
	CHECK( map.capacity() == 0 );
	CHECK( map.get_allocator() == allocator );
}

TEST_CASE( "dense_slot_map reserve slots constructor", "[slot_map]" )
{
	const mclo::dense_slot_map<int> map( 5 );
	check_empty( map );
	CHECK( map.slot_count() == 5 );
}

TEST_CASE( "dense_slot_map insert", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;

	const auto value = 42;
	const auto handle = map.insert( value );

	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );

	CHECK( map.is_valid( handle ) );

	auto ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
}

TEST_CASE( "dense_slot_map emplace", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;

	const auto value = 42;
	const auto handle = map.emplace( value );

	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );

	CHECK( map.is_valid( handle ) );

	auto ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
}

TEST_CASE( "dense_slot_map emplace_and_get", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;

	const auto value = 42;
	const auto [ object, handle ] = map.emplace_and_get( value );

	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );

	CHECK( map.is_valid( handle ) );

	auto ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
	CHECK( ptr == &object );
	CHECK( *ptr == object );
}

TEST_CASE( "dense_slot_map insert more than max_size", "[slot_map]" )
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

TEST_CASE( "dense_slot_map erase", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;

	const auto value = 42;
	auto handle = map.insert( value );
	map.erase( handle );

	check_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK_FALSE( map.is_valid( handle ) );
	auto ptr = map.lookup( handle );
	CHECK_FALSE( ptr );

	handle = map.insert( value );

	check_not_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 1 );

	CHECK( map.is_valid( handle ) );

	ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
}

TEST_CASE( "dense_slot_map insert throwing", "[slot_map]" )
{
	mclo::dense_slot_map<throwing_tester> map;
	const auto succeess_handle = map.emplace( 0 );

	try
	{
		( void )map.emplace( 5 );
	}
	catch ( ... )
	{
	}

	CHECK( map.size() == 1 );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() == 1 );
	CHECK( map.is_valid( succeess_handle ) );

	auto ptr = map.lookup( succeess_handle );
	REQUIRE( ptr );
	CHECK( ptr->i == 0 );
}

TEST_CASE( "dense_slot_map insert throwing free list", "[slot_map]" )
{
	mclo::dense_slot_map<throwing_tester> map;
	const auto succeess_handle = map.emplace( 0 );
	map.erase( succeess_handle );

	try
	{
		( void )map.emplace( 5 );
	}
	catch ( ... )
	{
	}

	check_empty( map );
	CHECK( map.slot_count() == 1 );
	CHECK( map.capacity() == 1 );
}

TEST_CASE( "dense_slot_map reserve_slots", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	map.reserve_slots( 2 );
	check_empty( map );
	CHECK( map.capacity() == 0 );
	CHECK( map.slot_count() == 2 );

	auto handle = map.insert( 5 );
	CHECK( map.capacity() == 1 );
	CHECK( map.slot_count() == 2 );

	handle = map.insert( 5 );
	CHECK( map.capacity() == 2 );
	CHECK( map.slot_count() == 2 );
}

TEST_CASE( "dense_slot_map reserve", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	map.reserve( 2 );
	check_empty( map );
	CHECK( map.capacity() >= 2 );
	CHECK( map.slot_count() == 2 );

	auto handle = map.insert( 5 );
	CHECK( map.capacity() == 2 );
	CHECK( map.slot_count() == 2 );

	handle = map.insert( 5 );
	CHECK( map.capacity() == 2 );
	CHECK( map.slot_count() == 2 );
}

TEST_CASE( "dense_slot_map reserve_slots more than max_size", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	CHECK_THROWS_AS( map.reserve_slots( map.max_size() + 1 ), std::length_error );
}

TEST_CASE( "dense_slot_map reserve more than max_size", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	CHECK_THROWS_AS( map.reserve( map.max_size() + 1 ), std::length_error );
}

TEST_CASE( "dense_slot_map pop", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	const auto handle = map.insert( 42 );

	const std::optional<int> popped = map.pop( handle );
	REQUIRE( popped );
	CHECK( *popped == 42 );

	CHECK_FALSE( map.is_valid( handle ) );
	CHECK_FALSE( map.lookup( handle ) );

	const std::optional<int> empty_popped = map.pop( handle );
	CHECK_FALSE( empty_popped );
}

TEST_CASE( "dense_slot_map clear", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	const std::vector initial_handles{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };

	map.clear();
	check_empty( map );

	const std::vector new_handles{ map.insert( 1 ), map.insert( 2 ), map.insert( 3 ) };

	for ( const auto handle : initial_handles )
	{
		CHECK_FALSE( map.is_valid( handle ) );
		CHECK_FALSE( map.lookup( handle ) );
	}

	int val = 1;
	for ( const auto handle : new_handles )
	{
		CHECK( map.is_valid( handle ) );
		const int* const ptr = map.lookup( handle );
		REQUIRE( ptr );
		CHECK( *ptr == val );
		++val;
	}
}

TEST_CASE( "dense_slot_map reset", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	const std::vector initial_handles{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };

	map.reset();
	check_empty( map );

	const std::vector new_handles{ map.insert( 1 ), map.insert( 2 ), map.insert( 3 ) };

	auto check_handles = [ &map ]( const auto& vec ) {
		int val = 1;
		for ( const auto handle : vec )
		{
			CHECK( map.is_valid( handle ) );
			const int* const ptr = map.lookup( handle );
			REQUIRE( ptr );
			CHECK( *ptr == val );
			++val;
		}
	};

	check_handles( initial_handles );
	check_handles( new_handles );
}

TEST_CASE( "dense_slot_map swap", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;
	( void )map.insert( 42 );

	mclo::dense_slot_map<int> map2;
	( void )map2.insert( 99 );

	map.swap( map2 );

	CHECK( map.front() == 99 );
	CHECK( map2.front() == 42 );

	using std::swap;
	swap( map, map2 );

	CHECK( map.front() == 42 );
	CHECK( map2.front() == 99 );
}

TEST_CASE( "dense_slot_map iterate", "[slot_map]" )
{
	mclo::dense_slot_map<int> map;

	std::vector handles{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };

	CHECK_THAT( map, Catch::Matchers::UnorderedRangeEquals( std::array{ 42, 16, 99 } ) );

	map.erase( handles.front() );
	handles.front() = map.insert( 202 );

	CHECK_THAT( map, Catch::Matchers::UnorderedRangeEquals( std::array{ 202, 16, 99 } ) );
}

TEST_CASE( "paged_slot_map default constructor", "[slot_map]" )
{
	const test_paged map;
	check_empty( map );
	CHECK( map.page_count() == 0 );
	CHECK( map.capacity() == 0 );
}

TEST_CASE( "paged_slot_map allocator constructor", "[slot_map]" )
{
	typename test_paged::allocator_type allocator;
	const test_paged map( allocator );
	check_empty( map );
	CHECK( map.page_count() == 0 );
	CHECK( map.capacity() == 0 );
	CHECK( map.get_allocator() == allocator );
}

//TEST_CASE( "paged_slot_map reserve slots constructor", "[slot_map]" )
//{
//	const test_paged map( 5 );
//	check_empty( map );
//	CHECK( map.page_count() == 1 );
//}

TEST_CASE( "paged_slot_map insert", "[slot_map]" )
{
	test_paged map;

	const auto value = "42";
	const auto handle = map.insert( value );

	check_not_empty( map );
	CHECK( map.page_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );

	CHECK( map.is_valid( handle ) );

	auto ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
}

TEST_CASE( "paged_slot_map emplace", "[slot_map]" )
{
	test_paged map;

	const auto value = "42";
	const auto handle = map.emplace( value );

	check_not_empty( map );
	CHECK( map.page_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );

	CHECK( map.is_valid( handle ) );

	auto ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
}

TEST_CASE( "paged_slot_map emplace_and_get", "[slot_map]" )
{
	test_paged map;

	const auto value = "42";
	const auto [ object, handle ] = map.emplace_and_get( value );

	check_not_empty( map );
	CHECK( map.page_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );

	CHECK( map.is_valid( handle ) );

	auto ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
	CHECK( ptr == &object );
	CHECK( *ptr == object );
}

//TEST_CASE( "paged_slot_map insert more than max_size", "[slot_map]" )
//{
//	test_paged map;
//	using size_type = typename mclo::dense_slot_map<int>::size_type;
//
//	const size_type max_size = map.max_size();
//	map.reserve( max_size );
//
//	for ( size_type i = 0; i < max_size; ++i )
//	{
//		( void )map.insert( i );
//	}
//
//	CHECK_THROWS_AS( map.insert( 0 ), std::length_error );
//	CHECK_THROWS_AS( map.emplace( 0 ), std::length_error );
//	CHECK_THROWS_AS( map.emplace_and_get( 0 ), std::length_error );
//}
//
TEST_CASE( "paged_slot_map erase", "[slot_map]" )
{
	test_paged map;

	const auto value = "42";
	auto handle = map.insert( value );
	map.erase( handle );

	check_empty( map );
	CHECK( map.page_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK_FALSE( map.is_valid( handle ) );
	auto ptr = map.lookup( handle );
	CHECK_FALSE( ptr );

	handle = map.insert( value );

	check_not_empty( map );
	CHECK( map.page_count() == 1 );
	CHECK( map.capacity() >= 1 );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 1 );

	CHECK( map.is_valid( handle ) );

	ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
}

TEST_CASE( "paged_slot_map insert to new page", "[slot_map]" )
{
	test_paged map;

	auto handle = map.insert( "1" );
	auto handle2 = map.insert( "2" );
	auto handle3 = map.insert( "3" );
	auto handle4 = map.insert( "4" );

	map.erase( handle2 );

	auto handle5 = map.insert( "4" );
	CHECK( handle2.index == handle5.index );
}
//
//TEST_CASE( "paged_slot_map insert throwing", "[slot_map]" )
//{
//	mclo::paged_slot_map<throwing_tester> map;
//	const auto succeess_handle = map.emplace( 0 );
//
//	try
//	{
//		( void )map.emplace( 5 );
//	}
//	catch ( ... )
//	{
//	}
//
//	CHECK( map.size() == 1 );
//	CHECK( map.page_count() == 1 );
//	CHECK( map.capacity() == 1 );
//	CHECK( map.is_valid( succeess_handle ) );
//
//	auto ptr = map.lookup( succeess_handle );
//	REQUIRE( ptr );
//	CHECK( ptr->i == 0 );
//}
//
//TEST_CASE( "paged_slot_map insert throwing free list", "[slot_map]" )
//{
//	mclo::paged_slot_map<throwing_tester> map;
//	const auto succeess_handle = map.emplace( 0 );
//	map.erase( succeess_handle );
//
//	try
//	{
//		( void )map.emplace( 5 );
//	}
//	catch ( ... )
//	{
//	}
//
//	check_empty( map );
//	CHECK( map.page_count() == 1 );
//	CHECK( map.capacity() == 1 );
//}
//
//TEST_CASE( "paged_slot_map reserve_slots", "[slot_map]" )
//{
//	test_paged map;
//	map.reserve_slots( 2 );
//	check_empty( map );
//	CHECK( map.capacity() == 0 );
//	CHECK( map.page_count() == 2 );
//
//	auto handle = map.insert( 5 );
//	CHECK( map.capacity() == 1 );
//	CHECK( map.page_count() == 2 );
//
//	handle = map.insert( 5 );
//	CHECK( map.capacity() == 2 );
//	CHECK( map.page_count() == 2 );
//}
//
//TEST_CASE( "paged_slot_map reserve", "[slot_map]" )
//{
//	test_paged map;
//	map.reserve( 2 );
//	check_empty( map );
//	CHECK( map.capacity() >= 2 );
//	CHECK( map.page_count() == 2 );
//
//	auto handle = map.insert( 5 );
//	CHECK( map.capacity() == 2 );
//	CHECK( map.page_count() == 2 );
//
//	handle = map.insert( 5 );
//	CHECK( map.capacity() == 2 );
//	CHECK( map.page_count() == 2 );
//}
//
//TEST_CASE( "paged_slot_map reserve_slots more than max_size", "[slot_map]" )
//{
//	test_paged map;
//	CHECK_THROWS_AS( map.reserve_slots( map.max_size() + 1 ), std::length_error );
//}
//
//TEST_CASE( "paged_slot_map reserve more than max_size", "[slot_map]" )
//{
//	test_paged map;
//	CHECK_THROWS_AS( map.reserve( map.max_size() + 1 ), std::length_error );
//}
//
//TEST_CASE( "paged_slot_map pop", "[slot_map]" )
//{
//	test_paged map;
//	const auto handle = map.insert( 42 );
//
//	const std::optional<int> popped = map.pop( handle );
//	REQUIRE( popped );
//	CHECK( *popped == 42 );
//
//	CHECK_FALSE( map.is_valid( handle ) );
//	CHECK_FALSE( map.lookup( handle ) );
//
//	const std::optional<int> empty_popped = map.pop( handle );
//	CHECK_FALSE( empty_popped );
//}
//
//TEST_CASE( "paged_slot_map clear", "[slot_map]" )
//{
//	test_paged map;
//	const std::vector initial_handles{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };
//
//	map.clear();
//	check_empty( map );
//
//	const std::vector new_handles{ map.insert( 1 ), map.insert( 2 ), map.insert( 3 ) };
//
//	for ( const auto handle : initial_handles )
//	{
//		CHECK_FALSE( map.is_valid( handle ) );
//		CHECK_FALSE( map.lookup( handle ) );
//	}
//
//	int val = 1;
//	for ( const auto handle : new_handles )
//	{
//		CHECK( map.is_valid( handle ) );
//		const int* const ptr = map.lookup( handle );
//		REQUIRE( ptr );
//		CHECK( *ptr == val );
//		++val;
//	}
//}
//
//TEST_CASE( "paged_slot_map reset", "[slot_map]" )
//{
//	test_paged map;
//	const std::vector initial_handles{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };
//
//	map.reset();
//	check_empty( map );
//
//	const std::vector new_handles{ map.insert( 1 ), map.insert( 2 ), map.insert( 3 ) };
//
//	auto check_handles = [ &map ]( const auto& vec ) {
//		int val = 1;
//		for ( const auto handle : vec )
//		{
//			CHECK( map.is_valid( handle ) );
//			const int* const ptr = map.lookup( handle );
//			REQUIRE( ptr );
//			CHECK( *ptr == val );
//			++val;
//		}
//	};
//
//	check_handles( initial_handles );
//	check_handles( new_handles );
//}
//
//TEST_CASE( "paged_slot_map swap", "[slot_map]" )
//{
//	test_paged map;
//	( void )map.insert( 42 );
//
//	test_paged map2;
//	( void )map2.insert( 99 );
//
//	map.swap( map2 );
//
//	CHECK( map.front() == 99 );
//	CHECK( map2.front() == 42 );
//
//	using std::swap;
//	swap( map, map2 );
//
//	CHECK( map.front() == 42 );
//	CHECK( map2.front() == 99 );
//}
//
//TEST_CASE( "paged_slot_map iterate", "[slot_map]" )
//{
//	test_paged map;
//
//	std::vector handles{ map.insert( 42 ), map.insert( 16 ), map.insert( 99 ) };
//
//	CHECK_THAT( map, Catch::Matchers::UnorderedRangeEquals( std::array{ 42, 16, 99 } ) );
//
//	map.erase( handles.front() );
//	handles.front() = map.insert( 202 );
//
//	CHECK_THAT( map, Catch::Matchers::UnorderedRangeEquals( std::array{ 202, 16, 99 } ) );
//}
