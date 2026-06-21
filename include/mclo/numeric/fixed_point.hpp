#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"
#include "mclo/numeric/128_bit_integer.hpp"
#include "mclo/numeric/standard_integer_type.hpp"
#include "mclo/utility/from_underlying.hpp"

#include <climits>
#include <concepts>
#include <limits>
#include <type_traits>

namespace mclo
{
	/// @brief Selects the radix of a fixed_point type's fractional scale, determining both its performance and which
	/// values it can represent exactly
	enum class fixed_point_base : std::uint8_t
	{
		/// @brief Scale is a power of two so the Fraction parameter counts fractional bits. Scaling is a bit shift,
		/// making arithmetic fast and bit-for-bit reproducible across platforms, but base ten fractions such as 0.1
		/// cannot be represented exactly. Prefer for performance sensitive or deterministic code such as simulation,
		/// physics and lockstep networking, where exact decimal values are not required.
		binary,

		/// @brief Scale is a power of ten so the Fraction parameter counts fractional decimal digits. Scaling needs a
		/// multiply or divide by a power of ten rather than a shift, but base ten fractions are represented exactly.
		/// Prefer for human facing or accounting values such as currency, percentages and displayed statistics, where
		/// exact decimal representation matters more than raw speed.
		decimal,
	};

	namespace detail
	{
		/// @brief Compute base raised to exp at compile time using only integer arithmetic
		template <typename T>
		[[nodiscard]] constexpr T fixed_point_int_pow( const T base, const int exp ) noexcept
		{
			T result = 1;
			for ( int i = 0; i < exp; ++i )
			{
				result *= base;
			}
			return result;
		}

		/// @brief Selects an integer type at least twice as wide as Rep, preserving signedness, used to hold the
		/// intermediary result of a multiplication or division without losing magnitude
		template <std::integral Rep>
		struct fixed_point_wider
		{
			static constexpr std::size_t bits = sizeof( Rep ) * CHAR_BIT * 2;
			using unsigned_type = std::conditional_t<( bits <= 64 ), uint_least_t<bits>, uint128_t>;
			using signed_type = std::conditional_t<( bits <= 64 ), int_least_t<bits>, int128_t>;
			using type = std::conditional_t<std::is_signed_v<Rep>, signed_type, unsigned_type>;
		};

		template <std::integral Rep>
		using fixed_point_wider_t = typename fixed_point_wider<Rep>::type;
	}

	/// @brief A fixed point arithmetic type storing a real number as an integer scaled by a power of the chosen base
	///
	/// @details The stored integer equals @c real_value * scale where @c scale is @c base^Fraction and @c base is 2 for
	/// fixed_point_base::binary or 10 for fixed_point_base::decimal. See @ref fixed_point_base for guidance on choosing
	/// a base. Arithmetic wraps on overflow to match built in integer behaviour and truncates toward zero when
	/// precision is lost, matching integer division. Conversions to and from a real value are explicit because scaling
	/// into the representation is lossy and, for a binary base, implicitly converting out to a floating point would
	/// forfeit the determinism the type exists to provide.
	///
	/// @tparam Rep Standard integer type used to store the scaled representation, may be signed or unsigned
	/// @tparam Fraction Number of fractional bits (binary) or fractional digits (decimal) reserved within Rep
	/// @tparam Base Radix of the fractional scale, see @ref fixed_point_base
	template <std::integral Rep, int Fraction, fixed_point_base Base = fixed_point_base::binary>
	class fixed_point
	{
		static_assert( Fraction >= 0, "Fraction must be non-negative" );

		static constexpr int radix = Base == fixed_point_base::binary ? 2 : 10;

		using intermediary_type = detail::fixed_point_wider_t<Rep>;
		using unsigned_type = std::make_unsigned_t<Rep>;

		static_assert( detail::fixed_point_int_pow<intermediary_type>( radix, Fraction ) <=
						   static_cast<intermediary_type>( std::numeric_limits<Rep>::max() ),
					   "The scale base^Fraction overflows the representation type Rep" );

	public:
		using underlying_type = Rep;

		/// @brief The factor base^Fraction by which a real value is multiplied to form the stored representation
		/// @details Computed in intermediary_type, which is at least twice as wide as Rep, so the running product never
		/// overflows while building up base^Fraction; the result is asserted to fit Rep above.
		static constexpr underlying_type scale =
			static_cast<underlying_type>( detail::fixed_point_int_pow<intermediary_type>( radix, Fraction ) );

		constexpr fixed_point() noexcept = default;

		/// @brief Construct directly from a raw scaled representation without applying any scaling
		constexpr fixed_point( from_underlying_t, const underlying_type value ) noexcept
			: m_value( value )
		{
		}

		/// @brief Construct from a whole integer value, scaling it into the representation
		template <std::integral Int>
		constexpr explicit fixed_point( const Int value ) noexcept
			: m_value( static_cast<Rep>( static_cast<Rep>( value ) * scale ) )
		{
		}

		/// @brief Construct from a real floating point value, scaling and truncating toward zero. The caller is
		/// responsible for passing a value whose scaled magnitude fits within Rep.
		template <std::floating_point Float>
		constexpr explicit fixed_point( const Float value ) noexcept
			: m_value( static_cast<Rep>( value * static_cast<Float>( scale ) ) )
		{
		}

		/// @brief Convert from another fixed_point configuration, rescaling the stored value into this type
		///
		/// @details Rescales using integer arithmetic in a 128 bit intermediary so no precision is lost beyond the
		/// unavoidable truncation toward zero when this type has fewer fractional positions. Explicit because the
		/// conversion is generally lossy; prefer it over routing through a floating point value so binary conversions
		/// stay exact and deterministic.
		template <std::integral Rep2, int Fraction2, fixed_point_base Base2>
		constexpr explicit fixed_point( const fixed_point<Rep2, Fraction2, Base2> other ) noexcept
		{
			using wide = std::conditional_t<std::is_signed_v<Rep> || std::is_signed_v<Rep2>, int128_t, uint128_t>;
			const wide scaled = static_cast<wide>( other.underlying() ) * static_cast<wide>( scale );
			m_value = static_cast<Rep>( scaled / static_cast<wide>( decltype( other )::scale ) );
		}

		/// @brief Convert back to a real floating point value
		template <std::floating_point Float>
		[[nodiscard]] constexpr explicit operator Float() const noexcept
		{
			return static_cast<Float>( m_value ) / static_cast<Float>( scale );
		}

		/// @brief Access the raw scaled representation
		[[nodiscard]] constexpr underlying_type underlying() const noexcept
		{
			return m_value;
		}

		/// @brief The whole number portion, truncated toward zero
		[[nodiscard]] constexpr underlying_type integer_part() const noexcept
		{
			return static_cast<Rep>( m_value / scale );
		}

		/// @brief The fractional portion as a raw scaled representation, keeping the sign of the value
		///
		/// @details For a decimal base this is directly the fractional digits, while for a binary base it is the
		/// remainder measured in units of one over the scaling factor.
		[[nodiscard]] constexpr underlying_type fractional_part() const noexcept
		{
			return static_cast<Rep>( m_value % scale );
		}

		[[nodiscard]] constexpr auto operator<=>( const fixed_point& other ) const noexcept = default;
		[[nodiscard]] constexpr bool operator==( const fixed_point& other ) const noexcept = default;

		[[nodiscard]] constexpr fixed_point operator-() const noexcept
			requires( std::is_signed_v<Rep> )
		{
			return { from_underlying, static_cast<Rep>( unsigned_type( 0 ) - static_cast<unsigned_type>( m_value ) ) };
		}

		[[nodiscard]] constexpr friend fixed_point operator+( const fixed_point lhs, const fixed_point rhs ) noexcept
		{
			return { from_underlying,
					 static_cast<Rep>( static_cast<unsigned_type>( lhs.m_value ) +
									   static_cast<unsigned_type>( rhs.m_value ) ) };
		}

		[[nodiscard]] constexpr friend fixed_point operator-( const fixed_point lhs, const fixed_point rhs ) noexcept
		{
			return { from_underlying,
					 static_cast<Rep>( static_cast<unsigned_type>( lhs.m_value ) -
									   static_cast<unsigned_type>( rhs.m_value ) ) };
		}

		[[nodiscard]] constexpr friend fixed_point operator*( const fixed_point lhs, const fixed_point rhs ) noexcept
		{
			const intermediary_type full =
				static_cast<intermediary_type>( lhs.m_value ) * static_cast<intermediary_type>( rhs.m_value );
			return { from_underlying, static_cast<Rep>( full / scale ) };
		}

		[[nodiscard]] constexpr friend fixed_point operator/( const fixed_point lhs,
															  const fixed_point rhs ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( rhs.m_value != 0, "Division by zero" );
			const intermediary_type full =
				static_cast<intermediary_type>( lhs.m_value ) * static_cast<intermediary_type>( scale );
			return { from_underlying, static_cast<Rep>( full / static_cast<intermediary_type>( rhs.m_value ) ) };
		}

		[[nodiscard]] constexpr friend fixed_point operator*( const fixed_point lhs, const Rep rhs ) noexcept
		{
			return {
				from_underlying,
				static_cast<Rep>( static_cast<unsigned_type>( lhs.m_value ) * static_cast<unsigned_type>( rhs ) ) };
		}

		[[nodiscard]] constexpr friend fixed_point operator*( const Rep lhs, const fixed_point rhs ) noexcept
		{
			return rhs * lhs;
		}

		[[nodiscard]] constexpr friend fixed_point operator/( const fixed_point lhs, const Rep rhs ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( rhs != 0, "Division by zero" );
			return { from_underlying, static_cast<Rep>( lhs.m_value / rhs ) };
		}

		constexpr fixed_point& operator+=( const fixed_point other ) noexcept
		{
			*this = *this + other;
			return *this;
		}

		constexpr fixed_point& operator-=( const fixed_point other ) noexcept
		{
			*this = *this - other;
			return *this;
		}

		constexpr fixed_point& operator*=( const fixed_point other ) noexcept
		{
			*this = *this * other;
			return *this;
		}

		constexpr fixed_point& operator*=( const Rep other ) noexcept
		{
			*this = *this * other;
			return *this;
		}

		constexpr fixed_point& operator/=( const fixed_point other ) MCLO_NOEXCEPT_TESTS
		{
			*this = *this / other;
			return *this;
		}

		constexpr fixed_point& operator/=( const Rep other ) MCLO_NOEXCEPT_TESTS
		{
			*this = *this / other;
			return *this;
		}

		/// @brief Increment by one whole unit
		constexpr fixed_point& operator++() noexcept
		{
			m_value = static_cast<Rep>( static_cast<unsigned_type>( m_value ) + static_cast<unsigned_type>( scale ) );
			return *this;
		}

		/// @brief Increment by one whole unit, returning the value before the change
		constexpr fixed_point operator++( int ) noexcept
		{
			const fixed_point old = *this;
			++*this;
			return old;
		}

		/// @brief Decrement by one whole unit
		constexpr fixed_point& operator--() noexcept
		{
			m_value = static_cast<Rep>( static_cast<unsigned_type>( m_value ) - static_cast<unsigned_type>( scale ) );
			return *this;
		}

		/// @brief Decrement by one whole unit, returning the value before the change
		constexpr fixed_point operator--( int ) noexcept
		{
			const fixed_point old = *this;
			--*this;
			return old;
		}

		/// @brief Absolute value, found by ADL to act as a customization point for std::abs style usage
		[[nodiscard]] constexpr friend fixed_point abs( const fixed_point value ) noexcept
			requires( std::is_signed_v<Rep> )
		{
			return value.m_value < 0 ? -value : value;
		}

		/// @brief Largest whole value not greater than value, rounding toward negative infinity
		[[nodiscard]] constexpr friend fixed_point floor( const fixed_point value ) noexcept
		{
			Rep whole = static_cast<Rep>( value.m_value / scale );
			if constexpr ( std::is_signed_v<Rep> )
			{
				if ( value.m_value % scale != 0 && value.m_value < 0 )
				{
					--whole;
				}
			}
			return { from_underlying, static_cast<Rep>( whole * scale ) };
		}

		/// @brief Smallest whole value not less than value, rounding toward positive infinity
		[[nodiscard]] constexpr friend fixed_point ceil( const fixed_point value ) noexcept
		{
			Rep whole = static_cast<Rep>( value.m_value / scale );
			if ( value.m_value % scale != 0 && value.m_value > 0 )
			{
				++whole;
			}
			return { from_underlying, static_cast<Rep>( whole * scale ) };
		}

		/// @brief Whole value with the fractional part discarded, rounding toward zero
		[[nodiscard]] constexpr friend fixed_point trunc( const fixed_point value ) noexcept
		{
			return { from_underlying, static_cast<Rep>( ( value.m_value / scale ) * scale ) };
		}

		/// @brief Nearest whole value, rounding halves away from zero
		[[nodiscard]] constexpr friend fixed_point round( const fixed_point value ) noexcept
		{
			constexpr Rep half = static_cast<Rep>( scale / 2 );
			Rep biased;
			if constexpr ( std::is_signed_v<Rep> )
			{
				biased = static_cast<Rep>( value.m_value >= 0 ? value.m_value + half : value.m_value - half );
			}
			else
			{
				biased = static_cast<Rep>( value.m_value + half );
			}
			return { from_underlying, static_cast<Rep>( ( biased / scale ) * scale ) };
		}

		/// @brief Fractional part, equal to value - trunc( value ), keeping the sign of value
		[[nodiscard]] constexpr friend fixed_point frac( const fixed_point value ) noexcept
		{
			return { from_underlying, static_cast<Rep>( value.m_value % scale ) };
		}

		/// @brief Linear interpolation, a + ( b - a ) * t, with t typically in the range zero to one
		[[nodiscard]] constexpr friend fixed_point lerp( const fixed_point a,
														 const fixed_point b,
														 const fixed_point t ) noexcept
		{
			return a + ( b - a ) * t;
		}

		/// @brief Clamp value into the inclusive range [ low, high ]
		[[nodiscard]] constexpr friend fixed_point clamp( const fixed_point value,
														  const fixed_point low,
														  const fixed_point high ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !( high < low ), "clamp: low must not be greater than high" );
			return value < low ? low : high < value ? high : value;
		}

		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const fixed_point value ) noexcept
		{
			hash_append( hasher, value.m_value );
		}

	private:
		Rep m_value = 0;
	};

	/// @brief Binary fixed point with 8 fractional bits stored in 16 bits (Q8.8)
	using fixed_b16 = fixed_point<std::int16_t, 8>;
	/// @brief Binary fixed point with 16 fractional bits stored in 32 bits (Q16.16)
	using fixed_b32 = fixed_point<std::int32_t, 16>;
	/// @brief Binary fixed point with 32 fractional bits stored in 64 bits (Q32.32)
	using fixed_b64 = fixed_point<std::int64_t, 32>;

	/// @brief Decimal fixed point with 3 fractional digits stored in 32 bits, leaving an integer range of about +/- 2.1
	/// million
	using fixed_d32 = fixed_point<std::int32_t, 3, fixed_point_base::decimal>;
	/// @brief Decimal fixed point with 9 fractional digits stored in 64 bits
	using fixed_d64 = fixed_point<std::int64_t, 9, fixed_point_base::decimal>;

	inline namespace literals
	{
		inline namespace fixed_point_literals
		{
			/// @brief Create a fixed_b16 from a floating point literal
			[[nodiscard]] constexpr fixed_b16 operator""_fb16( const long double value ) noexcept
			{
				return fixed_b16( static_cast<double>( value ) );
			}
			/// @brief Create a fixed_b16 from an integer literal
			[[nodiscard]] constexpr fixed_b16 operator""_fb16( const unsigned long long value ) noexcept
			{
				return fixed_b16( static_cast<std::int16_t>( value ) );
			}

			/// @brief Create a fixed_b32 from a floating point literal
			[[nodiscard]] constexpr fixed_b32 operator""_fb32( const long double value ) noexcept
			{
				return fixed_b32( static_cast<double>( value ) );
			}
			/// @brief Create a fixed_b32 from an integer literal
			[[nodiscard]] constexpr fixed_b32 operator""_fb32( const unsigned long long value ) noexcept
			{
				return fixed_b32( static_cast<std::int32_t>( value ) );
			}

			/// @brief Create a fixed_b64 from a floating point literal
			[[nodiscard]] constexpr fixed_b64 operator""_fb64( const long double value ) noexcept
			{
				return fixed_b64( static_cast<double>( value ) );
			}
			/// @brief Create a fixed_b64 from an integer literal
			[[nodiscard]] constexpr fixed_b64 operator""_fb64( const unsigned long long value ) noexcept
			{
				return fixed_b64( static_cast<std::int64_t>( value ) );
			}

			/// @brief Create a fixed_d32 from a floating point literal
			[[nodiscard]] constexpr fixed_d32 operator""_fd32( const long double value ) noexcept
			{
				return fixed_d32( static_cast<double>( value ) );
			}
			/// @brief Create a fixed_d32 from an integer literal
			[[nodiscard]] constexpr fixed_d32 operator""_fd32( const unsigned long long value ) noexcept
			{
				return fixed_d32( static_cast<std::int32_t>( value ) );
			}

			/// @brief Create a fixed_d64 from a floating point literal
			[[nodiscard]] constexpr fixed_d64 operator""_fd64( const long double value ) noexcept
			{
				return fixed_d64( static_cast<double>( value ) );
			}
			/// @brief Create a fixed_d64 from an integer literal
			[[nodiscard]] constexpr fixed_d64 operator""_fd64( const unsigned long long value ) noexcept
			{
				return fixed_d64( static_cast<std::int64_t>( value ) );
			}
		}
	}
}

template <std::integral Rep, int Fraction, mclo::fixed_point_base Base>
class std::numeric_limits<mclo::fixed_point<Rep, Fraction, Base>>
{
	using type = mclo::fixed_point<Rep, Fraction, Base>;
	using underlying_limits = std::numeric_limits<Rep>;

public:
	static constexpr bool is_specialized = true;
	static constexpr bool is_signed = underlying_limits::is_signed;
	static constexpr bool is_integer = false;
	static constexpr bool is_exact = false;
	static constexpr bool has_infinity = false;
	static constexpr bool has_quiet_NaN = false;
	static constexpr bool has_signaling_NaN = false;
	static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
	static constexpr bool has_denorm_loss = false;
	static constexpr std::float_round_style round_style = std::round_toward_zero;
	static constexpr bool is_iec559 = false;
	static constexpr bool is_bounded = true;
	static constexpr bool is_modulo = true;
	static constexpr int digits = underlying_limits::digits;
	static constexpr int digits10 = underlying_limits::digits10;
	static constexpr int max_digits10 = underlying_limits::max_digits10;
	static constexpr int radix = Base == mclo::fixed_point_base::binary ? 2 : 10;
	static constexpr int min_exponent = 0;
	static constexpr int min_exponent10 = 0;
	static constexpr int max_exponent = 0;
	static constexpr int max_exponent10 = 0;
	static constexpr bool traps = underlying_limits::traps;
	static constexpr bool tinyness_before = false;

	static constexpr type min() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::min() );
	}

	static constexpr type lowest() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::lowest() );
	}

	static constexpr type max() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::max() );
	}

	static constexpr type epsilon() noexcept
	{
		return type( mclo::from_underlying, Rep( 1 ) );
	}

	static constexpr type round_error() noexcept
	{
		return type( mclo::from_underlying, Rep( 0 ) );
	}
};

template <std::integral Rep, int Fraction, mclo::fixed_point_base Base>
struct std::hash<mclo::fixed_point<Rep, Fraction, Base>> : mclo::hash<mclo::fixed_point<Rep, Fraction, Base>>
{
};
