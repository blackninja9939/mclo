#pragma once

#if __cplusplus >= 202002L

#include "platform.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <numeric>
#include <utility>

namespace mclo
{
	template <typename T>
	constexpr std::size_t fnv1a_hash( T first, T last, std::size_t seed ) noexcept
	{
		std::size_t d = ( 0x811c9dc5 ^ seed ) * static_cast<std::size_t>( 0x01000193 );
		while ( first != last )
		{
			d = ( d ^ static_cast<std::size_t>( *first++ ) ) * static_cast<std::size_t>( 0x01000193 );
		}
		return d >> 8;
	}

	template <typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
	struct mph_hash
	{
		constexpr std::size_t operator()( const T& value, const std::size_t seed ) const noexcept
		{
			const auto bytes = std::bit_cast<std::array<std::byte, sizeof( T )>>( value );
			return fnv1a_hash( bytes.begin(), bytes.end(), seed );
		}
	};

	template <typename T>
	struct mph_hash<T, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
	{
		constexpr std::size_t operator()( const T& value, const std::size_t seed ) const noexcept
		{
			std::size_t key = seed ^ static_cast<std::size_t>( value );
			key = ( ~key ) + ( key << 21 );
			key = key ^ ( key >> 24 );
			key = ( key + ( key << 3 ) ) + ( key << 8 );
			key = key ^ ( key >> 14 );
			key = ( key + ( key << 2 ) ) + ( key << 4 );
			key = key ^ ( key >> 28 );
			key = key + ( key << 31 );
			return key;
		}
	};

	template <>
	struct mph_hash<std::string_view>
	{
		constexpr std::size_t operator()( const std::string_view& value, const std::size_t seed ) const noexcept
		{
			return fnv1a_hash( value.begin(), value.end(), seed );
		}
	};

	template <>
	struct mph_hash<const char*>
	{
		constexpr std::size_t operator()( const char* const value, const std::size_t seed ) const noexcept
		{
			return fnv1a_hash( value, value + std::char_traits<char>::length( value ), seed );
		}
	};

	template <typename Key,
			  typename MappedType,
			  typename StoredValue,
			  typename Hash,
			  typename KeyEquals,
			  typename GetKey,
			  std::size_t Size>
	class MCLO_EMPTY_BASES mph_base : private Hash, private KeyEquals, private GetKey
	{
	private:
		template <typename T>
		using sized_array = std::array<T, Size>;

		static constexpr std::size_t primary_seed = 42;

		struct primary_bucket_entry
		{
			std::size_t m_data_index = 0;
			primary_bucket_entry* m_next = nullptr;
		};

		struct primary_bucket
		{
			std::size_t m_size = 0;
			primary_bucket_entry* m_head = nullptr;
		};

	public:
		using key_type = Key;
		using mapped_type = MappedType;
		using value_type = StoredValue;
		using storage_type = sized_array<value_type>;
		using size_type = typename storage_type::size_type;
		using difference_type = typename storage_type::difference_type;
		using reference = typename storage_type::reference;
		using const_reference = typename storage_type::const_reference;
		using pointer = typename storage_type::pointer;
		using const_pointer = typename storage_type::const_pointer;
		using iterator = typename storage_type::iterator;
		using const_iterator = typename storage_type::const_iterator;
		using reverse_iterator = typename storage_type::reverse_iterator;
		using const_reverse_iterator = typename storage_type::const_reverse_iterator;

		constexpr mph_base( const storage_type& data )
		{
			sized_array<primary_bucket_entry> bucket_entries{};
			sized_array<primary_bucket> buckets{};

			for ( std::size_t data_index = 0; data_index < Size; ++data_index )
			{
				primary_bucket_entry& entry = bucket_entries[ data_index ];
				entry.m_data_index = data_index;

				const std::size_t bucket_index = hash( get_key( data[ data_index ] ), primary_seed ) % Size;
				primary_bucket& bucket = buckets[ bucket_index ];
				++bucket.m_size;

				entry.m_next = bucket.m_head;
				bucket.m_head = &entry;
			}

			sized_array<std::size_t> sorted_bucket_indices{};
			std::iota( sorted_bucket_indices.begin(), sorted_bucket_indices.end(), 0 );

			std::sort( sorted_bucket_indices.begin(),
					   sorted_bucket_indices.end(),
					   [ &buckets ]( const std::size_t lhs_index, const std::size_t rhs_index ) {
						   return buckets[ lhs_index ].m_size > buckets[ rhs_index ].m_size;
					   } );

			sized_array<bool> used_slots{};

			std::size_t sorted_index = 0;
			for ( ; sorted_index < Size; ++sorted_index )
			{
				const std::size_t bucket_index = sorted_bucket_indices[ sorted_index ];
				const primary_bucket& bucket = buckets[ bucket_index ];
				if ( bucket.m_size <= 1 )
				{
					break;
				}

				std::int32_t salt = 0;

				sized_array<bool> potential_used_slots = used_slots;
				primary_bucket_entry* next = bucket.m_head;

				while ( next )
				{
					const std::size_t data_index = next->m_data_index;
					const std::size_t slot = hash( get_key( data[ data_index ] ), salt ) % Size;
					if ( potential_used_slots[ slot ] )
					{
						// Start over increase the seed we use
						next = bucket.m_head;
						potential_used_slots = used_slots;
						++salt;
					}
					else
					{
						// Keep trying to see if we keep this salt
						potential_used_slots[ slot ] = true;
						reference value = m_storage[ slot ];
						std::destroy_at( &value );
						std::construct_at( &value, data[ data_index ] );
						next = next->m_next;
					}
				}

				used_slots = potential_used_slots;
				m_salts[ bucket_index ] = salt;
			}

			std::size_t empty_search_start = 0;
			for ( ; sorted_index < Size; ++sorted_index )
			{
				const std::size_t bucket_index = sorted_bucket_indices[ sorted_index ];
				const primary_bucket& bucket = buckets[ bucket_index ];
				if ( bucket.m_size == 0 )
				{
					break;
				}

				for ( std::size_t index = empty_search_start; index < Size; ++index )
				{
					if ( !used_slots[ index ] )
					{
						m_salts[ bucket_index ] = -static_cast<std::int32_t>( index ) - 1;
						empty_search_start = index + 1;
						break;
					}
				}
			}
		}

		constexpr const_iterator find( const key_type& key ) const
		{
			const std::size_t data_index = find_data_index( key );
			const auto it = begin() + data_index;
			return equals( get_key( *it ), key ) ? it : end();
		}

		constexpr bool contains( const key_type& key ) const
		{
			const std::size_t data_index = find_data_index( key );
			return equals( get_key( m_storage[ data_index ] ), key );
		}

		constexpr iterator begin() noexcept
		{
			return m_storage.begin();
		}
		constexpr const_iterator begin() const noexcept
		{
			return m_storage.begin();
		}
		constexpr const_iterator cbegin() const noexcept
		{
			return m_storage.cbegin();
		}

		constexpr iterator end() noexcept
		{
			return m_storage.end();
		}
		constexpr const_iterator end() const noexcept
		{
			return m_storage.end();
		}
		constexpr const_iterator cend() const noexcept
		{
			return m_storage.cend();
		}

		constexpr reverse_iterator rbegin() noexcept
		{
			return m_storage.rbegin();
		}
		constexpr const_reverse_iterator rbegin() const noexcept
		{
			return m_storage.rbegin();
		}
		constexpr const_reverse_iterator crbegin() const noexcept
		{
			return m_storage.crbegin();
		}

		constexpr reverse_iterator rend() noexcept
		{
			return m_storage.rend();
		}
		constexpr const_reverse_iterator rend() const noexcept
		{
			return m_storage.rend();
		}
		constexpr const_reverse_iterator crend() const noexcept
		{
			return m_storage.crend();
		}

	protected:
		constexpr size_type find_data_index( const key_type& key ) const
		{
			const std::size_t salt_index = hash( key, primary_seed ) % Size;
			const std::int32_t salt_value = m_salts[ salt_index ];
			return salt_value < 0 ? ( -salt_value - 1 ) : ( hash( key, salt_value ) % Size );
		}

		constexpr std::size_t hash( const Key& key, const std::size_t seed ) const noexcept
		{
			return static_cast<const Hash&>( *this )( key, seed );
		}
		constexpr std::size_t equals( const Key& lhs, const Key& rhs ) const noexcept
		{
			return static_cast<const KeyEquals&>( *this )( lhs, rhs );
		}
		constexpr const Key& get_key( const StoredValue& value ) const noexcept
		{
			return static_cast<const GetKey&>( *this )( value );
		}

	private:
		storage_type m_storage{};
		sized_array<std::int32_t> m_salts{};
	};

	namespace detail
	{
		struct pair_key
		{
			template <typename T, typename U>
			[[nodiscard]] constexpr const T& operator()( const std::pair<const T, U>& pair ) const noexcept
			{
				return pair.first;
			}
		};
	}

	template <typename Value,
			  std::size_t Size,
			  typename Hash = mph_hash<Value>,
			  typename KeyEquals = std::equal_to<Value>>
	class mph_set : public mph_base<Value, Value, Value, Hash, KeyEquals, std::identity, Size>
	{
		using base = mph_base<Value, Value, Value, Hash, KeyEquals, std::identity, Size>;

	public:
		using base::base;
	};

	template <typename Key,
			  typename Value,
			  std::size_t Size,
			  typename Hash = mph_hash<Key>,
			  typename KeyEquals = std::equal_to<Key>>
	class mph_map : public mph_base<Key, Value, std::pair<const Key, Value>, Hash, KeyEquals, detail::pair_key, Size>
	{
		using base = mph_base<Key, Value, std::pair<const Key, Value>, Hash, KeyEquals, detail::pair_key, Size>;

	public:
		using base::base;

		using base::find;

		constexpr typename base::iterator find( const typename base::key_type& key )
		{
			const std::size_t data_index = find_data_index( key );
			const auto it = base::begin() + data_index;
			return base::equals( base::get_key( *it ), key ) ? it : base::end();
		}
	};
}

#endif
