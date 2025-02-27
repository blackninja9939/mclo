#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/enum/enum_set.hpp"

#include "mclo/container/span.hpp"
#include "mclo/enum/enum_range.hpp"

using namespace Catch::Matchers;

namespace
{
	enum class test_enum
	{
		first,
		second,
		third,
		fourth,
		fifth,
		enum_size,
	};

	static_assert( std::ranges::forward_range<mclo::enum_set<test_enum>>, "enum_set models a forward range" );

	void _expectSetContains( const mclo::enum_set<test_enum>& set, mclo::span<const test_enum> expectedSet )
	{
		DEBUG_ASSERT( std::is_sorted( expectedSet.begin(), expectedSet.end() ) );

		CHECK( expectedSet.empty() == set.empty() );
		CHECK( expectedSet.size() == set.size() );

		auto expectedSetIt = expectedSet.begin();
		const auto expectedSetEnd = expectedSet.end();
		for ( const test_enum e : mclo::enum_range<test_enum>() )
		{
			if ( expectedSetIt != expectedSetEnd && e == *expectedSetIt )
			{
				const auto it = set.find( e );
				REQUIRE( set.end() != it );
				CHECK( e == *it );
				CHECK( set.contains( e ) );
				expectedSetIt++;
			}
			else
			{
				CHECK( set.end() == set.find( e ) );
				CHECK_FALSE( set.contains( e ) );
			}
		}

		std::vector<test_enum> setValues;
		set.forEachSet( [ &setValues ]( const test_enum e ) { setValues.push_back( e ); } );
		CHECK_THAT( setValues, RangeEquals( expectedSet ) );
		CHECK_THAT( set, RangeEquals( expectedSet ) );
		CHECK_THAT( std::as_const( set ), RangeEquals( expectedSet ) );
	}
}

TEST_CASE( "DefaultSet_Construct_IsEmpty", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set;

	_expectSetContains( set, {} );
}

TEST_CASE( "DefaultSet_ConstructFromIteratorPair_ValuesAreSet", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	const mclo::enum_set<test_enum> set{ values.begin(), values.end() };

	_expectSetContains( set, values );
}

TEST_CASE( "DefaultSet_ConstructFromRange_ValuesAreSet", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	const mclo::enum_set<test_enum> set{ values };

	_expectSetContains( set, values );
}

TEST_CASE( "DefaultSet_ConstructFromInitializerList_ValuesAreSet", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set{ test_enum::second, test_enum::third, test_enum::fifth };

	_expectSetContains( set, std::array{ test_enum::second, test_enum::third, test_enum::fifth } );
}

TEST_CASE( "EmptySet_InsertSingle_ValueIsSet", "[enum_set]" )
{
	mclo::enum_set<test_enum> set;

	const auto [ it, inserted ] = set.insert( test_enum::second );

	REQUIRE( set.end() != it );
	CHECK( test_enum::second == *it );
	CHECK( inserted );
	_expectSetContains( set, std::array{ test_enum::second } );
}

TEST_CASE( "EmptySet_InsertSameValueTwice_OnlyOneSet", "[enum_set]" )
{
	mclo::enum_set<test_enum> set;

	const auto [ it, inserted ] = set.insert( test_enum::second );
	const auto [ it2, inserted2 ] = set.insert( test_enum::second );

	REQUIRE( set.end() != it );
	CHECK( test_enum::second == *it );
	CHECK( inserted );
	REQUIRE( set.end() != it2 );
	CHECK( test_enum::second == *it2 );
	CHECK_FALSE( inserted2 );
	_expectSetContains( set, std::array{ test_enum::second } );
}

TEST_CASE( "EmptySet_InsertSingleTwice_ValuesAreSet", "[enum_set]" )
{
	mclo::enum_set<test_enum> set;

	set.insert( test_enum::second );
	set.insert( test_enum::fifth );

	_expectSetContains( set, std::array{ test_enum::second, test_enum::fifth } );
}

TEST_CASE( "EmptySet_InsertIteratorPair_ValuesAreSet", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	mclo::enum_set<test_enum> set;

	set.insert( values.begin(), values.end() );

	_expectSetContains( set, values );
}

TEST_CASE( "EmptySet_InsertRange_ValuesAreSet", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	mclo::enum_set<test_enum> set;

	set.insert( values );

	_expectSetContains( set, values );
}

TEST_CASE( "EmptySet_InsertInitializerList_ValuesAreSet", "[enum_set]" )
{
	mclo::enum_set<test_enum> set;

	set.insert( { test_enum::second, test_enum::third, test_enum::fifth } );

	_expectSetContains( set, std::array{ test_enum::second, test_enum::third, test_enum::fifth } );
}

TEST_CASE( "SetWithValues_Clear_IsEmpty", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	mclo::enum_set<test_enum> set{ values };

	set.clear();

	_expectSetContains( set, {} );
}

TEST_CASE( "SetWithValues_ErasePresentKey_ValueIsRemoved", "[enum_set]" )
{
	mclo::enum_set<test_enum> set{ test_enum::second, test_enum::third, test_enum::fifth };

	set.erase( test_enum::second );

	_expectSetContains( set, std::array{ test_enum::third, test_enum::fifth } );
}

TEST_CASE( "SetWithValues_EraseNotPresentKey_Noop", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	mclo::enum_set<test_enum> set{ values };

	set.erase( test_enum::first );

	_expectSetContains( set, values );
}

TEST_CASE( "SetWithValues_ErasePresentIterator_ValueIsRemoved", "[enum_set]" )
{
	mclo::enum_set<test_enum> set{ test_enum::second, test_enum::third, test_enum::fifth };

	set.erase( set.find( test_enum::third ) );

	_expectSetContains( set, std::array{ test_enum::second, test_enum::fifth } );
}

TEST_CASE( "SetWithValues_EraseNotPresentIterator_Noop", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	mclo::enum_set<test_enum> set{ values };

	set.erase( set.find( test_enum::first ) );

	_expectSetContains( set, values );
}

TEST_CASE( "TwoDifferentSets_CompareIterators_NotEqual", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1;
	const mclo::enum_set<test_enum> set2;

	CHECK( set1.begin() != set2.begin() );
	CHECK( set1.end() != set2.end() );
}

TEST_CASE( "TwoEmptySets_CompareSets_AreEqual", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1;
	const mclo::enum_set<test_enum> set2;

	CHECK( set1 == set2 );
}

TEST_CASE( "TwoSetsWithSameValues_CompareSets_AreEqual", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	const mclo::enum_set<test_enum> set1{ values };
	const mclo::enum_set<test_enum> set2{ values };

	CHECK( set1 == set2 );
}

TEST_CASE( "TwoSetsWithDifferentValues_CompareSets_AreNotEqual", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::third, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::fifth };

	CHECK( set1 != set2 );
}

TEST_CASE( "TwoSetsWithSameValues_Merge_AreEqual", "[enum_set]" )
{
	static constexpr std::array values{ test_enum::second, test_enum::third, test_enum::fifth };
	mclo::enum_set<test_enum> set1{ values };
	const mclo::enum_set<test_enum> set2{ values };

	set1.merge( set2 );

	CHECK( set1 == set2 );
}

TEST_CASE( "TwoSetsWithDifferentValues_Merge_ResultHasAll", "[enum_set]" )
{
	mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::third, test_enum::fifth };

	set1.merge( set2 );

	_expectSetContains( set1, std::array{ test_enum::second, test_enum::third, test_enum::fourth, test_enum::fifth } );
}

TEST_CASE( "SetWithValues_MergeWithEmpty_ResultUnchanged", "[enum_set]" )
{
	mclo::enum_set<test_enum> set{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> empty;

	set.merge( empty );

	_expectSetContains( set, std::array{ test_enum::second, test_enum::fourth, test_enum::fifth } );
}

TEST_CASE( "TwoSetsWithDifferentValues_Intersect_ResultHasOverlapping", "[enum_set]" )
{
	mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::third, test_enum::fifth };

	set1.intersect( set2 );

	_expectSetContains( set1, std::array{ test_enum::second, test_enum::fifth } );
}

TEST_CASE( "SetWithValues_IntersectWithEmpty_ResultEmpty", "[enum_set]" )
{
	mclo::enum_set<test_enum> set{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> empty;

	set.intersect( empty );

	_expectSetContains( set, {} );
}

TEST_CASE( "TwoSetsWithDifferentValues_Difference_ResultHasDifference", "[enum_set]" )
{
	mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::third, test_enum::fifth };

	set1.difference( set2 );

	_expectSetContains( set1, std::array{ test_enum::third, test_enum::fourth } );
}

TEST_CASE( "SetWithValues_DifferenceWithEmpty_ResultUnchanged", "[enum_set]" )
{
	mclo::enum_set<test_enum> set{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> empty;

	set.difference( empty );

	_expectSetContains( set, std::array{ test_enum::second, test_enum::fourth, test_enum::fifth } );
}

TEST_CASE( "SetAndSubset_Includes_ReturnsTrue", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::fifth };

	CHECK( set1.includes( set2 ) );
}

TEST_CASE( "SetAndOverlappingSet_Includes_ReturnsFalse", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::third, test_enum::fifth };

	CHECK_FALSE( set1.includes( set2 ) );
}

TEST_CASE( "SetAndOverlappingSet_Overlaps_ReturnsTrue", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::fifth };

	CHECK( set1.overlaps( set2 ) );
}

TEST_CASE( "SetAndDisjointSet_Overlaps_ReturnsFalse", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::first, test_enum::third };

	CHECK_FALSE( set1.overlaps( set2 ) );
}

TEST_CASE( "SetAndDisjointSet_Disjoint_ReturnsTrue", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::first, test_enum::third };

	CHECK( set1.disjoint( set2 ) );
}

TEST_CASE( "SetAndOverlappingSet_Disjoint_ReturnsFalse", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set1{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> set2{ test_enum::second, test_enum::fifth };

	CHECK_FALSE( set1.disjoint( set2 ) );
}

TEST_CASE( "SetAndEmptySet_Includes_ReturnsTrue", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> empty;

	CHECK( set.includes( empty ) );
}

TEST_CASE( "EmptySetAndEmptySet_Includes_ReturnsTrue", "[enum_set]" )
{
	const mclo::enum_set<test_enum> empty;

	CHECK( empty.includes( empty ) );
}

TEST_CASE( "SetAndEmptySet_Overlaps_ReturnsFalse", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> empty;

	CHECK_FALSE( set.overlaps( empty ) );
}

TEST_CASE( "EmptySetAndEmptySet_Overlaps_ReturnsFalse", "[enum_set]" )
{
	const mclo::enum_set<test_enum> empty;

	CHECK_FALSE( empty.overlaps( empty ) );
}

TEST_CASE( "SetAndEmptySet_Disjoint_ReturnsTrue", "[enum_set]" )
{
	const mclo::enum_set<test_enum> set{ test_enum::second, test_enum::fourth, test_enum::fifth };
	const mclo::enum_set<test_enum> empty;

	CHECK( set.disjoint( empty ) );
}

TEST_CASE( "EmptySetAndEmptySet_Disjoint_ReturnsTrue", "[enum_set]" )
{
	const mclo::enum_set<test_enum> empty;

	CHECK( empty.disjoint( empty ) );
}
