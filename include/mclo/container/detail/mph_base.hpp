#pragma once

#include "mclo/container/detail/nontrivial_dummy_type.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/hash/constexpr_hash.hpp"
#include "mclo/platform/attributes.hpp"
#include "mclo/platform/cpp_feature_compat.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <numeric>
#include <utility>

namespace mclo
{
	/// @brief Customisation point providing a constexpr, salted hash for keys of a minimal perfect hash container.
	/// @details The hash takes a key and a salt and must be a simple, fully @c constexpr operation that still yields a
	/// good spread of entropy, since it is evaluated repeatedly at compile time while building the perfect hash.
	/// Specialise
	/// @c mph_hash for custom key types. Specialisations are provided for integral and enum types, types convertible to
	/// @c std::string_view, and @c std::optional.
	/// @tparam T The key type to hash.
	template <typename T>
	struct mph_hash;

	template <typename T>
		requires( std::is_integral_v<T> || std::is_enum_v<T> )
	struct mph_hash<T>
	{
		MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const T& value, const std::size_t salt )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			std::size_t key = salt ^ static_cast<std::size_t>( value );
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

	template <typename T>
		requires( std::convertible_to<T, std::string_view> )
	struct mph_hash<T>
	{
		MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const T& value, const std::size_t salt )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			const std::string_view view( value );
			return mclo::constexpr_hash( view.data(), view.size(), salt );
		}
	};

	template <typename T>
	struct mph_hash<std::optional<T>>
	{
		MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()(
			const std::optional<T>& value, const std::size_t salt ) MCLO_CONST_CALL_OPERATOR noexcept
		{
			if ( value )
			{
				return mph_hash<T>()( *value, salt );
			}
			return 0;
		}
	};
}

namespace mclo::detail
{
	/// @brief Shared implementation for the minimal perfect hash containers @ref mclo::mph_map and @ref mclo::mph_set.
	/// @details Builds, at construction (which is usable in a constant expression), a minimal perfect hash over a fixed
	/// set of @p Size keys so that every key maps to a unique slot with no collisions and no wasted space. Each key is
	/// first hashed with a primary salt into a bucket; buckets are then resolved largest-first, each finding a
	/// secondary salt that scatters its entries into free slots, while single-entry buckets are placed directly.
	/// Lookups therefore touch at most two hashes and one comparison. The container is fixed-size and immutable after
	/// construction.
	/// @tparam Key The key type used for lookups.
	/// @tparam StoredValue The element type stored in each slot (the key itself for a set, a key/value pair for a map).
	/// @tparam Hash The salted hash functor, see @ref mclo::mph_hash.
	/// @tparam KeyEquals The key equality comparator.
	/// @tparam GetKey A functor extracting the @p Key from a @p StoredValue.
	/// @tparam Size The exact number of elements.
	template <typename Key, typename StoredValue, typename Hash, typename KeyEquals, typename GetKey, std::size_t Size>
	class MCLO_EMPTY_BASES mph_base : private Hash, private KeyEquals, private GetKey
	{
	private:
		/*
		 * Every entry gets hashed into a bucket using a primary salt, multiple entries can hash to the same bucket
		 * We distribute them into the actual slots by using a second hash with a value that guarantees the bucket
		 * puts things into empty slots, we process the largest buckets first. We store this secondary salt for lookup.
		 *
		 * Single item buckets just take whatever slots are left and instead of storing the salt they store the index
		 * in the actual entry storage, they store this as -index - 1. That way we can tell later by the sign how to
		 * handle.
		 *
		 * To look up something we hash the key with the primary salt to find the bucket it would be in, we load the
		 * secondary salt for that bucket and if it is positive then it is a salt we use to hash again to find the
		 * object slot. If it is negative we take -index - 1 to get the index of the direct slot it got place in.
		 *
		 * This guarantees every object hashes to one single slot with minimal extra data overhead and that we handle
		 * conflicts in such a way that we find a perfect set of secondary salts in the minimal number of loops.
		 */

		template <typename T>
		using sized_array = std::array<T, Size>;

		// 0 is used as a sentinel value in setup so must be less than max
		static_assert( Size < std::numeric_limits<std::size_t>::max(), "Too many entries" );

		static constexpr std::size_t primary_salt = 42;

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

		using storage_array = sized_array<StoredValue>;

	public:
		using key_type = Key;
		using value_type = StoredValue;
		using size_type = typename storage_array::size_type;
		using difference_type = typename storage_array::difference_type;
		using hasher = Hash;
		using key_equal = KeyEquals;
		using reference = typename storage_array::reference;
		using const_reference = typename storage_array::const_reference;
		using pointer = typename storage_array::pointer;
		using const_pointer = typename storage_array::const_pointer;
		using iterator = typename storage_array::iterator;
		using const_iterator = typename storage_array::const_iterator;
		using reverse_iterator = typename storage_array::reverse_iterator;
		using const_reverse_iterator = typename storage_array::const_reverse_iterator;

		/// @brief Constructs the container by building a minimal perfect hash over @p data.
		/// @details Distributes the @p Size keys into buckets, resolves secondary salts so each key lands in a unique
		/// slot, and then constructs the stored values into those slots.
		/// @param data The exact set of @p Size elements to store; keys must be unique.
		constexpr mph_base( const sized_array<value_type>& data )
		{
			// Every entry gets hashed into a bucket, the bucket maintains a linked list of everything
			// hashed into it and we then spread them out later
			sized_array<primary_bucket_entry> bucket_entries{};
			sized_array<primary_bucket> buckets{};

			for ( std::size_t data_index = 0; data_index < Size; ++data_index )
			{
				primary_bucket_entry& entry = bucket_entries[ data_index ];
				entry.m_data_index = data_index;

				const std::size_t bucket_index = hash( get_key( data[ data_index ] ), primary_salt ) % Size;
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
						// Start over increase the salt we use
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
				ASSUME( slot_data_index[ slot ] != 0, "Data at slot should be non zero", slot, slot_data_index );
				const std::size_t data_index = slot_data_index[ slot ] - 1;
				const pointer storage = std::addressof( m_storage[ slot ] );
				std::destroy_at( storage );
				std::construct_at( storage, data[ data_index ] );
			}
		}

		/// @brief Finds the element with the given @p key.
		/// @param key The key to look up.
		/// @return A const iterator to the matching element, or @ref end() if @p key is not present.
		[[nodiscard]] constexpr const_iterator find( const key_type& key ) const
		{
			const std::size_t data_index = find_data_index( key );
			const auto it = begin() + data_index;
			return equals( get_key( *it ), key ) ? it : end();
		}

		/// @brief Checks whether an element with the given @p key is present.
		/// @param key The key to look up.
		/// @return @c true if @p key is stored in the container.
		[[nodiscard]] constexpr bool contains( const key_type& key ) const
		{
			const std::size_t data_index = find_data_index( key );
			return equals( get_key( m_storage[ data_index ] ), key );
		}

		/// @brief Returns the number of elements, which is always @p Size.
		[[nodiscard]] static constexpr size_type size() noexcept
		{
			return Size;
		}

		/// @brief Returns the maximum number of elements, which is always @p Size.
		[[nodiscard]] static constexpr size_type max_size() noexcept
		{
			return Size;
		}

		/// @brief Returns a pointer to the underlying contiguous element storage.
		[[nodiscard]] constexpr pointer data() noexcept
		{
			return m_storage.data();
		}
		/// @copydoc data()
		[[nodiscard]] constexpr const_pointer data() const noexcept
		{
			return m_storage.data();
		}

		/// @brief Returns an iterator to the first element.
		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return m_storage.begin();
		}
		/// @copydoc begin()
		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return m_storage.begin();
		}
		/// @brief Returns a const iterator to the first element.
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return m_storage.cbegin();
		}

		/// @brief Returns an iterator past the last element.
		[[nodiscard]] constexpr iterator end() noexcept
		{
			return m_storage.end();
		}
		/// @copydoc end()
		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return m_storage.end();
		}
		/// @brief Returns a const iterator past the last element.
		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return m_storage.cend();
		}

		/// @brief Returns a reverse iterator to the last element.
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept
		{
			return m_storage.rbegin();
		}
		/// @copydoc rbegin()
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
		{
			return m_storage.rbegin();
		}
		/// @brief Returns a const reverse iterator to the last element.
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return m_storage.crbegin();
		}

		/// @brief Returns a reverse iterator before the first element.
		[[nodiscard]] constexpr reverse_iterator rend() noexcept
		{
			return m_storage.rend();
		}
		/// @copydoc rend()
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
		{
			return m_storage.rend();
		}
		/// @brief Returns a const reverse iterator before the first element.
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
		{
			return m_storage.crend();
		}

	protected:
		/// @brief Computes the storage slot index a @p key maps to via the perfect hash.
		/// @details Resolves the bucket's stored salt: a negative value encodes a direct slot index, a non-negative
		/// value is the secondary salt used to hash @p key to its slot. The result is unverified, callers must still
		/// compare the key at that slot, since a key not in the set will still map to some slot.
		/// @param key The key to locate.
		/// @return The slot index @p key hashes to.
		[[nodiscard]] constexpr size_type find_data_index( const key_type& key ) const
		{
			const std::size_t salt_index = hash( key, primary_salt ) % Size;
			const std::int32_t salt_value = m_salts[ salt_index ];
			return salt_value < 0 ? ( -salt_value - 1 ) : ( hash( key, salt_value ) % Size );
		}

		/// @brief Hashes @p key with @p salt using the @p Hash functor.
		[[nodiscard]] constexpr std::size_t hash( const Key& key, const std::size_t salt ) const noexcept
		{
			return static_cast<const Hash&>( *this )( key, salt );
		}
		/// @brief Compares @p lhs and @p rhs for equality using the @p KeyEquals functor.
		[[nodiscard]] constexpr std::size_t equals( const Key& lhs, const Key& rhs ) const noexcept
		{
			return static_cast<const KeyEquals&>( *this )( lhs, rhs );
		}
		/// @brief Extracts the key from a stored @p value using the @p GetKey functor.
		[[nodiscard]] constexpr const Key& get_key( const StoredValue& value ) const noexcept
		{
			return static_cast<const GetKey&>( *this )( value );
		}

	private:
		storage_array m_storage{};
		sized_array<std::int32_t> m_salts{};
	};
}
