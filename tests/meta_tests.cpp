#include "mclo/meta.hpp"

#include "mclo/type_traits.hpp"

namespace
{
	using namespace mclo::meta;

	using test_list = type_list<int, float, bool>;

	static_assert( is_list<test_list> );
	static_assert( !is_list<int> );

	static_assert( size<test_list> == 3 );
	static_assert( size<type_list<>> == 0 );

	static_assert( !empty<test_list> );
	static_assert( empty<type_list<>> );

	static_assert( std::is_same_v<nth<0, test_list>, int> );
	static_assert( std::is_same_v<nth<1, test_list>, float> );
	static_assert( std::is_same_v<nth<2, test_list>, bool> );

	static_assert( std::is_same_v<first<test_list>, int> );
	static_assert( std::is_same_v<last<test_list>, bool> );

	static_assert( std::is_same_v<pop_first<test_list>, type_list<float, bool>> );

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
}
