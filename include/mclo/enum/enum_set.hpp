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
	/// @brief A fixed-capacity set of enumerators of @p TEnum, backed by a bitset.
	/// @details Stores membership as one bit per possible enumerator in a @ref mclo::bitset, giving compact storage
	/// and O(1) insertion, removal, and lookup with no allocation. Also offers the usual set-algebra operations
	/// (union, intersection, difference, etc.) as both in-place and value-returning overloads.
	/// @tparam TEnum The enumeration element type; must be an enumeration.
	/// @tparam SizeEnum The number of distinct enumerators, defaulting to @ref enum_size for @p TEnum.
	template <typename TEnum, TEnum SizeEnum = enum_size<TEnum>>
	class enum_set
	{
		static_assert( static_cast<std::underlying_type_t<TEnum>>( SizeEnum ) > 0,
					   "SizeEnum should be a positive value" );
		static constexpr std::size_t size_max = static_cast<std::size_t>( SizeEnum );

	public:
		static_assert( std::is_enum_v<TEnum>, "TEnum must be an enumeration type" );

		using underlying_container = mclo::bitset<size_max>;
		using value_type = TEnum;
		using size_type = std::size_t;

		/// @brief Constructs an empty set.
		constexpr enum_set() noexcept = default;

		constexpr enum_set( const enum_set& other ) noexcept = default;
		constexpr enum_set& operator=( const enum_set& other ) noexcept = default;

		/// @brief Constructs a set directly from an underlying bitset of membership bits.
		/// @param container The bitset whose set bits become the set's members.
		constexpr explicit enum_set( const underlying_container& container ) noexcept
			: m_container( container )
		{
		}

		/// @brief Constructs a set containing the enumerators in the given range.
		/// @param first Iterator to the first enumerator.
		/// @param last Sentinel marking the end of the range.
		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, value_type> )
		constexpr enum_set( It first, Sentinel last )
		{
			insert( std::move( first ), std::move( last ) );
		}

		/// @brief Constructs a set containing the enumerators in @p range.
		/// @param range The range of enumerators to insert.
		template <std::ranges::input_range Range>
			requires( std::convertible_to<std::ranges::range_reference_t<Range>, value_type> )
		constexpr enum_set( Range&& range ) noexcept
			: enum_set( std::ranges::begin( range ), std::ranges::end( range ) )
		{
		}

		/// @brief Constructs a set from an initializer list of enumerators.
		/// @param init_list The enumerators to insert.
		constexpr enum_set( std::initializer_list<value_type> init_list ) noexcept
			: enum_set( init_list.begin(), init_list.end() )
		{
		}

		/// @brief Returns whether the set contains no enumerators.
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_container.none();
		}
		/// @brief Returns whether the set contains every possible enumerator.
		[[nodiscard]] constexpr bool full() const noexcept
		{
			return m_container.all();
		}
		/// @brief Returns the number of enumerators in the set.
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_container.count();
		}
		/// @brief Returns the maximum number of enumerators the set can hold, which is @p SizeEnum.
		[[nodiscard]] static constexpr size_type max_size() noexcept
		{
			return size_max;
		}

		/// @brief Returns the underlying bitset of membership bits.
		[[nodiscard]] constexpr const underlying_container& underlying() const noexcept
		{
			return m_container;
		}

		/// @brief Removes all enumerators from the set.
		constexpr void clear() noexcept
		{
			m_container.reset();
		}

		/// @brief Inserts every possible enumerator into the set.
		constexpr void fill() noexcept
		{
			m_container.set();
		}

		/// @brief Inserts @p value into the set.
		/// @param value The enumerator to insert.
		constexpr void insert( const value_type value ) noexcept
		{
			m_container.set( static_cast<size_type>( value ) );
		}

		/// @brief Inserts every enumerator in the given range into the set.
		/// @param first Iterator to the first enumerator.
		/// @param last Sentinel marking the end of the range.
		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, value_type> )
		constexpr void insert( It first, Sentinel last )
		{
			for ( ; first != last; ++first )
			{
				insert( *first );
			}
		}

		/// @brief Inserts every enumerator in @p range into the set.
		/// @param range The range of enumerators to insert.
		template <std::ranges::input_range Range>
			requires( std::convertible_to<std::ranges::range_reference_t<Range>, value_type> )
		constexpr void insert( Range&& range ) noexcept
		{
			insert( std::ranges::begin( range ), std::ranges::end( range ) );
		}

		/// @brief Inserts every enumerator in the initializer list into the set.
		/// @param init_list The enumerators to insert.
		constexpr void insert( std::initializer_list<value_type> init_list ) noexcept
		{
			insert( init_list.begin(), init_list.end() );
		}

		/// @brief Removes @p key from the set if present.
		/// @param key The enumerator to remove.
		constexpr void erase( const value_type key ) noexcept
		{
			m_container.reset( static_cast<size_type>( key ) );
		}

		/// @brief Inserts or removes @p key according to @p value.
		/// @param key The enumerator to update.
		/// @param value @c true to insert @p key, @c false to remove it.
		constexpr void assign( const value_type key, const bool value ) noexcept
		{
			m_container.set( static_cast<size_type>( key ), value );
		}

		/// @brief Returns whether @p key is a member of the set.
		/// @param key The enumerator to test.
		[[nodiscard]] constexpr bool contains( const value_type key ) const noexcept
		{
			return m_container.test( static_cast<size_type>( key ) );
		}

		/// @brief Merges @p other into this set, forming the union (elements in either set).
		/// @param other The set to merge in.
		constexpr void merge( const enum_set& other ) noexcept
		{
			m_container |= other.m_container;
		}

		/// @brief Returns the union of this set and @p other (elements in either set).
		/// @param other The other set.
		[[nodiscard]] constexpr enum_set merge( const enum_set& other ) const noexcept
		{
			auto copy = *this;
			copy.merge( other );
			return copy;
		}

		/// @brief Reduces this set to its intersection with @p other (elements in both sets).
		/// @param other The set to intersect with.
		constexpr void intersect( const enum_set& other ) noexcept
		{
			m_container &= other.m_container;
		}

		/// @brief Returns the intersection of this set and @p other (elements in both sets).
		/// @param other The other set.
		[[nodiscard]] constexpr enum_set intersect( const enum_set& other ) const noexcept
		{
			auto copy = *this;
			copy.intersect( other );
			return copy;
		}

		/// @brief Reduces this set to its symmetric difference with @p other (elements in exactly one set).
		/// @param other The other set.
		constexpr void symmetric_difference( const enum_set& other ) noexcept
		{
			m_container ^= other.m_container;
		}

		/// @brief Returns the symmetric difference of this set and @p other (elements in exactly one set).
		/// @param other The other set.
		[[nodiscard]] constexpr enum_set symmetric_difference( const enum_set& other ) const noexcept
		{
			auto copy = *this;
			copy.symmetric_difference( other );
			return copy;
		}

		/// @brief Removes from this set every element also in @p other (set difference).
		/// @param other The set whose elements to remove.
		constexpr void difference( const enum_set& other ) noexcept
		{
			m_container &= ~other.m_container;
		}

		/// @brief Returns the difference of this set and @p other (elements in this set but not @p other).
		/// @param other The other set.
		[[nodiscard]] constexpr enum_set difference( const enum_set& other ) const noexcept
		{
			auto copy = *this;
			copy.difference( other );
			return copy;
		}

		/// @brief Inverts the set in place, so it holds exactly the enumerators it previously did not.
		constexpr void complement() noexcept
		{
			m_container.flip();
		}

		/// @brief Returns the complement of the set (every possible enumerator not currently a member).
		[[nodiscard]] constexpr enum_set complement() const noexcept
		{
			auto copy = *this;
			copy.complement();
			return copy;
		}

		/// @brief Returns whether this set contains every element of @p other (superset test).
		/// @param other The candidate subset.
		[[nodiscard]] constexpr bool includes( const enum_set& other ) const noexcept
		{
			return ( m_container & other.m_container ) == other.m_container;
		}

		/// @brief Returns whether this set shares any element with @p other.
		/// @param other The other set.
		[[nodiscard]] constexpr bool overlaps( const enum_set& other ) const noexcept
		{
			return ( m_container & other.m_container ).any();
		}

		/// @brief Returns whether this set shares no element with @p other.
		/// @param other The other set.
		[[nodiscard]] constexpr bool disjoint( const enum_set& other ) const noexcept
		{
			return ( m_container & other.m_container ).none();
		}

		/// @brief Invokes @p func with each enumerator present in the set, in ascending order.
		/// @param func The callable invoked with every set enumerator.
		constexpr void for_each_set( std::invocable<value_type> auto func ) const noexcept
		{
			m_container.for_each_set(
				[ func = std::move( func ) ]( const size_type index ) { func( static_cast<value_type>( index ) ); } );
		}

		/// @brief Swaps the contents of this set with @p other.
		constexpr void swap( enum_set& other ) noexcept
		{
			using std::swap;
			swap( m_container, other.m_container );
		}

		/// @brief Swaps the contents of @p lhs and @p rhs.
		friend constexpr void swap( enum_set& lhs, enum_set& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		/// @brief Compares two sets for equality by membership.
		[[nodiscard]] constexpr bool operator==( const enum_set& other ) const noexcept = default;

	private:
		underlying_container m_container;
	};
}
