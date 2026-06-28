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
	/// @brief A random-access iterator over an @ref enum_map, yielding key/value pairs.
	/// @details Adapts an underlying value-container iterator, pairing each element with the enumerator whose index it
	/// occupies. Dereferencing produces a @c std::pair of the enumerator key and a reference to the value.
	/// @tparam TWrappedIterator The underlying contiguous-container iterator being adapted.
	/// @tparam TEnum The enumeration key type.
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

		/// @brief Constructs a singular iterator.
		constexpr enum_map_iterator() noexcept = default;

		/// @brief Constructs an iterator wrapping @p it at logical position @p index.
		/// @param it The underlying value-container iterator.
		/// @param index The index, used to reconstruct the enumerator key.
		constexpr enum_map_iterator( TWrappedIterator it, const difference_type index ) noexcept
			: m_it( std::move( it ) )
			, m_index( index )
		{
		}

		/// @brief Returns the current key/value pair.
		[[nodiscard]] constexpr reference operator*() const noexcept
		{
			return reference{ static_cast<key_type>( m_index ), *m_it };
		}

		/// @brief Returns a proxy granting member access to the current key/value pair.
		[[nodiscard]] constexpr pointer operator->() const noexcept
		{
			return { operator*() };
		}

		/// @brief Returns the key/value pair @p diff positions away.
		[[nodiscard]] constexpr reference operator[]( const difference_type diff ) const noexcept
		{
			return reference{ static_cast<key_type>( m_index + diff ), m_it[ diff ] };
		}

		/// @brief Advances to the next entry.
		constexpr enum_map_iterator& operator++() noexcept
		{
			++m_it;
			++m_index;
			return *this;
		}

		/// @brief Advances to the next entry, returning the previous position.
		constexpr enum_map_iterator operator++( int ) noexcept
		{
			enum_map_iterator copy( *this );
			++*this;
			return copy;
		}

		/// @brief Retreats to the previous entry.
		constexpr enum_map_iterator& operator--() noexcept
		{
			--m_it;
			--m_index;
			return *this;
		}

		/// @brief Retreats to the previous entry, returning the previous position.
		constexpr enum_map_iterator operator--( int ) noexcept
		{
			enum_map_iterator copy( *this );
			--*this;
			return copy;
		}

		/// @brief Advances the iterator by @p diff entries.
		constexpr enum_map_iterator& operator+=( const difference_type diff ) noexcept
		{
			m_it += diff;
			m_index += diff;
			return *this;
		}

		/// @brief Retreats the iterator by @p diff entries.
		constexpr enum_map_iterator& operator-=( const difference_type diff ) noexcept
		{
			m_it -= diff;
			m_index -= diff;
			return *this;
		}

		/// @brief Orders two iterators by their position.
		[[nodiscard]] constexpr friend std::strong_ordering operator<=>( const enum_map_iterator& lhs,
																		 const enum_map_iterator& rhs ) noexcept
		{
			return lhs.m_index <=> rhs.m_index;
		}
		/// @brief Compares two iterators for equality by position.
		[[nodiscard]] constexpr friend bool operator==( const enum_map_iterator& lhs,
														const enum_map_iterator& rhs ) noexcept
		{
			return lhs.m_index == rhs.m_index;
		}

		/// @brief Returns an iterator advanced @p diff entries from @p it.
		[[nodiscard]] constexpr friend enum_map_iterator operator+( const enum_map_iterator& it,
																	const difference_type diff ) noexcept
		{
			auto temp = it;
			temp += diff;
			return temp;
		}
		/// @brief Returns an iterator advanced @p diff entries from @p it.
		[[nodiscard]] constexpr friend enum_map_iterator operator+( const difference_type diff,
																	const enum_map_iterator& it ) noexcept
		{
			return it + diff;
		}
		/// @brief Returns an iterator retreated @p diff entries from @p it.
		[[nodiscard]] constexpr friend enum_map_iterator operator-( const enum_map_iterator& it,
																	const difference_type diff ) noexcept
		{
			auto temp = it;
			temp -= diff;
			return temp;
		}
		/// @brief Returns the number of entries between @p lhs and @p rhs.
		[[nodiscard]] constexpr friend difference_type operator-( const enum_map_iterator& lhs,
																  const enum_map_iterator& rhs ) noexcept
		{
			return lhs.m_index - rhs.m_index;
		}

	private:
		TWrappedIterator m_it;
		difference_type m_index;
	};

	/// @brief A fixed-size associative container mapping every enumerator of @p TEnum to a value.
	/// @details Backed by a @c std::array indexed directly by the enumerator's underlying value, giving O(1) access
	/// with no hashing or allocation. Every key in [0, @p SizeEnum) always has a value, so there is no insertion or
	/// erasure; it behaves like an array keyed by an enumeration.
	/// @tparam TEnum The enumeration key type; must be an enumeration.
	/// @tparam TValue The mapped value type.
	/// @tparam SizeEnum The number of keys, defaulting to @ref enum_size for @p TEnum.
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

		/// @brief Constructs a map with every value default-initialised.
		constexpr enum_map() = default;

		constexpr enum_map( const enum_map& other ) = default;
		constexpr enum_map( enum_map&& other ) = default;

		constexpr enum_map& operator=( const enum_map& other ) = default;
		constexpr enum_map& operator=( enum_map&& other ) = default;

		/// @brief Constructs a map with every value initialised to a copy of @p fill_value.
		/// @param fill_value The value broadcast to every key.
		constexpr explicit enum_map( const_reference fill_value ) noexcept(
			std::is_nothrow_copy_constructible_v<value_type> )
			requires( max_size != 1 )
			: m_container( mclo::broadcast_array<max_size>( fill_value ) )
		{
		}

		/// @brief Constructs a map from a range of key/value pairs, assigning each value to its key.
		/// @param first Iterator to the first pair.
		/// @param last Sentinel marking the end of the pairs.
		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, pair_type> )
		constexpr enum_map( It first, Sentinel last )
		{
			while ( first != last )
			{
				pair_type pair = *first++;
				operator[]( pair.first ) = std::move( pair.second );
			}
		}

		/// @brief Constructs a map from a range of key/value pairs, assigning each value to its key.
		/// @param range The range of pairs.
		template <std::ranges::input_range Range>
			requires( std::convertible_to<std::ranges::range_reference_t<Range>, pair_type> )
		constexpr explicit enum_map( Range&& range )
		{
			for ( const auto& pair : range )
			{
				operator[]( pair.first ) = pair.second;
			}
		}

		/// @brief Constructs a map from an initializer list of key/value pairs.
		/// @param init_list The pairs to assign.
		constexpr enum_map( const std::initializer_list<pair_type> init_list )
			: enum_map( init_list.begin(), init_list.end() )
		{
		}

		/// @brief Constructs a map by copying values, in key order, from a range.
		/// @param first Iterator to the first value.
		/// @param last Sentinel marking the end of the values.
		/// @pre The range must contain no more than @ref size() values.
		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::convertible_to<std::iter_reference_t<It>, value_type> )
		constexpr enum_map( It first, Sentinel last )
		{
			MCLO_DEBUG_ASSERT( std::ranges::distance( first, last ) <= max_size,
							   "Iterator pair is over a range larger than this container's max size" );
			std::ranges::copy( first, last, m_container.begin() );
		}

		/// @brief Constructs a map by copying values, in key order, from a range.
		/// @param range The range of values.
		/// @pre The range must contain no more than @ref size() values.
		template <std::ranges::input_range Range>
			requires( std::convertible_to<std::ranges::range_reference_t<Range>, value_type> )
		constexpr explicit enum_map( Range&& range )
		{
			MCLO_DEBUG_ASSERT( std::ranges::distance( range ) <= max_size,
							   "Range size is larger than this container's max size" );
			std::ranges::copy( range, m_container.begin() );
		}

		/// @brief Constructs a map from exactly one value per key, in key order.
		/// @param args One argument for each key, used to construct the corresponding value.
		template <std::constructible_from<value_type>... Ts>
			requires( sizeof...( Ts ) == max_size )
		constexpr enum_map( Ts&&... args ) noexcept( ( std::is_nothrow_constructible_v<value_type, Ts> && ... ) )
			: m_container{ static_cast<value_type>( std::forward<Ts>( args ) )... }
		{
		}

		/// @brief Accesses the value mapped to @p key.
		/// @param key The enumerator whose value to access.
		/// @return A reference to the mapped value.
		[[nodiscard]] constexpr reference operator[]( const key_type key ) noexcept
		{
			return index_direct( static_cast<size_type>( key ) );
		}
		/// @copydoc operator[](const key_type)
		[[nodiscard]] constexpr const_reference operator[]( const key_type key ) const noexcept
		{
			return index_direct( static_cast<size_type>( key ) );
		}

		/// @brief Accesses the value at the raw underlying @p index, bypassing the enumerator cast.
		/// @param index The zero-based index into the backing array.
		/// @return A reference to the value at that index.
		[[nodiscard]] constexpr reference index_direct( const size_type index ) noexcept
		{
			return m_container[ index ];
		}
		/// @copydoc index_direct(const size_type)
		[[nodiscard]] constexpr const_reference index_direct( const size_type index ) const noexcept
		{
			return m_container[ index ];
		}

		/// @brief Assigns @p value to every key.
		/// @param value The value copied into each entry.
		constexpr void fill( const_reference value ) noexcept( std::is_nothrow_copy_assignable_v<value_type> )
		{
			m_container.fill( value );
		}

		/// @brief Swaps the contents of this map with @p other.
		constexpr void swap( enum_map& other ) noexcept( std::is_nothrow_swappable_v<value_type> )
		{
			m_container.swap( other.m_container );
		}

		/// @brief Swaps the contents of @p lhs and @p rhs.
		friend constexpr void swap( enum_map& lhs, enum_map& rhs ) noexcept( std::is_nothrow_swappable_v<value_type> )
		{
			lhs.swap( rhs );
		}

		/// @brief Returns a span over the mapped values in key order.
		[[nodiscard]] constexpr mclo::span<value_type, max_size> as_span() noexcept
		{
			return m_container;
		}

		/// @copydoc as_span()
		[[nodiscard]] constexpr mclo::span<const value_type, max_size> as_span() const noexcept
		{
			return m_container;
		}

		/// @brief Returns the number of keys, which is always @p SizeEnum.
		[[nodiscard]] static constexpr size_type size() noexcept
		{
			return max_size;
		}

		/// @brief Returns a reference to the value of the first key.
		[[nodiscard]] constexpr reference front() noexcept
		{
			return m_container.front();
		}
		/// @copydoc front()
		[[nodiscard]] constexpr const_reference front() const noexcept
		{
			return m_container.front();
		}

		/// @brief Returns a reference to the value of the last key.
		[[nodiscard]] constexpr reference back() noexcept
		{
			return m_container.back();
		}
		/// @copydoc back()
		[[nodiscard]] constexpr const_reference back() const noexcept
		{
			return m_container.back();
		}

		/// @brief Returns an iterator to the first key/value entry.
		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return { m_container.begin(), 0 };
		}
		/// @copydoc begin()
		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return { m_container.begin(), 0 };
		}
		/// @brief Returns a const iterator to the first key/value entry.
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return { m_container.cbegin(), 0 };
		}

		/// @brief Returns an iterator one past the last key/value entry.
		[[nodiscard]] constexpr iterator end() noexcept
		{
			return { m_container.end(), max_size };
		}
		/// @copydoc end()
		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return { m_container.end(), max_size };
		}
		/// @brief Returns a const iterator one past the last key/value entry.
		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return { m_container.cend(), max_size };
		}

		/// @brief Returns a reverse iterator to the last key/value entry.
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		/// @copydoc rbegin()
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
		{
			return std::make_reverse_iterator( end() );
		}
		/// @brief Returns a const reverse iterator to the last key/value entry.
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return std::make_reverse_iterator( cend() );
		}

		/// @brief Returns a reverse iterator one before the first key/value entry.
		[[nodiscard]] constexpr reverse_iterator rend() noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		/// @copydoc rend()
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
		{
			return std::make_reverse_iterator( begin() );
		}
		/// @brief Returns a const reverse iterator one before the first key/value entry.
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
		{
			return std::make_reverse_iterator( cbegin() );
		}

		/// @brief Lexicographically compares two maps by their values in key order.
		constexpr auto operator<=>( const enum_map& other ) const = default;

	private:
		container_type m_container{};
	};
}
