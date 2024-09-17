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
		count
	};

	static constexpr std::array<test_enum, 5> ALL_VALUES{
		test_enum::first,
		test_enum::second,
		test_enum::third,
		test_enum::fourth,
		test_enum::fifth,
	};

	enum class enum_Size
	{
		first,
		Size,
	};
	static_assert( enum_Size::Size == mclo::enum_size<enum_Size> );

	enum class enum_size
	{
		first,
		size,
	};
	static_assert( enum_size::size == mclo::enum_size<enum_size> );

	enum class enum__Size
	{
		first,
		_Size,
	};
	static_assert( enum__Size::_Size == mclo::enum_size<enum__Size> );

	enum class enum__size
	{
		first,
		_size,
	};
	static_assert( enum__size::_size == mclo::enum_size<enum__size> );

	enum class enum_Count
	{
		first,
		Count,
	};
	static_assert( enum_Count::Count == mclo::enum_size<enum_Count> );

	enum class enum_count
	{
		first,
		count,
	};
	static_assert( enum_count::count == mclo::enum_size<enum_count> );

	enum class enum__Count
	{
		first,
		_Count,
	};
	static_assert( enum__Count::_Count == mclo::enum_size<enum__Count> );

	enum class enum__count
	{
		first,
		_count,
	};
	static_assert( enum__count::_count == mclo::enum_size<enum__count> );

	static_assert( specialized_size_enum::my_size == mclo::enum_size<specialized_size_enum> );

	enum class enum_max_underlying
	{
		first,
		max_underlying = std::numeric_limits<int>::max(),
	};
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
