#pragma once

namespace mclo
{
	/// @brief Combines multiple callables into one whose @c operator() overloads across all of them.
	/// @details Inherits from each callable and brings every @c operator() into scope, producing a single object
	/// that can be called with any of the inherited signatures. Commonly used to build inline visitors for
	/// @c std::visit.
	/// @code
	/// std::visit( mclo::overloaded{
	///     []( int i ) { /* ... */ },
	///     []( const std::string& s ) { /* ... */ },
	/// }, my_variant );
	/// @endcode
	/// @tparam Ts The callable types to combine.
	template <typename... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};

	template <class... Ts>
	overloaded( Ts... ) -> overloaded<Ts...>;
}
