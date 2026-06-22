#include "mclo/threading/adaptive_waiter.hpp"

#include <thread>

namespace mclo
{
	void adaptive_waiter::yield() noexcept
	{
		std::this_thread::yield();
	}
}
