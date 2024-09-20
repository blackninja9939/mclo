#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/enum_range.hpp"

#include <array>
#include <ranges>
#include <span>

using namespace Catch::Matchers;

namespace
{
	enum class specialized_size_enum
	{
		first,
		my_size,
	};
}

template <>
inline constexpr specialized_size_enum mclo::enum_size<specialized_size_enum> = specialized_size_enum::my_size;

namespace
{
	enum class test_enum
	{
		first,
		second,
		third,
		fourth,
		fifth,
		enum_size
	};

	static constexpr std::array<test_enum, 5> ALL_VALUES{
		test_enum::first,
		test_enum::second,
		test_enum::third,
		test_enum::fourth,
		test_enum::fifth,
	};

	static_assert( mclo::has_enum_size<specialized_size_enum> );
	static_assert( specialized_size_enum::my_size == mclo::enum_size<specialized_size_enum> );

	enum class enum_no_size
	{
		first,
		last,
	};
	static_assert( !mclo::has_enum_size<enum_no_size> );

	enum class enum_max_underlying
	{
		first,
		max_underlying = std::numeric_limits<int>::max(),
	};
	static_assert( !mclo::has_enum_size<enum_max_underlying> );

	static_assert( std::ranges::random_access_range<mclo::enum_range<test_enum>> );
}

TEST_CASE( "enum_range over entire range", "[enum_range]" )
{
	const mclo::enum_range<test_enum> range;
	CHECK_THAT( range, UnorderedRangeEquals( ALL_VALUES ) );
}

TEST_CASE( "enum_range over inclusive pair", "[enum_range]" )
{
	const mclo::enum_range range( test_enum::second, test_enum::fourth );
	CHECK_THAT( range, UnorderedRangeEquals( std::span( ALL_VALUES ).subspan<1, 3>() ) );
}

TEST_CASE( "enum_range over exclusive range", "[enum_range]" )
{
	const mclo::enum_range range( mclo::exclusive_enum_range, test_enum::second, test_enum::fourth );
	CHECK_THAT( range, UnorderedRangeEquals( std::span( ALL_VALUES ).subspan<1, 2>() ) );
}

TEST_CASE( "enum_range over empty range", "[enum_range]" )
{
	const mclo::enum_range range( mclo::exclusive_enum_range, test_enum::second, test_enum::second );
	CHECK_THAT( range, IsEmpty() );
}
