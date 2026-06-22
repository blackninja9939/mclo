#pragma once

#include "mclo/container/detail/mph_base.hpp"

namespace mclo
{
	namespace detail
	{
		struct pair_key
		{
			template <typename T, typename U>
			[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr const T& operator()( const std::pair<const T, U>& pair )
				MCLO_CONST_CALL_OPERATOR noexcept
			{
				return pair.first;
			}
		};
	}

	/// @brief A fixed-size, immutable map built on a minimal perfect hash, ideal for compile-time constant tables.
	/// @details Stores a fixed set of @p Size key/value pairs known at construction and supports collision-free lookup
	/// in at most two hashes and one comparison. The whole map can be a @c constexpr value. See @ref detail::mph_base
	/// for the underlying lookup mechanism and @ref mph_hash for customising the key hash.
	/// @warning Building the perfect hash is a fairly heavy compile-time computation (repeatedly searching for salts).
	/// It is constant-evaluation capable but can noticeably hurt compile times if used for many or large tables, so
	/// use it judiciously.
	/// @tparam Key The key type.
	/// @tparam Value The mapped value type.
	/// @tparam Size The exact number of key/value pairs.
	/// @tparam Hash The salted hash functor for keys.
	/// @tparam KeyEquals The key equality comparator.
	template <typename Key,
			  typename Value,
			  std::size_t Size,
			  typename Hash = mph_hash<Key>,
			  typename KeyEquals = std::equal_to<Key>>
	class mph_map : public detail::mph_base<Key, std::pair<const Key, Value>, Hash, KeyEquals, detail::pair_key, Size>
	{
		using base = detail::mph_base<Key, std::pair<const Key, Value>, Hash, KeyEquals, detail::pair_key, Size>;

	public:
		using mapped_type = Value;

		using base::base;

		using base::find;

		/// @brief Finds the element with the given @p key.
		/// @param key The key to look up.
		/// @return An iterator to the matching key/value pair, or @c end() if @p key is not present.
		[[nodiscard]] constexpr typename base::iterator find( const typename base::key_type& key )
		{
			const std::size_t data_index = base::find_data_index( key );
			const auto it = base::begin() + data_index;
			return base::equals( base::get_key( *it ), key ) ? it : base::end();
		}
	};
}
