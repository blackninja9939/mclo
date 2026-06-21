#pragma once

namespace mclo
{
	/// @brief True if T is a specialization of the class template Template
	/// @tparam T The type to inspect
	/// @tparam Template The class template to test against
	template <typename T, template <typename...> class Template>
	constexpr bool is_specialization_of_v = false;

	template <template <typename...> class Template, typename... Args>
	constexpr bool is_specialization_of_v<Template<Args...>, Template> = true;

	/// @brief Requires that T is a specialization of Template
	template <typename T, template <typename...> class Template>
	concept specialization_of = is_specialization_of_v<T, Template>;
}
