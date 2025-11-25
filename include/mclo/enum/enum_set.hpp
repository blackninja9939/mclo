#pragma once

#include "mclo/concepts/always_false.hpp"
#include "mclo/container/bitset.hpp"
#include "mclo/enum/enum_size.hpp"

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

		using underlying_type = typename container_type::underlying_type;
		using value_type = TEnum;
		using size_type = std::size_t;

		constexpr enum_set() noexcept = default;

		constexpr enum_set( const enum_set& other ) noexcept = default;
		constexpr enum_set& operator=( const enum_set& other ) noexcept = default;

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, value_type> )
		constexpr enum_set( It first, Sentinel last )
		{
			insert( std::move( first ), std::move( last ) );
		}
		
		template <std::ranges::input_range Range>
			requires( std::convertible_to<std::ranges::range_reference_t<Range>, value_type> )
		constexpr enum_set( Range&& range ) noexcept
			: enum_set( std::ranges::begin( range ), std::ranges::end( range ) )
		{
		}

		constexpr enum_set( std::initializer_list<value_type> init_list ) noexcept
			: enum_set( init_list.begin(), init_list.end() )
		{
		}

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_container.none();
		}
		[[nodiscard]] constexpr bool full() const noexcept
		{
			return m_container.all();
		}
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_container.count();
		}
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return size_max;
		}

		[[nodiscard]] constexpr underlying_type to_mask() const noexcept
		{
			if constexpr ( requires { m_container.to_mask(); } )
			{
				return m_container.to_mask();
			}
			else
			{
				static_assert( always_false<value_type>, "Too many enumerations to create a single mask integer from" );
			}
		}

		constexpr void clear() noexcept
		{
			m_container.reset();
		}

		constexpr void fill() noexcept
		{
			m_container.set();
		}

		constexpr void insert( const value_type value ) noexcept
		{
			m_container.set( static_cast<size_type>( value ) );
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, value_type> )
		constexpr void insert( It first, Sentinel last )
		{
			for ( ; first != last; ++first )
			{
				insert( *first );
			}
		}

		template <std::ranges::input_range Range>
			requires( std::convertible_to<std::ranges::range_reference_t<Range>, value_type> )
		constexpr void insert( Range&& range ) noexcept
		{
			insert( std::ranges::begin( range ), std::ranges::end( range ) );
		}

		constexpr void insert( std::initializer_list<value_type> init_list ) noexcept
		{
			insert( init_list.begin(), init_list.end() );
		}

		constexpr void erase( const value_type key ) noexcept
		{
			m_container.reset( static_cast<size_type>( key ) );
		}

		constexpr void assign( const value_type key, const bool value ) noexcept
		{
			m_container.set( static_cast<size_type>( key ), value );
		}

		[[nodiscard]] constexpr bool contains( const value_type key ) const noexcept
		{
			return m_container.test( static_cast<size_type>( key ) );
		}

		// Compute the union of the two sets, that is the set of elements that are in either set
		constexpr void merge( const enum_set& other ) noexcept
		{
			m_container |= other.m_container;
		}

		constexpr enum_set merge( const enum_set& other ) const noexcept
		{
			auto copy = *this;
			copy.merge( other );
			return copy;
		}

		// Compute the intersection of the two sets, that is the set of elements that are in both sets
		constexpr void intersect( const enum_set& other ) noexcept
		{
			m_container &= other.m_container;
		}

		constexpr enum_set intersect( const enum_set& other ) const noexcept
		{
			auto copy = *this;
			copy.intersect( other );
			return copy;
		}

		// Compute the difference of the two sets, that is the set of elements that are in one of the sets but not in
		// both
		constexpr void difference( const enum_set& other ) noexcept
		{
			m_container ^= other.m_container;
		}

		constexpr enum_set difference( const enum_set& other ) const noexcept
		{
			auto copy = *this;
			copy.difference( other );
			return copy;
		}

		// Check if this set include all elements of the other set
		[[nodiscard]] constexpr bool includes( const enum_set& other ) const noexcept
		{
			return ( m_container & other.m_container ) == other.m_container;
		}

		// Check if this set include any elements of the other set
		[[nodiscard]] constexpr bool overlaps( const enum_set& other ) const noexcept
		{
			return ( m_container & other.m_container ).any();
		}

		// Check if this set include no elements of the other set
		[[nodiscard]] constexpr bool disjoint( const enum_set& other ) const noexcept
		{
			return ( m_container & other.m_container ).none();
		}

		constexpr void for_each_set( std::invocable<value_type> auto func ) const noexcept
		{
			m_container.for_each_set(
				[ func = std::move( func ) ]( const size_type index ) { func( static_cast<value_type>( index ) ); } );
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
