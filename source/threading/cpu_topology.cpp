#include "mclo/threading/cpu_topology.hpp"

#include "mclo/container/small_vector.hpp"
#include "mclo/platform/os_detection.hpp"

#include <algorithm>

#ifdef MCLO_OS_WINDOWS

#include "mclo/platform/windows_wrapper.hpp"

#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

namespace
{
	mclo::cpu_topology query_platform()
	{
		mclo::cpu_topology topology;

		DWORD length = 0;
		if ( GetLogicalProcessorInformationEx( RelationProcessorCore, nullptr, &length ) ||
			 GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		{
			return topology;
		}

		auto buffer = std::make_unique_for_overwrite<std::byte[]>( length );
		auto* const first = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>( buffer.get() );
		if ( !GetLogicalProcessorInformationEx( RelationProcessorCore, first, &length ) )
		{
			return topology;
		}

		// The efficiency class is a relative ranking, higher is faster. Collect the raw values first so the highest
		// ranked cores can be tagged as performance cores once the full range is known.
		struct raw_core
		{
			std::uint64_t m_mask;
			BYTE m_efficiency;
		};
		mclo::small_vector<raw_core, 64> raw_cores;

		BYTE min_efficiency = std::numeric_limits<BYTE>::max();
		BYTE max_efficiency = 0;

		auto* cursor = reinterpret_cast<std::byte*>( first );
		std::byte* const end = cursor + length;
		while ( cursor < end )
		{
			auto* const info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>( cursor );
			if ( info->Relationship == RelationProcessorCore && info->Processor.GroupCount > 0 )
			{
				// Only processor group 0 is represented, matching the 64 logical processor model.
				const BYTE efficiency = info->Processor.EfficiencyClass;
				const auto mask = static_cast<std::uint64_t>( info->Processor.GroupMask[ 0 ].Mask );
				raw_cores.push_back( { mask, efficiency } );
				min_efficiency = std::min( min_efficiency, efficiency );
				max_efficiency = std::max( max_efficiency, efficiency );
			}
			cursor += info->Size;
		}

		const bool hybrid = max_efficiency != min_efficiency;
		topology.m_is_hybrid = hybrid;
		topology.m_cores.reserve( raw_cores.size() );
		for ( const raw_core& core : raw_cores )
		{
			mclo::core_class classification = mclo::core_class::unknown;
			if ( hybrid )
			{
				classification =
					core.m_efficiency == max_efficiency ? mclo::core_class::performance : mclo::core_class::efficiency;
			}
			topology.m_cores.push_back( { core.m_mask, classification } );
		}

		return topology;
	}
}

#elif defined( MCLO_OS_LINUX )

#include <charconv>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	// Reads the first line of a sysfs file, or nothing when it cannot be opened (an offline or absent processor).
	std::optional<std::string> read_sysfs_line( const std::string& path )
	{
		std::ifstream file( path );
		if ( !file )
		{
			return std::nullopt;
		}
		std::string line;
		std::getline( file, line );
		return line;
	}

	// Parses a Linux CPU bitmask such as "00000000,00000003" (comma separated 32 bit words, most significant first)
	// into a 64 bit mask. Bits for processors past 63 are dropped, matching the 64 processor model.
	std::uint64_t parse_cpu_mask( const std::string_view text )
	{
		std::uint64_t mask = 0;
		std::size_t start = 0;
		while ( true )
		{
			const std::size_t comma = text.find( ',', start );
			const std::string_view token =
				text.substr( start, comma == std::string_view::npos ? std::string_view::npos : comma - start );

			std::uint32_t word = 0;
			std::from_chars( token.data(), token.data() + token.size(), word, 16 );
			mask = ( mask << 32 ) | word;

			if ( comma == std::string_view::npos )
			{
				break;
			}
			start = comma + 1;
		}
		return mask;
	}

	mclo::cpu_topology query_platform()
	{
		mclo::cpu_topology topology;

		// Each physical core is represented once, by its lowest numbered logical processor. The raw capacity is kept
		// so the performance and efficiency cores can be classified once the whole range is known.
		struct raw_core
		{
			std::uint64_t m_mask;
			unsigned long m_capacity;
		};
		mclo::small_vector<raw_core, 64> raw_cores;
		bool have_capacities = true;

		for ( unsigned int cpu = 0; cpu < 64; ++cpu )
		{
			const std::string base = "/sys/devices/system/cpu/cpu" + std::to_string( cpu );

			const std::optional<std::string> siblings = read_sysfs_line( base + "/topology/thread_siblings" );
			if ( !siblings )
			{
				continue; // Processor is offline or absent.
			}

			const std::uint64_t sibling_mask = parse_cpu_mask( *siblings );

			// Only the lowest numbered sibling represents the physical core, so each core is added exactly once.
			const std::uint64_t lowest = sibling_mask & ( ~sibling_mask + 1 );
			if ( lowest != ( std::uint64_t{ 1 } << cpu ) )
			{
				continue;
			}

			unsigned long capacity = 0;
			if ( const std::optional<std::string> text = read_sysfs_line( base + "/cpu_capacity" ) )
			{
				std::from_chars( text->data(), text->data() + text->size(), capacity );
			}
			else
			{
				have_capacities = false;
			}

			raw_cores.push_back( { sibling_mask, capacity } );
		}

		// A machine is hybrid only when every core reported a capacity and they are not all equal; the highest
		// capacity cores are the performance cores.
		unsigned long min_capacity = 0;
		unsigned long max_capacity = 0;
		if ( have_capacities && !raw_cores.empty() )
		{
			min_capacity = max_capacity = raw_cores.front().m_capacity;
			for ( const raw_core& core : raw_cores )
			{
				min_capacity = std::min( min_capacity, core.m_capacity );
				max_capacity = std::max( max_capacity, core.m_capacity );
			}
		}

		topology.m_is_hybrid = max_capacity != min_capacity;
		topology.m_cores.reserve( raw_cores.size() );
		for ( const raw_core& core : raw_cores )
		{
			mclo::core_class classification = mclo::core_class::unknown;
			if ( topology.m_is_hybrid )
			{
				classification =
					core.m_capacity == max_capacity ? mclo::core_class::performance : mclo::core_class::efficiency;
			}
			topology.m_cores.push_back( { core.m_mask, classification } );
		}

		return topology;
	}
}

#else

#include <thread>

namespace
{
	mclo::cpu_topology query_platform()
	{
		// No topology support: report each logical processor as its own unclassified core.
		mclo::cpu_topology topology;

		unsigned int count = std::thread::hardware_concurrency();
		if ( count == 0 )
		{
			count = 1;
		}
		count = std::min( count, 64u );

		topology.m_cores.reserve( count );
		for ( unsigned int cpu = 0; cpu < count; ++cpu )
		{
			topology.m_cores.push_back( { std::uint64_t{ 1 } << cpu, mclo::core_class::unknown } );
		}

		return topology;
	}
}

#endif

mclo::cpu_topology mclo::query_cpu_topology()
{
	cpu_topology topology = query_platform();

	// Never hand back an empty topology; a single unclassified core lets callers always assign at least one thread.
	if ( topology.m_cores.empty() )
	{
		topology.m_cores.push_back( { std::uint64_t{ 1 }, core_class::unknown } );
	}

	return topology;
}
