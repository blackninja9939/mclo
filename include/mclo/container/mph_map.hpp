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

	template <typename Key,
			  typename Value,
			  std::size_t Size,
			  typename Hash = mph_hash<Key>,
			  typename KeyEquals = std::equal_to<Key>>
	class mph_map : public mph_base<Key, std::pair<const Key, Value>, Hash, KeyEquals, detail::pair_key, Size>
	{
		using base = mph_base<Key, std::pair<const Key, Value>, Hash, KeyEquals, detail::pair_key, Size>;

	public:
		using mapped_type = Value;

		using base::base;

		using base::find;

		[[nodiscard]] constexpr typename base::iterator find( const typename base::key_type& key )
		{
			const std::size_t data_index = base::find_data_index( key );
			const auto it = base::begin() + data_index;
			return base::equals( base::get_key( *it ), key ) ? it : base::end();
		}
	};
}
