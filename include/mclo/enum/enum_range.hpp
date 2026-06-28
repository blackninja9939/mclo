#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/enum/enum_size.hpp"

#include <concepts>
#include <iterator>
#include <limits>
#include <ranges>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
		template <std::integral T, std::integral U>
		[[nodiscard]] constexpr bool is_safe_addition( const T lhs, const U rhs ) noexcept
		{
			constexpr T max = std::numeric_limits<T>::max();
			constexpr T min = std::numeric_limits<T>::min();
			if ( rhs > 0 && lhs > max - rhs )
			{
				return false;
			}
			if ( rhs < 0 && lhs < min - rhs )
			{
				return false;
			}
			return true;
		}

		template <typename TEnum>
		constexpr TEnum enum_add( const TEnum value, const std::ptrdiff_t amount ) noexcept
		{
			using underlying_t = std::underlying_type_t<TEnum>;
			const auto underlying = static_cast<underlying_t>( value );
			MCLO_DEBUG_ASSERT( is_safe_addition( underlying, amount ), "Addition would overflow" );
			return static_cast<TEnum>( underlying + amount );
		}
	}

	/// @brief A random-access iterator yielding successive enumerators of @p TEnum.
	/// @details Traverses an enumeration by incrementing its underlying integer value, allowing enums to be used as a
	/// range. Dereferencing returns the enumerator by value. Used by @ref enum_range.
	/// @tparam TEnum The enumeration type to iterate over.
	template <typename TEnum>
	class enum_iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using iterator_concept = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = TEnum;
		using reference = TEnum;

		/// @brief Constructs an iterator referring to the value-initialised enumerator.
		constexpr enum_iterator() noexcept = default;

		/// @brief Constructs an iterator referring to @p value.
		/// @param value The enumerator the iterator points at.
		constexpr explicit enum_iterator( const TEnum value ) noexcept
			: m_value( value )
		{
		}

		/// @brief Returns the current enumerator.
		[[nodiscard]] constexpr reference operator*() const noexcept
		{
			return m_value;
		}

		/// @brief Returns the enumerator @p diff positions away from the current one.
		/// @param diff The signed offset to apply.
		[[nodiscard]] constexpr reference operator[]( const difference_type diff ) const noexcept
		{
			return detail::enum_add( m_value, diff );
		}

		/// @brief Advances to the next enumerator.
		constexpr enum_iterator& operator++() noexcept
		{
			add( 1 );
			return *this;
		}

		/// @brief Advances to the next enumerator, returning the previous position.
		constexpr enum_iterator operator++( int ) noexcept
		{
			enum_iterator copy( *this );
			++*this;
			return copy;
		}

		/// @brief Retreats to the previous enumerator.
		constexpr enum_iterator& operator--() noexcept
		{
			add( -1 );
			return *this;
		}

		/// @brief Retreats to the previous enumerator, returning the previous position.
		constexpr enum_iterator operator--( int ) noexcept
		{
			enum_iterator copy( *this );
			--*this;
			return copy;
		}

		/// @brief Advances the iterator by @p diff enumerators.
		constexpr enum_iterator& operator+=( const difference_type diff ) noexcept
		{
			add( diff );
			return *this;
		}

		/// @brief Retreats the iterator by @p diff enumerators.
		constexpr enum_iterator& operator-=( const difference_type diff ) noexcept
		{
			add( -diff );
			return *this;
		}

		/// @brief Orders two iterators by the enumerator they refer to.
		[[nodiscard]] constexpr std::strong_ordering operator<=>( const enum_iterator& other ) const noexcept = default;

		/// @brief Returns an iterator advanced @p diff enumerators from @p it.
		[[nodiscard]] constexpr friend enum_iterator operator+( const enum_iterator& it,
																const difference_type diff ) noexcept
		{
			auto temp = it;
			temp += diff;
			return temp;
		}
		/// @brief Returns an iterator advanced @p diff enumerators from @p it.
		[[nodiscard]] constexpr friend enum_iterator operator+( const difference_type diff,
																const enum_iterator& it ) noexcept
		{
			return it + diff;
		}
		/// @brief Returns an iterator retreated @p diff enumerators from @p it.
		[[nodiscard]] constexpr friend enum_iterator operator-( const enum_iterator& it,
																const difference_type diff ) noexcept
		{
			auto temp = it;
			temp -= diff;
			return temp;
		}
		/// @brief Returns the number of enumerators between @p lhs and @p rhs.
		[[nodiscard]] constexpr friend difference_type operator-( const enum_iterator& lhs,
																  const enum_iterator& rhs ) noexcept
		{
			return static_cast<difference_type>( lhs.m_value ) - static_cast<difference_type>( rhs.m_value );
		}

	private:
		constexpr void add( const difference_type amount ) noexcept
		{
			m_value = detail::enum_add( m_value, amount );
		}

		TEnum m_value{};
	};

	/// @brief Tag type selecting the exclusive-upper-bound constructor of @ref enum_range.
	struct exclusive_enum_range_t
	{
		explicit exclusive_enum_range_t() = default;
	};

	/// @brief Tag constant for constructing an @ref enum_range with an exclusive upper bound.
	inline constexpr exclusive_enum_range_t exclusive_enum_range;

	/// @brief A view over a contiguous span of enumerators of @p TEnum.
	/// @details Models @c std::ranges::view, yielding each enumerator in turn via @ref enum_iterator. By default it
	/// covers every enumerator of an enumeration that opts in through @ref enum_size.
	/// @tparam TEnum The enumeration type to range over; must be an enumeration.
	template <typename TEnum>
	class enum_range : public std::ranges::view_interface<enum_range<TEnum>>
	{
		using iterator = enum_iterator<TEnum>;

	public:
		static_assert( std::is_enum_v<TEnum>, "TEnum must be an enumeration type" );

		/// @brief Constructs a range spanning every enumerator, from @c 0 up to (but excluding) @ref enum_size.
		/// @details Only available when @p TEnum opts in via @ref enum_size.
		constexpr enum_range() noexcept
			requires mclo::has_enum_size<TEnum>
			: enum_range( exclusive_enum_range, static_cast<TEnum>( 0 ), enum_size<TEnum> )
		{
		}

		/// @brief Constructs a range over the inclusive interval [@p first, @p last].
		/// @param first The first enumerator in the range.
		/// @param last The last enumerator in the range, included.
		/// @warning This constructor is inclusive, so passing @ref enum_size as @p last includes the sentinel value;
		/// prefer the default or @ref exclusive_enum_range constructor in that case.
		constexpr enum_range( const TEnum first, const TEnum last ) noexcept
			: enum_range( exclusive_enum_range, first, detail::enum_add( last, 1 ) )
		{
			if constexpr ( mclo::has_enum_size<TEnum> )
			{
				MCLO_DEBUG_ASSERT(
					last != enum_size<TEnum>,
					"This constructor is inclusive to its arguments, so passing in EnumSize will include it in the "
					"range, this is likely an error, either use the default or exclusive_enum_range constructor" );
			}
		}

		/// @brief Constructs a range over the half-open interval [@p first, @p last).
		/// @param first The first enumerator in the range.
		/// @param last One past the last enumerator in the range.
		constexpr enum_range( exclusive_enum_range_t, const TEnum first, const TEnum last ) noexcept
			: m_begin( first )
			, m_end( last )
		{
			MCLO_DEBUG_ASSERT( first <= last, "Iterators must form a valid range" );
		}

		/// @brief Returns an iterator to the first enumerator in the range.
		[[nodiscard]] constexpr iterator begin() const noexcept
		{
			return m_begin;
		}
		/// @brief Returns an iterator one past the last enumerator in the range.
		[[nodiscard]] constexpr iterator end() const noexcept
		{
			return m_end;
		}

	private:
		iterator m_begin;
		iterator m_end;
	};
}
