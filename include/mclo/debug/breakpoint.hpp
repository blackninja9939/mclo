#pragma once

namespace mclo
{
	/// @brief Triggers a debugger breakpoint at the call site
	void breakpoint() noexcept;

	/// @brief Triggers a debugger breakpoint only when a debugger is attached
	void breakpoint_if_debugging() noexcept;
}
