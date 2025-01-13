#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/container/small_vector.hpp"
#include "mclo/container/static_vector.hpp"

#include "assert_macros.hpp"

#include <array>
#include <span>

using namespace Catch::Matchers;

struct MoveFromTester
{
	MoveFromTester() noexcept = default;
	explicit MoveFromTester( const int value ) noexcept
		: mValue( value )
	{
	}

	MoveFromTester( MoveFromTester&& other ) noexcept
		: mValue( other.mValue )
	{
		other.mWasMoved = true;
	}

	MoveFromTester& operator=( MoveFromTester&& other ) noexcept
	{
		other.mWasMoved = true;
		mWasMoved = false;
		mValue = other.mValue;
		return *this;
	}

	[[nodiscard]] constexpr auto operator<=>( const MoveFromTester& other ) const noexcept
	{
		return mValue <=> other.mValue;
	}
	[[nodiscard]] constexpr bool operator==( const MoveFromTester& other ) const noexcept
	{
		return mValue == other.mValue;
	}

	int mValue = 0;
	bool mWasMoved = false;
};

using EmptyArray = std::array<int, 0>;

void _expectVectorEmpty( const mclo::small_vector_base<int>& vec )
{
	CHECK( vec.empty() );
	CHECK( 0u == vec.size() );
	CHECK_THAT( vec, RangeEquals( EmptyArray{} ) );
	CHECK_THROWS_AS( vec.at( 0 ), std::out_of_range );
	CHECK_ASSERTS( vec.front(), "Container is empty" );
	CHECK_ASSERTS( vec.back(), "Container is empty" );
	CHECK_ASSERTS( vec[ 0 ], "Index out of range" );
}

static_assert( std::is_convertible_v<mclo::small_vector<int, 40>&, mclo::small_vector_base<int>&> );
static_assert( std::is_convertible_v<mclo::small_vector<int, 40>&, const mclo::small_vector_base<int>&> );
static_assert( std::is_convertible_v<const mclo::small_vector<int, 40>&, const mclo::small_vector_base<int>&> );
static_assert( !std::is_convertible_v<const mclo::small_vector<int, 40>&, mclo::small_vector_base<int>&> );

static_assert( std::is_convertible_v<mclo::small_vector<int, 40>&, std::span<int>> );
static_assert( std::is_convertible_v<mclo::small_vector<int, 40>&, std::span<const int>> );
static_assert( std::is_convertible_v<const mclo::small_vector<int, 40>&, std::span<const int>> );
static_assert( !std::is_convertible_v<const mclo::small_vector<int, 40>&, std::span<int>> );

TEST_CASE( "SmallVector_DefaultConstructed_IsEmpty", "[small_vector]" )
{
	const mclo::small_vector<int, 4> vec;

	_expectVectorEmpty( vec );
	CHECK( 4u == vec.capacity() );
}

TEST_CASE( "SmallVector_CopyAssignment_IsCorrect", "[small_vector]" )
{
	const std::initializer_list<int> initList{ 4, 9, 10, 16, 1 };
	const mclo::small_vector<int, 4> vec( initList );
	mclo::small_vector<int, 4> vec2( 6 );

	vec2 = vec;

	CHECK_THAT( vec, RangeEquals( initList ) );
	CHECK_THAT( vec2, RangeEquals( initList ) );
	CHECK( vec == vec2 );
	CHECK( vec.data() != vec2.data() );
}

TEST_CASE( "SmallVector_MoveAssignmentInBufferSameOrSmaller_IsCorrect", "[small_vector]" )
{
	const std::initializer_list<int> initList{ 4, 9 };
	mclo::small_vector<int, 4> vec( initList );
	mclo::small_vector<int, 4> vec2( 4 );
	const int* const vec2OriginalData = vec2.data();

	vec2 = std::move( vec );

	_expectVectorEmpty( vec );
	CHECK_THAT( vec2, RangeEquals( initList ) );
	CHECK( vec.data() != vec2.data() );
	CHECK( vec2OriginalData == vec2.data() );
}

TEST_CASE( "SmallVector_MoveAssignmentInBufferLarger_IsCorrect", "[small_vector]" )
{
	const std::initializer_list<int> initList{ 4, 9, 10, 16 };
	mclo::small_vector<int, 4> vec( initList );
	mclo::small_vector<int, 4> vec2( 2 );
	const int* const vec2OriginalData = vec2.data();

	vec2 = std::move( vec );

	_expectVectorEmpty( vec );
	CHECK_THAT( vec2, RangeEquals( initList ) );
	CHECK( vec.data() != vec2.data() );
	CHECK( vec2OriginalData == vec2.data() );
}

TEST_CASE( "SmallVector_MoveAssignmentOutOfBuffer_IsCorrect", "[small_vector]" )
{
	const std::initializer_list<int> initList{ 4, 9, 10, 16, 1 };
	mclo::small_vector<int, 4> vec( initList );
	mclo::small_vector<int, 4> vec2( 6 );
	const int* const vecOriginalData = vec.data();

	vec2 = std::move( vec );

	_expectVectorEmpty( vec );
	CHECK_THAT( vec2, RangeEquals( initList ) );
	CHECK( vec.data() != vec2.data() );
	CHECK( vecOriginalData == vec2.data() );
}

TEST_CASE( "SmallVector_ConstructorResize_IsCorrect", "[small_vector]" )
{
	const mclo::small_vector<int, 4> vec( 3 );

	CHECK( 3u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 0, 0, 0 } ) ) );
}

TEST_CASE( "SmallVector_ConstructorResizeInitValue_IsCorrect", "[small_vector]" )
{
	const mclo::small_vector<int, 4> vec( 3, 8 );

	CHECK( 3u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 8, 8, 8 } ) ) );
}

TEST_CASE( "SmallVector_ConstructorInitializerList_IsCorrect", "[small_vector]" )
{
	const mclo::small_vector<int, 4> vec{ 4, 9, 10, 16, 1 };

	CHECK( 5u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 4, 9, 10, 16, 1 } ) ) );
}

TEST_CASE( "SmallVector_ConstructorIterators_IsCorrect", "[small_vector]" )
{
	constexpr std::array arr{ 4, 9, 10, 16, 1 };
	const mclo::small_vector<int, 4> vec( arr.begin(), arr.end() );

	CHECK( 5u == vec.size() );
	CHECK_THAT( vec, RangeEquals( arr ) );
}

TEST_CASE( "SmallVector_PushBackLValue_HasObject", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;
	const int value = 2;

	const int& ref = vec.push_back( value );

	CHECK_FALSE( vec.empty() );
	CHECK( 1u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( value == *vec.data() );
	CHECK( value == vec.front() );
	CHECK( value == vec.back() );
	CHECK( value == vec[ 0 ] );
	CHECK( value == vec.at( 0 ) );
	CHECK_THAT( vec, RangeEquals( std::array{ value } ) );
	CHECK( &ref == vec.data() );
}

TEST_CASE( "SmallVector_PushBackRValue_HasObjectMoved", "[small_vector]" )
{
	mclo::small_vector<MoveFromTester, 4> vec;
	const int value = 2;
	MoveFromTester tester( value );
	REQUIRE_FALSE( tester.mWasMoved );

	const MoveFromTester& ref = vec.push_back( std::move( tester ) );

	CHECK( tester.mWasMoved );
	CHECK_FALSE( vec.empty() );
	CHECK( 1u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( value == vec.data()->mValue );
	CHECK( value == vec.front().mValue );
	CHECK( value == vec.back().mValue );
	CHECK( value == vec[ 0 ].mValue );
	CHECK( value == vec.at( 0 ).mValue );
	CHECK_THAT( vec, RangeEquals( std::array{ MoveFromTester( value ) } ) );
	CHECK( &ref == vec.data() );
}

TEST_CASE( "SmallVector_EmplaceBack_HasObject", "[small_vector]" )
{
	mclo::small_vector<MoveFromTester, 4> vec;
	const int value = 2;

	const MoveFromTester& ref = vec.emplace_back( value );

	CHECK_FALSE( vec.empty() );
	CHECK( 1u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( value == vec.data()->mValue );
	CHECK( value == vec.front().mValue );
	CHECK( value == vec.back().mValue );
	CHECK( value == vec[ 0 ].mValue );
	CHECK( value == vec.at( 0 ).mValue );
	CHECK_THAT( vec, RangeEquals( std::array{ MoveFromTester( value ) } ) );
	CHECK( &ref == vec.data() );
}

TEST_CASE( "SmallVector_EmplaceBackMultipleInBuffer_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;

	vec.emplace_back( 1 );
	vec.emplace_back( 2 );
	vec.emplace_back( 3 );
	vec.emplace_back( 4 );

	CHECK_FALSE( vec.empty() );
	CHECK( 4u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 4 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 4 } ) ) );
	for ( std::uint32_t i = 0; i < vec.size(); ++i )
	{
		const int next = i + 1;
		CHECK( next == vec[ i ] );
		CHECK( next == vec.at( i ) );
		CHECK( &vec[ i ] == vec.data() + i );
	}
}

TEST_CASE( "SmallVector_EmplaceBackMultipleOutOfBuffer_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;

	vec.emplace_back( 1 );
	vec.emplace_back( 2 );
	vec.emplace_back( 3 );
	vec.emplace_back( 4 );
	vec.emplace_back( 5 );
	vec.emplace_back( 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 6u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 6 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 4, 5, 6 } ) ) );
	for ( std::uint32_t i = 0; i < vec.size(); ++i )
	{
		const int next = i + 1;
		CHECK( next == vec[ i ] );
		CHECK( next == vec.at( i ) );
		CHECK( &vec[ i ] == vec.data() + i );
	}
}

TEST_CASE( "SmallVector_ReserveOnEmpty_IsEmpty", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;

	vec.reserve( 8 );

	_expectVectorEmpty( vec );
	CHECK( vec.capacity() >= 8u );
}

TEST_CASE( "SmallVector_ReserveMoreNonEmpty_SameObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2 };

	vec.reserve( 8 );

	CHECK_FALSE( vec.empty() );
	CHECK( 2u == vec.size() );
	CHECK( vec.capacity() >= 8u );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 2 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2 } ) ) );
}

TEST_CASE( "SmallVector_ReserveLessNonEmpty_IsNoop", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;
	vec.reserve( 8 );
	vec.push_back( 1 );
	vec.push_back( 2 );

	vec.reserve( 2 );

	CHECK_FALSE( vec.empty() );
	CHECK( 2u == vec.size() );
	CHECK( vec.capacity() >= 8u );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 2 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2 } ) ) );
}

TEST_CASE( "SmallVector_ResizeMore_Grows", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;

	vec.resize( 8 );

	CHECK_FALSE( vec.empty() );
	CHECK( 8u == vec.size() );
	CHECK( vec.capacity() >= 8u );
	REQUIRE( vec.data() );
	CHECK( 0 == *vec.data() );
	CHECK( 0 == vec.front() );
	CHECK( 0 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 0, 0, 0, 0, 0, 0, 0, 0 } ) ) );
}

TEST_CASE( "SmallVector_ResizeLess_Shrinks", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 8 );

	vec.resize( 2 );

	CHECK_FALSE( vec.empty() );
	CHECK( 2u == vec.size() );
	CHECK( vec.capacity() >= 8u );
	REQUIRE( vec.data() );
	CHECK( 0 == *vec.data() );
	CHECK( 0 == vec.front() );
	CHECK( 0 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 0, 0 } ) ) );
}

TEST_CASE( "SmallVector_ResizeFillValueMore_Grows", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;

	vec.resize( 8, 42 );

	CHECK_FALSE( vec.empty() );
	CHECK( 8u == vec.size() );
	CHECK( vec.capacity() >= 8u );
	REQUIRE( vec.data() );
	CHECK( 42 == *vec.data() );
	CHECK( 42 == vec.front() );
	CHECK( 42 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 42, 42, 42, 42, 42, 42, 42, 42 } ) ) );
}

TEST_CASE( "SmallVector_ResizeFillValueLess_Shrinks", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 8 );

	vec.resize( 2, 42 );

	CHECK_FALSE( vec.empty() );
	CHECK( 2u == vec.size() );
	CHECK( vec.capacity() >= 8u );
	REQUIRE( vec.data() );
	CHECK( 0 == *vec.data() );
	CHECK( 0 == vec.front() );
	CHECK( 0 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 0, 0 } ) ) );
}

TEST_CASE( "SmallVector_ShrinkToFitInternal_Noop", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1 };

	vec.shrink_to_fit();

	CHECK( 4u == vec.capacity() );
}

TEST_CASE( "SmallVector_ShrinkToFitExternal_Shrinks", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4, 5, 6 };
	vec.erase( vec.begin(), vec.begin() + 3 );

	vec.shrink_to_fit();

	CHECK( 3u == vec.capacity() );
}

// I did try to test that push_back after resize( max_size() ) throws but that just crashes for using so much
// memory anyway

TEST_CASE( "SmallVector_EmplaceOneAtEnd_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4, 5 };

	const auto it = vec.emplace( vec.end(), 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 6u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 6 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 4, 5, 6 } ) ) );
	CHECK( 6 == *it );
	CHECK( vec.end() - 1 == it );
	for ( std::uint32_t i = 0; i < vec.size(); ++i )
	{
		const int next = i + 1;
		CHECK( next == vec[ i ] );
		CHECK( next == vec.at( i ) );
		CHECK( &vec[ i ] == vec.data() + i );
	}
}

TEST_CASE( "SmallVector_EmplaceOneAtStart_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4, 5 };

	const auto it = vec.emplace( vec.begin(), 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 6u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 6 == *vec.data() );
	CHECK( 6 == vec.front() );
	CHECK( 5 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 6, 1, 2, 3, 4, 5 } ) ) );
	CHECK( 6 == *it );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_EmplaceOneAtEndReallocate_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	const auto it = vec.emplace( vec.end(), 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 5u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 6 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 4, 6 } ) ) );
	CHECK( 6 == *it );
	CHECK( vec.end() - 1 == it );
}

TEST_CASE( "SmallVector_EmplaceOneAtStartReallocate_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	const auto it = vec.emplace( vec.begin(), 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 5u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 6 == *vec.data() );
	CHECK( 6 == vec.front() );
	CHECK( 4 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 6, 1, 2, 3, 4 } ) ) );
	CHECK( 6 == *it );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertCountZero_Noop", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	const auto it = vec.insert( vec.begin(), 0, 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 4u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 4 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertCountReallocate_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	const auto it = vec.insert( vec.begin(), 3, 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 7u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 6 == *vec.data() );
	CHECK( 6 == vec.front() );
	CHECK( 4 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 6, 6, 6, 1, 2, 3, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertCountAsOneAtEnd_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3 };

	const auto it = vec.insert( vec.end(), 1, 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 4u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 6 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 6 } ) ) );
	CHECK( vec.end() - 1 == it );
}

TEST_CASE( "SmallVector_InsertCountAtEnd_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;
	vec.reserve( 8 );
	vec.emplace_back( 1 );
	vec.emplace_back( 2 );
	vec.emplace_back( 3 );
	vec.emplace_back( 4 );
	vec.emplace_back( 5 );

	const auto it = vec.insert( vec.end(), 3, 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 8u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 1 == *vec.data() );
	CHECK( 1 == vec.front() );
	CHECK( 6 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 4, 5, 6, 6, 6 } ) ) );
	CHECK( vec.end() - 3 == it );
}

TEST_CASE( "SmallVector_InsertCountAtStart_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;
	vec.reserve( 8 );
	vec.emplace_back( 1 );
	vec.emplace_back( 2 );
	vec.emplace_back( 3 );
	vec.emplace_back( 4 );
	vec.emplace_back( 5 );

	const auto it = vec.insert( vec.begin(), 3, 6 );

	CHECK_FALSE( vec.empty() );
	CHECK( 8u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 6 == *vec.data() );
	CHECK( 6 == vec.front() );
	CHECK( 5 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 6, 6, 6, 1, 2, 3, 4, 5 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertRangeAtEndReallocates_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };
	constexpr std::array ar{ 1, 2, 3, 2, 1 };

	const auto it = vec.insert( vec.end(), ar.begin(), ar.end() );

	CHECK( 7u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 5, 4, 1, 2, 3, 2, 1 } ) ) );
	CHECK( vec.begin() + 2 == it );
}

TEST_CASE( "SmallVector_InsertRangeAtStartReallocates_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };
	constexpr std::array ar{ 1, 2, 3, 2, 1 };

	const auto it = vec.insert( vec.begin(), ar.begin(), ar.end() );

	CHECK( 7u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 2, 1, 5, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertRangeAtEndNoAlloc_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };
	constexpr std::array ar{ 1, 2 };

	const auto it = vec.insert( vec.end(), ar.begin(), ar.end() );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 5, 4, 1, 2 } ) ) );
	CHECK( vec.begin() + 2 == it );
}

TEST_CASE( "SmallVector_InsertRangeAtStartNoAlloc_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };
	constexpr std::array ar{ 1, 2 };

	const auto it = vec.insert( vec.begin(), ar.begin(), ar.end() );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 5, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertInputIteratorEmptyRange_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };

	const auto it = vec.insert( vec.begin(), std::istream_iterator<int>(), std::istream_iterator<int>() );

	CHECK( 2u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 5, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertInputIteratorValidRange_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };
	std::istringstream stream( "1 23 45 99 13" );

	const auto it = vec.insert( vec.begin(), std::istream_iterator<int>( stream ), std::istream_iterator<int>() );

	CHECK( 7u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 23, 45, 99, 13, 5, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertInitializerListAtEndReallocates_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };

	const auto it = vec.insert( vec.end(), { 1, 2, 3, 2, 1 } );

	CHECK( 7u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 5, 4, 1, 2, 3, 2, 1 } ) ) );
	CHECK( vec.begin() + 2 == it );
}

TEST_CASE( "SmallVector_InsertInitializerListAtStartReallocates_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };

	const auto it = vec.insert( vec.begin(), { 1, 2, 3, 2, 1 } );

	CHECK( 7u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 3, 2, 1, 5, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_InsertInitializerListAtEndNoAlloc_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };

	const auto it = vec.insert( vec.end(), { 1, 2 } );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 5, 4, 1, 2 } ) ) );
	CHECK( vec.begin() + 2 == it );
}

TEST_CASE( "SmallVector_InsertInitializerListAtStartNoAlloc_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 4 };

	const auto it = vec.insert( vec.begin(), { 1, 2 } );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 5, 4 } ) ) );
	CHECK( vec.begin() == it );
}

TEST_CASE( "SmallVector_PopBackEmpty_Asserts", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;
	CHECK_ASSERTS( vec.pop_back(), "Container is empty" );
}

TEST_CASE( "SmallVector_PopBackNotEmpty_RemovesLast", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 2 };

	vec.pop_back();

	CHECK_FALSE( vec.empty() );
	CHECK( 1u == vec.size() );
	REQUIRE( vec.data() );
	CHECK( 5 == *vec.data() );
	CHECK( 5 == vec.front() );
	CHECK( 5 == vec.back() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 5 } ) ) );
}

TEST_CASE( "SmallVector_Clear_IsEmpty", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 5, 2 };

	vec.clear();

	_expectVectorEmpty( vec );
}

TEST_CASE( "SmallVector_AssignCountOnEmpty_HasObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;

	vec.assign( 4, 4 );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 4, 4, 4, 4 } ) ) );
}

TEST_CASE( "SmallVector_AssignCountNotEmpty_OverwritesObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 4, 4 );

	vec.assign( 4, 6 );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 6, 6, 6, 6 } ) ) );
}

TEST_CASE( "SmallVector_AssignLargerCount_GrowsObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 4, 4 );

	vec.assign( 8, 6 );

	CHECK( 8u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 6, 6, 6, 6, 6, 6, 6, 6 } ) ) );
}

TEST_CASE( "SmallVector_AssignSmallerCount_ShrinksObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 4, 4 );

	vec.assign( 2, 6 );

	CHECK( 2u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 6, 6 } ) ) );
}

TEST_CASE( "SmallVector_AssignRange_OverwritesObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 4, 4 );
	constexpr std::array arr{ 1, 2, 3, 4 };

	vec.assign( arr.begin(), arr.end() );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( arr ) );
}

TEST_CASE( "SmallVector_AssignLargerRange_GrowsObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 2, 4 );
	constexpr std::array arr{ 1, 2, 3, 4 };

	vec.assign( arr.begin(), arr.end() );

	CHECK( 4u == vec.size() );
	CHECK_THAT( vec, RangeEquals( arr ) );
}

TEST_CASE( "SmallVector_AssignSmallerRange_ShrinksObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec( 4, 4 );
	constexpr std::array arr{ 1, 2 };

	vec.assign( arr.begin(), arr.end() );

	CHECK( 2u == vec.size() );
	CHECK_THAT( vec, RangeEquals( arr ) );
}

TEST_CASE( "SmallVector_AssignRangeOutOfCapacity_ReallocatesObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;
	constexpr std::array arr{ 1, 2, 3, 4, 5, 6 };

	vec.assign( arr.begin(), arr.end() );

	CHECK( 6u == vec.size() );
	CHECK_THAT( vec, RangeEquals( arr ) );
}

TEST_CASE( "SmallVector_EraseInvalidIterator_Asserts", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec;

	CHECK_ASSERTS( vec.erase( vec.end() + 1 ), "pos must be an iterator in this container" );
}

TEST_CASE( "SmallVector_EraseIterator_RemovesObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	const auto it = vec.erase( vec.begin() + 2 );

	CHECK( 3u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 2, 4 } ) ) );
	CHECK( 4 == *it );
}

TEST_CASE( "SmallVector_EraseInvalidIteratorRange_Asserts", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	CHECK_ASSERTS( [ & ] { vec.erase( vec.end() + 1, vec.end() + 2 ); }(), "must be an iterator in this container" );
}

TEST_CASE( "SmallVector_EraseTransposedIteratorRange_Asserts", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	CHECK_ASSERTS( [ & ] { vec.erase( vec.end(), vec.begin() ); }(),
				   "first and last must form a valid range in this container" );
}

TEST_CASE( "SmallVector_EraseIteratorRange_RemovesObjects", "[small_vector]" )
{
	mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	const auto it = vec.erase( vec.begin() + 1, vec.begin() + 3 );

	CHECK( 2u == vec.size() );
	CHECK_THAT( vec, RangeEquals( ( std::array{ 1, 4 } ) ) );
	CHECK( 4 == *it );
}

TEST_CASE( "SmallVector_SpanConversion_IsSuccess", "[small_vector]" )
{
	const mclo::small_vector<int, 4> vec{ 1, 2, 3, 4 };

	const std::span sp = vec;

	CHECK_THAT( sp, RangeEquals( vec ) );
}

TEST_CASE( "SmallVector_SwapLhsLarger_IsSuccess", "[small_vector]" )
{
	const std::initializer_list<int> lhsList{ 1, 2, 3, 4 };
	const std::initializer_list<int> rhsList{ 9, 8, 7 };
	mclo::small_vector<int, 4> lhs{ lhsList };
	mclo::small_vector<int, 4> rhs{ rhsList };

	lhs.swap( rhs );

	CHECK_THAT( rhs, RangeEquals( lhsList ) );
	CHECK_THAT( lhs, RangeEquals( rhsList ) );
}

TEST_CASE( "SmallVector_SwapRhsLarger_IsSuccess", "[small_vector]" )
{
	const std::initializer_list<int> lhsList{ 1, 2, 3 };
	const std::initializer_list<int> rhsList{ 9, 8, 7, 6 };
	mclo::small_vector<int, 4> lhs{ lhsList };
	mclo::small_vector<int, 4> rhs{ rhsList };

	lhs.swap( rhs );

	CHECK_THAT( rhs, RangeEquals( lhsList ) );
	CHECK_THAT( lhs, RangeEquals( rhsList ) );
}

TEST_CASE( "SmallVector_SwapBothExternal_IsSuccess", "[small_vector]" )
{
	const std::initializer_list<int> lhsList{ 1, 2, 3, 4, 5 };
	const std::initializer_list<int> rhsList{ 9, 8, 7, 6, 5 };
	mclo::small_vector<int, 4> lhs{ lhsList };
	mclo::small_vector<int, 4> rhs{ rhsList };

	lhs.swap( rhs );

	CHECK_THAT( rhs, RangeEquals( lhsList ) );
	CHECK_THAT( lhs, RangeEquals( rhsList ) );
}

TEST_CASE( "SmallVector_SwapSame_IsNoop", "[small_vector]" )
{
	const std::initializer_list<int> initList{ 1, 2, 3, 4, 5 };
	mclo::small_vector<int, 4> vec{ initList };

	vec.swap( vec );

	CHECK_THAT( vec, RangeEquals( initList ) );
}

TEST_CASE( "SmallVector_ReverseIterate_IsCorrect", "[small_vector]" )
{
	const mclo::small_vector<int, 4> vec{ 4, 9, 10, 16 };
	mclo::small_vector<int, 4> reversedVec = vec;
	std::reverse( reversedVec.begin(), reversedVec.end() );

	const bool result = std::equal( vec.rbegin(), vec.rend(), reversedVec.begin(), reversedVec.end() );

	CHECK( result );
}

TEST_CASE( "StaticVector_ResizeMore_Asserts", "[small_vector]" )
{
	mclo::static_vector<int, 4> vec;

	CHECK_ASSERTS( vec.resize( 8 ), "Attempting to grow fixed capacity inline buffer vector" );
}
