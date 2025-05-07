#pragma once

namespace mclo::meta
{
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

	template <typename T>
	constexpr type_id_t type_id = &detail::type_id<T>::id;

	template <auto id>
	using type_of = typename decltype( get( detail::type_of<id>{} ) )::value_type;
}
