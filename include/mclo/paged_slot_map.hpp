#pragma once

#include "mclo/platform.hpp"
#include "mclo/slot_map_handle.hpp"

#include <cinttypes>
#include <cstddef>
#include <new>
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
	/// This paged slot map is designed for faster look up speed at the cost of slower iteration, all the objects are
	/// stored in pages of memory and never moved to provide stable locations for the lookup. This causes iteration to
	/// no longer be contiguous.
	///
	/// Pros:
	/// * Lookup is only one indirection so faster than contiguous
	/// * Uses a jumping skip field to retain as good as possible iteration times
	/// * All operations perform in a constant time
	///
	/// Cons:
	/// * Memory is not contiguous so it is not as fast for pure iteration speed
	/// * Weak reference mechanics add overhead compared to similar containers like plf::colony which perform in a
	/// similar style but faster
	///
	/// @tparam T The type of objects to store
	/// @tparam PageSize The amount of entries to allocate per page of data
	/// @tparam HandleTotalBits Total number of bits for the handle type to use, smallest integer type to fit these bits
	/// will be the handle representation type, defaults to a std::uint32_t. More bits = larger max size & generation
	/// but larget handle.
	/// @tparam GenerationBits Number of the totaly bits in the handle representation type used for generation checking,
	/// defaults to 1/4 of the total bits. More bits = lower max size but higher generation before wrapping
	/// @tparam Allocator The allocator used by the backing memory
	template <typename T,
			  std::uint16_t PageSize = 1024,
			  std::size_t HandleTotalBits = sizeof( std::uint32_t ) * CHAR_BIT,
			  std::size_t GenerationBits = HandleTotalBits / 4,
			  typename Allocator = std::allocator<T>>
	class paged_slot_map
	{
	public:
		using handle_type = slot_map_handle<T, HandleTotalBits, GenerationBits>;
		using skipfield_type = mclo::uint_least_t<std::bit_width( PageSize )>;
		using value_type = T;
		using allocator_type = Allocator;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using difference_type = std::ptrdiff_t;
		// We can't have more items than our index represents anyway
		using size_type = typename handle_type::representation_type;
		// using reverse_iterator = typename underlying_container::reverse_iterator;
		// using const_reverse_iterator = typename underlying_container::const_reverse_iterator;

		static_assert( mclo::is_pow2( PageSize ), "PageSize must be a power of 2" );

	private:
		struct slot_entry
		{
			template <typename... Args>
			reference construct( allocator_type& alloc, Args&&... args ) noexcept( std::is_nothrow_constructible_v<value_type, Args...> )
			{
				const pointer ptr = reinterpret_cast<pointer>( &m_storage );
				alloc_traits::construct( alloc, ptr, std::forward<Args>( args )... );
				return *std::launder( ptr );
			}

			void destroy( allocator_type& alloc ) noexcept
			{
				alloc_traits::destroy( alloc, typed() );
			}

			pointer typed() noexcept
			{
				return std::launder( reinterpret_cast<pointer>( &m_storage ) );
			}
			const_pointer typed() const noexcept
			{
				return std::launder( reinterpret_cast<const_pointer>( &m_storage ) );
			}

			/// @brief Storage for the object
			alignas( value_type ) std::byte m_storage[ sizeof( value_type ) ];

			/// @brief Handle for the object, if in free list index is to the start of the next skip block
			handle_type m_handle{};

			/// @brief If in the free list index to the start of the previous skip block
			skipfield_type m_free_list_prev = 0;
		};

		struct slot_page
		{
			explicit slot_page(const size_type page_index) noexcept
				: m_page_index( page_index )
			{
			}

			void reset()
			{
				m_skipfield.fill( 0 );
				m_next_page = nullptr;
				m_prev_page = nullptr;
				m_next_page_with_free_list_slots = nullptr;
				m_prev_page_with_free_list_slots = nullptr;
				m_size = 0;
				m_free_list_head = PageSize;
				m_free_list_tail = PageSize;
			}

			std::array<skipfield_type, PageSize + 1> m_skipfield{};
			std::array<slot_entry, PageSize> m_slots;

			/// @brief What index this page is in the vector of pages
			const size_type m_page_index = 0;

			/// @brief The number of active objects on this page
			size_type m_size = 0;

			/// @brief Pointer to the next page which has slots in its page local free list
			slot_page* m_next_page_with_free_list_slots = nullptr;

			/// @brief Pointer to the previous page which has slots in its page local free list
			slot_page* m_prev_page_with_free_list_slots = nullptr;

			/// @brief Pointer to the next page, either the next page with elements or if in the free list of pages the next page in the free list
			slot_page* m_next_page = nullptr;

			/// @brief Pointer to the previous page, either the previous page with elements or if in the free list of pages the previous page in the free list
			slot_page* m_prev_page = nullptr;

			/// @brief Index to the first slot on this page which is in the free list
			skipfield_type m_free_list_head = PageSize;

			/// @brief Index to the last slot on this page which is in the free list
			skipfield_type m_free_list_tail = PageSize;
		};

		static constexpr size_type index_divisor = mclo::log2_ceil( PageSize );

		struct slot_entry_index
		{
			size_type page_index;
			size_type entry_index;
		};

		slot_entry_index index_to_slot( const size_type index ) const noexcept
		{
			const size_type page_index = mclo::divide_pow2( index, index_divisor );
			const size_type entry_index = mclo::modulo_pow2( index, static_cast<size_type>( PageSize ) );
			return { page_index, entry_index };
		}

		const slot_entry& get_slot( const slot_entry_index index ) const noexcept
		{
			return m_pages[ index.page_index ]->m_slots[ index.entry_index ];
		}

		const slot_entry& get_slot( const size_type index ) const noexcept
		{
			return get_slot( index_to_slot( index ) );
		}

	public:
		paged_slot_map() noexcept( noexcept( Allocator() ) ) = default;

		explicit paged_slot_map( const Allocator& allocator ) noexcept
			: m_pages( allocator )
			, m_object_allocator( allocator )
			, m_page_allocator( allocator )
		{
		}

		explicit paged_slot_map( const size_type /*slot_count*/, const Allocator& allocator = Allocator() )
			: paged_slot_map( allocator )
		{
			// reserve_slots( slot_count );
		}

		~paged_slot_map()
		{
			delete_all();
		}

		/// @brief Result of creating an object containing a reference to it and its handle
		struct emplace_result
		{
			/// @brief Reference to the created object itself
			reference object;

			/// @brief handle to the created object
			handle_type handle;
		};

		template <bool IsConst>
		class iterator_base
		{
		public:
			friend class paged_slot_map;

			using value_type = typename paged_slot_map::value_type;
			using difference_type = typename paged_slot_map::difference_type;
			using pointer =
				std::conditional_t<IsConst, typename paged_slot_map::const_pointer, typename paged_slot_map::pointer>;
			using reference = std::
				conditional_t<IsConst, typename paged_slot_map::const_reference, typename paged_slot_map::reference>;
			using iterator_category = std::bidirectional_iterator_tag;
			using iterator_concept = std::bidirectional_iterator_tag;

			iterator_base() noexcept = default;

			explicit iterator_base( slot_page& page ) noexcept
				: iterator_base( &page, page.m_slots.data(), page.m_skipfield.data() )
			{
			}

			iterator_base( const iterator_base& other ) noexcept = default;

			// Can copy from non-const if this is const
			iterator_base( const iterator_base<false>& other ) noexcept
				requires( IsConst )
				: iterator_base( other.m_current_page, other.m_entry_in_page, other.m_skipfield_in_page )
			{
			}

			iterator_base& operator=( const iterator_base& other ) noexcept = default;

			// Can copy from non-const if this is const
			iterator_base& operator=( const iterator_base<false>& other ) noexcept
				requires( IsConst )
			{
				m_current_page = other.m_current_page;
				m_entry_in_page = other.m_entry_in_page;
				m_skipfield_in_page = other.m_skipfield_in_page;
				return *this;
			}

			iterator_base& operator++() noexcept
			{
				assert( m_current_page );
				skipfield_type skip = *( ++m_skipfield_in_page );
				m_entry_in_page += skip + 1u;

				// At end, go to next page
				if ( m_entry_in_page == m_current_page->m_slots.data() + PageSize && m_current_page->m_next_page )
				{
					m_current_page = m_current_page->m_next_page;
					slot_entry* const elements = m_current_page->m_slots.data();
					const skipfield_type* const skipfield = m_current_page->m_skipfield.data();
					skip = *skipfield;
					m_entry_in_page = elements + skip;
					m_skipfield_in_page = skipfield;
				}

				m_skipfield_in_page += skip;
				return *this;
			}

			iterator_base operator++( int ) noexcept
			{
				iterator_base copy( *this );
				++*this;
				return copy;
			}

			iterator_base& operator--() noexcept
			{
				assert( m_current_page );
				if ( --m_skipfield_in_page >= m_current_page->m_skipfield.data() ) // Not at beginning
				{
					m_entry_in_page -= *m_skipfield_in_page + 1u;
					m_skipfield_in_page -= *m_skipfield_in_page;

					// Still not beginning
					if ( m_skipfield_in_page >= m_current_page->m_skipfield.data() )
					{
						return *this;
					}
				}

				m_current_page = m_current_page->previous_group;
				const skipfield_type* const skipfield = m_current_page->m_skipfield.data() + PageSize - 1;
				const skipfield_type skip = *skipfield;
				m_entry_in_page = ( m_current_page->m_slots.data() + PageSize - 1 ) - skip;
				m_skipfield_in_page = skipfield - skip;
				return *this;
			}

			iterator_base operator--( int ) noexcept
			{
				iterator_base copy( *this );
				--*this;
				return copy;
			}

			[[nodiscard]] auto operator<=>( const iterator_base& other ) const noexcept = default;

			reference operator*() const
			{
				return *m_entry_in_page->typed();
			}

			pointer operator->() const
			{
				return m_entry_in_page->typed();
			}

		private:
			iterator_base( slot_page* page, slot_entry* entries, const skipfield_type* const skipfield ) noexcept
				: m_current_page( page )
				, m_entry_in_page( entries )
				, m_skipfield_in_page( skipfield )
			{
			}

			slot_page* m_current_page = nullptr;
			slot_entry* m_entry_in_page = nullptr;
			const skipfield_type* m_skipfield_in_page = nullptr;
		};

		using iterator = iterator_base<false>;
		using const_iterator = iterator_base<true>;

		/// @brief Emplace an entry into the slot map constructed from arguments
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @tparam ...Args Type of arguments to construct from
		/// @param ...arguments Arguments to construct from
		/// @return Result containing a reference to the created entry and the handle to it
		template <typename... Args>
		[[nodiscard]] emplace_result emplace_and_get( Args... arguments )
		{
			if ( m_num_objects == max_size() ) [[unlikely]]
			{
				throw_too_big();
			}
			
			if ( m_pages.empty() ) [[unlikely]] // Setup first page
			{
				assert( empty() );
				slot_page& page = add_page();
				m_head = m_tail = iterator( page );
			}
			else if ( slot_page& tail_page = *m_tail.m_current_page; tail_page.m_size == PageSize ) // Last page is full
			{
				slot_page& page = add_page();
				assert( !tail_page.m_next_page );
				tail_page.m_next_page = &page;
				page.m_prev_page = &tail_page;
				m_tail = iterator( page );
			}

			// Insert into tail page, which may be a new allocated page from above

			slot_page& tail_page = *m_tail.m_current_page;
			assert( tail_page.m_size < PageSize );

			// Page has no free list, just insert at end
			if ( tail_page.m_free_list_head == PageSize )
			{
				slot_entry& entry = *m_tail.m_entry_in_page;
				reference object = entry.construct( m_object_allocator, std::forward<Args>( arguments )... );

				// Didn't throw, increment values
				++tail_page.m_size;
				++m_tail.m_entry_in_page;
				++m_tail.m_skipfield_in_page;
				++m_num_objects;

				return { object, entry.m_handle };
			}

			const size_type entry_index = tail_page.m_free_list_head;
			slot_entry& entry = tail_page.m_slots[ entry_index ];

			reference object = entry.construct( m_object_allocator, std::forward<Args>( arguments )... );

			/// Didn't throw, update container meta data

			// Update skip frield
			const size_type free_list_next = entry.m_handle.index;
			skipfield_type* const entry_skip = &tail_page.m_skipfield[ entry_index ];
			const skipfield_type new_skip_value = *entry_skip - 1;
			
			if ( new_skip_value == 0 ) // Single node skipblock, remove skipblock, update free list
			{
				// May be empty free list if next is PageSize
				tail_page.m_free_list_head = static_cast<skipfield_type>(free_list_next); 
			}
			else // Start of multi node skipblock
			{
				entry_skip[ new_skip_value ] = *( entry_skip + 1 ) = new_skip_value;
				++tail_page.m_free_list_head;
			}

			*entry_skip = 0;

			// Update size counts
			++tail_page.m_size;
			++m_num_objects;

			// Update tail iterator
			if ( &entry + 1 > m_tail.m_entry_in_page )
			{
				m_tail.m_entry_in_page = &entry + 1;
			}
			if ( entry_skip + 1 > m_tail.m_skipfield_in_page )
			{
				m_tail.m_skipfield_in_page = entry_skip + 1;
			}

			// New index for handle
			entry.m_handle.index = tail_page.m_page_index * PageSize + entry_index;
			return { object, entry.m_handle };
		}

		/// @brief Emplace an entry into the slot map constructed from arguments
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @tparam ...Args Type of arguments to construct from
		/// @param ...arguments Arguments to construct from
		/// @return handle to the created entry
		template <typename... Args>
		[[nodiscard]] handle_type emplace( Args... arguments )
		{
			return emplace_and_get( std::forward<Args>( arguments )... ).handle;
		}

		/// @brief Insert a copy into the slot map
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @param object Reference to the object to copy into the slot map
		/// @return handle to the created entry
		[[nodiscard]] handle_type insert( const_reference object )
		{
			return emplace( object );
		}

		/// @brief Insert a move into the slot map
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @param object Reference to the object to move into the slot map
		/// @return handle to the created entry
		[[nodiscard]] handle_type insert( value_type&& object )
		{
			return emplace( std::move( object ) );
		}

		/// @brief Reserve capacity for elements and old_num_slots in the slot map
		/// @post Provides a basic exception guarantee, if an exception is thrown invariants are held and no leaks occur
		/// @param amount Number of elements to reserve size for
		//void reserve( const size_type amount )
		//{
		//	const size_type num_pages = mclo::ceil_divide( amount, PageSize );
		//	reserve_slots( amount );
		//	m_data.reserve( amount );
		//	m_data_reverse_map.reserve( amount );
		//}

		/// @brief Erase an entry in the slot map from its handle
		/// @param handle The handle to the object to erase
		void erase( handle_type handle )
		{
			const size_type index = handle.index;
			if ( index >= size() )
			{
				return;
			}

			const auto [ page_index, slot_index ] = index_to_slot( index );

			slot_page& page = *m_pages[ page_index ];
			slot_entry& slot = page.m_slots[ slot_index ];

			if ( slot.m_handle.generation != handle.generation )
			{
				return;
			}

			// todo is this actually true?
			assert( slot.m_handle.index == index );

			slot.destroy( m_object_allocator );

			++slot.m_handle.generation;
			
			--m_num_objects;
			--page.m_size;

			// Page empty, add to free list of pages
			if ( page.m_size == 0 )
			{
				const bool is_first_page = m_head.m_current_page == &page;
				const bool is_last_page = m_tail.m_current_page == &page;

				// Is the only page, just reset the page instead of juggling free list
				if ( is_first_page && is_last_page )
				{
					page.reset();
					m_head = m_tail = iterator( page );
					return;
				}

				if ( is_first_page && !is_last_page )
				{
					// Set new head page
					slot_page& new_head_page = *page.m_next_page;
					m_head.m_current_page = &new_head_page;
					new_head_page.m_prev_page = nullptr;
					const skipfield_type skip = new_head_page.m_skipfield.front();
					m_head.m_entry_in_page = new_head_page.m_slots.data() + skip;
					m_head.m_skipfield_in_page = new_head_page.m_skipfield.data() + skip;

					// Put old head into free list
					add_free_page( page );
					return;
				}

				if ( !is_first_page && is_last_page )
				{
					// Set new tail page
					slot_page& new_tail_page = *page.m_prev_page;
					m_tail.m_current_page = &new_tail_page;
					new_tail_page.m_next_page = nullptr;
					m_tail.m_entry_in_page = new_tail_page.m_slots.data() + PageSize;
					m_tail.m_skipfield_in_page = new_tail_page.m_skipfield.data() + PageSize;

					// Put old tail into free list
					add_free_page( page );
					return;
				}

				// Extract from page chain
				page.m_next_page->m_prev_page = page.m_prev_page;
				page.m_prev_page->m_next_page = page.m_next_page;

				// Put into free list
				add_free_page( page );
				return;
			}
			
			skipfield_type* const skip = &page.m_skipfield[ slot_index ];

			// Page has elements still, update skipfield and add to page's slot free list
			const bool prev_is_skipfield = *( skip - ( slot_index != 0 ) ) != 0;
			const bool next_is_skipfield = *( skip + 1 ) != 0;

			if ( !prev_is_skipfield && !next_is_skipfield ) // Single node skip block
			{
				*skip = 1;
				if ( page.m_free_list_head == PageSize ) // No free list, start it
				{
					page.m_free_list_head = static_cast<skipfield_type>( slot_index );
				}
				else // Has free list, chain to existing tail
				{
					page.m_slots[ page.m_free_list_tail ].m_handle.index = slot_index;
					slot.m_free_list_prev = page.m_free_list_tail;
				}
				page.m_free_list_tail = static_cast<skipfield_type>( slot_index );
			}
			else if ( prev_is_skipfield && !next_is_skipfield ) // Join to last skip block as new end
			{
				const skipfield_type left_skip_value = *( skip - 1 );
				const skipfield_type left_node_index = static_cast<skipfield_type>( slot_index - left_skip_value );
				page.m_skipfield[ left_node_index ] = *skip = left_skip_value + 1;
			}
			else if ( !prev_is_skipfield && next_is_skipfield ) // Join to right block as new start
			{
				const skipfield_type right_skip_value = *( skip + 1 );
				const skipfield_type right_node_index = static_cast<skipfield_type>( slot_index + right_skip_value );
				page.m_skipfield[ right_node_index ] = *skip = right_skip_value + 1;

				const slot_entry& old_skip_start = *( &slot + 1 );
				slot.m_free_list_prev = old_skip_start.m_free_list_prev;
				page.m_slots[ slot.m_free_list_prev ].m_handle.index = slot_index;
				slot.m_handle.index = old_skip_start.m_handle.index;

				if ( page.m_free_list_head == slot_index + 1 )
				{
					--page.m_free_list_head;
				}
			}
			else // Join adjacent skip blocks
			{
				assert( prev_is_skipfield && next_is_skipfield );
				*skip = 1;
				const skipfield_type left_skip_value = *( skip - 1 );
				const skipfield_type right_skip_value = *( skip + 1 );

				const skipfield_type left_node_index = static_cast<skipfield_type>(slot_index - left_skip_value);
				const skipfield_type right_node_index = static_cast<skipfield_type>(slot_index + right_skip_value);
				page.m_skipfield[ left_node_index ] = page.m_skipfield[ right_node_index ] =
					left_skip_value + right_skip_value + 1;

				const slot_entry& old_right_skip_start = *( &slot + 1 );
				slot_entry& skip_start = page.m_slots[ left_node_index ];
				slot_entry& next_skip = page.m_slots[ old_right_skip_start.m_handle.index ];
				skip_start.m_handle.index = old_right_skip_start.m_handle.index;
				next_skip.m_free_list_prev = left_node_index;

				if ( page.m_free_list_head == slot_index + 1 )
				{
					page.m_free_list_head = left_node_index;
				}
			}
		}

		/// @brief Check if a handle is a valid entry into the slot map
		/// @param handle The handle to check
		/// @return If the handle refers to a valid entry in the slot map
		[[nodiscard]] bool is_valid( const handle_type handle ) const noexcept
		{
			const size_type index = handle.index;
			if ( index >= size() )
			{
				return false;
			}

			const slot_entry& slot = get_slot( index );
			return slot.m_handle.generation == handle.generation;
		}

		/// @brief Lookup an entry in the slot map
		/// @param handle The handle to lookup
		/// @return Const pointer to the object the handle refers to, or nullptr if an invalid handle
		[[nodiscard]] const_pointer lookup( const handle_type handle ) const noexcept
		{
			const size_type index = handle.index;
			if ( index >= size() )
			{
				return nullptr;
			}

			const slot_entry& slot = get_slot( index );
			if ( slot.m_handle.generation != handle.generation )
			{
				return nullptr;
			}

			return slot.typed();
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
			return m_num_objects;
		}

		/// @brief Get the total capacity of the objects
		[[nodiscard]] size_type capacity() const noexcept
		{
			return static_cast<size_type>( m_pages.size() * PageSize );
		}

		/// @brief Is the slot map empty
		[[nodiscard]] bool empty() const noexcept
		{
			return m_num_objects == 0;
		}

		/// @brief Get the number of pages
		[[nodiscard]] size_type page_count() const noexcept
		{
			return static_cast<size_type>( m_pages.size() );
		}

		/// @brief Get the maximum number of objects
		[[nodiscard]] size_type max_size() const noexcept
		{
			return handle_type::max_index + 1;
		}

		/// @brief Get the allocator used
		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return m_object_allocator;
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return m_head;
		}
		[[nodiscard]] iterator end() noexcept
		{
			return m_tail;
		}
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return m_head;
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return m_tail;
		}
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return m_head;
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return m_tail;
		}

		// todo[mc] implement
		using reverse_iterator = iterator;
		using const_reverse_iterator = const_iterator;
		[[nodiscard]] reverse_iterator rbegin() noexcept
		{
			return begin();
		}
		[[nodiscard]] reverse_iterator rend() noexcept
		{
			return end();
		}
		[[nodiscard]] const_reverse_iterator rbegin() const noexcept
		{
			return begin();
		}
		[[nodiscard]] const_reverse_iterator rend() const noexcept
		{
			return end();
		}
		[[nodiscard]] const_reverse_iterator crbegin() const noexcept
		{
			return cbegin();
		}
		[[nodiscard]] const_reverse_iterator crend() const noexcept
		{
			return cend();
		}

		/// @brief Swap this with Other
		/// @param Other The target of the swap
		void swap( paged_slot_map& other ) noexcept
		{
			using std::swap;
			swap( m_pages, other.m_pages );
			swap( m_head, other.m_head );
			swap( m_tail, other.m_tail );
			swap( m_page_free_list_start, other.m_page_free_list_start );
			swap( m_page_free_list_end, other.m_page_free_list_end );
			swap( m_num_objects, other.m_num_objects );
			swap( m_object_allocator, other.m_object_allocator );
			swap( m_page_allocator, other.m_page_allocator );
		}

		friend void swap( paged_slot_map& lhs, paged_slot_map& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		using alloc_traits = std::allocator_traits<allocator_type>;
		using page_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<slot_page>;
		
		[[noreturn]] void throw_too_big() const
		{
			throw std::length_error( "Slot map too large for maximum handle index" );
		}

		slot_page& add_page()
		{
			if ( !m_page_free_list_start )
			{
				slot_page* ptr = m_page_allocator.allocate( 1 );
				try
				{
					std::allocator_traits<page_allocator>::construct( m_page_allocator, ptr, page_count() );
				}
				catch ( ... )
				{
					m_page_allocator.deallocate( ptr, 1 );
				}
				return *m_pages.emplace_back( ptr );
			}

			slot_page& page = *m_page_free_list_start;
			m_page_free_list_start = page.m_next_page;
			page.reset();
			return page;
		}

		void add_free_page( slot_page& page ) noexcept
		{
			// Chain with existing end
			if ( m_page_free_list_end )
			{
				m_page_free_list_end->m_next_page = &page;
				page.m_prev_page = m_page_free_list_end;
			}

			// Make the new end
			page.m_next_page = nullptr;
			m_page_free_list_end = &page;
		}

		void delete_all() noexcept
		{
			if ( m_pages.empty() )
			{
				return;
			}

			if constexpr ( !std::is_trivially_destructible_v<value_type> )
			{
				for ( ; m_head != m_tail; ++m_head )
				{
					m_head.m_entry_in_page->destroy( m_object_allocator );
				}
			}

			for ( slot_page* page : m_pages )
			{
				std::allocator_traits<page_allocator>::destroy( m_page_allocator, page );
				m_page_allocator.deallocate( page, 1 );
			}

			m_pages.clear();
		}

		/// @brief Vector of pages, slots never move away from their page once placed there
		/// @details Not moving provides worse iteration time since it is no longer contiguous memory but provides
		/// lookup with only one indirection. To offset the lack of contiguous memory the pages use a skipfield to
		/// maintain constant iteration time.
		std::vector<slot_page*, typename alloc_traits::template rebind_alloc<slot_page*>> m_pages;

		/// @brief Iterator to the head page
		iterator m_head;

		/// @brief Iterator to the tail page
		iterator m_tail;

		/// @brief Pointer to the first page in the free list of pages
		/// @details If nullptr the free list is empty
		slot_page* m_page_free_list_start = nullptr;

		/// @brief Pointer to the last page in the free list of pages
		slot_page* m_page_free_list_end = nullptr;

		/// @brief Pointer to the first page which has slots in a free list
		slot_page* m_page_with_free_list_slots_start = nullptr;

		/// @brief Pointer to the last page which has slots in a free list
		slot_page* m_page_with_free_list_slots_start = nullptr;

		/// @brief Number of active objects
		size_type m_num_objects = 0;

		/// @brief Allocator for the slot objects
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_object_allocator;

		/// @brief Allocator for the slot pages
		MCLO_NO_UNIQUE_ADDRESS page_allocator m_page_allocator;
	};
}
