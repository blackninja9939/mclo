#pragma once

namespace mclo
{
	template <typename T>
	class pass_key
	{
		friend T;
		pass_key() noexcept = default;
	};
}
