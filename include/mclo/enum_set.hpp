#pragma once

#include "mclo/bitset.hpp"
#include "mclo/detail/enum_container.hpp"

#include <concepts>
#include <iterator>

namespace mclo
{
	template <typename TEnum, TEnum SizeEnum = detail::enum_size<TEnum>()>
	class enum_set
	{
		static_assert( detail::to_underlying( SizeEnum ) >= 0, "SizeEnum cannot have a negative value" );
		static constexpr std::size_t enum_size = static_cast<std::size_t>( SizeEnum );
		using container_type = mclo::bitset<enum_size>;

	public:
		using key_type = TEnum;
		using value_type = TEnum;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = TEnum;
		using const_reference = TEnum;

		friend class iterator;

		class iterator
		{
		public:
			friend class enum_set;

			using iterator_category = std::forward_iterator_tag;
			using iterator_concept = std::forward_iterator_tag;
			using difference_type = enum_set::difference_type;
			using value_type = value_type;
			using reference = value_type;

			constexpr iterator() noexcept = default;

			constexpr reference operator*() const noexcept
			{
				return reference{ static_cast<key_type>( m_index ) };
			}

			constexpr iterator& operator++() noexcept
			{
				assert( m_set );
				m_index = m_set->m_container.find_first_set( m_index + 1 );
				return *this;
			}

			constexpr iterator operator++( int ) noexcept
			{
				iterator copy( *this );
				++*this;
				return copy;
			}

			[[nodiscard]] constexpr friend bool operator==( const iterator& lhs,
															const iterator& rhs ) noexcept = default;

		private:
			constexpr iterator( const enum_set& set, const size_type index ) noexcept
				: m_set( &set )
				, m_index( index )
			{
			}

			const enum_set* m_set = nullptr;
			size_type m_index = container_type::npos;
		};

		using const_iterator = iterator;

		constexpr enum_set() noexcept = default;

		template <std::input_iterator It>
		constexpr enum_set( It first, It last )
		{
			insert( first, last );
		}

		constexpr enum_set( std::initializer_list<value_type> init_list ) noexcept
			: enum_set( init_list.begin(), init_list.end() )
		{
		}

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_container.none();
		}
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_container.count();
		}
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return max_size;
		}

		constexpr void clear() noexcept
		{
			m_container.reset();
		}

		constexpr std::pair<iterator, bool> insert( const value_type value ) noexcept
		{
			const size_type index = static_cast<size_type>( value );
			const bool already_set = m_container.test_set( index );
			return {
				iterator{*this, index},
                already_set
            };
		}

		template <std::input_iterator It>
		constexpr void insert( It first, It last )
		{
			for ( ; first != last; ++first )
			{
				m_container.set( static_cast<size_type>( *first ) );
			}
		}

		constexpr void insert( std::initializer_list<value_type> init_list ) noexcept
		{
			insert( init_list.begin(), init_list.end() );
		}

		constexpr bool erase( const value_type key ) noexcept
		{
			return m_container.test_set( static_cast<size_type>( key ), false );
		}

		constexpr iterator erase( const const_iterator pos ) noexcept
		{
			const size_type next_set = m_container.reset( pos.m_index ).find_first_set( pos.m_index + 1 );
			return iterator{ *this, next_set };
		}

		constexpr iterator erase( const_iterator first, const const_iterator last ) noexcept
		{
			if ( first == last )
			{
				return last;
			}

			size_type last_index = 0;
			for ( ; first != last; ++first )
			{
				last_index = first.m_index;
				m_container.reset( last_index );
			}

			const size_type next_set = m_container.find_first_set( last_index + 1 );
			return iterator{ *this, next_set };
		}

		[[nodiscard]] constexpr const_iterator find( const value_type key ) const noexcept
		{
			const size_type index = static_cast<size_type>( key );
			const bool is_set = m_container.test( index );
			return iterator{ *this, is_set ? index : container_type::npos };
		}

		[[nodiscard]] constexpr bool contains( const value_type key ) const noexcept
		{
			return m_container.test( static_cast<size_type>( key ) );
		}

		constexpr void for_each_set( std::invocable<std::size_t> auto func ) const noexcept
		{
			m_container.for_each_set( func );
		}

		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return cbegin();
		}
		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return cbegin();
		}
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return iterator{ *this, m_container.find_first_set() };
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return cend();
		}
		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return cend();
		}
		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return iterator{ *this, container_type::npos };
		}

		constexpr void swap( enum_set& other ) noexcept
		{
			using std::swap;
			swap( m_container, other.m_container );
		}

		friend constexpr void swap( enum_set& lhs, enum_set& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		container_type m_container;
	};
}
