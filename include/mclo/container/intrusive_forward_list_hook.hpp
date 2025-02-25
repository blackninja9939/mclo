#pragma once

namespace mclo
{
	template <typename Tag = void>
	struct intrusive_forward_list_hook
	{
		intrusive_forward_list_hook* m_next = nullptr;
	};
}
