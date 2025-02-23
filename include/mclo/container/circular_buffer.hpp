#pragma once

#include "mclo/container/span.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <compare>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>

namespace mclo
{
	template <typename Buffer, bool IsConst>
	class circular_buffer_iterator
	{
		template <typename OtherBuffer, bool OtherIsConst>
		friend class circular_buffer_iterator;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using iterator_concept = std::random_access_iterator_tag;
		using difference_type = typename Buffer::difference_type;
		using value_type = typename Buffer::value_type;
		using reference = std::conditional_t<IsConst, typename Buffer::const_reference, typename Buffer::reference>;
		using pointer = std::conditional_t<IsConst, typename Buffer::const_pointer, typename Buffer::pointer>;

		circular_buffer_iterator() noexcept = default;

		template <bool OtherConst>
		circular_buffer_iterator( const circular_buffer_iterator<Buffer, OtherConst>& other ) noexcept
			: m_buffer( other.m_buffer )
			, m_ptr( const_cast<pointer>( other.m_ptr ) )
		{
		}

		circular_buffer_iterator( const Buffer* buffer, const pointer index ) noexcept
			: m_buffer( buffer )
			, m_ptr( index )
		{
		}

		[[nodiscard]] reference operator*() const noexcept
		{
			DEBUG_ASSERT( m_buffer, "Dereferencing an invalid iterator" );
			DEBUG_ASSERT( m_ptr, "Dereferencing an invalid iterator" );
			return *m_ptr;
		}

		[[nodiscard]] pointer operator->() const noexcept
		{
			DEBUG_ASSERT( m_buffer, "Dereferencing an invalid iterator" );
			DEBUG_ASSERT( m_ptr, "Dereferencing an invalid iterator" );
			return m_ptr;
		}

		[[nodiscard]] reference operator[]( const difference_type diff ) const noexcept
		{
			return *( *this + diff );
		}

		circular_buffer_iterator& operator++() noexcept
		{
			DEBUG_ASSERT( m_buffer, "Incrementing an invalid iterator" );
			DEBUG_ASSERT( m_ptr, "Incrementing an invalid iterator" );
			m_buffer->increment( m_ptr );
			if ( m_ptr == m_buffer->m_tail )
			{
				m_ptr = nullptr;
			}
			return *this;
		}

		circular_buffer_iterator operator++( int ) noexcept
		{
			circular_buffer_iterator temp( *this );
			++( *this );
			return temp;
		}

		circular_buffer_iterator& operator--() noexcept
		{
			DEBUG_ASSERT( m_buffer, "Decrementing an invalid iterator" );
			DEBUG_ASSERT( m_ptr != m_buffer->m_data, "Decrementing begin iterator" );
			if ( !m_ptr )
			{
				m_ptr = m_buffer->m_tail;
			}
			m_buffer->decrement( m_ptr );
			return *this;
		}

		circular_buffer_iterator operator--( int ) noexcept
		{
			circular_buffer_iterator temp( *this );
			--( *this );
			return temp;
		}

		circular_buffer_iterator& operator+=( const difference_type amount ) noexcept
		{
			DEBUG_ASSERT( m_buffer, "Incrementing an invalid iterator" );
			if ( amount > 0 )
			{
				m_buffer->increment( m_ptr, amount );
				if ( m_ptr == m_buffer->m_tail )
				{
					m_ptr = nullptr;
				}
			}
			else if ( amount < 0 )
			{
				*this -= -amount;
			}
			return *this;
		}

		circular_buffer_iterator& operator-=( const difference_type amount ) noexcept
		{
			DEBUG_ASSERT( m_buffer, "Incrementing an invalid iterator" );
			if ( amount > 0 )
			{
				if ( !m_ptr )
				{
					m_ptr = m_buffer->m_tail;
				}
				m_buffer->decrement( m_ptr, amount );
			}
			else if ( amount < 0 )
			{
				*this += -amount;
			}
			return *this;
		}

		[[nodiscard]] constexpr friend circular_buffer_iterator operator+( const circular_buffer_iterator& it,
																		   const difference_type diff ) noexcept
		{
			auto temp = it;
			temp += diff;
			return temp;
		}
		[[nodiscard]] constexpr friend circular_buffer_iterator operator+( const difference_type diff,
																		   const circular_buffer_iterator& it ) noexcept
		{
			return it + diff;
		}
		[[nodiscard]] constexpr friend circular_buffer_iterator operator-( const circular_buffer_iterator& it,
																		   const difference_type diff ) noexcept
		{
			auto temp = it;
			temp -= diff;
			return temp;
		}
		[[nodiscard]] constexpr friend difference_type operator-( const circular_buffer_iterator& lhs,
																  const circular_buffer_iterator& rhs ) noexcept
		{
			DEBUG_ASSERT( lhs.m_buffer == rhs.m_buffer, "Iterators are not comparable" );
			return lhs.contiguous_ptr() - rhs.contiguous_ptr();
		}

		template <bool OtherConst>
		[[nodiscard]] bool operator==( const circular_buffer_iterator<Buffer, OtherConst>& other ) const
			MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_buffer == other.m_buffer, "Iterators are not comparable" );
			return m_ptr == other.m_ptr;
		}
		template <bool OtherConst>
		[[nodiscard]] auto operator<=>( const circular_buffer_iterator<Buffer, OtherConst>& other ) const
			MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_buffer == other.m_buffer, "Iterators are not comparable" );
			return contiguous_ptr() <=> other.contiguous_ptr();
		}

	private:
		[[nodiscard]] pointer contiguous_ptr() const noexcept
		{
			if ( !m_ptr )
			{
				return m_buffer->m_data + m_buffer->m_size;
			}
			if ( m_ptr < m_buffer->m_head )
			{
				return m_ptr + ( m_buffer->m_data_end - m_buffer->m_head );
			}
			return m_buffer->m_data + ( m_ptr - m_buffer->m_head );
		}

		const Buffer* m_buffer = nullptr;
		pointer m_ptr = nullptr;
	};

	template <typename T, typename Allocator = std::allocator<T>>
	class circular_buffer
	{
		friend circular_buffer_iterator<circular_buffer, true>;
		friend circular_buffer_iterator<circular_buffer, false>;

	public:
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using allocator_type = Allocator;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using iterator = circular_buffer_iterator<circular_buffer, false>;
		using const_iterator = circular_buffer_iterator<circular_buffer, true>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		// Constructors and assignment

		circular_buffer() noexcept = default;

		~circular_buffer()
		{
			destroy_internal();
			m_allocator.deallocate( m_data, capacity() );
		}

		explicit circular_buffer( const allocator_type& alloc )
			: m_allocator( alloc )
		{
		}

		explicit circular_buffer( const size_type capacity, const allocator_type& alloc = allocator_type() )
			: m_allocator( alloc )
		{
			resize( capacity );
		}

		circular_buffer( const circular_buffer& other, const allocator_type& alloc )
			: m_allocator( alloc )
		{
			resize( other.size() );
			const auto [ first_span, second_span ] = other.as_contiguous();
			auto it = std::uninitialized_copy( first_span.begin(), first_span.end(), m_data );
			std::uninitialized_copy( second_span.begin(), second_span.end(), it );
		}

		circular_buffer( const circular_buffer& other )
			: circular_buffer( other,
							   std::allocator_traits<allocator_type>::select_on_container_copy_construction(
								   other.get_allocator() ) )
		{
		}

		circular_buffer( circular_buffer&& other, const allocator_type& alloc )
			: m_allocator( alloc )
			, m_data( std::exchange( other.m_data, nullptr ) )
			, m_data_end( std::exchange( other.m_data_end, nullptr ) )
			, m_head( std::exchange( other.m_head, nullptr ) )
			, m_tail( std::exchange( other.m_tail, nullptr ) )
			, m_size( std::exchange( other.m_size, 0 ) )
		{
		}

		circular_buffer( circular_buffer&& other )
			: m_allocator( std::move( other.m_allocator ) )
			, m_data( std::exchange( other.m_data, nullptr ) )
			, m_data_end( std::exchange( other.m_data_end, nullptr ) )
			, m_head( std::exchange( other.m_head, nullptr ) )
			, m_tail( std::exchange( other.m_tail, nullptr ) )
			, m_size( std::exchange( other.m_size, 0 ) )
		{
		}

		circular_buffer& operator=( const circular_buffer& other )
		{
			if ( this == &other )
			{
				return *this;
			}

			clear();

			if constexpr ( std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value )
			{
				if ( m_allocator != other.m_allocator )
				{
					m_allocator.deallocate( m_data, capacity() );
					m_data = m_data_end = nullptr;
				}
				m_allocator = other.m_allocator;
			}

			resize( other.size() );
			const auto [ first_span, second_span ] = other.as_contiguous();
			auto it = std::uninitialized_copy( first_span.begin(), first_span.end(), m_data );
			std::uninitialized_copy( second_span.begin(), second_span.end(), it );
			return *this;
		}

		circular_buffer& operator=( circular_buffer&& other ) noexcept(
			std::allocator_traits<allocator_type>::is_always_equal::value )
		{
			if ( this == &other )
			{
				return *this;
			}

			clear();

			if constexpr ( std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value )
			{
				m_allocator.deallocate( m_data, capacity() );
				m_data = m_data_end = nullptr;
				m_allocator = std::move( other.m_allocator );
			}
			else if ( m_allocator == other.m_allocator )
			{
				m_data = std::exchange( other.m_data, nullptr );
				m_data_end = std::exchange( other.m_data_end, nullptr );
				m_head = std::exchange( other.m_head, nullptr );
				m_tail = std::exchange( other.m_tail, nullptr );
				m_size = std::exchange( other.m_size, 0 );
			}
			else
			{
				m_allocator = std::move( other.m_allocator );
				resize( other.size() );
				const auto [ first_span, second_span ] = other.as_contiguous();
				auto it = std::uninitialized_move( first_span.begin(), first_span.end(), m_data );
				std::uninitialized_move( second_span.begin(), second_span.end(), it );
				other.clear();
			}

			return *this;
		}

		// Size and capacity

		[[nodiscard]] size_type size() const noexcept
		{
			return m_size;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return m_size == 0;
		}

		[[nodiscard]] size_type capacity() const noexcept
		{
			return m_data_end - m_data;
		}

		[[nodiscard]] size_type max_size() const noexcept
		{
			return std::numeric_limits<size_type>::max();
		}

		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return m_allocator;
		}

		// Element access

		[[nodiscard]] reference front() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return *m_head;
		}
		[[nodiscard]] const_reference front() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return *m_head;
		}

		[[nodiscard]] reference back() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return m_tail[ -1 ];
		}
		[[nodiscard]] const_reference back() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return m_tail[ -1 ];
		}

		[[nodiscard]] reference operator[]( const size_type index ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( index < size(), "Index out of range" );
			pointer ptr = m_head;
			increment( ptr, index );
			return *ptr;
		}

		[[nodiscard]] const_reference operator[]( const size_type index ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( index < size(), "Index out of range" );
			pointer ptr = m_head;
			increment( ptr, index );
			return *ptr;
		}

		[[nodiscard]] reference at( const size_type index )
		{
			if ( index >= size() )
			{
				throw std::out_of_range( "Index out of range" );
			}
			return ( *this )[ index ];
		}

		[[nodiscard]] const_reference at( const size_type index ) const
		{
			if ( index >= size() )
			{
				throw std::out_of_range( "Index out of range" );
			}
			return ( *this )[ index ];
		}

		// Contiguous views

		[[nodiscard]] std::pair<span<value_type>, span<value_type>> as_contiguous() noexcept
		{
			const bool wraps_around = m_tail <= m_head && !empty();
			mclo::span<value_type> first_span( m_head, wraps_around ? m_data_end : m_tail );
			mclo::span<value_type> second_span( m_data, wraps_around ? m_tail : m_data );
			return { first_span, second_span };
		}

		[[nodiscard]] std::pair<span<const value_type>, span<const value_type>> as_contiguous() const noexcept
		{
			const bool wraps_around = m_tail <= m_head && !empty();
			mclo::span<const value_type> first_span( m_head, wraps_around ? m_data_end : m_tail );
			mclo::span<const value_type> second_span( m_data, wraps_around ? m_tail : m_data );
			return { first_span, second_span };
		}

		span<value_type> make_contiguous() noexcept( std::is_nothrow_move_constructible_v<value_type> )
		{
			// todo(mc) this is WIP
			if ( empty() )
			{
				return {};
			}
			if ( full() )
			{
				m_head = m_data;
				m_tail = m_data + m_size;
				return { m_data, m_size };
			}
			if ( m_head <= m_tail )
			{
				return { m_head, m_tail };
			}
			// todo(mc) this will always move the "first" half to the front, but it could be optimized to move the
			// smaller half
			const size_type first_size = m_data_end - m_head;
			std::uninitialized_move_n( m_head, first_size, m_tail );
			std::destroy_n( m_head, first_size );
			m_head = m_data;
			m_tail += first_size;
			return { m_data, m_size };
		}

		// Back modifiers

		reference push_back( const T& value )
		{
			return emplace_back( value );
		}

		reference push_back( T&& value )
		{
			return emplace_back( std::move( value ) );
		}

		template <typename... Args>
		reference emplace_back( Args&&... args )
		{
			if ( full() )
			{
				reference result = replace( m_tail, std::forward<Args>( args )... );
				increment( m_tail );
				m_head = m_tail;
				return result;
			}
			else
			{
				reference result = *std::construct_at( m_tail, std::forward<Args>( args )... );
				increment( m_tail );
				++m_size;
				return result;
			}
		}

		void pop_back()
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			decrement( m_tail );
			std::destroy_at( m_tail );
			--m_size;
		}

		// Front modifiers

		reference push_front( const T& value )
		{
			return emplace_front( value );
		}

		reference push_front( T&& value )
		{
			return emplace_front( std::move( value ) );
		}

		template <typename... Args>
		reference emplace_front( Args&&... args )
		{
			if ( full() )
			{
				decrement( m_head );
				reference result = replace( m_head, std::forward<Args>( args )... );
				m_tail = m_head;
				return result;
			}
			else
			{
				decrement( m_head );
				reference result = *std::construct_at( m_head, std::forward<Args>( args )... );
				++m_size;
				return result;
			}
		}

		void pop_front()
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			std::destroy_at( m_head );
			increment( m_head );
			--m_size;
		}

		// Position modifiers
		// todo(mc) emplace, insert, erase

		iterator erase( const_iterator pos )
		{
			DEBUG_ASSERT( pos >= begin() && pos <= end(), "pos must be an iterator in this container" );
			const iterator it = pos;
			std::move( it + 1, end(), it );
			pop_back();
			return it;
		}

		iterator erase( const_iterator first, const_iterator last )
		{
			DEBUG_ASSERT( first >= begin() && first <= end(), "first must be an iterator in this container" );
			DEBUG_ASSERT( last >= begin() && last <= end(), "last must be an iterator in this container" );
			DEBUG_ASSERT( first <= last, "first and last must form a valid range in this container" );

			const iterator first_mut = first;
			const iterator last_mut = last;

			if ( first_mut != last_mut )
			{
				const iterator new_last = std::move( last_mut, end(), first_mut );
				std::destroy( new_last, end() );
				const size_type count = static_cast<size_type>( std::distance( new_last, end() ) );
				decrement( m_tail, count );
				m_size -= count;
			}

			return first_mut;
		}

		// Full modifiers

		void resize( const size_type new_capacity )
		{
			if ( new_capacity == capacity() )
			{
				return;
			}
			const pointer new_data = m_allocator.allocate( new_capacity );
			const size_type new_size = std::min( new_capacity, m_size );
			if ( !empty() )
			{
				const auto [ first_span, second_span ] = as_contiguous();
				auto it = std::uninitialized_move( first_span.begin(), first_span.end(), new_data );
				std::uninitialized_move( second_span.begin(), second_span.end(), it );
			}
			set_data( new_data, new_size, new_capacity );
		}

		void shrink_to_fit()
		{
			resize( m_size );
		}

		void clear() noexcept
		{
			destroy_internal();
			m_size = 0;
			m_head = m_data;
			m_tail = m_data;
		}

		void swap( circular_buffer& other ) MCLO_NOEXCEPT_TESTS
		{
			if ( this == &other )
			{
				return;
			}
			using std::swap;
			if constexpr ( std::allocator_traits<allocator_type>::propagate_on_container_swap::value )
			{
				swap( m_allocator, other.m_allocator );
			}
			else
			{
				DEBUG_ASSERT( m_allocator == other.m_allocator, "containers incompatible for swap" );
			}
			swap( m_data, other.m_data );
			swap( m_data_end, other.m_data_end );
			swap( m_size, other.m_size );
			swap( m_head, other.m_head );
			swap( m_tail, other.m_tail );
		}

		friend void swap( circular_buffer& lhs, circular_buffer& rhs ) MCLO_NOEXCEPT_TESTS
		{
			lhs.swap( rhs );
		}

		// Iterators

		iterator begin() noexcept
		{
			return iterator( this, empty() ? nullptr : m_head );
		}
		const_iterator begin() const noexcept
		{
			return const_iterator( this, empty() ? nullptr : m_head );
		}
		const_iterator cbegin() const noexcept
		{
			return const_iterator( this, empty() ? nullptr : m_head );
		}

		iterator end() noexcept
		{
			return iterator( this, nullptr );
		}
		const_iterator end() const noexcept
		{
			return const_iterator( this, nullptr );
		}
		const_iterator cend() const noexcept
		{
			return const_iterator( this, nullptr );
		}

		reverse_iterator rbegin() noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		const_reverse_iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		const_reverse_iterator crbegin() const noexcept
		{
			return std::make_reverse_iterator( cend() );
		}

		reverse_iterator rend() noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		const_reverse_iterator rend() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		const_reverse_iterator crend() const noexcept
		{
			return std::make_reverse_iterator( cbegin() );
		}

	private:
		[[nodiscard]] bool full() const noexcept
		{
			return m_size == capacity();
		}

		template <typename Pointer>
		void increment( Pointer& ptr ) const noexcept
		{
			++ptr;
			if ( ptr == m_data_end )
			{
				ptr = m_data;
			}
		}
		template <typename Pointer>
		void decrement( Pointer& ptr ) const noexcept
		{
			if ( ptr == m_data )
			{
				ptr = m_data_end;
			}
			--ptr;
		}

		template <typename Pointer>
		void increment( Pointer& ptr, const difference_type count ) const noexcept
		{
			ptr += ( count < ( m_data_end - ptr ) ) ? count : ( count - ( m_data_end - m_data ) );
		}
		template <typename Pointer>
		void decrement( Pointer& ptr, const difference_type count ) const noexcept
		{
			ptr -= ( count > ( ptr - m_data ) ) ? ( count - ( m_data_end - m_data ) ) : count;
		}

		template <typename... Args>
		reference replace( const pointer ptr, Args&&... args )
		{
			return *ptr = value_type( std::forward<Args>( args )... );
		}

		void set_data( const pointer new_data, const size_type new_size, const size_type new_capacity ) noexcept
		{
			DEBUG_ASSERT( new_capacity >= new_size, "Capacity must be greater than or equal to size" );
			m_allocator.deallocate( m_data, capacity() );
			m_data = new_data;
			m_data_end = m_data + new_capacity;
			m_head = m_data;
			m_tail = m_data + new_size;
			m_size = new_size;
		}

		void destroy_internal() noexcept
		{
			if constexpr ( !std::is_trivially_destructible_v<value_type> )
			{
				const auto [ first_span, second_span ] = as_contiguous();
				std::destroy( first_span.begin(), first_span.end() );
				std::destroy( second_span.begin(), second_span.end() );
			}
		}

		pointer m_data = nullptr;
		pointer m_data_end = 0;
		pointer m_head = nullptr;
		pointer m_tail = nullptr;
		size_type m_size = 0;
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_allocator;
	};
}
