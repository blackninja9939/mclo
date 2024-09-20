#pragma once

#include <memory>

namespace mclo
{
	template <typename Reference>
	struct arrow_proxy
	{
		Reference r;

		Reference* operator->()
		{
			return std::addressof( r );
		}
	};
}
