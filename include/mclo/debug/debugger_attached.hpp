#pragma once

namespace mclo
{
	/// @brief Checks whether a debugger is currently attached to the process
	/// @return True if a debugger is attached, false otherwise
	bool is_debugger_attached() noexcept;
}
