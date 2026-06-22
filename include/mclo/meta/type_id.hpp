#pragma once

namespace mclo::meta
{
	/// @brief A unique, comparable compile-time identifier for a type, usable as a non-type template parameter.
	using type_id_t = const int*;

	namespace detail
	{
		// Use friend injection to be able to get the type out of the type_id object

		template <type_id_t>
		struct type_of
		{
			constexpr auto friend get( type_of );
		};

		template <typename T>
		struct type_id
		{
			using value_type = T;

			static constexpr int id = 0;

			constexpr auto friend get( type_of<&id> )
			{
				return type_id{};
			}
		};
	}

	/// @brief A unique @ref type_id_t identifying the type @p T.
	/// @details Each distinct type yields a distinct value that is stable within a program. Because it is an address
	/// constant it can be used as a non-type template parameter and recovered back into the type via @ref type_of.
	/// @tparam T The type to identify.
	template <typename T>
	constexpr type_id_t type_id = &detail::type_id<T>::id;

	/// @brief Recovers the type that a @ref type_id value was produced from.
	/// @tparam id A @ref type_id_t value, as produced by @ref type_id.
	template <auto id>
	using type_of = typename decltype( get( detail::type_of<id>{} ) )::value_type;
}
