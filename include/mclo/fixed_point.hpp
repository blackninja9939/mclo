#pragma once

#include "mclo/assume.hpp"
#include "mclo/math.hpp"

#include <cassert>
#include <cinttypes>
#include <limits>
#include <utility>
#include <type_traits>

namespace mclo
{
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	class decimal_fixed_point;
}

namespace std
{
	template <typename UnderlyingT1, std::size_t DigitsVal1, typename UnderlyingT2, std::size_t DigitsVal2>
	struct common_type<mclo::decimal_fixed_point<UnderlyingT1, DigitsVal1>,
					   mclo::decimal_fixed_point<UnderlyingT2, DigitsVal2>>
	{
		using type = typename mclo::decimal_fixed_point<std::common_type_t<UnderlyingT1, UnderlyingT2>,
														std::max( DigitsVal1, DigitsVal2 )>;
	};
}

namespace mclo
{
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	class decimal_fixed_point
	{
	private:
		using intermediate_type = std::conditional_t<std::is_signed_v<TUnderlying>, std::int64_t, std::uint64_t>;

	public:
		using underlying_type = TUnderlying;
		static constexpr std::uint8_t Decimals = DecimalsValue;
		static constexpr std::size_t Resolution = mclo::pow10( Decimals );

		static_assert( std::is_integral_v<underlying_type>, "Underlying type must be an integral" );
		static_assert( Decimals < std::numeric_limits<underlying_type>::digits10,
					   "Cannot have more decimals than underlying has digits" );

		constexpr decimal_fixed_point() noexcept = default;
		constexpr decimal_fixed_point( const decimal_fixed_point& other ) noexcept = default;
		constexpr decimal_fixed_point& operator=( const decimal_fixed_point& other ) noexcept = default;
		constexpr decimal_fixed_point( decimal_fixed_point&& other ) noexcept = default;
		constexpr decimal_fixed_point& operator=( decimal_fixed_point&& other ) noexcept = default;

		template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
		constexpr decimal_fixed_point( const T value ) noexcept
			: m_underlying( static_cast<underlying_type>( value * Resolution ) )
		{
			MCLO_ASSERT_ASSUME( std::in_range<underlying_type>( intermediate_type( value ) * Resolution ) );
		}

		template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
		constexpr decimal_fixed_point( const T value ) noexcept
			: m_underlying( static_cast<underlying_type>( value * Resolution + ( value >= 0 ? T( 0.5 ) : T( -0.5 ) ) ) )
		{
		}

#define CHECK_EXPRESSION_OVERFLOW( EXPRESSION )                                                                        \
	MCLO_ASSERT_ASSUME( std::in_range<underlying_type>( intermediate_type( m_underlying ) EXPRESSION ) )

		/*
		 * todo:
		 * converting constructor for other fixed point types
		 * assignment for integers, floats, fixed point types
		 */

		// Underlying value

		[[nodiscard]] constexpr underlying_type get_underlying() const noexcept
		{
			return m_underlying;
		}

		constexpr void set_underlying( const underlying_type value ) noexcept
		{
			m_underlying = value;
		}

		static constexpr decimal_fixed_point create_from_underlying( const underlying_type value ) noexcept
		{
			return basic_decimal_fixed_point( from_underlying_tag{}, value );
		}

		// integer/float casts

		constexpr underlying_type truncated() const noexcept
		{
			return m_underlying / Resolution;
		}

		template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
		constexpr explicit operator T() const noexcept
		{
			return static_cast<T>( m_underlying ) / Resolution;
		}

		// Numeric operations

		template <typename T = TUnderlying, std::enable_if_t<std::is_signed_v<T>, int> = 0>
		[[nodiscard]] constexpr decimal_fixed_point operator-() const noexcept
		{
			return create_from_underlying( -m_underlying );
		}

		constexpr decimal_fixed_point& operator++() noexcept
		{
			CHECK_EXPRESSION_OVERFLOW( + Resolution );
			m_underlying += Resolution;
			return *this;
		}
		constexpr decimal_fixed_point operator++( int ) noexcept
		{
			auto temp( *this );
			++( *this );
			return temp;
		}

		constexpr decimal_fixed_point& operator--() noexcept
		{
			CHECK_EXPRESSION_OVERFLOW( -Resolution );
			m_underlying -= Resolution;
			return *this;
		}
		constexpr decimal_fixed_point operator--( int ) noexcept
		{
			auto temp( *this );
			--( *this );
			return temp;
		}

		constexpr decimal_fixed_point& operator+=( const decimal_fixed_point value ) noexcept
		{
			CHECK_EXPRESSION_OVERFLOW( +value.m_underlying );
			m_underlying += value.m_underlying;
			return *this;
		}

		constexpr decimal_fixed_point& operator-=( const decimal_fixed_point value ) noexcept
		{
			CHECK_EXPRESSION_OVERFLOW( -value.m_underlying );
			m_underlying -= value.m_underlying;
			return *this;
		}

		constexpr decimal_fixed_point& operator*=( const decimal_fixed_point value ) noexcept
		{
			MCLO_ASSERT_ASSUME( std::in_range<underlying_type>( intermediate_type( truncated() ) *
													 intermediate_type( value.truncated() ) ) );
			const underlying_type integer_part = truncated() * value.truncated();
			const underlying_type decimal_part = decimals() * value.decimals();

			MCLO_ASSERT_ASSUME( std::in_range<underlying_type>( intermediate_type( integer_part ) +
													 intermediate_type( decimal_part ) ) );
			m_underlying = integer_part + decimal_part;
			return *this;
		}

		template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
		constexpr decimal_fixed_point& operator*=( const T value ) noexcept
		{
			CHECK_EXPRESSION_OVERFLOW( *value );
			m_underlying *= value;
			return *this;
		}

		/*
		 * 150.5 / 50.2 = 2.998007
		 * 150 / 50 = 3
		 */

		constexpr decimal_fixed_point& operator/=( const decimal_fixed_point value ) noexcept
		{
			CHECK_EXPRESSION_OVERFLOW( *( Resolution / value.m_underlying ) );
			m_underlying *= Resolution / value.m_underlying;
			return *this;
		}

		template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
		constexpr decimal_fixed_point& operator/=( const T value ) noexcept
		{
			MCLO_ASSERT_ASSUME( value != 0 );
			m_underlying /= value;
			return *this;
		}

#undef CHECK_EXPRESSION_OVERFLOW

#ifdef __cpp_impl_three_way_comparison
		constexpr auto operator<=>( const decimal_fixed_point& other ) const noexcept = default;
#endif

	private:
		constexpr underlying_type decimals() const noexcept
		{
			// End value is still in the range for the Resolution, eg: 0.5 -> 5 x Resolution
			return m_underlying - ( m_underlying / Resolution * Resolution );
		}

		struct from_underlying_tag
		{
		};

		decimal_fixed_point( from_underlying_tag, const underlying_type value ) noexcept
			: m_underlying( value )
		{
		}

		underlying_type m_underlying = 0;
	};

#ifndef __cpp_impl_three_way_comparison
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	constexpr bool operator==( const decimal_fixed_point<TUnderlying, DecimalsValue> lhs,
							   const decimal_fixed_point<TUnderlying, DecimalsValue> rhs ) noexcept
	{
		return lhs.get_underlying() == rhs.get_underlying();
	}
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	constexpr bool operator!=( const decimal_fixed_point<TUnderlying, DecimalsValue> lhs,
							   const decimal_fixed_point<TUnderlying, DecimalsValue> rhs ) noexcept
	{
		return lhs.get_underlying() != rhs.get_underlying();
	}
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	constexpr bool operator<( const decimal_fixed_point<TUnderlying, DecimalsValue> lhs,
							  const decimal_fixed_point<TUnderlying, DecimalsValue> rhs ) noexcept
	{
		return lhs.get_underlying() < rhs.get_underlying();
	}
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	constexpr bool operator<=( const decimal_fixed_point<TUnderlying, DecimalsValue> lhs,
							   const decimal_fixed_point<TUnderlying, DecimalsValue> rhs ) noexcept
	{
		return lhs.get_underlying() <= rhs.get_underlying();
	}
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	constexpr bool operator>( const decimal_fixed_point<TUnderlying, DecimalsValue> lhs,
							  const decimal_fixed_point<TUnderlying, DecimalsValue> rhs ) noexcept
	{
		return lhs.get_underlying() > rhs.get_underlying();
	}
	template <typename TUnderlying, std::uint8_t DecimalsValue>
	constexpr bool operator>=( const decimal_fixed_point<TUnderlying, DecimalsValue> lhs,
							   const decimal_fixed_point<TUnderlying, DecimalsValue> rhs ) noexcept
	{
		return lhs.get_underlying() >= rhs.get_underlying();
	}
#endif

	template <typename CharT, typename Traits, typename TUnderlying, std::uint8_t DecimalsValue>
	std::basic_ostream<CharT, Traits>& operator<<( std::basic_ostream<CharT, Traits>& os,
												   mclo::decimal_fixed_point<TUnderlying, DecimalsValue> num )
	{
		return os << static_cast<long double>( num );
	}

	template <typename CharT, typename Traits, typename TUnderlying, std::uint8_t DecimalsValue>
	std::basic_istream<CharT, Traits>& operator>>( std::basic_istream<CharT, Traits>& is,
												   mclo::decimal_fixed_point<TUnderlying, DecimalsValue>& num )
	{
		long double float_num;
		if ( is >> float_num )
		{
			num = float_num;
		}
		return is;
	}

	using fixed32 = decimal_fixed_point<std::int32_t, 3>;
	using fixed64 = decimal_fixed_point<std::int32_t, 5>;

	using fixedu32 = decimal_fixed_point<std::uint32_t, 3>;
	using fixedu64 = decimal_fixed_point<std::uint32_t, 5>;

	inline namespace literals
	{
		inline namespace fixed_point_literals
		{
			constexpr fixed32 operator"" _fixed32( unsigned long long int val )
			{
				return val;
			}
			constexpr fixed32 operator"" _fixed32( long double val )
			{
				return val;
			}
			constexpr fixed64 operator"" _fixed64( unsigned long long int val )
			{
				return val;
			}
			constexpr fixed64 operator"" _fixed64( long double val )
			{
				return val;
			}
			constexpr fixedu32 operator"" _fixedu32( unsigned long long int val )
			{
				return val;
			}
			constexpr fixedu32 operator"" _fixedu32( long double val )
			{
				return val;
			}
			constexpr fixedu64 operator"" _fixedu64( unsigned long long int val )
			{
				return val;
			}
			constexpr fixedu64 operator"" _fixedu64( long double val )
			{
				return val;
			}
		}
	}
}
