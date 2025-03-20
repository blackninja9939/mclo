#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/enum/enum_size.hpp"
#include "mclo/numeric/math.hpp"

#include <iterator>
#include <limits>
#include <ranges>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
		template <typename TEnum>
		constexpr TEnum enum_add( const TEnum value, const std::ptrdiff_t amount ) noexcept
		{
			using underlying_t = std::underlying_type_t<TEnum>;
			const auto underlying = static_cast<underlying_t>( value );
			DEBUG_ASSERT( mclo::is_safe_addition( underlying, amount ), "Addition would overflow" );
			return static_cast<TEnum>( underlying + amount );
		}
	}

	template <typename TEnum>
	class enum_iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using iterator_concept = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = TEnum;
		using reference = TEnum;

		constexpr enum_iterator() noexcept = default;

		constexpr explicit enum_iterator( const TEnum value ) noexcept
			: m_value( value )
		{
		}

		[[nodiscard]] constexpr reference operator*() const noexcept
		{
			return m_value;
		}

		[[nodiscard]] constexpr reference operator[]( const difference_type diff ) const noexcept
		{
			return detail::enum_add( m_value, diff );
		}

		constexpr enum_iterator& operator++() noexcept
		{
			add( 1 );
			return *this;
		}

		constexpr enum_iterator operator++( int ) noexcept
		{
			enum_iterator copy( *this );
			++*this;
			return copy;
		}

		constexpr enum_iterator& operator--() noexcept
		{
			add( -1 );
			return *this;
		}

		constexpr enum_iterator operator--( int ) noexcept
		{
			enum_iterator copy( *this );
			--*this;
			return copy;
		}

		constexpr enum_iterator& operator+=( const difference_type diff ) noexcept
		{
			add( diff );
			return *this;
		}

		constexpr enum_iterator& operator-=( const difference_type diff ) noexcept
		{
			add( -diff );
			return *this;
		}

		[[nodiscard]] constexpr std::strong_ordering operator<=>( const enum_iterator& other ) const noexcept = default;

		[[nodiscard]] constexpr friend enum_iterator operator+( const enum_iterator& it,
																const difference_type diff ) noexcept
		{
			auto temp = it;
			temp += diff;
			return temp;
		}
		[[nodiscard]] constexpr friend enum_iterator operator+( const difference_type diff,
																const enum_iterator& it ) noexcept
		{
			return it + diff;
		}
		[[nodiscard]] constexpr friend enum_iterator operator-( const enum_iterator& it,
																const difference_type diff ) noexcept
		{
			auto temp = it;
			temp -= diff;
			return temp;
		}
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

	struct exclusive_enum_range_t
	{
		explicit exclusive_enum_range_t() = default;
	};

	inline constexpr exclusive_enum_range_t exclusive_enum_range;

	template <typename TEnum>
	class enum_range : public std::ranges::view_interface<enum_range<TEnum>>
	{
		using iterator = enum_iterator<TEnum>;

	public:
		static_assert( std::is_enum_v<TEnum>, "TEnum must be an enumeration type" );

		constexpr enum_range() MCLO_NOEXCEPT_TESTS
			requires mclo::has_enum_size<TEnum>
			: enum_range( exclusive_enum_range, static_cast<TEnum>( 0 ), enum_size<TEnum> )
		{
		}

		constexpr enum_range( const TEnum first, const TEnum last ) MCLO_NOEXCEPT_TESTS
			: enum_range( exclusive_enum_range, first, detail::enum_add( last, 1 ) )
		{
			if constexpr ( mclo::has_enum_size<TEnum> )
			{
				DEBUG_ASSERT(
					last != enum_size<TEnum>,
					"This constructor is inclusive to its arguments, so passing in EnumSize will include it in the "
					"range, this is likely an error, either use the default or exclusive_enum_range constructor" );
			}
		}

		constexpr enum_range( exclusive_enum_range_t, const TEnum first, const TEnum last ) MCLO_NOEXCEPT_TESTS
			: m_begin( first ),
			  m_end( last )
		{
			DEBUG_ASSERT( first <= last, "Iterators must form a valid range" );
		}

		[[nodiscard]] constexpr iterator begin() const noexcept
		{
			return m_begin;
		}
		[[nodiscard]] constexpr iterator end() const noexcept
		{
			return m_end;
		}

	private:
		iterator m_begin;
		iterator m_end;
	};
}
