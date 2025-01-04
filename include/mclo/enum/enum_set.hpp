#pragma once

#include "mclo/bitset.hpp"
#include "mclo/enum_size.hpp"

#include <concepts>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace mclo
{
	template <typename TEnum, TEnum SizeEnum = enum_size<TEnum>>
	class enum_set
	{
		static_assert( static_cast<std::underlying_type_t<TEnum>>( SizeEnum ) > 0,
					   "SizeEnum should be a positive value" );
		static constexpr std::size_t size_max = static_cast<std::size_t>( SizeEnum );
		using container_type = mclo::bitset<size_max>;

	public:
		static_assert( std::is_enum_v<TEnum>, "TEnum must be an enumeration type" );

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
			using difference_type = typename enum_set::difference_type;
			using value_type = TEnum;
			using reference = TEnum;
			using pointer = void;

			constexpr iterator() noexcept = default;

			constexpr reference operator*() const noexcept
			{
				return reference{ static_cast<key_type>( m_index ) };
			}

			constexpr iterator& operator++() noexcept
			{
				assert( m_set && "Underlying set is missing" );
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

		template <typename InputIt>
		constexpr enum_set( InputIt first, InputIt last )
		{
			insert( std::move( first ), std::move( last ) );
		}

		template <typename Range>
		constexpr enum_set( Range&& range ) noexcept
			: enum_set( std::begin( range ), std::end( range ) )
		{
		}

		constexpr enum_set( std::initializer_list<value_type> initList ) noexcept
			: enum_set( std::begin( initList ), std::end( initList ) )
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
			return size_max;
		}

		constexpr void clear() noexcept
		{
			m_container.reset();
		}

		constexpr std::pair<iterator, bool> insert( const value_type value ) noexcept
		{
			const size_type index = static_cast<size_type>( value );
			const bool alreadySet = m_container.test_set( index );
			return {
				iterator{ *this, index },
                !alreadySet
            };
		}

		template <typename InputIt>
		constexpr void insert( InputIt first, InputIt last )
		{
			for ( ; first != last; ++first )
			{
				m_container.set( static_cast<size_type>( *first ) );
			}
		}

		template <typename Range>
			requires( !std::convertible_to<Range, value_type> )
		constexpr void insert( Range&& range ) noexcept
		{
			insert( std::begin( range ), std::end( range ) );
		}

		constexpr void insert( std::initializer_list<value_type> initList ) noexcept
		{
			insert( std::begin( initList ), std::end( initList ) );
		}

		constexpr bool erase( const value_type key ) noexcept
		{
			return m_container.test_set( static_cast<size_type>( key ), false );
		}

		void merge( const enum_set& other ) noexcept
		{
			m_container |= other.m_container;
		}

		constexpr iterator erase( const const_iterator pos ) noexcept
		{
			if ( pos.m_index == container_type::npos )
			{
				return pos;
			}
			const size_type nextSet = m_container.reset( pos.m_index ).find_first_set( pos.m_index + 1 );
			return iterator{ *this, nextSet };
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

		constexpr void forEachSet( std::invocable<value_type> auto func ) const noexcept
		{
			m_container.for_each_set(
				[ func = std::move( func ) ]( const size_type index ) { func( static_cast<value_type>( index ) ); } );
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

		[[nodiscard]] constexpr bool operator==( const enum_set& other ) const noexcept = default;

	private:
		container_type m_container;
	};
}
