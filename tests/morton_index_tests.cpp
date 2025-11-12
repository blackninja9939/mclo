#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "consteval_check.hpp"

#include "mclo/numeric/morton_index.hpp"

#include "mclo/meta/type_list.hpp"

namespace
{
	using test_types = mclo::meta::type_list<mclo::morton_index64, mclo::morton_index32, mclo::morton_index16>;
}

TEMPLATE_LIST_TEST_CASE( "morton_index encoding", "[morton_index]", test_types )
{
	using coord_type = typename TestType::coord_type;
	constexpr coord_type x = 5;
	constexpr coord_type y = 9;

	constexpr TestType morton( x, y );

	CONSTEVAL_CHECK( morton.decode() == std::pair( x, y ) );
}

TEMPLATE_LIST_TEST_CASE( "morton_index compare equal", "[morton_index]", test_types )
{
	using coord_type = typename TestType::coord_type;
	constexpr coord_type x = 5;
	constexpr coord_type y = 9;

	constexpr TestType morton( x, y );
	constexpr TestType morton2( x, y );

	CONSTEVAL_CHECK( morton == morton2 );
	CONSTEVAL_CHECK_FALSE( morton != morton2 );
	CONSTEVAL_CHECK_FALSE( morton < morton2 );
	CONSTEVAL_CHECK( morton <= morton2 );
	CONSTEVAL_CHECK_FALSE( morton > morton2 );
	CONSTEVAL_CHECK( morton >= morton2 );
	CONSTEVAL_CHECK( ( morton <=> morton2 ) == std::strong_ordering::equal );
}

TEMPLATE_LIST_TEST_CASE( "morton_index compare less", "[morton_index]", test_types )
{
	using coord_type = typename TestType::coord_type;
	constexpr coord_type x = 5;
	constexpr coord_type y = 9;

	constexpr TestType morton( x, y );
	constexpr TestType morton2( x + 1, y );

	CONSTEVAL_CHECK_FALSE( morton == morton2 );
	CONSTEVAL_CHECK( morton != morton2 );
	CONSTEVAL_CHECK( morton < morton2 );
	CONSTEVAL_CHECK( morton <= morton2 );
	CONSTEVAL_CHECK_FALSE( morton > morton2 );
	CONSTEVAL_CHECK_FALSE( morton >= morton2 );
	CONSTEVAL_CHECK( ( morton <=> morton2 ) == std::strong_ordering::less );
}

TEMPLATE_LIST_TEST_CASE( "morton_index compare greater", "[morton_index]", test_types )
{
	using coord_type = typename TestType::coord_type;
	constexpr coord_type x = 5;
	constexpr coord_type y = 9;

	constexpr TestType morton( x, y );
	constexpr TestType morton2( x - 1, y );

	CONSTEVAL_CHECK_FALSE( morton == morton2 );
	CONSTEVAL_CHECK( morton != morton2 );
	CONSTEVAL_CHECK_FALSE( morton < morton2 );
	CONSTEVAL_CHECK_FALSE( morton <= morton2 );
	CONSTEVAL_CHECK( morton > morton2 );
	CONSTEVAL_CHECK( morton >= morton2 );
	CONSTEVAL_CHECK( ( morton <=> morton2 ) == std::strong_ordering::greater );
}
