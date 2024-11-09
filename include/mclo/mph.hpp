#pragma once

#if __cplusplus >= 202002L

#include <mclo/fnva1.hpp>
#include <mclo/platform.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <numeric>
#include <utility>

namespace mclo
{
	template <typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
	struct mph_hash
	{
		MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const T& value, const std::size_t seed )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			const auto bytes = std::bit_cast<std::array<std::byte, sizeof( T )>>( value );
			return mclo::fnv1a( bytes.data(), bytes.size(), seed );
		}
	};

	template <typename T>
	struct mph_hash<T, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
	{
		MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const T& value, const std::size_t seed )
			MCLO_CONST_CALL_OPERATOR noexcept
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
		MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()(
			const std::string_view& value, const std::size_t seed ) MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::fnv1a( value.data(), value.size(), seed );
		}
	};

	template <>
	struct mph_hash<const char*>
	{
		MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const char* const value, const std::size_t seed )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::fnv1a( value, std::char_traits<char>::length( value ), seed );
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
		/*
		 * Every entry gets hashed into a bucket using a primary seed, multiple entries can hash to the same bucket
		 * We distribute them into the actual slots by using a second hash with a value that guarantees the bucket
		 * puts things into empty slots, we process the largest buckets first. We store this secondary seed for lookup.
		 *
		 * Single item buckets just take whatever slots are left and instead of storing the seed they store the index
		 * in the actual entry storage, they store this as -index - 1. That way we can tell later by the sign how to
		 * handle.
		 *
		 * To look up something we hash the key with the primary seed to find the bucket it would be in, we load the
		 * secondary seed for that bucket and if it is positive then it is a seed we use to hash again to find the
		 * object slot. If it is negative we take -index - 1 to get the index of the direct slot it got place in.
		 *
		 * This guarantees every object hashes to one single slot with minimal extra data overhead and that we handle
		 * conflicts in such a way that we find a perfect set of secondary seeds in the minimal number of loops.
		 */

		template <typename T>
		using sized_array = std::array<T, Size>;

		// 0 is used as a sentinel value in setup so must be less than max
		static_assert( Size < std::numeric_limits<std::size_t>::max(), "Too many entries" );

		static constexpr std::size_t primary_seed = 42;

		// Buckets are stored inline in setup in a singly linked list
		struct primary_bucket_entry
		{
			std::size_t m_data_index = 0;
			primary_bucket_entry* m_next = nullptr;
		};

		// We store the size so we can sort by it since the linked list does not have O(1) size itself
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
			// Every entry gets hashed into a bucket, the bucket maintains a linked list of everything
			// hashed into it and we then spread them out later
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

			// We sort the indices so we process the largest buckets first as they are hardest to find a valid
			// salt for
			sized_array<std::size_t> sorted_bucket_indices{};
			std::iota( sorted_bucket_indices.begin(), sorted_bucket_indices.end(), 0 );

			std::sort( sorted_bucket_indices.begin(),
					   sorted_bucket_indices.end(),
					   [ &buckets ]( const std::size_t lhs_index, const std::size_t rhs_index ) {
						   return buckets[ lhs_index ].m_size > buckets[ rhs_index ].m_size;
					   } );

			// Stores data index + 1, 0 means unused
			sized_array<std::size_t> slot_data_index{};

			std::size_t sorted_index = 0;
			for ( ; sorted_index < Size; ++sorted_index )
			{
				const std::size_t bucket_index = sorted_bucket_indices[ sorted_index ];
				const primary_bucket& bucket = buckets[ bucket_index ];

				// Single elements can be handled simply later
				if ( bucket.m_size <= 1 )
				{
					break;
				}

				// We repeatedly try salts for this bucket to put each entry into an empty slot
				std::int32_t salt = 0;

				sized_array<std::size_t> potential_slot_data_index = slot_data_index;
				primary_bucket_entry* next = bucket.m_head;

				while ( next )
				{
					const std::size_t data_index = next->m_data_index;
					const std::size_t slot = hash( get_key( data[ data_index ] ), salt ) % Size;
					if ( potential_slot_data_index[ slot ] != 0 )
					{
						// Start over increase the seed we use
						next = bucket.m_head;
						potential_slot_data_index = slot_data_index;
						++salt;
					}
					else
					{
						// Keep trying to see if we keep this salt
						potential_slot_data_index[ slot ] = data_index + 1;
						next = next->m_next;
					}
				}

				// Found a valid salt for this bucket, we store it and our new slot configuration for next loop
				slot_data_index = potential_slot_data_index;
				m_salts[ bucket_index ] = salt;
			}

			// Single element buckets we just distribute them in order of whatever slot is free
			std::size_t empty_search_start = 0;
			for ( ; sorted_index < Size; ++sorted_index )
			{
				const std::size_t bucket_index = sorted_bucket_indices[ sorted_index ];
				const primary_bucket& bucket = buckets[ bucket_index ];

				// Bucket indices are sorted so first zero means we are done
				if ( bucket.m_size == 0 )
				{
					break;
				}

				for ( std::size_t index = empty_search_start; index < Size; ++index )
				{
					if ( slot_data_index[ index ] == 0 )
					{
						m_salts[ bucket_index ] = -static_cast<std::int32_t>( index ) - 1;
						empty_search_start = index + 1;
						slot_data_index[ index ] = bucket.m_head->m_data_index + 1;
						break;
					}
				}
			}

			// All slots are used so we now finally construct the real values
			for ( std::size_t slot = 0; slot < Size; ++slot )
			{
				assert( slot_data_index[ slot ] != 0 );
				const std::size_t data_index = slot_data_index[ slot ] - 1;
				reference value = m_storage[ slot ];
				std::destroy_at( &value );
				std::construct_at( &value, data[ data_index ] );
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
			[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr const T& operator()( const std::pair<const T, U>& pair )
				MCLO_CONST_CALL_OPERATOR noexcept
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
