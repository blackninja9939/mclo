#pragma once

#include "mclo/allocator/new_delete_resource.hpp"
#include "mclo/allocator/upstream_resource.hpp"

namespace
{
	mclo::upstream_resource default_upstream( mclo::new_delete_memory_resource::instance() );
}

namespace mclo
{
	upstream_resource get_default_upstream_resource()
	{
		return default_upstream;
	}

	void set_default_upstream_resource( upstream_resource resource )
	{
		default_upstream = resource;
	}
}
