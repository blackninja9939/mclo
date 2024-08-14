#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include "consteval_check.h"

#include "mclo/dense_slot_map.hpp"
#include "mclo/meta/type_list.hpp"

#include <random>

namespace
{
	using test_types = mclo::meta::type_list<mclo::dense_slot_map<std::int32_t>,
											 mclo::dense_slot_map<std::string>,
											 mclo::pmr::dense_slot_map<std::string>>;
	template <typename Return>
	Return make_value( const std::int32_t number ) noexcept = delete;

	template<>
	std::int32_t make_value<std::int32_t>( const std::int32_t number ) noexcept
	{
		return number;
	}

	template <>
	std::string make_value<std::string>( const std::int32_t number ) noexcept
	{
		return std::to_string( number );
	}
}

#define MAKE_VALUE( NUMBER ) make_value<typename TestType::value_type>( NUMBER )

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

TEMPLATE_LIST_TEST_CASE( "dense_slot_map default constructor", "[slot_map]", test_types )
{
	const TestType map;
	CHECK( map.slot_count() == 0 );
	CHECK( map.size() == 0 );
	CHECK( map.empty() );
	CHECK( map.capacity() == 0 );
	CHECK( map.begin() == map.end() );
	CHECK( map.cbegin() == map.cend() );
	CHECK( map.rbegin() == map.rend() );
	CHECK( map.crbegin() == map.crend() );
}

TEMPLATE_LIST_TEST_CASE( "dense_slot_map allocator constructor", "[slot_map]", test_types )
{
	typename TestType::allocator_type allocator;
	const TestType map( allocator );
	CHECK( map.slot_count() == 0 );
	CHECK( map.size() == 0 );
	CHECK( map.empty() );
	CHECK( map.capacity() == 0 );
	CHECK( map.begin() == map.end() );
	CHECK( map.cbegin() == map.cend() );
	CHECK( map.rbegin() == map.rend() );
	CHECK( map.crbegin() == map.crend() );
	CHECK( map.get_allocator() == allocator );
}

TEMPLATE_LIST_TEST_CASE( "dense_slot_map reserve slots constructor", "[slot_map]", test_types )
{
	const TestType map( 5 );
	CHECK( map.slot_count() == 5 );
	CHECK( map.size() == 0 );
	CHECK( map.empty() );
	CHECK( map.capacity() == 0 );
	CHECK( map.begin() == map.end() );
	CHECK( map.cbegin() == map.cend() );
	CHECK( map.rbegin() == map.rend() );
	CHECK( map.crbegin() == map.crend() );
}

TEMPLATE_LIST_TEST_CASE( "dense_slot_map insert", "[slot_map]", test_types )
{
	TestType map;

	const auto value = MAKE_VALUE( 42 );
	const auto handle = map.insert( value );

	CHECK( map.size() == 1 );
	CHECK_FALSE( map.empty() );
	CHECK( map.capacity() >= 1 );
	CHECK( map.begin() != map.end() );
	CHECK( map.cbegin() != map.cend() );
	CHECK( map.rbegin() != map.rend() );
	CHECK( map.crbegin() != map.crend() );

	CHECK( handle.index == 0 );
	CHECK( handle.generation == 0 );

	CHECK( map.is_valid( handle ) );
	
	auto ptr = map.lookup( handle );
	REQUIRE( ptr );
	CHECK( *ptr == value );
}

TEST_CASE( "Contiguous slot map", "[slot_map]" )
{
	mclo::dense_slot_map<std::int32_t> SlotMap;
	auto Handle = SlotMap.insert( 5 );
	const std::int32_t* pValue = SlotMap.lookup( Handle );
	REQUIRE( pValue );
	CHECK( *pValue == 5 );

	auto Handle2 = SlotMap.insert( 6 );

	pValue = SlotMap.lookup( Handle );
	REQUIRE( pValue );
	CHECK( *pValue == 5 );

	pValue = SlotMap.lookup( Handle2 );
	REQUIRE( pValue );
	CHECK( *pValue == 6 );

	SlotMap.erase( Handle );

	pValue = SlotMap.lookup( Handle );
	CHECK( !pValue );

	pValue = SlotMap.lookup( Handle2 );
	REQUIRE( pValue );
	CHECK( *pValue == 6 );

	auto Handle3 = SlotMap.insert( 7 );
	pValue = SlotMap.lookup( Handle3 );

	REQUIRE( pValue );
	CHECK( *pValue == 7 );

	auto Handle4 = SlotMap.insert( 42 );
	pValue = SlotMap.lookup( Handle4 );

	REQUIRE( pValue );
	CHECK( *pValue == 42 );

	{
		mclo::dense_slot_map<std::int32_t> RandomMap( 5 );

		auto it = RandomMap.insert( 1 );
		it = RandomMap.insert( 1 );
		it = RandomMap.insert( 1 );
		it = RandomMap.insert( 1 );
		it = RandomMap.insert( 1 );

		RandomMap.reserve( 10 );

		it = RandomMap.insert( 1 );
		it = RandomMap.insert( 1 );
		it = RandomMap.insert( 1 );
		it = RandomMap.insert( 1 );
	}
	{
		mclo::dense_slot_map<std::int32_t> RandomMap;

		RandomMap.reserve( 30 );

		std::mt19937 random( 42 );
		std::uniform_int_distribution<> erase_chance( 0, 100 );
		std::uniform_int_distribution<> value_distribution;

		std::vector<mclo::dense_slot_map<std::int32_t>::handle_type> Handles;

		for ( std::size_t Counter = 0; Counter < 1000; ++Counter )
		{
			if ( erase_chance( random ) < 25 && !Handles.empty() )
			{
				const std::size_t HandleIndex =
					std::uniform_int_distribution<std::size_t>( 0, Handles.size() - 1 )( random );
				RandomMap.erase( Handles[ HandleIndex ] );
				std::swap( Handles[ HandleIndex ], Handles.back() );
				Handles.pop_back();
			}
			else
			{
				Handles.push_back( RandomMap.insert( value_distribution( random ) ) );
			}

			for ( const auto H : Handles )
			{
				CHECK( RandomMap.is_valid( H ) );
			}
		}
	}
}

#undef MAKE_VALUE
