#pragma once

#include "mclo/numeric/saturated_math.hpp"

#include <cinttypes>
#include <concepts>
#include <limits>

namespace mclo
{
	struct from_underlying_t
	{
		explicit from_underlying_t() = default;
	};
	inline constexpr from_underlying_t from_underlying{};

	/// @brief Represents a Float object as normalized between 0.0 and 1.0 using an Underlying unsigned integer,
	/// saturates input value and arithmetic to maintain nomralization invariant
	/// @tparam Intermediary unsigned_integral type used for multiplication and division to prevent overflow, must be
	/// larger type than Underlying
	/// @tparam Underlying unsigned_integral type used to store the representation of the normalized value
	/// @tparam Float floating_point type to be normalized, only digits that can fit into the Underlying are preserved
	/// in instances of 1 / Underlying::max
	template <std::floating_point Float, std::unsigned_integral Underlying, std::unsigned_integral Intermediary>
	class normalized_float
	{
		static_assert( sizeof( Intermediary ) > sizeof( Underlying ),
					   "Intermediary must be larger than underlying to fit a multiplication result" );

		static constexpr Underlying scale = std::numeric_limits<Underlying>::max();

		[[nodiscard]] static constexpr Float clamp( const Float value ) noexcept
		{
			return value < Float( 0.0 ) ? Float( 0.0 ) : value > Float( 1.0 ) ? Float( 1.0 ) : value;
		}

	public:
		using underlying_type = Underlying;

		constexpr normalized_float() noexcept = default;

		constexpr normalized_float( from_underlying_t, const underlying_type value ) noexcept
			: m_value( value )
		{
		}

		constexpr normalized_float( const Float value ) noexcept
			: m_value( static_cast<underlying_type>( clamp( value ) * scale ) )
		{
		}

		constexpr normalized_float& operator=( const Float value ) noexcept
		{
			m_value = static_cast<underlying_type>( clamp( value ) * scale );
			return *this;
		}

		constexpr normalized_float& operator=( const normalized_float& other ) noexcept = default;
		constexpr normalized_float( const normalized_float& other ) noexcept = default;

		[[nodiscard]] constexpr operator Float() const noexcept
		{
			return static_cast<Float>( m_value ) / scale;
		}

		[[nodiscard]] constexpr underlying_type underlying() const noexcept
		{
			return m_value;
		}

		[[nodiscard]] constexpr auto operator<=>( const normalized_float other ) const noexcept
		{
			return m_value <=> other.m_value;
		}

		[[nodiscard]] constexpr friend normalized_float operator+( const normalized_float lhs,
																   const normalized_float rhs ) noexcept
		{
			return { from_underlying, mclo::add_sat( lhs.m_value, rhs.m_value ) };
		}

		[[nodiscard]] constexpr friend normalized_float operator-( const normalized_float lhs,
																   const normalized_float rhs ) noexcept
		{
			return { from_underlying, mclo::sub_sat( lhs.m_value, rhs.m_value ) };
		}

		[[nodiscard]] constexpr friend normalized_float operator*( const normalized_float lhs,
																   const normalized_float rhs ) noexcept
		{
			// cast truncates overflow value back into saturated range directly
			const auto full = static_cast<Intermediary>( lhs.m_value ) * static_cast<Intermediary>( rhs.m_value );
			return { from_underlying, static_cast<Underlying>( full / scale ) };
		}

		[[nodiscard]] constexpr friend normalized_float operator*( const normalized_float lhs,
																   const Underlying rhs ) noexcept
		{
			// cast truncates overflow value back into saturated range directly
			const auto full = static_cast<Intermediary>( lhs.m_value ) * static_cast<Intermediary>( rhs );
			return { from_underlying, static_cast<Underlying>( full ) };
		}

		[[nodiscard]] constexpr friend normalized_float operator*( const Underlying lhs,
																   const normalized_float rhs ) noexcept
		{
			// cast truncates overflow value back into saturated range directly
			const auto full = static_cast<Intermediary>( lhs ) * static_cast<Intermediary>( rhs.m_value );
			return { from_underlying, static_cast<Underlying>( full ) };
		}

		[[nodiscard]] constexpr friend normalized_float operator/( const normalized_float lhs,
																   const normalized_float rhs ) noexcept
		{
			// If lhs > rhs then division will result in a multiplication so we must saturate back
			const auto full =
				static_cast<Intermediary>( lhs.m_value ) * scale / ( static_cast<Intermediary>( rhs.m_value ) );
			return { from_underlying, mclo::saturate_cast<Underlying>( full ) };
		}

		[[nodiscard]] constexpr friend normalized_float operator/( const normalized_float lhs,
																   const Underlying rhs ) noexcept
		{
			// Integer division truncates to zero and always results in a smaller value when rhs is an integer
			// so we do not need to handle any saturation manually
			// Cast is because of integer promotion rules
			return { from_underlying, static_cast<Underlying>( lhs.m_value / rhs ) };
		}

		[[nodiscard]] constexpr normalized_float& operator+=( const normalized_float other ) noexcept
		{
			*this = *this + other;
			return *this;
		}

		[[nodiscard]] constexpr normalized_float& operator-=( const normalized_float other ) noexcept
		{
			*this = *this - other;
			return *this;
		}

		[[nodiscard]] constexpr normalized_float& operator*=( const normalized_float other ) noexcept
		{
			*this = *this * other;
			return *this;
		}

		[[nodiscard]] constexpr normalized_float& operator/=( const normalized_float other ) noexcept
		{
			*this = *this / other;
			return *this;
		}

	private:
		underlying_type m_value = 0;
	};

	using normalized_float8 = normalized_float<float, std::uint8_t, std::uint16_t>;
	using normalized_float16 = normalized_float<double, std::uint16_t, std::uint32_t>;

	inline namespace literals
	{
		inline namespace normalized_float_literals
		{
			[[nodiscard]] constexpr normalized_float8 operator""_nf8( const long double value ) noexcept
			{
				return static_cast<float>( value );
			}

			[[nodiscard]] constexpr normalized_float8 operator""_nf8( const unsigned long long value ) noexcept
			{
				return static_cast<float>( value );
			}

			[[nodiscard]] constexpr normalized_float16 operator""_nf16( const long double value ) noexcept
			{
				return static_cast<float>( value );
			}

			[[nodiscard]] constexpr normalized_float16 operator""_nf16( const unsigned long long value ) noexcept
			{
				return static_cast<float>( value );
			}
		}
	}
}

template <std::floating_point Float, std::unsigned_integral Underlying, std::unsigned_integral Intermediary>
class std::numeric_limits<mclo::normalized_float<Float, Underlying, Intermediary>>
{
	using type = mclo::normalized_float<Float, Underlying, Intermediary>;
	using underlying_limits = std::numeric_limits<Underlying>;

public:
	static constexpr bool is_specialized = true;
	static constexpr bool is_signed = false;
	static constexpr bool is_integer = false;
	static constexpr bool is_exact = false;
	static constexpr bool has_infinity = false;
	static constexpr bool has_quiet_NaN = false;
	static constexpr bool has_signaling_NaN = false;
	static constexpr float_denorm_style has_denorm = std::denorm_absent;
	static constexpr bool has_denorm_loss = false;
	static constexpr std::float_round_style round_style = std::round_toward_zero;
	static constexpr bool is_iec559 = false;
	static constexpr bool is_bounded = true;
	static constexpr bool is_modulo = true;
	static constexpr int digits = underlying_limits::digits;
	static constexpr int digits10 = underlying_limits::digits10;
	static constexpr int max_digits10 = underlying_limits::max_digits10;
	static constexpr int radix = underlying_limits::radix;
	static constexpr int min_exponent = 0;
	static constexpr int min_exponent10 = 0;
	static constexpr int max_exponent = 0;
	static constexpr int max_exponent10 = 0;
	static constexpr bool traps = false;
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
		return type( mclo::from_underlying, Underlying( 1 ) );
	}

	static constexpr type round_error() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::round_error() );
	}

	static constexpr type infinity() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::infinity() );
	}

	static constexpr type quiet_NaN() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::quiet_NaN() );
	}

	static constexpr type signaling_NaN() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::signaling_NaN() );
	}

	static constexpr type denorm_min() noexcept
	{
		return type( mclo::from_underlying, underlying_limits::denorm_min() );
	}
};
