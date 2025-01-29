#pragma once

#include "mclo/allocator/upstream_resource.hpp"

namespace mclo
{
	upstream_resource get_default_upstream_resource();
	void set_default_upstream_resource( upstream_resource resource );
}
