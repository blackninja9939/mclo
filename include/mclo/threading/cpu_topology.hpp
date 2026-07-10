#pragma once

#include <cstdint>
#include <vector>

namespace mclo
{
	/// @brief The performance classification of a physical CPU core.
	enum class core_class : std::uint8_t
	{
		/// @brief The classification is unknown, for example on a homogeneous machine or an unsupported platform.
		unknown,

		/// @brief A power efficient core, typically slower, such as an Intel E-core.
		efficiency,

		/// @brief A high performance core, typically faster, such as an Intel P-core.
		performance,
	};

	/// @brief A single physical CPU core and the logical processors it contains.
	struct cpu_core
	{
		/// @brief A bitmask of the logical processors that make up this physical core.
		/// @details On a simultaneous multithreading core this has one bit set per sibling; pinning a thread to the
		/// whole mask grants it the entire physical core. Only the first 64 logical processors are representable.
		std::uint64_t m_logical_mask = 0;

		/// @brief The performance classification of this core.
		core_class m_class = core_class::unknown;
	};

	/// @brief A description of the host machine's physical CPU cores.
	struct cpu_topology
	{
		/// @brief The physical cores, in operating system enumeration order.
		std::vector<cpu_core> m_cores;

		/// @brief Whether the machine has a mix of performance and efficiency cores.
		/// @details When false every core is the same class and @ref cpu_core::m_class is @ref core_class::unknown.
		bool m_is_hybrid = false;
	};

	/// @brief Queries the host machine's physical CPU topology.
	/// @details Only the first 64 logical processors are represented; on Windows this is processor group 0. On
	/// platforms without topology support each logical processor is reported as its own @ref core_class::unknown core.
	/// @return The physical cores of the machine, or a single core if the query fails.
	[[nodiscard]] cpu_topology query_cpu_topology();
}
