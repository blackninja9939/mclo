#pragma once

#include "mclo/container/slot_map_handle.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <cinttypes>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace mclo
{
	namespace detail
	{
		template <class Allocator>
		class [[nodiscard]] uninitialized_alloc_guard
		{
		public:
			using value_type = typename Allocator::value_type;
			using traits = std::allocator_traits<Allocator>;

			uninitialized_alloc_guard( Allocator& alloc, const std::size_t count )
				: m_allocator( alloc )
				, m_value( traits::allocate( alloc, count ) )
				, m_count( count )
			{
			}

			uninitialized_alloc_guard( const uninitialized_alloc_guard& ) = delete;
			uninitialized_alloc_guard& operator=( const uninitialized_alloc_guard& ) = delete;

			~uninitialized_alloc_guard()
			{
				traits::deallocate( m_allocator, m_value, m_count );
			}

			[[nodiscard]] value_type* value() const noexcept
			{
				return m_value;
			}

			void release() noexcept
			{
				m_value = nullptr;
				m_count = 0;
			}

		private:
			Allocator& m_allocator;
			value_type* m_value = nullptr;
			std::size_t m_count = 0;
		};

		/// @brief Underlying data storage for dense slot map, it allocates the values and reverse map in one allocation
		/// to reduce allocations
		template <typename Value, auto MaxSize, typename Allocator>
		class dense_slot_map_data
		{
		public:
			using value_type = Value;
			using size_type = decltype( MaxSize );

		private:
			struct alignas( value_type ) aligned_buffer
			{
				std::byte buffer[ alignof( value_type ) ];
			};

		public:
			using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<aligned_buffer>;

			dense_slot_map_data() = default;

			explicit dense_slot_map_data( const allocator_type& allocator ) noexcept
				: m_allocator( allocator )
			{
			}

			dense_slot_map_data( const dense_slot_map_data& other )
				: m_allocator( alloc_traits::select_on_container_copy_construction( other.m_allocator ) )
			{
				copy_from( other );
			}

			dense_slot_map_data( dense_slot_map_data&& other ) noexcept
				: m_data( std::exchange( other.m_data, nullptr ) )
				, m_data_reverse_map( std::exchange( other.m_data_reverse_map, nullptr ) )
				, m_size( std::exchange( other.m_size, 0 ) )
				, m_capacity( std::exchange( other.m_capacity, 0 ) )
				, m_allocator( std::move( other.m_allocator ) )
			{
			}

			dense_slot_map_data& operator=( const dense_slot_map_data& other )
			{
				if ( this == &other )
				{
					return *this;
				}
				clear();
				if constexpr ( alloc_traits::propagate_on_container_copy_assignment::value )
				{
					m_allocator = other.m_allocator;
				}
				else
				{
					DEBUG_ASSERT( m_allocator == other.m_allocator, "containers incompatible for copy assignment" );
				}
				copy_from( other );
				return *this;
			}

			dense_slot_map_data& operator=( dense_slot_map_data&& other ) noexcept
			{
				if ( this == &other )
				{
					return *this;
				}
				clear();
				if constexpr ( alloc_traits::propagate_on_container_move_assignment::value )
				{
					m_allocator = std::move( other.m_allocator );
				}
				else
				{
					DEBUG_ASSERT( m_allocator == other.m_allocator, "containers incompatible for move assignment" );
				}
				m_data = std::exchange( other.m_data, nullptr );
				m_data_reverse_map = std::exchange( other.m_data_reverse_map, nullptr );
				m_size = std::exchange( other.m_size, 0 );
				m_capacity = std::exchange( other.m_capacity, 0 );
				return *this;
			}

			~dense_slot_map_data()
			{
				std::destroy_n( m_data, m_size );
				std::destroy_n( m_data_reverse_map, m_size );
				const std::size_t buffer_count = size_in_aligned_buffers( m_capacity );
				alloc_traits::deallocate( m_allocator, reinterpret_cast<aligned_buffer*>( m_data ), buffer_count );
			}

			void swap( dense_slot_map_data& other ) noexcept
			{
				if ( this == &other )
				{
					return;
				}
				using std::swap;
				if constexpr ( alloc_traits::propagate_on_container_swap::value )
				{
					swap( m_allocator, other.m_allocator );
				}
				else
				{
					DEBUG_ASSERT( m_allocator == other.m_allocator, "containers incompatible for swap" );
				}
				swap( m_data, other.m_data );
				swap( m_data_reverse_map, other.m_data_reverse_map );
				swap( m_size, other.m_size );
				swap( m_capacity, other.m_capacity );
			}

			friend void swap( dense_slot_map_data& lhs, dense_slot_map_data& rhs ) noexcept
			{
				lhs.swap( rhs );
			}

			[[nodiscard]] size_type capacity() const noexcept
			{
				return m_capacity;
			}

			[[nodiscard]] size_type size() const noexcept
			{
				return m_size;
			}

			[[nodiscard]] size_type max_size() const noexcept
			{
				return MaxSize;
			}

			[[nodiscard]] allocator_type get_allocator() const noexcept
			{
				return m_allocator;
			}

			[[nodiscard]] value_type* values() const noexcept
			{
				return m_data;
			}
			[[nodiscard]] size_type* data_reverse_map() const noexcept
			{
				return m_data_reverse_map;
			}

			template <typename... Args>
			void emplace_back( const size_type index, Args&&... args )
			{
				reserve( m_size + 1 );
				DEBUG_ASSERT( m_size < m_capacity, "Size should be less than capacity" );
				std::construct_at( m_data + m_size, std::forward<Args>( args )... );
				std::construct_at( m_data_reverse_map + m_size, index );
				++m_size;
			}

			void pop_back() MCLO_NOEXCEPT_TESTS
			{
				DEBUG_ASSERT( m_size > 0, "Size should be greater than 0" );
				--m_size;
				std::destroy_at( m_data + m_size );
				std::destroy_at( m_data_reverse_map + m_size );
			}

			bool swap_and_pop_at( const size_type index )
				MCLO_NOEXCEPT_TESTS_IF( std::is_nothrow_move_assignable_v<value_type> )
			{
				DEBUG_ASSERT( index < m_size, "Index should be less than size" );
				if ( index == m_size - 1 )
				{
					pop_back();
					return false;
				}

				m_data[ index ] = std::move( m_data[ m_size - 1 ] );
				m_data_reverse_map[ index ] = m_data_reverse_map[ m_size - 1 ];
				pop_back();
				return true;
			}

			void clear() noexcept
			{
				std::destroy_n( m_data, m_size );
				std::destroy_n( m_data_reverse_map, m_size );
				m_size = 0;
			}

			void reserve( const size_type num_objects )
			{
				if ( num_objects <= m_capacity )
				{
					return;
				}

				const size_type new_capacity = calculate_new_capacity( num_objects );
				uninitialized_alloc_guard alloc_guard( m_allocator, size_in_aligned_buffers( new_capacity ) );

				value_type* const new_data = reinterpret_cast<value_type*>( alloc_guard.value() );
				size_type* const new_data_reverse_map = reinterpret_cast<size_type*>( new_data + new_capacity );

				// The uninitialized_move_n algorithms take care of destroying elements if an exception is thrown part
				// way through, so we only need to handle deallocation in our guard not destruction here
				std::uninitialized_move_n( m_data, m_size, new_data );

				static_assert( std::is_nothrow_move_constructible_v<size_type>,
							   "Moving size does not throw exceptions, if it does we need a guard object for our "
							   "constructed objects" );
				std::uninitialized_move_n( m_data_reverse_map, m_size, new_data_reverse_map );

				alloc_traits::deallocate(
					m_allocator, reinterpret_cast<aligned_buffer*>( m_data ), size_in_aligned_buffers( m_capacity ) );

				alloc_guard.release();

				m_data = new_data;
				m_data_reverse_map = new_data_reverse_map;
				m_capacity = new_capacity;
			}

		private:
			static constexpr float growth_factor = 1.5f;

			[[nodiscard]] static constexpr std::size_t size_in_aligned_buffers( const size_type num_objects ) noexcept
			{
				const std::size_t object_size = sizeof( value_type ) * num_objects;
				const std::size_t data_reverse_map_size = sizeof( size_type ) * num_objects;
				const std::size_t data_size = object_size + data_reverse_map_size;
				const std::size_t aligned_size =
					( data_size + alignof( aligned_buffer ) - 1 ); // Round up to alignof(aligned_buffer)
				return aligned_size / alignof( aligned_buffer );
			}

			[[nodiscard]] size_type calculate_new_capacity( const size_type new_size ) const noexcept
			{
				const size_type old_capacity = m_capacity;
				const size_type max_capacity = max_size();

				// Growing would overflow
				if ( old_capacity > max_capacity - static_cast<size_type>( old_capacity / growth_factor ) ) [[unlikely]]
				{
					return max_capacity;
				}

				const size_type new_capacity = static_cast<size_type>( old_capacity * growth_factor );
				return std::max( new_capacity, new_size );
			}

			void copy_from( const dense_slot_map_data& other )
			{
				DEBUG_ASSERT( m_size == 0, "Size should be 0" );
				reserve( other.m_size );
				std::uninitialized_copy_n( other.m_data, other.m_size, m_data );
				static_assert( std::is_nothrow_copy_constructible_v<size_type>,
							   "Copying size does not throw exceptions, if it does we need a guard object for our "
							   "constructed objects" );
				std::uninitialized_copy_n( other.m_data_reverse_map, other.m_size, m_data_reverse_map );
				m_size = other.m_size;
			}

			using alloc_traits = std::allocator_traits<allocator_type>;

			/// @brief Contiguous array of data for fast iteration over all elements
			/// @details Order will be changed upon erasure
			value_type* m_data = nullptr;

			/// @brief Indirection array, lookup from index in data -> public handles
			/// @details Array of indexes that let an entry in the data array know what handle in the public handles is
			/// referring to it.
			/// Used so that upon erasure we can update the indirection to refer to an object's new location in data
			size_type* m_data_reverse_map = nullptr;

			/// @brief Number of objects in the slot map
			size_type m_size = 0;

			/// @brief Number of objects the slot map can hold before reallocation
			size_type m_capacity = 0;

			/// @brief Allocator used for the backing data
			MCLO_NO_UNIQUE_ADDRESS allocator_type m_allocator{};
		};
	} // namespace detail

	/// @brief A slot map storing contiguous data and provides stable weak references in the form of handles
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
	/// * All operations perform in a constant time unless reallocating
	///
	/// Cons:
	/// * Lookup is two indirections, one from public handle to indirection list, then indirection list to object
	/// * When allocations are needed it can be a slowdown spikes as its moves all objects into the new block
	///		* This can be somewhat mitigated with up front reserves
	///		* This can be somewhat mitigated with a custom allocator
	///		* For large objects or objects with slow moves they can be wrapped with a smart pointer indirection
	///
	/// @tparam T The type of objects to store
	/// @tparam HandleTotalBits Total number of bits for the handle type to use, smallest integer type to fit these bits
	/// will be the handle representation type, defaults to a std::uint32_t. More bits = larger max size & generation
	/// but larger handle.
	/// @tparam GenerationBits Number of the total bits in the handle representation type used for generation checking,
	/// defaults to 1/4 of the total bits. More bits = lower max size but higher generation before wrapping
	/// @tparam Allocator The allocator used by the backing vectors
	template <typename T,
			  std::size_t HandleTotalBits = sizeof( std::uint32_t ) * CHAR_BIT,
			  std::size_t GenerationBits = HandleTotalBits / 4,
			  typename Allocator = std::allocator<T>>
	class dense_slot_map
	{
	public:
		using handle_type = slot_map_handle<T, HandleTotalBits, GenerationBits>;

	private:
		using underlying_container = detail::dense_slot_map_data<T, handle_type::max_index + 1, Allocator>;

		class [[nodiscard]] emplace_guard
		{
		public:
			explicit emplace_guard( underlying_container& map ) noexcept
				: ptr( &map )
			{
			}

			emplace_guard( const emplace_guard& ) = delete;
			emplace_guard& operator=( const emplace_guard& ) = delete;

			~emplace_guard()
			{
				if ( ptr ) [[unlikely]]
				{
					ptr->pop_back();
				}
			}

			void release() noexcept
			{
				ptr = nullptr;
			}

		private:
			underlying_container* ptr = nullptr;
		};

		struct [[nodiscard]] noop_guard
		{
			explicit noop_guard( const underlying_container& ) noexcept
			{
			}

			void release() noexcept
			{
			}
		};

	public:
		using value_type = typename underlying_container::value_type;
		using allocator_type = Allocator;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using difference_type = std::ptrdiff_t;
		// We can't have more items than our index represents anyway
		using size_type = typename underlying_container::size_type;
		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		dense_slot_map() noexcept( noexcept( allocator_type() ) ) = default;

		explicit dense_slot_map( const allocator_type& allocator ) noexcept
			: m_data( allocator )
			, m_slot_indirection( allocator )
		{
		}

		explicit dense_slot_map( const size_type slot_count, const allocator_type& allocator = allocator_type() )
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
		[[nodiscard]] emplace_result emplace_and_get( Args&&... arguments )
		{
			const size_type current_size = size();
			if ( current_size >= max_size() ) [[unlikely]]
			{
				throw_too_big();
			}
			if ( current_size < capacity() )
			{
				DEBUG_ASSERT( current_size <= slot_count(), "Size should be less than or equal to slot_count" );
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
		[[nodiscard]] handle_type emplace( Args&&... arguments )
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

		/// @brief Reserve space for amount slots
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @details Beneficial as allocating more slots than values causes the generations to increase
		/// more evenly distributed across the slots compared to recycling the same slots forever.
		/// @param amount The number of slots to reserve for
		void reserve_slots( const size_type amount )
		{
			if ( amount > max_size() ) [[unlikely]]
			{
				throw_too_big();
			}

			m_slot_indirection.reserve( amount );

			const size_type old_num_slots = slot_count();

			// If we reserved more slots then link into the free list
			// This links the new slots in reverse order and inserts them as the new head, this is because
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

					// Chain together all the other slots
					while ( newest_slot != last_new_slot )
					{
						m_slot_indirection.push_back( handle_type{ newest_slot++, {} } );
					}

					// Make the end of our new slots the new head
					m_free_list_head = newest_slot;
				}
				catch ( ... )
				{
					m_slot_indirection.resize( old_num_slots );
					throw;
				}
			}
		}

		/// @brief Reserve capacity for elements and slots in the slot map
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @param amount Number of elements to reserve size for
		void reserve( const size_type amount )
		{
			reserve_slots( amount );
			m_data.reserve( amount );
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

		/// @brief Erase an entry in the slot map from its iterator
		/// @param pos The iterator to the object to erase
		/// @return Iterator to the next element after the erased one
		iterator erase( const const_iterator pos ) noexcept( std::is_nothrow_move_assignable_v<T> )
		{
			if ( pos == end() ) [[unlikely]]
			{
				return end();
			}
			const size_type data_index = static_cast<size_type>( std::distance( cbegin(), pos ) );
			erase_valid_handle( get_valid_handle_at( data_index ) );
			return begin() + data_index;
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

		/// @brief Destroy all active elements and put all slots back into the free list
		/// @details Increases the generation counter of all slots, so existing handles are valid to use
		/// @warning Doing this frequently can cause your slots to increase in version faster than via normal
		/// use, if you want to clear everything out and have no old handles you'll want to use Reset instead
		void clear() noexcept
		{
			m_data.clear();

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

		/// @brief Destroy all active elements and clear all slots
		/// @warning This invalidates all existing handles as it resets generation counters, be sure none in use else
		/// it'll lead to collisions
		void reset() noexcept
		{
			m_data.clear();
			m_slot_indirection.clear();
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

			return m_data.values() + indirection_handle.index;
		}

		/// @brief Lookup an entry in the slot map
		/// @param handle The handle to lookup
		/// @return Mutable pointer to the object the handle refers to, or nullptr if an invalid handle
		[[nodiscard]] pointer lookup( const handle_type handle ) noexcept
		{
			return const_cast<pointer>( std::as_const( *this ).lookup( handle ) );
		}

		/// @brief Get the handle for the entry the iterator refers to
		/// @param pos The iterator to get the handle for
		/// @return The handle to the entry for the iterator, or null handle if end iterator
		[[nodiscard]] handle_type get_handle( const const_iterator pos ) const noexcept
		{
			if ( pos == end() ) [[unlikely]]
			{
				return {};
			}
			const size_type data_index = static_cast<size_type>( std::distance( cbegin(), pos ) );
			return get_valid_handle_at( data_index );
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
			return size() == 0;
		}

		/// @brief Get the number of slots, guaranteed >= size()
		[[nodiscard]] size_type slot_count() const noexcept
		{
			return static_cast<size_type>( m_slot_indirection.size() );
		}

		/// @brief Get the maximum number of objects
		[[nodiscard]] size_type max_size() const noexcept
		{
			return m_data.max_size();
		}

		/// @brief Get the allocator used
		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return allocator_type( m_data.get_allocator() );
		}

		[[nodiscard]] reference front() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Should not be empty" );
			return *data();
		}
		[[nodiscard]] reference back() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Should not be empty" );
			return data()[ size() - 1 ];
		}

		[[nodiscard]] const_reference front() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Should not be empty" );
			return *data();
		}
		[[nodiscard]] const_reference back() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Should not be empty" );
			return data()[ size() - 1 ];
		}

		[[nodiscard]] pointer data() noexcept
		{
			return m_data.values();
		}
		[[nodiscard]] const_pointer data() const noexcept
		{
			return m_data.values();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return data();
		}
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return data();
		}
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return data();
		}

		[[nodiscard]] iterator end() noexcept
		{
			return data() + size();
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return data() + size();
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return data() + size();
		}

		[[nodiscard]] reverse_iterator rbegin() noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] const_reverse_iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] const_reverse_iterator crbegin() const noexcept
		{
			return std::make_reverse_iterator( cend() );
		}

		[[nodiscard]] reverse_iterator rend() noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] const_reverse_iterator rend() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] const_reverse_iterator crend() const noexcept
		{
			return std::make_reverse_iterator( cbegin() );
		}

		/// @brief Swap this with Other
		/// @param Other The target of the swap
		void swap( dense_slot_map& other ) noexcept
		{
			using std::swap;
			swap( m_data, other.m_data );
			swap( m_slot_indirection, other.m_slot_indirection );
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
		[[nodiscard]] emplace_result emplace_and_get_with_guard( Args&&... arguments )
		{
			const size_type slot_index = m_free_list_head;

			// Insert our data, if this throws we've no clean up needed so we do it first since it is most possible to
			// throw due to user constructors
			m_data.emplace_back( slot_index, std::forward<Args>( arguments )... );

			GuardType guard( m_data );

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

			DEBUG_ASSERT( new_num_slots == slot_count(), "New num slots should equal slot_count" );

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
			const size_type data_index = m_data.size() - 1;
			handle.index = data_index;

			// We're safe now to return without cleanup
			guard.release();

			// We return the original slot index and the generation, generation will be changed in erasure for
			// invalidating existing handles
			return {
				m_data.values()[ data_index ], {slot_index, handle.generation}
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
				const pointer values = m_data.values();
				size_type* const data_reverse_map = m_data.data_reverse_map();

				// Overwrite our object with tail this maintains contiguous array of data
				values[ data_index ] = std::move( values[ data_last_index ] );
				data_reverse_map[ data_index ] = std::move( data_reverse_map[ data_last_index ] );

				// Use data we overwrote with to find and update its indirection array link with the data for our new
				// location
				m_slot_indirection[ data_reverse_map[ data_index ] ].index = data_index;
			}

			// We pop the tail as that is either us directly or what we moved from to overwrite ourselves
			m_data.pop_back();

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

		/// @brief Get the handle for the entry the iterator refers to
		/// @param pos The iterator to get the handle for
		/// @return The handle to the entry for the iterator, or null handle if end iterator
		[[nodiscard]] handle_type get_valid_handle_at( const size_type data_index ) const noexcept
		{
			const size_type indirection_index = m_data.data_reverse_map()[ data_index ];
			const size_type generation = m_slot_indirection[ indirection_index ].generation;
			return handle_type{ indirection_index, generation };
		}

		using alloc_traits = std::allocator_traits<allocator_type>;

		/// @brief Underlying container containing the data and reverse map
		underlying_container m_data;

		/// @brief Indirection array, lookup from handle.index -> m_data
		/// @details Handles returned by the API provide indexes into this array, which is stable, to then index into
		/// the actual data.
		/// The handle's indexes are also an in built free list, if an entry is in the free list its index points to the
		/// next entry in this array that is part of the free list.
		/// If the index is itself then it is the end of the free list.
		std::vector<handle_type, typename alloc_traits::template rebind_alloc<handle_type>> m_slot_indirection;

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

namespace std
{
	template <typename T,
			  std::size_t HandleTotalBits,
			  std::size_t GenerationBits,
			  typename Allocator,
			  typename Predicate>
	auto erase_if( mclo::dense_slot_map<T, HandleTotalBits, GenerationBits, Allocator>& map, Predicate pred )
	{
		auto first = map.begin();
		const auto old_size = map.size();
		while ( first != map.end() )
		{
			if ( pred( *first ) )
			{
				first = map.erase( first );
			}
			else
			{
				++first;
			}
		}
		return old_size - map.size();
	}
}
