#pragma once

#include "mclo/container/slot_map_handle.hpp"

#include <cinttypes>
#include <optional>
#include <stdexcept>
#include <vector>

namespace mclo
{
	/// @brief A slot map which provides stable weak references in the form of handles to data
	/// @details A slot map provides weak references that can be queried if they are valid and upon using return
	/// nullptrs. The object handles have a customizable amount of data to use for number of objects and object
	/// generation.
	///
	/// This contiguous slot map is designed for iteration speed at the cost of an extra lookup indirection, all the
	/// objects are stored in contiguous memory and re-shuffled on erasure for best iteration speed.
	///
	/// Pros:
	/// * Can iterate over objects in contiguous memory
	/// * Backing of a vector means amortized growth costs and low memory overhead
	/// * All operations perform in a constant time
	///
	/// Cons:
	/// * Lookup is two indirections, one from public handle to indirection list, then indirection list to object
	/// * When allocations are needed it can be a slowdown spikes as its moves all objects into the new block
	///		* This can be somewhat mitigated with up front reserves. todo(mc): Mention paged slot map once made
	///
	/// @tparam T The type of objects to store
	/// @tparam HandleTotalBits Total number of bits for the handle type to use, smallest integer type to fit these bits
	/// will be the handle representation type, defaults to a std::uint32_t. More bits = larger max size & generation
	/// but larget handle.
	/// @tparam GenerationBits Number of the totaly bits in the handle representation type used for generation checking,
	/// defaults to 1/4 of the total bits. More bits = lower max size but higher generation before wrapping
	/// @tparam Allocator The allocator used by the backing vectors
	template <typename T,
			  std::size_t HandleTotalBits = sizeof( std::uint32_t ) * CHAR_BIT,
			  std::size_t GenerationBits = HandleTotalBits / 4,
			  typename Allocator = std::allocator<T>>
	class dense_slot_map
	{
	private:
		using underlying_container = std::vector<T, Allocator>;

		class [[nodiscard]] emplace_guard
		{
		public:
			explicit emplace_guard( dense_slot_map& map ) noexcept
				: ptr( &map )
			{
			}

			emplace_guard( const emplace_guard& ) = delete;
			emplace_guard& operator=( const emplace_guard& ) = delete;

			~emplace_guard()
			{
				if ( ptr ) [[unlikely]]
				{
					if ( inserted_to_reverse_map )
					{
						ptr->m_data_reverse_map.pop_back();
					}
					ptr->m_data.pop_back();
				}
			}

			void release() noexcept
			{
				ptr = nullptr;
			}
			void set_inserted_to_reverse_map() noexcept
			{
				inserted_to_reverse_map = true;
			}

		private:
			dense_slot_map* ptr = nullptr;
			bool inserted_to_reverse_map = false;
		};

		struct [[nodiscard]] noop_guard
		{
			explicit noop_guard( const dense_slot_map& ) noexcept
			{
			}

			void release() noexcept
			{
			}
			void set_inserted_to_reverse_map() noexcept
			{
			}
		};

	public:
		using handle_type = slot_map_handle<T, HandleTotalBits, GenerationBits>;
		using value_type = T;
		using allocator_type = Allocator;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using difference_type = typename underlying_container::difference_type;
		// We can't have more items than our index represents anyway
		using size_type = typename handle_type::representation_type;
		using iterator = typename underlying_container::iterator;
		using const_iterator = typename underlying_container::const_iterator;
		using reverse_iterator = typename underlying_container::reverse_iterator;
		using const_reverse_iterator = typename underlying_container::const_reverse_iterator;

		dense_slot_map() noexcept( noexcept( Allocator() ) ) = default;

		explicit dense_slot_map( const Allocator& allocator ) noexcept
			: m_data( allocator )
			, m_data_reverse_map( allocator )
			, m_slot_indirection( allocator )
		{
		}

		explicit dense_slot_map( const size_type slot_count, const Allocator& allocator = Allocator() )
			: dense_slot_map( allocator )
		{
			reserve_slots( slot_count );
		}

		/// @brief Result of creating an object containing a reference to it and its handle
		struct emplace_result
		{
			/// @brief Reference to the created object itself
			reference object;

			/// @brief handle to the created object
			handle_type handle;
		};

		/// @brief Emplace an entry into the slot map constructed from arguments
		/// @post Provides a strong exception guarantee, if an exception is thrown no change to the container is made
		/// @tparam ...Args Type of arguments to construct from
		/// @param ...arguments Arguments to construct from
		/// @return Result containing a reference to the created entry and the handle to it
		template <typename... Args>
		[[nodiscard]] emplace_result emplace_and_get( Args... arguments )
		{
			const size_type current_size = size();
			if ( current_size == max_size() ) [[unlikely]]
			{
				throw_too_big();
			}
			if ( current_size < capacity() )
			{
				assert( current_size <= slot_count() );
				return emplace_and_get_with_guard<noop_guard>( std::forward<Args>( arguments )... );
			}
			return emplace_and_get_with_guard<emplace_guard>( std::forward<Args>( arguments )... );
		}

		/// @brief Emplace an entry into the slot map constructed from arguments
		/// @post Provides a strong exception guarantee, if an exception is thrown no change to the container is made
		/// @tparam ...Args Type of arguments to construct from
		/// @param ...arguments Arguments to construct from
		/// @return handle to the created entry
		template <typename... Args>
		[[nodiscard]] handle_type emplace( Args... arguments )
		{
			return emplace_and_get( std::forward<Args>( arguments )... ).handle;
		}

		/// @brief Insert a copy into the slot map
		/// @post Provides a strong exception guarantee, if an exception is thrown no change to the container is made
		/// @param object Reference to the object to copy into the slot map
		/// @return handle to the created entry
		[[nodiscard]] handle_type insert( const_reference object )
		{
			return emplace( object );
		}

		/// @brief Insert a move into the slot map
		/// @post Provides a strong exception guarantee, if an exception is thrown no change to the container is made
		/// @param object Reference to the object to move into the slot map
		/// @return handle to the created entry
		[[nodiscard]] handle_type insert( value_type&& object )
		{
			return emplace( std::move( object ) );
		}

		/// @brief Reserve space for amount old_num_slots
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @details Beneficial as allocating more old_num_slots than values causes the generations to increase
		/// more evenly distributed across the old_num_slots compared to recycling the same old_num_slots forever.
		/// @param amount The number of old_num_slots to reserve for
		void reserve_slots( const size_type amount )
		{
			if ( amount > max_size() ) [[unlikely]]
			{
				throw_too_big();
			}

			m_slot_indirection.reserve( amount );

			const size_type old_num_slots = slot_count();

			// If we reserved more old_num_slots then link into the free list
			// This links the new old_num_slots in reverse order and inserts them as the new head, this is because
			// they have fresh generation counters so we want to use them up first instead of ones with
			// potentially higher and uneven distribution of generations
			if ( old_num_slots < amount )
			{
				try
				{
					// Link in the first item as pointing to the current head
					m_slot_indirection.push_back( handle_type{ m_free_list_head, {} } );

					const size_type last_new_slot = amount - 1;
					size_type newest_slot = old_num_slots;

					// Chain together all the other old_num_slots
					while ( newest_slot != last_new_slot )
					{
						m_slot_indirection.push_back( handle_type{ newest_slot++, {} } );
					}

					// Make the end of our new old_num_slots the new head
					m_free_list_head = newest_slot;
				}
				catch ( ... )
				{
					m_slot_indirection.resize( old_num_slots );
					throw;
				}
			}
		}

		/// @brief Reserve capacity for elements and old_num_slots in the slot map
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @param amount Number of elements to reserve size for
		void reserve( const size_type amount )
		{
			reserve_slots( amount );
			m_data.reserve( amount );
			m_data_reverse_map.reserve( amount );
		}

		/// @brief Erase an entry in the slot map from its handle
		/// @param handle The handle to the object to erase
		void erase( handle_type handle ) noexcept( std::is_nothrow_move_assignable_v<T> )
		{
			if ( is_valid( handle ) ) [[likely]]
			{
				erase_valid_handle( handle );
			}
		}

		/// @brief Erase the element at the handle if it exists returning the data it contained, else return an empty
		/// optional
		/// @param handle The handle to move the data from then remove
		/// @return Contains the data at the handle's location, else an empty optional
		[[nodiscard]] std::optional<value_type> pop( const handle_type handle ) noexcept(
			std::is_nothrow_move_constructible_v<T> )
		{
			const pointer ptr = lookup( handle );
			if ( !ptr ) [[unlikely]]
			{
				return std::nullopt;
			}

			T object( std::move( *ptr ) );
			erase_valid_handle( handle );
			return std::move( object );
		}

		/// @brief Destroy all active elements and put all old_num_slots back into the free list
		/// @details Increases the generation counter of all old_num_slots, so existing handles are valid to use
		/// @warning Doing this frequently can cause your old_num_slots to increase in version faster than via normal
		/// use, if you want to clear everything out and have no old handles you'll want to use Reset instead
		void clear() noexcept
		{
			m_data.clear();
			m_data_reverse_map.clear();

			// Start from the first again
			m_free_list_head = 0;
			const size_type num_slots = slot_count();

			// Chain all following elements to the end
			for ( size_type index = 0; index < num_slots; ++index )
			{
				handle_type& handle = m_slot_indirection[ index ];
				handle.index = index + 1;
				++handle.generation;
			}

			// Set the last element as the tail and refer to itself as the end point
			m_free_list_tail = num_slots - 1;
			m_slot_indirection[ m_free_list_tail ].index = m_free_list_tail;
		}

		/// @brief Destroy all active elements and clear all old_num_slots
		/// @warning This invalidates all existing handles as it resets generation counters, be sure none in use else
		/// it'll lead to collisions
		void reset() noexcept
		{
			m_data.clear();
			m_slot_indirection.clear();
			m_data_reverse_map.clear();
			m_free_list_head = 0;
			m_free_list_tail = 0;
		}

		/// @brief Check if a handle is a valid entry into the slot map
		/// @param handle The handle to check
		/// @return If the handle refers to a valid entry in the slot map
		[[nodiscard]] bool is_valid( const handle_type handle ) const noexcept
		{
			// index is in bounds and matching generation, if slot re-used and our handle is out of date generation
			// mismatches
			return handle.index < slot_count() && m_slot_indirection[ handle.index ].generation == handle.generation;
		}

		/// @brief Lookup an entry in the slot map
		/// @param handle The handle to lookup
		/// @return Const pointer to the object the handle refers to, or nullptr if an invalid handle
		[[nodiscard]] const_pointer lookup( const handle_type handle ) const noexcept
		{
			if ( handle.index >= slot_count() ) [[unlikely]]
			{
				return nullptr;
			}

			const handle_type indirection_handle = m_slot_indirection[ handle.index ];
			if ( indirection_handle.generation != handle.generation )
			{
				return nullptr;
			}

			return &m_data[ indirection_handle.index ];
		}

		/// @brief Lookup an entry in the slot map
		/// @param handle The handle to lookup
		/// @return Mutable pointer to the object the handle refers to, or nullptr if an invalid handle
		[[nodiscard]] pointer lookup( const handle_type handle ) noexcept
		{
			return const_cast<pointer>( std::as_const( *this ).lookup( handle ) );
		}

		/// @brief Get the number of active objects
		[[nodiscard]] size_type size() const noexcept
		{
			return static_cast<size_type>( m_data.size() );
		}

		/// @brief Get the total capacity of the objects
		[[nodiscard]] size_type capacity() const noexcept
		{
			return static_cast<size_type>( m_data.capacity() );
		}

		/// @brief Is the slot map empty
		[[nodiscard]] bool empty() const noexcept
		{
			return m_data.empty();
		}

		/// @brief Get the number of slots, guaranteed >= size()
		[[nodiscard]] size_type slot_count() const noexcept
		{
			return static_cast<size_type>( m_slot_indirection.size() );
		}

		/// @brief Get the maximum number of objects
		[[nodiscard]] size_type max_size() const noexcept
		{
			return handle_type::max_index + 1;
		}

		/// @brief Get the allocator used
		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return m_data.get_allocator();
		}

		[[nodiscard]] reference front() noexcept
		{
			return m_data.front();
		}
		[[nodiscard]] reference back() noexcept
		{
			return m_data.back();
		}

		[[nodiscard]] const_reference front() const noexcept
		{
			return m_data.front();
		}
		[[nodiscard]] const_reference back() const noexcept
		{
			return m_data.back();
		}

		[[nodiscard]] pointer data() noexcept
		{
			return m_data.data();
		}
		[[nodiscard]] const_pointer data() const noexcept
		{
			return m_data.data();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return m_data.begin();
		}
		[[nodiscard]] iterator end() noexcept
		{
			return m_data.end();
		}
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return m_data.begin();
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return m_data.end();
		}
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return m_data.cbegin();
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return m_data.cend();
		}

		[[nodiscard]] reverse_iterator rbegin() noexcept
		{
			return m_data.rbegin();
		}
		[[nodiscard]] reverse_iterator rend() noexcept
		{
			return m_data.rend();
		}
		[[nodiscard]] const_reverse_iterator rbegin() const noexcept
		{
			return m_data.rbegin();
		}
		[[nodiscard]] const_reverse_iterator rend() const noexcept
		{
			return m_data.rend();
		}
		[[nodiscard]] const_reverse_iterator crbegin() const noexcept
		{
			return m_data.crbegin();
		}
		[[nodiscard]] const_reverse_iterator crend() const noexcept
		{
			return m_data.crend();
		}

		/// @brief Swap this with Other
		/// @param Other The target of the swap
		void swap( dense_slot_map& other ) noexcept
		{
			using std::swap;
			swap( m_data, other.m_data );
			swap( m_slot_indirection, other.m_slot_indirection );
			swap( m_data_reverse_map, other.m_data_reverse_map );
			swap( m_free_list_head, other.m_free_list_head );
			swap( m_free_list_tail, other.m_free_list_tail );
		}

		friend void swap( dense_slot_map& lhs, dense_slot_map& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		[[noreturn]] void throw_too_big() const
		{
			throw std::length_error( "Slot map too large for maximum handle index" );
		}

		/// @brief Emplace an entry into the slot map constructed from arguments
		/// @post Provides a strong exception guarantee, if an exception is thrown no change to the container is made
		/// @tparam ...Args Type of arguments to construct from
		/// @param ...arguments Arguments to construct from
		/// @return Result containing a reference to the created entry and the handle to it
		template <typename GuardType, typename... Args>
		[[nodiscard]] emplace_result emplace_and_get_with_guard( Args... arguments )
		{
			// Insert our data, if this throws we've no clean up needed so we do it first since it is most possible to
			// throw due to user constructors
			m_data.emplace_back( std::forward<Args>( arguments )... );

			GuardType guard( *this );

			// Take the free list head index, it points into our indexes array of what data to use
			// Push it into the data -> public handle so that we can do O(1) erasures
			const size_type slot_index = m_data_reverse_map.emplace_back( m_free_list_head );
			guard.set_inserted_to_reverse_map();

			const size_type old_num_slots = slot_count();

			// Free list empty so we add a new index
			const bool needs_new_index = m_free_list_head == old_num_slots;

			// Free list empty or last in free list, we need to update the head + tail to the new sentinel size
			const bool mark_empty_free_list = needs_new_index || m_free_list_head == m_free_list_tail;

			const size_type new_num_slots = old_num_slots + static_cast<size_type>( needs_new_index );

			if ( needs_new_index )
			{
				// If this throws we've not modified the free list head or tail yet
				m_slot_indirection.push_back( handle_type{ new_num_slots, {} } );
			}

			assert( new_num_slots == slot_count() );

			// Our re-used handle for this object
			handle_type& handle = m_slot_indirection[ slot_index ];

			if ( mark_empty_free_list )
			{
				m_free_list_tail = m_free_list_head = new_num_slots;
			}
			else // Advance free list
			{
				m_free_list_head = handle.index;
			}

			// Set handle's index to point to that
			handle.index = m_data.size() - 1;

			// We're safe now to return without cleanup
			guard.release();

			// We return the original slot index and the generation, generation will be changed in erasure for
			// invalidating existing handles
			return {
				m_data.back(), { slot_index, handle.generation }
            };
		}

		/// @brief Erase an entry in the slot map from its handle
		/// @param handle The handle to the object to erase
		/// @warning Assumes the handle passes is_valid
		void erase_valid_handle( handle_type handle ) noexcept( std::is_nothrow_move_assignable_v<T> )
		{
			const size_type handle_index = handle.index;

			// Increment generation, now remaining handles will mismatch on usage
			++m_slot_indirection[ handle_index ].generation;

			const size_type data_index = m_slot_indirection[ handle_index ].index;
			const size_type data_last_index = size() - 1;

			// If we are not the tail we overwrite our data with the tail so maintain a contiguous array of data
			if ( data_index != data_last_index ) [[likely]]
			{
				// Overwrite our object with tail this maintains contiguous array of data
				m_data[ data_index ] = std::move( m_data[ data_last_index ] );
				m_data_reverse_map[ data_index ] = std::move( m_data_reverse_map[ data_last_index ] );

				// Use data we overwrote with to find and update its indirection array link with the data for our new
				// location
				m_slot_indirection[ m_data_reverse_map[ data_index ] ].index = data_index;
			}

			// We pop the tail as that is either us directly or what we moved from to overwrite ourselves
			m_data.pop_back();
			m_data_reverse_map.pop_back();

			// Start free list if empty or chain into existing free list
			if ( m_free_list_head == slot_count() )
			{
				m_free_list_head = handle_index;
			}
			else
			{
				m_slot_indirection[ m_free_list_tail ].index = handle_index;
			}

			// Is always the new tail of the free list
			m_free_list_tail = handle_index;
		}

		using alloc_traits = std::allocator_traits<allocator_type>;

		/// @brief Contiguous array of data for fast iteration over all elements
		/// @details Order will be changed upon erasure
		underlying_container m_data;

		/// @brief Indirection array, lookup from handle.index -> m_data
		/// @details Handles returned by the API provide indexes into this array, which is stable, to then index into
		/// the actual data.
		/// The handle's indexes are also an in built free list, if an entry is in the free list its index points to the
		/// next entry in this array that is part of the free list.
		/// If the index is itself then it is the end of the free list.
		std::vector<handle_type, typename alloc_traits::template rebind_alloc<handle_type>> m_slot_indirection;

		/// @brief Indirection array, lookup from index in data -> public handles
		/// @details Array of indexes that let an entry in the data array know what handle in the public handles is
		/// referring to it.
		/// Used so that upon erasure we can update the indirection to refer to an object's new location in data
		std::vector<size_type, typename alloc_traits::template rebind_alloc<size_type>> m_data_reverse_map;

		/// @brief index into slot array for the start of the free list
		/// @details Is either equal to m_free_list_tail and slot_count() when empty
		/// Or is > 0 and < slot_count() and points to start of free list, with each slot pointing to the
		/// next entry in the free list until m_free_list_tail.
		/// Tail can be equal to head if there is only one free slot.
		size_type m_free_list_head = 0;

		/// @brief index into slot array for the end of the free list
		size_type m_free_list_tail = 0;
	};

	namespace pmr
	{
		template <typename T,
				  std::size_t HandleTotalBits = sizeof( std::uint32_t ) * CHAR_BIT,
				  std::size_t GenerationBits = HandleTotalBits / 4>
		using dense_slot_map =
			mclo::dense_slot_map<T, HandleTotalBits, GenerationBits, std::pmr::polymorphic_allocator<T>>;
	}
}
