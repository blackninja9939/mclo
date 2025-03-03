#pragma once

namespace mclo
{
	template <typename... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};

	template <class... Ts>
	overloaded( Ts... ) -> overloaded<Ts...>;
}
