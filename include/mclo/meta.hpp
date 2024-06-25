#pragma once

#include "type_traits.hpp"

#include <cstddef>
#include <utility>

namespace mclo::meta
{
	template <typename... Ts>
	struct type_list
	{
	};

	// Type accessing

	namespace detail
	{
		// Suffixed with impl so that using just the main type picks up the convenience alias instead of the the ones in
		// the shared detail namespace which require the manual typename :: type
		template <typename List>
		struct is_list_impl
		{
			static constexpr bool value = false;
		};

		template <typename... Ts>
		struct is_list_impl<type_list<Ts...>>
		{
			static constexpr bool value = true;
		};

		template <typename List>
		struct size_impl;

		template <typename... Ts>
		struct size_impl<type_list<Ts...>>
		{
			static constexpr std::size_t value = sizeof...( Ts );
		};

		template <std::size_t Index, typename List>
		struct nth_impl;

		template <std::size_t Index>
		struct nth_impl<Index, type_list<>>
		{
			static_assert( mclo::always_false<std::integral_constant<std::size_t, Index>>, "Empty type list" );
		};

		template <typename T, typename... Remaining>
		struct nth_impl<0, type_list<T, Remaining...>>
		{
			using type = T;
		};

		template <std::size_t Index, typename T, typename... Remaining>
		struct nth_impl<Index, type_list<T, Remaining...>> : nth_impl<Index - 1, type_list<Remaining...>>
		{
		};

		template <typename List>
		struct first_impl;

		template <typename T, typename... Ts>
		struct first_impl<type_list<T, Ts...>>
		{
			using type = T;
		};

		template <typename List>
		struct last_impl;

		template <typename... Ts>
		struct last_impl<type_list<Ts...>>
		{
			using type = typename decltype( ( type_identity<Ts>{}, ... ) )::type;
		};

		template <typename List>
		struct pop_first_impl;

		template <typename T, typename... Ts>
		struct pop_first_impl<type_list<T, Ts...>>
		{
			using type = type_list<Ts...>;
		};

		template <typename T, typename List>
		struct push_first_impl;

		template <typename T, typename... Ts>
		struct push_first_impl<T, type_list<Ts...>>
		{
			using type = type_list<T, Ts...>;
		};

		template <typename T, typename List>
		struct push_last_impl;

		template <typename T, typename... Ts>
		struct push_last_impl<T, type_list<Ts...>>
		{
			using type = type_list<Ts..., T>;
		};

		template <template <typename...> typename F, typename List>
		struct apply_impl;

		template <template <typename...> typename F, typename... Ts>
		struct apply_impl<F, type_list<Ts...>>
		{
			using type = typename F<Ts...>::type;
		};

		template <typename... Lists>
		struct join_impl
		{
			using type = type_list<>;
		};
		
		template <typename List>
		struct join_impl<List>
		{
			using type = List;
		};

		template <typename... Lhs, typename... Rhs>
		struct join_impl<type_list<Lhs...>, type_list<Rhs...>>
		{
			using type = type_list<Lhs..., Rhs...>;
		};

		template <typename... Lhs, typename... Rhs, typename... Lists>
		struct join_impl<type_list<Lhs...>, type_list<Rhs...>, Lists...>
		{
			using type = typename join_impl<type_list<Lhs..., Rhs...>, Lists...>::type;
		};
	}

	template <typename List>
	constexpr bool is_list = detail::is_list_impl<List>::value;

	template <typename List>
	constexpr std::size_t size = detail::size_impl<List>::value;

	template <typename List>
	constexpr bool empty = size<List> == 0;

	template <std::size_t Index, typename List>
	using nth = typename detail::nth_impl<Index, List>::type;

	template <typename List>
	using first = typename detail::first_impl<List>::type;

	template <typename List>
	using last = typename detail::last_impl<List>::type;

	template <typename List>
	using pop_first = typename detail::pop_first_impl<List>::type;

	template <typename T, typename List>
	using push_first = typename detail::push_first_impl<T, List>::type;

	template <typename T, typename List>
	using push_last = typename detail::push_last_impl<T, List>::type;

	template <template <typename...> typename F, typename List>
	using apply = typename detail::apply_impl<F, List>::type;

	template <template <typename...> typename F, typename List>
	constexpr auto apply_v = apply<F, List>::value;

	template <typename... Lists>
	using join = typename detail::join_impl<Lists...>::type;

	// Algorithms

	namespace detail
	{
		template <template <typename...> typename F, typename List>
		struct transform_impl;

		template <template <typename...> typename F, typename... Ts>
		struct transform_impl<F, type_list<Ts...>>
		{
			using type = type_list<typename F<Ts>::type...>;
		};

		template <template <typename...> typename Predicate, typename List>
		struct filter_impl;

		template <template <typename...> typename Predicate, typename... Ts>
		struct filter_impl<Predicate, type_list<Ts...>>
		{
			using type = join<std::conditional_t<Predicate<Ts>::value, type_list<Ts>, type_list<>>...>;
		};

		template <typename Indices, typename List>
		struct repeat_impl;

		template <std::size_t... Indices, typename List>
		struct repeat_impl<std::index_sequence<Indices...>, List>
		{
			using type = meta::join<std::decay_t<decltype( Indices, std::declval<List>() )>...>;
		};

		template <typename T>
		using as_list = std::conditional<meta::is_list<T>, T, type_list<T>>;

		template <template <typename> typename Function, typename... Lists>
		struct product_impl;

		//template <template <typename> typename Function>
		//struct product_impl<Function>
		//{
		//	using type = type_list<Function<>>;
		//};
	}

	template <template <typename...> typename F, typename List>
	using transform = typename detail::transform_impl<F, List>::type;

	template <template <typename...> typename Predicate, typename List>
	using filter = typename detail::filter_impl<Predicate, List>::type;

	template <std::size_t Amount, typename List>
	using repeat = typename detail::repeat_impl<std::make_index_sequence<Amount>, List>::type;

	template <typename List>
	using flatten = apply<detail::join_impl, transform<detail::as_list, List>>;

	template <template <typename> typename Function, typename... Lists>
	using product = typename detail::product_impl<Function, Lists...>::type;

	// Type list aliases
	using signed_integers = type_list<signed char, signed short, signed int, signed long, signed long long>;
	using unsigned_integers = type_list<unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long>;
	
	using integers = join<signed_integers, unsigned_integers>;

	using floating_points = type_list<float, double, long double>;
	using numeric_types = join<floating_points, integers>;

	using char_types = mclo::meta::type_list<char,
											 wchar_t,
#ifdef __cpp_char8_t
											 char8_t,
#endif
											 char16_t,
											 char32_t>;
}
