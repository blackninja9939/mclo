#include "mclo/meta/apply.hpp"
#include "mclo/meta/count.hpp"
#include "mclo/meta/filter.hpp"
#include "mclo/meta/first.hpp"
#include "mclo/meta/flatten.hpp"
#include "mclo/meta/index_of.hpp"
#include "mclo/meta/join.hpp"
#include "mclo/meta/last.hpp"
#include "mclo/meta/nth_type.hpp"
#include "mclo/meta/pop_first.hpp"
#include "mclo/meta/pop_last.hpp"
#include "mclo/meta/push_first.hpp"
#include "mclo/meta/push_last.hpp"
#include "mclo/meta/repeat.hpp"
#include "mclo/meta/transform.hpp"
#include "mclo/meta/type_aliases.hpp"
#include "mclo/meta/type_id.hpp"
#include "mclo/meta/type_list.hpp"
#include "mclo/meta/type_name.hpp"

#include <type_traits>

namespace
{
	using namespace mclo::meta;

	using test_list = type_list<int, float, bool>;

	static_assert( is_list<test_list> );
	static_assert( !is_list<int> );

	static_assert( test_list::size == 3 );
	static_assert( type_list<>::size == 0 );

	static_assert( !empty<test_list> );
	static_assert( empty<type_list<>> );

	static_assert( std::is_same_v<nth<0, test_list>, int> );
	static_assert( std::is_same_v<nth<1, test_list>, float> );
	static_assert( std::is_same_v<nth<2, test_list>, bool> );

	static_assert( index_of_v<int, test_list> == 0 );
	static_assert( index_of_v<float, test_list> == 1 );
	static_assert( index_of_v<bool, test_list> == 2 );

	static_assert( count_v<int, test_list> == 1 );
	static_assert( count_v<float, test_list> == 1 );
	static_assert( count_v<bool, test_list> == 1 );
	static_assert( count_v<int, type_list<int, int, bool, int, char>> == 3 );

	static_assert( std::is_same_v<first<test_list>, int> );
	static_assert( std::is_same_v<last<test_list>, bool> );

	static_assert( std::is_same_v<pop_first<test_list>, type_list<float, bool>> );
	static_assert( std::is_same_v<pop_last<test_list>, type_list<int, float>> );

	static_assert( std::is_same_v<push_first<void, test_list>, type_list<void, int, float, bool>> );
	static_assert( std::is_same_v<push_last<void, test_list>, type_list<int, float, bool, void>> );

	static_assert( apply_v<std::is_same, type_list<int, int>> );

	static_assert( std::is_same_v<join<>, type_list<>> );
	static_assert( std::is_same_v<join<type_list<int>, type_list<float>, type_list<bool>>, test_list> );
	static_assert( std::is_same_v<join<test_list>, test_list> );
	static_assert( std::is_same_v<join<test_list, test_list>, type_list<int, float, bool, int, float, bool>> );
	static_assert( std::is_same_v<join<test_list, test_list, test_list>,
								  type_list<int, float, bool, int, float, bool, int, float, bool>> );

	static_assert(
		std::is_same_v<transform<std::add_const, test_list>, type_list<const int, const float, const bool>> );

	static_assert( std::is_same_v<filter<std::is_integral, test_list>, type_list<int, bool>> );

	static_assert(
		std::is_same_v<repeat<3, test_list>, type_list<int, float, bool, int, float, bool, int, float, bool>> );
	static_assert( std::is_same_v<repeat<1, test_list>, test_list> );
	static_assert( std::is_same_v<repeat<0, test_list>, type_list<>> );

	static_assert( std::is_same_v<flatten<type_list<int, test_list>>, type_list<int, int, float, bool>> );

	constexpr auto type_id_int = type_id<int>;
	static_assert( type_id<int> == type_id_int );
	static_assert( type_id<int> != type_id<float> );
	static_assert( type_id<int> != type_id<const int> );
	static_assert( type_id<int> != type_id<volatile int> );
	static_assert( type_id<int> != type_id<int*> );
	static_assert( type_id<int> != type_id<void> );

	static_assert( std::is_same_v<type_of<type_id_int>, int> );
	static_assert( std::is_same_v<type_of<type_id<int>>, int> );
	static_assert( std::is_same_v<type_of<type_id<const int>>, const int> );
	static_assert( !std::is_same_v<type_of<type_id<const int>>, int> );

	static_assert( type_name_v<int> == "int" );
	static_assert( type_name_v<float> == "float" );
}
