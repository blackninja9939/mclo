#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include "mclo/algorithm/radix_sort.hpp"
#include "mclo/container/span.hpp"
#include "mclo/meta/type_list.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
	template <typename KeyT>
	struct keyed_value
	{
		KeyT key{};
		std::uint32_t payload{};

		auto operator<=>( const keyed_value& ) const = default;
	};

	struct extract_key
	{
		template <typename T>
		[[nodiscard]] constexpr auto operator()( const T& value ) const noexcept
		{
			return value.key;
		}
	};

	template <typename T, typename KeyExtractor, std::size_t DataSize>
	struct radix_sort_test_case
	{
		using value_type = T;
		using key_extractor = KeyExtractor;
		using key_type = std::decay_t<std::invoke_result_t<KeyExtractor, const T&>>;
		static constexpr std::size_t data_size = DataSize;
	};

	template <typename T>
	[[nodiscard]] T make_value( const std::size_t index )
	{
		if constexpr ( std::same_as<T, bool> )
		{
			return ( index % 3 ) == 0;
		}
		else if constexpr ( std::integral<T> )
		{
			using unsigned_t = std::make_unsigned_t<T>;
			const auto mixed = static_cast<unsigned_t>( index * 0x9E3779B97F4A7C15ull + 0xBF58476D1CE4E5B9ull );
			return static_cast<T>( mixed );
		}
		else
		{
			const double x = static_cast<double>( ( index * 17 ) % 10007 ) - 5000.0;
			return static_cast<T>( x / 7.0 );
		}
	}

	template <typename T>
	[[nodiscard]] keyed_value<T> make_keyed_value( const std::size_t index )
	{
		return keyed_value<T>{ make_value<T>( index ), static_cast<std::uint32_t>( index * 13u + 7u ) };
	}

	template <typename T>
	void fill_sequence( std::vector<T>& values )
	{
		for ( std::size_t i = 0; i < values.size(); ++i )
		{
			values[ i ] = make_value<T>( i );
		}
	}

	template <typename T>
	void fill_sequence( std::vector<keyed_value<T>>& values )
	{
		for ( std::size_t i = 0; i < values.size(); ++i )
		{
			values[ i ] = make_keyed_value<T>( i );
		}
	}

	// clang-format off
	using non_bool_test_cases = mclo::meta::type_list<
		radix_sort_test_case<keyed_value<bool>, extract_key, 257>,
		radix_sort_test_case<std::uint8_t, std::identity, 257>,
		radix_sort_test_case<std::int8_t, std::identity, 257>,
		radix_sort_test_case<std::uint16_t, std::identity, 257>,
		radix_sort_test_case<std::int16_t, std::identity, 257>,
		radix_sort_test_case<std::uint32_t, std::identity, 257>,
		radix_sort_test_case<std::int32_t, std::identity, 257>,
		radix_sort_test_case<std::uint64_t, std::identity, 257>,
		radix_sort_test_case<std::int64_t, std::identity, 257>,
		radix_sort_test_case<float, std::identity, 257>,
		radix_sort_test_case<double, std::identity, 257>,
		radix_sort_test_case<keyed_value<std::uint8_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<std::int8_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<std::uint16_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<std::int16_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<std::uint32_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<std::int32_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<std::uint64_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<std::int64_t>, extract_key, 257>,
		radix_sort_test_case<keyed_value<float>, extract_key, 257>,
		radix_sort_test_case<keyed_value<double>, extract_key, 257>,
		// CountT boundary coverage for counting path (1-byte key)
		radix_sort_test_case<std::uint8_t, std::identity, 255>,
		radix_sort_test_case<std::uint8_t, std::identity, 256>,
		radix_sort_test_case<std::uint8_t, std::identity, 65535>,
		radix_sort_test_case<std::uint8_t, std::identity, 65536>,
		// CountT boundary coverage for radix path (multi-byte key)
		radix_sort_test_case<std::uint32_t, std::identity, 255>,
		radix_sort_test_case<std::uint32_t, std::identity, 256>,
		radix_sort_test_case<std::uint32_t, std::identity, 65535>,
		radix_sort_test_case<std::uint32_t, std::identity, 65536>
	>;
	// clang-format on
}

TEST_CASE( "radix_sort random bool input, radix_sort, same result as std::stable_sort", "[algorithm][radix_sort]" )
{
	constexpr std::size_t data_size = 65536;
	auto source = std::make_unique<bool[]>( data_size );
	auto output = std::make_unique<bool[]>( data_size );
	auto expected = std::make_unique<bool[]>( data_size );
	for ( std::size_t i = 0; i < data_size; ++i )
	{
		source[ i ] = make_value<bool>( i );
		expected[ i ] = source[ i ];
	}

	const auto result = mclo::radix_sort( source.get(), source.get() + data_size, output.get() );

	std::stable_sort( expected.get(), expected.get() + data_size );
	const bool* const actual = result == mclo::radix_sort_result::in_output ? output.get() : source.get();
	CHECK_THAT( mclo::span( actual, data_size ),
				Catch::Matchers::RangeEquals( mclo::span( expected.get(), data_size ) ) );
}

TEMPLATE_LIST_TEST_CASE( "radix_sort random input, radix_sort, same result as std::stable_sort",
						 "[algorithm][radix_sort]",
						 non_bool_test_cases )
{
	using value_type = typename TestType::value_type;
	using key_extractor = typename TestType::key_extractor;
	constexpr std::size_t data_size = TestType::data_size;
	std::vector<value_type> source( data_size );
	fill_sequence( source );

	std::vector<value_type> output( data_size );
	const auto result = mclo::radix_sort( source.begin(), source.end(), output.begin(), key_extractor{} );

	auto less = []( const value_type& lhs, const value_type& rhs ) {
		return std::invoke( key_extractor{}, lhs ) < std::invoke( key_extractor{}, rhs );
	};
	std::vector<value_type> expected = source;
	std::stable_sort( expected.begin(), expected.end(), less );
	const auto& actual = result == mclo::radix_sort_result::in_output ? output : source;
	CHECK( actual == expected );
}
