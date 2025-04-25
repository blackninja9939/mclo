#pragma once

#include <algorithm>

namespace mclo
{
	template <typename Container, typename T>
	void unstable_erase( Container& container, const T& value )
	{
		auto last = container.end();
		auto it = std::find( container.begin(), last, value );
		if ( it != last )
		{
			*it = std::move( *std::prev( last ) );
			container.pop_back();
		}
	}

	template <typename Container, typename Predicate>
	void unstable_erase_if( Container& container, Predicate pred )
	{
		auto last = container.end();
		auto it = std::find_if( container.begin(), last, pred );
		if ( it != last )
		{
			*it = std::move( *std::prev( last ) );
			container.pop_back();
		}
	}
}
