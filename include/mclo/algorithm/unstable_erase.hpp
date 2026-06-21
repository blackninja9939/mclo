#pragma once

#include <algorithm>

namespace mclo
{
	/// @brief Erases the first element equal to a value by swapping it with the last element.
	/// @details Does not preserve the relative order of the remaining elements but avoids shifting
	/// the tail of the container. Does nothing if no matching element is found.
	/// @tparam Container A container supporting random removal and pop_back.
	/// @tparam T The value type to compare elements against.
	/// @param container The container to erase from.
	/// @param value The value to find and erase.
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

	/// @brief Erases the first element satisfying a predicate by swapping it with the last element.
	/// @details Does not preserve the relative order of the remaining elements but avoids shifting
	/// the tail of the container. Does nothing if no element satisfies the predicate.
	/// @tparam Container A container supporting random removal and pop_back.
	/// @tparam Predicate A unary predicate invoked on each element.
	/// @param container The container to erase from.
	/// @param pred The predicate that selects the element to erase.
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
