#pragma once

#include <functional>

#if defined( __cpp_lib_function_ref ) && ( __cpp_lib_function_ref >= 202306L )

namespace mclo
{
	/// @brief Value construction tag used to construct type-erased callables from a compile-time callable.
	using std::nontype;

	/// @brief Value construction tag type used to construct type-erased callables from a compile-time callable.
	using std::nontype_t;
}

#else

namespace mclo
{
	/// @brief Value construction tag type used to construct type-erased callables from a compile-time callable.
	/// @details A polyfill for C++26 @c std::nontype_t, used when the standard library does not provide it. It carries
	/// a callable as the non-type template parameter @p Value so it can be baked into a type with no runtime storage,
	/// letting wrappers such as @ref function_ref bind free functions, pointers to members, and pointers to member
	/// objects. See @c std::nontype_t for the full interface.
	/// @tparam Value The callable baked into the tag type.
	template <auto Value>
	struct nontype_t
	{
		explicit nontype_t() = default;
	};

	/// @brief Value construction tag used to construct type-erased callables from a compile-time callable.
	/// @tparam Value The callable baked into the tag.
	template <auto Value>
	constexpr nontype_t<Value> nontype{};
}

#endif

namespace mclo::detail
{
	template <typename T>
	constexpr bool is_nontype_specialization = false;

	template <auto Value>
	constexpr bool is_nontype_specialization<nontype_t<Value>> = true;

	// nontype_t is parameterized by a non-type auto value, so it cannot be matched by mclo::specialization_of, which
	// only accepts template<typename...> class templates. Hence this dedicated concept.
	template <typename T>
	concept nontype_specialization = is_nontype_specialization<T>;
}
