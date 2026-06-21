#pragma once

#include <utility>

namespace mclo
{
	/// @brief Look up a key in the map, if present returns a copy of the value, else returns the provided default
	/// value.
	/// @details This function is useful for maps where the @c mapped_type is cheap to copy, and the caller does not
	/// need a reference to the actual value stored in the map. If the @c mapped_type is expensive to copy, or if the
	/// caller needs a reference to the actual value in the map, then @c lookup_ref_or may be more appropriate.
	/// @tparam Map The type of the map to perform the lookup on. Must support @c find() and @c mapped_type.
	/// @tparam Key The type of the key to look up in the map. Does not need to be the same as the map's key type, but
	/// must be comparable with it.
	/// @tparam Value The type of the default value to return if the key is not found in the map. Must be convertible to
	/// the map's @c mapped_type.
	/// @param map The map to perform the lookup on.
	/// @param key The key to look up in the map.
	/// @param default_value The value to return if the key is not found in the map.
	/// @return A copy of the value corresponding to @p key in @p map if it exists, else the forwarded @p default_value.
	template <typename Map, typename Key, typename Value>
	[[nodiscard]] typename Map::mapped_type lookup_value_or( const Map& map, const Key& key, Value&& default_value )
	{
		const auto it = map.find( key );
		if ( it == map.end() )
		{
			return std::forward<Value>( default_value );
		}
		return it->second;
	}

	/// @brief Look up a key in the map, if present returns a reference to the value, else returns a reference to the
	/// provided default value.
	/// @details This function is useful for maps where the @c mapped_type is expensive to copy, or if the caller needs
	/// a reference to the actual value in the map. If the @c mapped_type is cheap to copy, and the caller does not need
	/// a reference to the actual value in the map, then @c lookup_value_or may be more appropriate.
	/// @tparam Map The type of the map to perform the lookup on. Must support @c find() and @c mapped_type.
	/// @tparam Key The type of the key to look up in the map. Does not need to be the same as the map's key type, but
	/// must be comparable with it.
	/// @param map The map to perform the lookup on.
	/// @param key The key to look up in the map.
	/// @param default_value Reference to the value to return if the key is not found in the map.
	/// @return A reference to the value corresponding to @p key in @p map if it exists, else a reference to @p
	/// default_value.
	/// @warning The @p default_value must outlive the returned reference, and passing an rvalue as @p default_value is
	/// prohibited (deleted overload below) to prevent returning a reference to a destroyed temporary. But you can still
	/// create a dangling reference by passing a reference to an object that is destroyed after the call but before you
	/// use the return value.
	template <typename Map, typename Key>
	[[nodiscard]] const typename Map::mapped_type& lookup_ref_or( const Map& map,
																  const Key& key,
																  const typename Map::mapped_type& default_value )
	{
		const auto it = map.find( key );
		if ( it == map.end() )
		{
			return default_value;
		}
		return it->second;
	}

	/// @brief Deleted overload to prevent passing an rvalue as the default value to @c lookup_ref_or, which would
	/// result in returning a reference to a destroyed temporary.
	template <typename Map, typename Key, typename Value>
	const typename Map::mapped_type& lookup_ref_or( const Map& map,
													const Key& key,
													const Value&& default_value ) = delete;
}
