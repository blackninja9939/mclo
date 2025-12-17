#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "assert_macros.hpp"

#include "mclo/container/bitset.hpp"
#include "mclo/container/dynamic_bitset.hpp"
#include "mclo/container/small_vector.hpp"
#include "mclo/meta/type_list.hpp"

using namespace Catch::Matchers;

namespace
{
	constexpr std::size_t bitset_size = 34;

	template <std::unsigned_integral UnderlyingType, typename UnderlyingContainer = std::vector<UnderlyingType>>
	class dynamic_bitset_wrapper : public mclo::dynamic_bitset<UnderlyingType, UnderlyingContainer>
	{
	public:
		using base = mclo::dynamic_bitset<UnderlyingType, UnderlyingContainer>;
		using base::base;

		dynamic_bitset_wrapper()
			: base( bitset_size )
		{
		}
		dynamic_bitset_wrapper( base bs )
			: base( std::move( bs ) )
		{
		}

		[[nodiscard]] constexpr bool operator==( const dynamic_bitset_wrapper& other ) const noexcept = default;
	};

	using test_types =
		mclo::meta::type_list<mclo::bitset<bitset_size, std::uint32_t>,
							  dynamic_bitset_wrapper<std::uint32_t>,
							  dynamic_bitset_wrapper<std::uint32_t, mclo::small_vector<std::uint32_t, 4>>,
							  mclo::bitset<bitset_size, std::uint64_t>,
							  dynamic_bitset_wrapper<std::uint64_t>,
							  dynamic_bitset_wrapper<std::uint64_t, mclo::small_vector<std::uint64_t, 4>>>;

	template <typename Bitset, std::size_t Size>
	void check_only_these_set( const Bitset& bitset, std::array<std::size_t, Size> indices )
	{
		using size_type = typename Bitset::size_type;
		std::ranges::sort( indices );
		size_type indices_index = 0;

		bitset.for_each_set( [ & ]( const size_type set_index ) { CHECK( set_index == indices[ indices_index++ ] ); } );
		CHECK( indices_index == Size );
	}

	template <std::size_t... indices>
	constexpr std::array<std::size_t, sizeof...( indices )> index_array()
	{
		return { indices... };
	}

	template <typename CharT, std::size_t Size>
	static std::basic_string<CharT> make_string_for_bits( const std::array<std::size_t, Size>& set_indices,
														  const CharT unset_char,
														  const CharT set_char )
	{
		std::basic_string<CharT> str( bitset_size, unset_char );
		for ( const std::size_t index : set_indices )
		{
			str[ index ] = set_char;
		}
		std::reverse( str.begin(), str.end() );
		return str;
	}

	template <typename Container, std::size_t Size>
	static void fill_bool_container_for_bits( const std::array<std::size_t, Size>& set_indices, Container& container )
	{
		auto first = container.begin();
		std::size_t current_position = 0;
		for ( const std::size_t index : set_indices )
		{
			std::advance( first, index - current_position );
			*first = true;
			current_position = index; // Track where we actually are
		}
	}

	template <template <typename...> typename Container, std::size_t Size>
	static Container<bool> make_bool_container_for_bits( const std::array<std::size_t, Size>& set_indices )
	{
		Container<bool> container( bitset_size );
		fill_bool_container_for_bits( set_indices, container );
		return container;
	}
}

template <std::unsigned_integral UnderlyingType, typename UnderlyingContainer>
struct std::hash<dynamic_bitset_wrapper<UnderlyingType, UnderlyingContainer>>
{
	using type = dynamic_bitset_wrapper<UnderlyingType, UnderlyingContainer>;
	[[nodiscard]] std::size_t operator()( const type& wrapper ) const noexcept
	{
		return std::hash<typename type::base>()( wrapper );
	}
};

TEMPLATE_LIST_TEST_CASE( "bitset default constructor", "[bitset]", test_types )
{
	using size_type = typename TestType::size_type;
	const TestType set;
	CHECK( set.size() == bitset_size );
	CHECK_FALSE( set.all() );
	CHECK_FALSE( set.any() );
	CHECK( set.none() );
	CHECK( set.count() == 0 );
	CHECK( set.find_first_set() == TestType::npos );
	CHECK( set.find_first_unset() == 0 );

	bool any = false;
	set.for_each_set( [ &any ]( const size_type ) { any = true; } );

	for ( size_type index = 0; index < set.size(); ++index )
	{
		CHECK_FALSE( set.test( index ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "Bitset, construct from string, correct are set", "[bitset]", test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	constexpr const char set_char = 'a';
	constexpr const char unset_char = '*';
	const std::string str = make_string_for_bits( set_indices, unset_char, set_char );

	const TestType set( str, unset_char, set_char );

	check_only_these_set( set, set_indices );
}

TEMPLATE_LIST_TEST_CASE( "Bitset, construct from string with invalid set char, throws invalid_argument",
						 "[bitset]",
						 test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	constexpr const char set_char = 'a';
	constexpr const char unset_char = '*';
	const std::string str = make_string_for_bits( set_indices, unset_char, 'q' );

	const auto func = [ & ] { [[maybe_unused]] const TestType set( str, unset_char, set_char ); };

	CHECK_THROWS_AS( func(), std::invalid_argument );
}

TEMPLATE_LIST_TEST_CASE( "Bitset, construct from string with invalid unset char, throws invalid_argument",
						 "[bitset]",
						 test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	constexpr const char set_char = 'a';
	constexpr const char unset_char = '*';
	const std::string str = make_string_for_bits( set_indices, '1', set_char );

	const auto func = [ & ] { [[maybe_unused]] const TestType set( str, unset_char, set_char ); };

	CHECK_THROWS_AS( func(), std::invalid_argument );
}

TEMPLATE_LIST_TEST_CASE( "Bitset, construct from bool convertible range, correct are set", "[bitset]", test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	const auto container = make_bool_container_for_bits<std::vector>( set_indices );

	const TestType set( container );

	check_only_these_set( set, set_indices );
}

TEMPLATE_LIST_TEST_CASE( "Bitset, construct from bool convertible list, correct are set", "[bitset]", test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	const auto container = make_bool_container_for_bits<std::list>( set_indices );

	const TestType set( container );

	check_only_these_set( set, set_indices );
}

TEST_CASE( "Bitset, construct from bool convertible forward list, correct are set", "[bitset]" )
{
	// Dynamic bitset must allocate so it requires a sized_range which forward_list is not
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	const auto container = make_bool_container_for_bits<std::forward_list>( set_indices );

	const mclo::bitset<bitset_size> set( container );

	check_only_these_set( set, set_indices );
}

TEMPLATE_LIST_TEST_CASE( "Bitset, construct from bool convertible array, correct are set", "[bitset]", test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	std::array<bool, bitset_size> container{};
	fill_bool_container_for_bits( set_indices, container );

	const TestType set( container );

	check_only_these_set( set, set_indices );
}

TEST_CASE( "Bitset, construct from too large bool convertible range, extra are ignored", "[bitset]" )
{
	constexpr auto set_indices = index_array<0, 1, 4>();
	std::array<bool, bitset_size> container{};
	fill_bool_container_for_bits( set_indices, container );
	container[ 16 ] = true;

	const mclo::bitset<8> set( container );

	check_only_these_set( set, set_indices );
}

TEMPLATE_LIST_TEST_CASE( "bitset set", "[bitset]", test_types )
{
	using size_type = typename TestType::size_type;
	TestType set;

	set.set( 32 );
	CHECK( set.test( 32 ) );
	CHECK_FALSE( set.all() );
	CHECK( set.any() );
	CHECK_FALSE( set.none() );
	CHECK( set.count() == 1 );
	CHECK( set.find_first_set() == 32 );
	CHECK( set.find_first_unset() == 0 );

	set.for_each_set( []( const size_type index ) { CHECK( index == 32 ); } );

	for ( size_type index = 0; index < set.size(); ++index )
	{
		if ( index == 32 )
		{
			continue;
		}
		CHECK_FALSE( set.test( index ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "bitset find_first_set loop", "[bitset]", test_types )
{
	using size_type = typename TestType::size_type;
	TestType set;
	set.set( 4 ).set( 32 );

	for ( size_type index = set.find_first_set(); index != TestType::npos; index = set.find_first_set( index + 1 ) )
	{
		CHECK( ( index == 4 || index == 32 ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "bitset find_first_unset loop", "[bitset]", test_types )
{
	using size_type = typename TestType::size_type;
	TestType set;
	set.set().reset( 4 ).reset( 32 ).reset( 33 );

	size_type count = 0;
	for ( size_type index = set.find_first_unset(); index != TestType::npos; index = set.find_first_unset( index + 1 ) )
	{
		CHECK( ( index == 4 || index == 32 || index == 33 ) );
		++count;
	}

	CHECK( count == 3 );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator&=", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	TestType other;
	other.set( 4 ).set( 33 );

	set &= other;
	check_only_these_set( set, index_array<4, 33>() );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator&", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	TestType other;
	other.set( 4 ).set( 33 );

	check_only_these_set( set & other, index_array<4, 33>() );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator|=", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	TestType other;
	other.set( 2 ).set( 16 ).set( 32 );

	set |= other;
	check_only_these_set( set, index_array<2, 4, 16, 32, 33>() );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator|", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	TestType other;
	other.set( 2 ).set( 16 ).set( 32 );

	check_only_these_set( set | other, index_array<2, 4, 16, 32, 33>() );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator^=", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	TestType other;
	other.set( 2 ).set( 16 ).set( 32 );

	set ^= other;
	check_only_these_set( set, index_array<2, 4, 16, 33>() );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator^", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	TestType other;
	other.set( 2 ).set( 16 ).set( 32 );

	check_only_these_set( set ^ other, index_array<2, 4, 16, 33>() );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator~", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	const TestType flipped( ~set );

	set.flip();
	CHECK( set == flipped );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator<<=", "[bitset]", test_types )
{
	TestType set;
	set.set( 0 ).set( 1 ).set( 4 ).set( 32 ).set( 33 );

	SECTION( "Shift small" )
	{
		set <<= 10;
		check_only_these_set( set, index_array<10, 11, 14>() );
	}
	SECTION( "Shift large" )
	{
		set <<= 33;
		check_only_these_set( set, index_array<33>() );
	}
}

TEMPLATE_LIST_TEST_CASE( "bitset operator>>=", "[bitset]", test_types )
{
	TestType set;
	set.set( 0 ).set( 1 ).set( 4 ).set( 32 ).set( 33 );

	SECTION( "Shift small" )
	{
		set >>= 10;
		check_only_these_set( set, index_array<22, 23>() );
	}
	SECTION( "Shift large" )
	{
		set >>= 33;
		check_only_these_set( set, index_array<0>() );
	}
}

TEMPLATE_LIST_TEST_CASE( "bitset operator<<", "[bitset]", test_types )
{
	TestType set;
	set.set( 0 ).set( 1 ).set( 4 ).set( 32 ).set( 33 );

	check_only_these_set( set << 10, index_array<10, 11, 14>() );

	check_only_these_set( set << 33, index_array<33>() );
}

TEMPLATE_LIST_TEST_CASE( "bitset operator>>", "[bitset]", test_types )
{
	TestType set;
	set.set( 0 ).set( 1 ).set( 4 ).set( 32 ).set( 33 );

	check_only_these_set( set >> 10, index_array<22, 23>() );

	check_only_these_set( set >> 33, index_array<0>() );
}

TEMPLATE_LIST_TEST_CASE( "Bitset, to string, result matches", "[bitset]", test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	constexpr const char set_char = 'a';
	constexpr const char unset_char = '*';
	const std::string expected = make_string_for_bits( set_indices, unset_char, set_char );
	const TestType set( expected, unset_char, set_char );

	const std::string str = set.to_string( unset_char, set_char );

	CHECK( expected == str );
}

TEMPLATE_LIST_TEST_CASE( "Bitset, to wide string, result matches", "[bitset]", test_types )
{
	constexpr auto set_indices = index_array<0, 1, 4, 32, 33>();
	constexpr const wchar_t set_char = L'a';
	constexpr const wchar_t unset_char = L'*';
	const std::wstring expected = make_string_for_bits( set_indices, unset_char, set_char );
	const TestType set( expected, unset_char, set_char );

	const std::wstring str = set.to_string( unset_char, set_char );

	CHECK( expected == str );
}

TEMPLATE_LIST_TEST_CASE( "bitset hash", "[bitset]", test_types )
{
	TestType set;
	set.set( 4 ).set( 32 ).set( 33 );

	TestType other;
	other.set( 2 ).set( 16 ).set( 32 );

	using hasher = std::hash<TestType>;

	const std::size_t set_hash = hasher{}( set );
	const std::size_t other_hash = hasher{}( other );

	CHECK( set_hash != other_hash );
}

TEST_CASE( "dynamic bitset equals", "[bitset]" )
{
	mclo::dynamic_bitset<std::uint32_t> set( 32 );
	mclo::dynamic_bitset<std::uint32_t> other( 32 );

	SECTION( "Same size same data" )
	{
		set.set( 4 ).set( 16 );
		other.set( 4 ).set( 16 );

		CHECK( set == other );
	}
	SECTION( "Same size different data" )
	{
		set.set( 4 ).set( 16 );
		other.set( 4 ).set( 20 );

		CHECK_FALSE( set == other );
	}
	SECTION( "Different size same data" )
	{
		set.set( 4 ).set( 16 );
		other.resize( 29 ).set( 4 ).set( 16 );

		CHECK_FALSE( set == other );
	}
	SECTION( "Different size different data" )
	{
		set.set( 4 ).set( 16 );
		other.resize( 29 ).set( 4 ).set( 20 );

		CHECK_FALSE( set == other );
	}
}

TEST_CASE( "Dynamic bitset with custom underlying container", "[bitset]" )
{
	mclo::dynamic_bitset<std::size_t, mclo::small_vector<std::size_t, 4>> set( 4 );
	set.set( 0 ).set( 2 ).set( 3 );

	set.resize( 8 );

	CHECK( set.size() == 8 );
	CHECK( set.count() == 3 );
}

TEST_CASE( "Dynamic bitset from underlying container", "[bitset]" )
{
	const mclo::dynamic_bitset<> set( std::size_t( 10 ), std::vector<std::uint64_t>{ 255 } );

	CHECK( set.size() == 10 );
	CHECK( set.count() == 8 );
}

TEST_CASE( "Dynamic bitset from underlying container larger than possible", "[bitset]" )
{
	const auto func = [] {
		[[maybe_unused]] const mclo::dynamic_bitset<> set( std::size_t( 65 ), std::vector<std::uint64_t>{ 255 } );
	};
	CHECK_ASSERTS( func(), "Size greater than max bits per value in container" );
}

TEST_CASE( "Dynamic bitset copy from underlying container", "[bitset]" )
{
	const std::vector<std::uint64_t> vec{ 255 };
	const mclo::dynamic_bitset<> set( std::size_t( 10 ), vec );

	CHECK( set.size() == 10 );
	CHECK( set.count() == 8 );
}

TEST_CASE( "Dynamic bitset copy from underlying container larger than possible", "[bitset]" )
{
	const std::vector<std::uint64_t> vec{ 255 };
	const auto func = [ &vec ] { [[maybe_unused]] const mclo::dynamic_bitset<> set( std::size_t( 65 ), vec ); };
	CHECK_ASSERTS( func(), "Size greater than max bits per value in container" );
}
