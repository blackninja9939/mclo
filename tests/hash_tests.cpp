#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/hash/fnv1a_hasher.hpp"
#include "mclo/hash/hash.hpp"
#include "mclo/hash/murmur_hash_3.hpp"
#include "mclo/hash/rapidhash.hpp"
#include "mclo/hash/std_types.hpp"
#include "mclo/meta/all_of.hpp"
#include "mclo/meta/transform.hpp"
#include "mclo/meta/type_aliases.hpp"
#include "mclo/meta/type_list.hpp"

namespace
{
	static_assert( sizeof( mclo::fnv1a_hasher ) == 8 );

	struct test_type
	{
		int a;
		int b;

		template <mclo::hasher Hasher>
		friend void hash_append( Hasher& hasher, const test_type& value ) noexcept
		{
			hash_append( hasher, value.a );
			hash_append( hasher, value.b );
		}
	};

	struct bitwise_hashable_type
	{
		int a;
		int b;
	};

	enum class test_enum
	{
		a,
		b,
		c
	};

	template <typename T>
	struct add_optional
	{
		using type = std::optional<T>;
	};

	using optional_type_list = mclo::meta::transform<add_optional, mclo::meta::integers>;

	template <typename T>
	struct is_bit_hashable
	{
		static constexpr bool value = mclo::enable_bitwise_hash<T>;
	};
	static_assert( mclo::meta::all_of_v<is_bit_hashable, mclo::meta::integers> );

	static_assert( mclo::default_hashable<std::string> );

	using hasher_types = mclo::meta::type_list<mclo::fnv1a_hasher, mclo::murmur_hash_3, mclo::rapidhash>;
}

template <>
inline constexpr bool mclo::enable_bitwise_hash<bitwise_hashable_type> = true;

TEMPLATE_LIST_TEST_CASE( "hash built in types", "[hash]", mclo::meta::integers )
{
	const std::size_t result = mclo::hash_object( TestType( 42 ) );
	CHECK( result != 42 );
}

TEMPLATE_LIST_TEST_CASE( "hash optional", "[hash]", optional_type_list )
{
	const std::size_t result = mclo::hash_object( TestType( 42 ) );
	CHECK( result != 42 );

	const std::size_t nullopt_result = mclo::hash_object( TestType( std::nullopt ) );

	CHECK( nullopt_result != result );
}

TEST_CASE( "hash enum", "[hash]" )
{
	const std::size_t result = mclo::hash_object( test_enum::b );
	CHECK( result != static_cast<std::size_t>( test_enum::b ) );
}

TEST_CASE( "hash custom type", "[hash]" )
{
	const std::size_t result = mclo::hash_object( test_type{ 42, 42 } );
	CHECK( result != 42 );
}

TEST_CASE( "hash bitwise type", "[hash]" )
{
	const std::size_t result = mclo::hash_object( bitwise_hashable_type{ 42, 42 } );
	CHECK( result != 42 );
}

TEST_CASE( "hash range", "[hash]" )
{
	const std::vector<test_type> vec{
		{42, 42},
		{11, 11},
		{16, 32},
	};
	const std::size_t result = mclo::hash_range( vec );
	CHECK( result != 0 );
}

TEST_CASE( "hash contiguous bitwise hashable range", "[hash]" )
{
	const std::vector<bitwise_hashable_type> vec{
		{42, 42},
		{11, 11},
		{16, 32},
	};
	const std::size_t result = mclo::hash_range( vec );
	CHECK( result != 0 );
}

TEMPLATE_LIST_TEST_CASE( "hash specific hashers", "[hash]", hasher_types )
{
	const std::vector<test_type> vec{
		{42, 42},
		{11, 11},
		{16, 32},
	};
	const std::size_t result = mclo::hash_range<TestType>( vec );
	CHECK( result != 0 );
}

TEMPLATE_LIST_TEST_CASE( "hash specific hashers bitwise types", "[hash]", hasher_types )
{
	const std::vector<bitwise_hashable_type> vec{
		{42, 42},
		{11, 11},
		{16, 32},
	};
	const std::size_t result = mclo::hash_range<TestType>( vec );
	CHECK( result != 0 );
}
