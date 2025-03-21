#pragma once

#include "mclo/container/arrow_proxy.hpp"
#include "mclo/container/span.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/enum/enum_size.hpp"
#include "mclo/utility/array.hpp"

#include <array>
#include <compare>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename TWrappedIterator, typename TEnum>
	class enum_map_iterator
	{
		using wrapped_traits = std::iterator_traits<TWrappedIterator>;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using iterator_concept = std::random_access_iterator_tag;
		using difference_type = typename wrapped_traits::difference_type;
		using key_type = TEnum;
		using value_type = std::pair<const key_type, typename wrapped_traits::value_type>;
		using reference = std::pair<const key_type, typename wrapped_traits::reference>;
		using pointer = arrow_proxy<reference>;

		constexpr enum_map_iterator() noexcept = default;

		constexpr enum_map_iterator( TWrappedIterator it, const difference_type index ) noexcept
			: m_it( std::move( it ) )
			, m_index( index )
		{
		}

		[[nodiscard]] constexpr reference operator*() const noexcept
		{
			return reference{ static_cast<key_type>( m_index ), *m_it };
		}

		[[nodiscard]] constexpr pointer operator->() const noexcept
		{
			return { operator*() };
		}

		[[nodiscard]] constexpr reference operator[]( const difference_type diff ) const noexcept
		{
			return reference{ static_cast<key_type>( m_index + diff ), m_it[ diff ] };
		}

		constexpr enum_map_iterator& operator++() noexcept
		{
			++m_it;
			++m_index;
			return *this;
		}

		constexpr enum_map_iterator operator++( int ) noexcept
		{
			enum_map_iterator copy( *this );
			++*this;
			return copy;
		}

		constexpr enum_map_iterator& operator--() noexcept
		{
			--m_it;
			--m_index;
			return *this;
		}

		constexpr enum_map_iterator operator--( int ) noexcept
		{
			enum_map_iterator copy( *this );
			--*this;
			return copy;
		}

		constexpr enum_map_iterator& operator+=( const difference_type diff ) noexcept
		{
			m_it += diff;
			m_index += diff;
			return *this;
		}

		constexpr enum_map_iterator& operator-=( const difference_type diff ) noexcept
		{
			m_it -= diff;
			m_index -= diff;
			return *this;
		}

		[[nodiscard]] constexpr friend std::strong_ordering operator<=>( const enum_map_iterator& lhs,
																		 const enum_map_iterator& rhs ) noexcept
		{
			return lhs.m_index <=> rhs.m_index;
		}
		[[nodiscard]] constexpr friend bool operator==( const enum_map_iterator& lhs,
														const enum_map_iterator& rhs ) noexcept
		{
			return lhs.m_index == rhs.m_index;
		}

		[[nodiscard]] constexpr friend enum_map_iterator operator+( const enum_map_iterator& it,
																	const difference_type diff ) noexcept
		{
			auto temp = it;
			temp += diff;
			return temp;
		}
		[[nodiscard]] constexpr friend enum_map_iterator operator+( const difference_type diff,
																	const enum_map_iterator& it ) noexcept
		{
			return it + diff;
		}
		[[nodiscard]] constexpr friend enum_map_iterator operator-( const enum_map_iterator& it,
																	const difference_type diff ) noexcept
		{
			auto temp = it;
			temp -= diff;
			return temp;
		}
		[[nodiscard]] constexpr friend difference_type operator-( const enum_map_iterator& lhs,
																  const enum_map_iterator& rhs ) noexcept
		{
			return lhs.m_index - rhs.m_index;
		}

	private:
		TWrappedIterator m_it;
		difference_type m_index;
	};

	struct sorted_unique_t
	{
		explicit sorted_unique_t() = default;
	};

	inline constexpr sorted_unique_t sorted_unique;

	template <typename TEnum, typename TValue, TEnum SizeEnum = enum_size<TEnum>>
	class enum_map
	{
		static_assert( static_cast<std::underlying_type_t<TEnum>>( SizeEnum ) > 0,
					   "SizeEnum should be a positive value" );
		static constexpr std::size_t max_size = static_cast<std::size_t>( SizeEnum );
		using container_type = std::array<TValue, max_size>;

	public:
		static_assert( std::is_enum_v<TEnum>, "TEnum must be an enumeration type" );

		using key_type = TEnum;
		using value_type = TValue;
		using size_type = typename container_type::size_type;
		using difference_type = typename container_type::difference_type;
		using reference = typename container_type::reference;
		using const_reference = typename container_type::const_reference;
		using pointer = typename container_type::pointer;
		using const_pointer = typename container_type::const_pointer;
		using iterator = enum_map_iterator<typename container_type::iterator, key_type>;
		using const_iterator = enum_map_iterator<typename container_type::const_iterator, key_type>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using pair_type = std::pair<key_type, value_type>;

		constexpr enum_map() noexcept( std::is_nothrow_default_constructible_v<value_type> ) = default;

		constexpr explicit enum_map( const_reference fill_value ) noexcept(
			std::is_nothrow_copy_constructible_v<value_type> )
			: m_container( mclo::broadcast_array<max_size>( fill_value ) )
		{
		}

		template <std::forward_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, pair_type> )
		constexpr enum_map( It first, Sentinel last )
		{
			while ( first != last )
			{
				pair_type pair = *first++;
				operator[]( pair.first ) = std::move( pair.second );
			}
		}

		template <std::ranges::forward_range Range>
			requires( !std::convertible_to<Range, const_reference> &&
					  std::convertible_to<std::ranges::range_reference_t<Range>, pair_type> )
		constexpr explicit enum_map( Range&& range )
		{
			for ( const auto& pair : range )
			{
				operator[]( pair.first ) = pair.second;
			}
		}

		constexpr enum_map( const std::initializer_list<pair_type> init_list )
			: enum_map( init_list.begin(), init_list.end() )
		{
		}

		template <typename... Ts>
			requires( max_size > 1 && sizeof...( Ts ) == max_size )
		constexpr enum_map( sorted_unique_t,
							Ts&&... values ) noexcept( std::is_nothrow_move_constructible_v<value_type> )
			: m_container{ std::forward<Ts>( values )... }
		{
		}

		template <std::forward_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, value_type> )
		constexpr enum_map( sorted_unique_t, It first, Sentinel last )
		{
			DEBUG_ASSERT( std::ranges::distance( first, last ) <= max_size,
						  "Iterator pair is over a range larger than this container's max size" );
			std::ranges::copy( first, last, m_container.begin() );
		}

		template <std::ranges::forward_range Range>
			requires( !std::convertible_to<Range, const_reference> &&
					  std::convertible_to<std::ranges::range_reference_t<Range>, value_type> )
		constexpr explicit enum_map( sorted_unique_t, Range&& range )
		{
			DEBUG_ASSERT( std::ranges::distance( range ) <= max_size,
						  "Range size is larger than this container's max size" );
			std::ranges::copy( range, m_container.begin() );
		}

		[[nodiscard]] constexpr reference operator[]( const key_type key ) noexcept
		{
			return index_direct( static_cast<size_type>( key ) );
		}
		[[nodiscard]] constexpr const_reference operator[]( const key_type key ) const noexcept
		{
			return index_direct( static_cast<size_type>( key ) );
		}

		[[nodiscard]] constexpr reference index_direct( const size_type index ) noexcept
		{
			return m_container[ index ];
		}
		[[nodiscard]] constexpr const_reference index_direct( const size_type index ) const noexcept
		{
			return m_container[ index ];
		}

		constexpr void fill( const_reference value ) noexcept( std::is_nothrow_copy_assignable_v<value_type> )
		{
			m_container.fill( value );
		}

		constexpr void swap( enum_map& other ) noexcept( std::is_nothrow_swappable_v<value_type> )
		{
			m_container.swap( other.m_container );
		}

		friend constexpr void swap( enum_map& lhs, enum_map& rhs ) noexcept( std::is_nothrow_swappable_v<value_type> )
		{
			lhs.swap( rhs );
		}

		[[nodiscard]] constexpr auto operator<=>( const enum_map& other ) const = default;

		[[nodiscard]] constexpr mclo::span<value_type, max_size> as_span() noexcept
		{
			return m_container;
		}

		[[nodiscard]] constexpr mclo::span<const value_type, max_size> as_span() const noexcept
		{
			return m_container;
		}

		[[nodiscard]] static constexpr size_type size() noexcept
		{
			return max_size;
		}

		[[nodiscard]] constexpr reference front() noexcept
		{
			return m_container.front();
		}
		[[nodiscard]] constexpr const_reference front() const noexcept
		{
			return m_container.front();
		}

		[[nodiscard]] constexpr reference back() noexcept
		{
			return m_container.back();
		}
		[[nodiscard]] constexpr const_reference back() const noexcept
		{
			return m_container.back();
		}

		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return { m_container.begin(), 0 };
		}
		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return { m_container.begin(), 0 };
		}
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return { m_container.cbegin(), 0 };
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return { m_container.end(), max_size };
		}
		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return { m_container.end(), max_size };
		}
		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return { m_container.cend(), max_size };
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return std::make_reverse_iterator( cend() );
		}

		[[nodiscard]] constexpr reverse_iterator rend() noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
		{
			return std::make_reverse_iterator( cbegin() );
		}

	private:
		container_type m_container{};
	};
}
