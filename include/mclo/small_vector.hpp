#pragma once

#include "mclo/platform.hpp"

#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mclo
{
	namespace detail
	{
		/// @brief Type independent code for the vector
		class small_vector_header
		{
		private:
			static constexpr std::size_t max_size_v = std::numeric_limits<std::size_t>::max();

		public:
			small_vector_header( std::byte* const data, std::size_t capacity ) noexcept
				: m_data( data )
				, m_capacity( capacity )
			{
			}

			[[nodiscard]] std::size_t size() const noexcept
			{
				return m_size;
			}
			[[nodiscard]] std::size_t capacity() const noexcept
			{
				return m_capacity;
			}

			[[nodiscard]] bool empty() const noexcept
			{
				return m_size == 0;
			}

			[[nodiscard]] std::size_t max_size() const noexcept
			{
				return max_size_v;
			}

		protected:
			static constexpr float growth_factor = 1.5f;

			std::byte* m_data = nullptr;
			std::size_t m_size = 0;
			std::size_t m_capacity = 0;
		};

		/// @brief Size helper mimics a small_vector size 1 to find the variable offset
		template <typename T>
		struct small_vector_offset_helper
		{
			small_vector_header m_header;
			alignas( T ) std::byte m_data[ sizeof( T ) ];
		};

		struct value_initialize_tag
		{
		};

		// https://en.cppreference.com/w/cpp/concepts/boolean-testable
		template <typename T>
		concept boolean_testable_impl = std::convertible_to<T, bool>;

		template <typename T>
		concept boolean_testable = boolean_testable_impl<T> && requires( T&& t ) {
			{
				!static_cast<T&&>( t )
			} -> boolean_testable_impl;
		};

		// https://en.cppreference.com/w/cpp/standard_library/synth-three-way
		struct synth_three_way
		{
			template <class T, class U>
			[[nodiscard]] constexpr auto operator()( const T& lhs, const U& rhs ) const
				requires requires {
					{
						lhs < rhs
					} -> boolean_testable;
					{
						rhs < lhs
					} -> boolean_testable;
				}
			{
				if constexpr ( std::three_way_comparable_with<T, U> )
				{
					return lhs <=> rhs;
				}
				else
				{
					if ( lhs < rhs )
					{
						return std::weak_ordering::less;
					}
					else if ( rhs < lhs )
					{
						return std::weak_ordering::greater;
					}
					else
					{
						return std::weak_ordering::equivalent;
					}
				}
			}
		};
	}

	/// @brief small_vector size independent code, mimics the API of std::vector
	/// @details Can not be constructed directly should only be used as a reference type in functions
	/// which should operate on any sized small_vector of a given type.
	/// @tparam T Type of objects stored
	template <typename T>
	class small_vector_base : private detail::small_vector_header
	{
		using base = detail::small_vector_header;

	public:
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		// Cannot know the derived internal capacity
		small_vector_base( const small_vector_base& other ) = delete;
		small_vector_base( small_vector_base&& other ) noexcept = delete;

		small_vector_base& operator=( const small_vector_base& other )
		{
			if ( this != &other )
			{
				assign( other.begin(), other.end() );
			}
			return *this;
		}

		small_vector_base& operator=( small_vector_base&& other ) noexcept
		{
			if ( this == &other )
			{
				return *this;
			}

			if ( other.is_internal() )
			{
				assign( std::make_move_iterator( other.begin() ), std::make_move_iterator( other.end() ) );
				other.clear();
			}
			else
			{
				std::destroy( begin(), end() );
				m_data = std::exchange( other.m_data, nullptr );
				m_size = std::exchange( other.m_size, 0 );
				m_capacity = std::exchange( other.m_capacity, 0 );
			}

			return *this;
		}

		small_vector_base& operator=( std::initializer_list<value_type> init_list )
		{
			assign( init_list );
			return *this;
		}

		void swap( small_vector_base& other )
		{
			if ( this == &other )
			{
				return;
			}

			if ( !is_internal() && !other.is_internal() )
			{
				std::swap( m_data, other.m_data );
				std::swap( m_size, other.m_size );
				std::swap( m_capacity, other.m_capacity );
				return;
			}

			reserve( other.m_size );
			other.reserve( m_size );

			const std::strong_ordering this_size_ordering = m_size <=> other.m_size;
			if ( this_size_ordering == std::strong_ordering::equal )
			{
				std::swap_ranges( begin(), end(), other.begin() );
			}
			else if ( this_size_ordering == std::strong_ordering::less )
			{
				swap_with_larger( other );
			}
			else if ( this_size_ordering == std::strong_ordering::greater )
			{
				other.swap_with_larger( *this );
			}
		}

		friend void swap( small_vector_base& lhs, small_vector_base& rhs )
		{
			lhs.swap( rhs );
		}

		using base::capacity;
		using base::empty;
		using base::max_size;
		using base::size;

		[[nodiscard]] reference operator[]( const size_type index ) noexcept
		{
			assert( index < m_size && "Index out of range" );
			return data()[ index ];
		}
		[[nodiscard]] const_reference operator[]( const size_type index ) const noexcept
		{
			assert( index < m_size && "Index out of range" );
			return data()[ index ];
		}

		[[nodiscard]] reference at( const size_type index )
		{
			if ( index >= m_size )
			{
				throw std::out_of_range( "Index out of range" );
			}
			return data()[ index ];
		}
		[[nodiscard]] const_reference at( const size_type index ) const
		{
			if ( index >= m_size )
			{
				throw std::out_of_range( "Index out of range" );
			}
			return data()[ index ];
		}

		[[nodiscard]] reference front() noexcept
		{
			assert( !empty() && "Container is empty" );
			return *data();
		}
		[[nodiscard]] const_reference front() const noexcept
		{
			assert( !empty() && "Container is empty" );
			return *data();
		}

		[[nodiscard]] reference back() noexcept
		{
			assert( !empty() && "Container is empty" );
			return data()[ m_size - 1 ];
		}
		[[nodiscard]] const_reference back() const noexcept
		{
			assert( !empty() && "Container is empty" );
			return data()[ m_size - 1 ];
		}

		[[nodiscard]] pointer data() noexcept
		{
			return std::launder( reinterpret_cast<pointer>( this->m_data ) );
		}
		[[nodiscard]] const_pointer data() const noexcept
		{
			return std::launder( reinterpret_cast<const_pointer>( this->m_data ) );
		}

		void reserve( const size_type amount )
		{
			if ( amount == max_size() ) [[unlikely]]
			{
				throw std::length_error( "Requested capacity greater than max_size" );
			}

			if ( m_capacity < amount )
			{
				reallocate( amount );
			}
		}

		void shrink_to_fit()
		{
			if ( m_size < m_capacity && !is_internal() )
			{
				reallocate( m_size );
			}
		}

		template <typename... Args>
		reference emplace_back( Args&&... args )
		{
			if ( m_size == m_capacity ) [[unlikely]]
			{
				return *emplace_reallocate( end(), std::forward<Args>( args )... );
			}
			return emplace_back_no_allocate( std::forward<Args>( args )... );
		}

		reference push_back( const_reference value )
		{
			return emplace_back( value );
		}
		reference push_back( value_type&& value )
		{
			return emplace_back( std::move( value ) );
		}

		void assign( const size_type count, const_reference value )
		{
			assign_internal(
				count,
				[ &value ]( const pointer ptr, const size_type amount ) {
					return std::uninitialized_fill_n( ptr, amount, value );
				},
				[ &value ]( const pointer ptr, const size_type amount ) { return std::fill_n( ptr, amount, value ); } );
		}

		template <std::input_iterator It>
		void assign( It first, It last )
		{
			if constexpr ( std::forward_iterator<It> )
			{
				assign_internal(
					static_cast<size_type>( std::distance( first, last ) ),
					[ &first ]( const pointer ptr, const size_type amount ) {
						return std::uninitialized_copy_n( std::move( first ), amount, ptr );
					},
					[ &first ]( const pointer ptr, const size_type amount ) {
						for ( size_type index = 0; index < amount; ++index, ++first )
						{
							ptr[ index ] = *first;
						}
						return ptr + amount;
					} );
			}
			else
			{
				iterator dest = begin();
				for ( ; first != last; ++first, ++dest )
				{
					*dest = *first;
				}
				std::destroy( dest, end() );
				m_size = static_cast<size_type>( std::distance( begin(), dest ) );
				insert( end(), std::move( first ), std::move( last ) );
			}
		}

		void assign( std::initializer_list<value_type> init_list )
		{
			assign( init_list.begin(), init_list.end() );
		}

		template <typename... Args>
		iterator emplace( const_iterator pos, Args&&... args )
		{
			assert( pos >= begin() && pos <= end() && "pos must be an iterator in this container" );

			if ( m_size == m_capacity )
			{
				return emplace_reallocate( pos, std::forward<Args>( args )... );
			}

			const iterator oldEnd = end();
			if ( pos == oldEnd )
			{
				return &emplace_back_no_allocate( std::forward<Args>( args )... );
			}

			// Cast is safe because we assert the iterator is inside this container which we are modifying
			const iterator insertPos = const_cast<iterator>( pos );

			value_type temp( std::forward<Args>( args )... );
			emplace_back_no_allocate( std::move( back() ) );
			std::move_backward( insertPos, oldEnd - 1, oldEnd );
			*insertPos = std::move( temp );

			return insertPos;
		}

		iterator insert( const_iterator pos, const_reference value )
		{
			return emplace( pos, value );
		}
		iterator insert( const_iterator pos, value_type&& value )
		{
			return emplace( pos, std::move( value ) );
		}

		iterator insert( const_iterator pos, const size_type amount, const_reference value )
		{
			assert( pos >= begin() && pos <= end() && "pos must be an iterator in this container" );

			// Cast is safe because we assert the iterator is inside this container which we are modifying
			const iterator insert_pos = const_cast<iterator>( pos );

			if ( amount == 0 )
			{
				return insert_pos;
			}

			const size_type where_pos = std::distance( begin(), insert_pos );
			const size_type old_size = m_size;
			const size_type new_size = old_size + amount;

			if ( new_size <= m_capacity )
			{ // No reallocation
				const iterator old_end = end();
				if ( amount == 1 && insert_pos == old_end )
				{
					emplace_back_no_allocate( value );
				}
				else
				{
					const size_type existing_modified = std::distance( insert_pos, old_end );

					value_type temp( value );
					if ( amount > existing_modified )
					{ // Fill unused capacity as well
						const size_type end_fill = amount - existing_modified;
						std::uninitialized_fill_n( old_end, end_fill, temp );
						m_size += end_fill;

						std::uninitialized_move( insert_pos, old_end, end() );
						std::fill( insert_pos, old_end, temp );
					}
					else
					{
						std::uninitialized_move( old_end - amount, old_end, old_end );
						m_size += amount;

						std::move_backward( insert_pos, old_end - amount, old_end );
						std::fill_n( insert_pos, amount, value );
					}
				}
			}
			else
			{
				// Reallocation needed
				resize_reallocate( new_size, value );
				std::rotate( begin() + where_pos, begin() + old_size, end() );
			}

			return begin() + where_pos;
		}

		template <std::input_iterator It>
		iterator insert( const_iterator pos, It first, It last )
		{
			const size_type where_pos = std::distance( cbegin(), pos );

			if constexpr ( std::forward_iterator<It> )
			{
				insert_sized( pos, first, static_cast<size_type>( std::distance( first, last ) ) );
			}
			else if ( first != last )
			{
				const size_type old_size = m_size;
				for ( ; first != last; ++first )
				{
					emplace_back( *first );
				}
				std::rotate( begin() + where_pos, begin() + old_size, end() );
			}

			return begin() + where_pos;
		}

		iterator insert( const_iterator pos, std::initializer_list<value_type> init_list )
		{
			const size_type where_pos = std::distance( cbegin(), pos );
			insert_sized( pos, init_list.begin(), init_list.size() );
			return begin() + where_pos;
		}

		void pop_back() noexcept
		{
			assert( !empty() && "Container is empty" );
			std::destroy_at( end() - 1 );
			--m_size;
		}

		iterator erase( const_iterator pos ) noexcept( std::is_nothrow_move_assignable_v<value_type> )
		{
			assert( pos >= begin() && pos <= end() && "pos must be an iterator in this container" );
			const iterator it = const_cast<iterator>( pos );
			std::move( it + 1, end(), it );
			--m_size;
			return it;
		}

		iterator erase( const_iterator first,
						const_iterator last ) noexcept( std::is_nothrow_move_assignable_v<value_type> )
		{
			assert( first >= begin() && first <= end() && "first must be an iterator in this container" );
			assert( last >= begin() && last <= end() && "last must be an iterator in this container" );
			assert( first <= last && "first and last must form a valid range in this container" );

			const iterator first_mut = const_cast<iterator>( first );
			const iterator last_mut = const_cast<iterator>( last );

			if ( first_mut != last_mut )
			{
				const iterator newLast = std::move( last_mut, end(), first_mut );
				std::destroy( newLast, end() );
				m_size -= std::distance( newLast, end() );
			}

			return first_mut;
		}

		void clear()
		{
			std::destroy( begin(), end() );
			m_size = 0;
		}

		void resize( const size_type amount )
		{
			resize_internal( amount, detail::value_initialize_tag{} );
		}

		void resize( const size_type amount, const_reference value )
		{
			resize_internal( amount, value );
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
			return data() + m_size;
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return data() + m_size;
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return data() + m_size;
		}

		[[nodiscard]] reverse_iterator rbegin() noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] const_reverse_iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] const_reverse_iterator crbegin() const noexcept
		{
			return std::make_reverse_iterator( cbegin() );
		}

		[[nodiscard]] reverse_iterator rend() noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] const_reverse_iterator rend() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] const_reverse_iterator crend() const noexcept
		{
			return std::make_reverse_iterator( cend() );
		}

		[[nodiscard]] friend bool operator==( const small_vector_base& lhs, const small_vector_base& rhs )
		{
			return lhs.m_size == rhs.m_size && std::equal( lhs.begin(), lhs.end(), rhs.begin() );
		}

		[[nodiscard]] friend auto operator<=>( const small_vector_base& lhs, const small_vector_base& rhs )
		{
			return std::lexicographical_compare_three_way(
				lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), detail::synth_three_way{} );
		}

	protected:
		explicit small_vector_base( std::size_t capacity ) noexcept
			: base( get_first_element(), capacity )
		{
		}

		~small_vector_base()
		{
			if ( !is_internal() )
			{
				::operator delete( static_cast<void*>( this->m_data ) );
			}
		}

	private:
		pointer allocate_uninitialized( const size_type amount )
		{
			return static_cast<pointer>( ::operator new( sizeof( value_type ) * amount ) );
		}

		void reallocate( const size_type new_capacity )
		{
			const pointer new_data = allocate_uninitialized( new_capacity );

			try
			{
				if constexpr ( std::is_nothrow_move_constructible_v<value_type> ||
							   !std::is_copy_constructible_v<value_type> )
				{
					std::uninitialized_move( begin(), end(), new_data );
				}
				else
				{
					std::uninitialized_copy( begin(), end(), new_data );
				}
			}
			catch ( ... )
			{
				::operator delete( new_data );
				throw;
			}

			set_data( new_data, new_capacity, m_size );
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

		template <typename TInsertCallback>
			requires( std::is_invocable_r_v<void, TInsertCallback, pointer> )
		iterator insert_reallocate( const_iterator pos, const size_type count, TInsertCallback insert_callback )
		{
			if ( count > max_size() - m_size ) [[unlikely]]
			{
				throw std::length_error( "New would be greater than max_size" );
			}

			const size_type new_size = m_size + count;
			const size_type new_capacity = calculate_new_capacity( new_size );

			// Cast is safe because we assert the iterator is inside this container which we are modifying
			const iterator insert_pos = const_cast<iterator>( pos );
			const size_type where_index = std::distance( begin(), insert_pos );

			const pointer new_data = allocate_uninitialized( new_capacity );
			const pointer first_new_element = new_data + where_index;
			const pointer insert_last = first_new_element + count;
			pointer insert_first = insert_last;

			try
			{
				insert_callback( first_new_element );
				insert_first = first_new_element;

				if ( insert_pos == end() )
				{
					if constexpr ( std::is_nothrow_move_constructible_v<value_type> ||
								   !std::is_copy_constructible_v<value_type> )
					{
						std::uninitialized_move( begin(), end(), new_data );
					}
					else
					{
						std::uninitialized_copy( begin(), end(), new_data );
					}
				}
				else
				{
					std::uninitialized_move( begin(), insert_pos, new_data );
					insert_first = new_data;
					std::uninitialized_move( insert_pos, end(), insert_last );
				}
			}
			catch ( ... )
			{
				std::destroy( insert_first, insert_last );
				::operator delete( new_data );
				throw;
			}

			set_data( new_data, new_capacity, new_size );
			return first_new_element;
		}

		template <typename... Args>
		iterator emplace_reallocate( const_iterator pos, Args&&... args )
		{
			assert( m_size == m_capacity && "No unused capacity" );

			return insert_reallocate( pos, 1, [... args = std::forward<Args>( args ) ]( pointer ptr ) mutable {
				std::construct_at( ptr, std::forward<Args>( args )... );
			} );
		}

		template <typename U>
		void resize_reallocate( const size_type new_size, const U& value )
		{
			const size_type amount_to_add = new_size - m_size;
			insert_reallocate( end(), amount_to_add, [ amount_to_add, &value ]( pointer ptr ) {
				if constexpr ( std::is_same_v<U, detail::value_initialize_tag> )
				{
					std::uninitialized_value_construct_n( ptr, amount_to_add );
				}
				else
				{
					std::uninitialized_fill_n( ptr, amount_to_add, value );
				}
			} );
		}

		template <typename... Args>
		reference emplace_back_no_allocate( Args&&... args )
		{
			pointer ptr = std::construct_at( end(), std::forward<Args>( args )... );
			++m_size;
			return *ptr;
		}

		template <typename U>
		void resize_internal( const size_type amount, const U& value )
		{
			const size_type old_size = m_size;

			if ( amount < old_size )
			{
				// Shrink
				std::destroy( begin() + amount, end() );
				m_size = amount;
			}
			else if ( amount > old_size )
			{
				// Grow
				if ( amount > m_capacity )
				{
					resize_reallocate( amount, value );
					return;
				}

				const size_type amount_to_add = amount - old_size;

				if constexpr ( std::is_same_v<U, detail::value_initialize_tag> )
				{
					std::uninitialized_value_construct_n( end(), amount_to_add );
				}
				else
				{
					std::uninitialized_fill_n( end(), amount_to_add, value );
				}

				m_size = amount;
			}
		}

		template <typename It>
		void insert_sized( const_iterator pos, It first, const size_type count )
		{
			if ( count == 0 )
			{
				return;
			}

			const size_type free_capacity = m_capacity - m_size;

			if ( count > free_capacity )
			{
				insert_reallocate( pos, count, [ count, first = std::move( first ) ]( pointer ptr ) {
					std::uninitialized_copy_n( std::move( first ), count, ptr );
				} );
				return;
			}

			// Cast is safe because we assert the iterator is inside this container which we are modifying
			const iterator insert_pos = const_cast<iterator>( pos );
			const iterator old_end = end();

			const size_type existing_modified = std::distance( insert_pos, old_end );

			if ( count < existing_modified )
			{ // Assign over existing elements
				std::uninitialized_move( old_end - count, old_end, old_end );
				m_size += count;

				std::move_backward( insert_pos, old_end - count, old_end );
				std::destroy( insert_pos, insert_pos + count );

				std::uninitialized_copy_n( std::move( first ), count, insert_pos );
			}
			else
			{ // No overlapping
				const iterator moved = insert_pos + count;
				std::uninitialized_move( insert_pos, old_end, moved );
				m_size += count;

				std::destroy( insert_pos, old_end );

				std::uninitialized_copy_n( std::move( first ), count, insert_pos );
			}
		}

		template <std::invocable<pointer, size_type> UninitializedFunc,
				  std::invocable<pointer, size_type> InitializedFunc>
		void assign_internal( const size_type count,
							  UninitializedFunc uninitialized_func,
							  InitializedFunc initialized_func )
		{
			if ( count > m_capacity )
			{
				const size_type new_capacity = calculate_new_capacity( count );
				const pointer new_data = allocate_uninitialized( new_capacity );
				try
				{
					uninitialized_func( new_data, count );
				}
				catch ( ... )
				{
					::operator delete( new_data );
					throw;
				}
				set_data( new_data, new_capacity, count );
				return;
			}

			if ( count > m_size )
			{
				initialized_func( begin(), m_size );
				uninitialized_func( end(), count - m_size );
				m_size = count;
				return;
			}

			const pointer new_end = initialized_func( begin(), count );
			std::destroy( new_end, end() );
			m_size = count;
		}

		void swap_with_larger( small_vector_base& larger )
		{
			assert( m_size < larger.m_size && "larger should actually be larger" );
			const size_type overlap_size = m_size;
			std::swap_ranges( begin(), begin() + overlap_size, larger.begin() );

			const auto larger_remainder_start = larger.begin() + overlap_size;
			std::uninitialized_copy( larger_remainder_start, larger.end(), end() );
			m_size = larger.m_size;

			std::destroy( larger_remainder_start, larger.end() );
			larger.m_size = overlap_size;
		}

		void set_data( const pointer new_data, const size_type new_capacity, const size_type new_size ) noexcept
		{
			std::destroy( begin(), end() );
			this->m_data = reinterpret_cast<std::byte*>( new_data );
			this->m_capacity = new_capacity;
			this->m_size = new_size;
		}

		static constexpr std::size_t offset = offsetof( detail::small_vector_offset_helper<T>, m_data );

		[[nodiscard]] std::byte* get_first_element() const noexcept
		{
			return const_cast<std::byte*>( reinterpret_cast<const std::byte*>( this ) + offset );
		}

		[[nodiscard]] bool is_internal() const noexcept
		{
			return get_first_element() == this->m_data;
		}
	};

	namespace detail
	{
		template <typename T>
		constexpr std::size_t default_inline_capacity_bytes =
			std::hardware_destructive_interference_size - sizeof( small_vector_base<T> );

		template <typename T>
			requires( sizeof( T ) <= 256 )
		constexpr std::size_t default_inline_capacity = default_inline_capacity_bytes<T> / sizeof( T );
	}

	/// @brief small_vector stores Capacity elements inline on the stack but can grow and allocate on the heap
	/// additional, mirrors std::vector API
	/// @details small_vector_base<T> provides a size erased type that provides all the same functions for code generic
	/// over size
	/// @tparam T Type of objects stored
	/// @tparam Capacity Number of elements to store inline, if not provided defaults to try and keep the overall stack
	/// size to one cache line (64 bytes)
	template <typename T, std::size_t Capacity = detail::default_inline_capacity<T>>
	class small_vector : public small_vector_base<T>
	{
		using base = small_vector_base<T>;

	public:
		static_assert( Capacity > 0, "Inline capacity must be larger than zero" );

		using typename base::const_iterator;
		using typename base::const_pointer;
		using typename base::const_reference;
		using typename base::const_reverse_iterator;
		using typename base::difference_type;
		using typename base::iterator;
		using typename base::pointer;
		using typename base::reference;
		using typename base::reverse_iterator;
		using typename base::size_type;
		using typename base::value_type;

		MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( 26495 )
		small_vector() noexcept
			: base( Capacity )
		{
		}
		MCLO_MSVC_POP_WARNINGS

		small_vector( const small_vector& other )
			: small_vector()
		{
			base::operator=( other );
		}

		small_vector( small_vector&& other ) noexcept
			: small_vector()
		{
			base::operator=( std::move( other ) );
		}

		small_vector& operator=( const small_vector& other )
		{
			base::operator=( other );
			return *this;
		}

		small_vector& operator=( small_vector&& other ) noexcept
		{
			base::operator=( std::move( other ) );
			return *this;
		}

		explicit small_vector( const size_type count )
			: small_vector()
		{
			base::resize( count );
		}

		small_vector( const size_type count, const_reference value )
			: small_vector()
		{
			base::resize( count, value );
		}

		template <std::input_iterator It>
		small_vector( It first, It last )
			: small_vector()
		{
			base::assign( first, last );
		}

		small_vector( std::initializer_list<value_type> init_list )
			: small_vector()
		{
			base::assign( init_list );
		}

		~small_vector()
		{
			std::destroy( base::begin(), base::end() );
		}

	private:
		alignas( T ) std::byte m_buffer[ sizeof( T ) * Capacity ];
	};
}