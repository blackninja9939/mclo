#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"
#include "mclo/container/span.hpp"

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
		using size_type = typename Buffer::size_type;

	public:
		circular_buffer_iterator() noexcept = default;

		template <bool OtherConst>
		circular_buffer_iterator( const circular_buffer_iterator<Buffer, OtherConst>& other ) noexcept
			: m_buffer( other.m_buffer )
			, m_index( other.m_index )
		{
		}

		circular_buffer_iterator( const Buffer* buffer, const size_type index ) noexcept
			: m_buffer( buffer )
			, m_index( index )
		{
		}

		using iterator_category = std::random_access_iterator_tag;
		using iterator_concept = std::random_access_iterator_tag;
		using difference_type = typename Buffer::difference_type;
		using value_type = typename Buffer::value_type;
		using reference = std::conditional_t<IsConst, typename Buffer::const_reference, typename Buffer::reference>;
		using pointer = std::conditional_t<IsConst, typename Buffer::const_pointer, typename Buffer::pointer>;

		[[nodiscard]] reference operator*() const noexcept
		{
			return m_buffer->m_data[ m_index ];
		}

		[[nodiscard]] pointer operator->() const noexcept
		{
			return m_buffer->m_data + m_index;
		}

		circular_buffer_iterator& operator++() noexcept
		{
			m_buffer->increment( m_index );
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
			m_buffer->decrement( m_index );
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
			if ( amount > 0 )
			{
				m_buffer->increment( m_index, amount );
			}
			else if ( amount < 0 )
			{
				m_buffer->decrement( m_index, -amount );
			}
			return *this;
		}

		circular_buffer_iterator& operator-=( const difference_type amount ) noexcept
		{
			if ( amount > 0 )
			{
				m_buffer->decrement( m_index, amount );
			}
			else if ( amount < 0 )
			{
				m_buffer->increment( m_index, -amount );
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
			return lhs.linear_index() - rhs.linear_index();
		}

		template <bool OtherConst>
		[[nodiscard]] bool operator==( const circular_buffer_iterator<Buffer, OtherConst>& other ) const
			MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_buffer == other.m_buffer, "Iterators are not comparable" );
			return m_index == other.m_index;
		}
		template <bool OtherConst>
		[[nodiscard]] auto operator<=>( const circular_buffer_iterator<Buffer, OtherConst>& other ) const
			MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( m_buffer == other.m_buffer, "Iterators are not comparable" );
			return linear_index() <=> other.linear_index();
		}

	private:
		[[nodiscard]] size_type linear_index() const noexcept
		{
			if ( m_index < m_buffer->m_head )
			{
				return m_index + ( m_buffer->m_tail - m_buffer->m_head );
			}
			return m_index - m_buffer->m_head;
		}

		const Buffer* m_buffer = nullptr;
		size_type m_index = 0;
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

		circular_buffer() noexcept = default;

		explicit circular_buffer( const size_type capacity )
		{
			resize( capacity );
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
			return m_capacity;
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
			return m_data[ m_head ];
		}
		[[nodiscard]] const_reference front() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return m_data[ m_head ];
		}

		[[nodiscard]] reference back() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return m_data[ m_tail - 1 ];
		}
		[[nodiscard]] const_reference back() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return m_data[ m_tail - 1 ];
		}

		[[nodiscard]] reference operator[]( const size_type index ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( index < size(), "Index out of range" );
			return m_data[ ( m_head + index ) % capacity ];
		}

		[[nodiscard]] const_reference operator[]( const size_type index ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( index < size(), "Index out of range" );
			return m_data[ ( m_head + index ) % capacity ];
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
			mclo::span<value_type> first_span = { m_data + m_head, ( wraps_around ? m_capacity : m_tail ) - m_head };
			mclo::span<value_type> second_span = { m_data, wraps_around ? m_tail : 0 };
			return { first_span, second_span };
		}

		[[nodiscard]] std::pair<span<const value_type>, span<const value_type>> as_contiguous() const noexcept
		{
			const bool wraps_around = m_tail <= m_head && !empty();
			mclo::span<const value_type> first_span = { m_data + m_head, ( wraps_around ? m_capacity : m_tail ) - m_head };
			mclo::span<const value_type> second_span = { m_data, wraps_around ? m_tail : 0 };
			return { first_span, second_span };
		}

		span<value_type> make_contiguous() noexcept( std::is_nothrow_move_constructible_v<value_type> )
		{
			// todo(mc) this is WIP
			if ( empty() )
			{
				return { m_data, m_size };
			}
			if ( full() )
			{
				m_head = 0;
				m_tail = m_size;
				return { m_data, m_size };
			}
			if ( m_head <= m_tail )
			{
				return { m_data + m_head, m_size };
			}
			// todo(mc) this will always move the "first" half to the front, but it could be optimized to move the
			// smaller half
			const size_type first_size = m_capacity - m_head;
			std::uninitialized_move_n( m_data + m_head, first_size, m_data + m_tail );
			std::destroy_n( m_data + m_head, first_size );
			m_head = 0;
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
				reference result = *std::construct_at( m_data + m_tail, std::forward<Args>( args )... );
				increment( m_tail );
				++m_size;
				return result;
			}
		}

		void pop_back()
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			decrement( m_tail );
			std::destroy_at( m_data + m_tail );
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
				reference result = *std::construct_at( m_data + m_head, std::forward<Args>( args )... );
				++m_size;
				return result;
			}
		}

		void pop_front()
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			std::destroy_at( m_data + m_head );
			increment( m_head );
			--m_size;
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
			// todo(mc) optimize this to leverage contiguous spans
			std::uninitialized_move_n( begin(), new_size, new_data );
			set_data( new_data, new_size, new_capacity );
		}

		// todo: insert, erase, assign, as contiguous, make contiguous

		void clear() noexcept
		{
			// todo(mc) optimize this to leverage contiguous spans
			std::destroy( begin(), end() );
			m_size = 0;
			m_head = 0;
			m_tail = 0;
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
			swap( m_capacity, other.m_capacity );
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
			return iterator( this, m_head );
		}
		const_iterator begin() const noexcept
		{
			return const_iterator( this, m_head );
		}
		const_iterator cbegin() const noexcept
		{
			return const_iterator( this, m_head );
		}

		iterator end() noexcept
		{
			return iterator( this, m_tail );
		}
		const_iterator end() const noexcept
		{
			return const_iterator( this, m_tail );
		}
		const_iterator cend() const noexcept
		{
			return const_iterator( this, m_tail );
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
			return m_size == m_capacity;
		}

		void increment( size_type& index, size_type count = 1 ) const noexcept
		{
			index = ( index + count ) % m_capacity;
		}
		void decrement( size_type& index, size_type count = 1 ) const noexcept
		{
			index = ( index - count + m_capacity ) % m_capacity;
		}

		template <typename... Args>
		reference replace( const size_type index, Args&&... args )
		{
			return m_data[ index ] = value_type( std::forward<Args>( args )... );
		}

		void set_data( pointer data, const size_type size, const size_type capacity ) noexcept
		{
			DEBUG_ASSERT( capacity >= size, "Capacity must be greater than or equal to size" );
			m_allocator.deallocate( m_data, m_capacity );
			m_data = data;
			m_capacity = capacity;
			m_size = size;
			m_head = 0;
			m_tail = m_size;
		}

		pointer m_data = nullptr;
		size_type m_capacity = 0;
		size_type m_size = 0;
		size_type m_head = 0;
		size_type m_tail = 0;
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_allocator;
	};
}
