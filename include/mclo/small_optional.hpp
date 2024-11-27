#pragma once

#include "platform.hpp"

#include <cassert>
#include <compare>
#include <concepts>
#include <functional>
#include <limits>
#include <optional>

namespace mclo
{
	namespace detail
	{
		template <typename T>
		struct small_optional_storage;

		enum class small_optional_natvis_type : std::uint8_t
		{
			integer,
			floating_point,
			unknown
		};

		template <std::integral T>
		struct small_optional_storage<T>
		{
			static constexpr small_optional_natvis_type natvis_type = small_optional_natvis_type::integer;

			// If signed just narrow the negative range because two's compliment gives negatives one extra value and the
			// uniformity makes me happy
			static constexpr T invalid =
				std::is_signed_v<T> ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();

			[[nodiscard]] constexpr bool has_value() const noexcept
			{
				return m_value != invalid;
			}
			constexpr void reset() noexcept
			{
				m_value = invalid;
			}

			[[nodiscard]] constexpr T get() const noexcept
			{
				return m_value;
			}
			constexpr void set( const T value ) noexcept
			{
				assert( value != invalid );
				m_value = value;
			}

			T m_value = invalid;
		};

		template <>
		struct small_optional_storage<bool> : small_optional_storage<std::uint8_t>
		{
			using base = small_optional_storage<std::uint8_t>;
			using base::natvis_type;
		};

		template <std::floating_point Float, std::integral IntRep, IntRep QuietNaN>
		struct small_optional_float_storage
		{
			static_assert( sizeof( Float ) == sizeof( IntRep ) );
			static_assert( std::numeric_limits<Float>::is_iec559 );

			static constexpr small_optional_natvis_type natvis_type = small_optional_natvis_type::floating_point;
			static constexpr IntRep invalid = QuietNaN;

			[[nodiscard]] constexpr bool has_value() const noexcept
			{
				return m_value != invalid;
			}
			constexpr void reset() noexcept
			{
				m_value = invalid;
			}

			[[nodiscard]] constexpr Float get() const noexcept
			{
				return std::bit_cast<Float>( m_value );
			}
			constexpr void set( const Float value ) noexcept
			{
				m_value = std::bit_cast<IntRep>( value );
				assert( m_value != invalid );
			}

			// We store as an integer so that we can compare against invalid without the NaNs are never equal kicking in
			union
			{
				IntRep m_value = invalid;
				Float m_debugger_value; // Debugger cannot bit_cast so we store but never use this member for it to read
			};
		};

		// These are unused NaN bit patterns
		template <>
		struct small_optional_storage<float> : small_optional_float_storage<float, std::int32_t, 0x7fedcba9>
		{
		};

		template <>
		struct small_optional_storage<double> : small_optional_float_storage<double, std::int64_t, 0x7ff8fedcba987654>
		{
		};

		template <>
		struct small_optional_storage<long double>
			: small_optional_float_storage<long double, std::int64_t, 0x7ff8fedcba987654>
		{
		};

		template <typename T>
			requires( std::is_pointer_v<T> )
		struct small_optional_storage<T> : small_optional_storage<std::uintptr_t>
		{
			// std::uintptr_t max is not a valid pointer value on any meaningful platform
			using base = small_optional_storage<std::uintptr_t>;

			[[nodiscard]] T get() const noexcept
			{
				return reinterpret_cast<T>( base::get() );
			}
			void set( const T value ) noexcept
			{
				base::set( reinterpret_cast<std::uintptr_t>( value ) );
			}
		};

		template <typename T>
			requires( std::is_enum_v<T> )
		struct small_optional_storage<T> : small_optional_storage<std::underlying_type_t<T>>
		{
			using underlying = std::underlying_type_t<T>;
			using base = small_optional_storage<underlying>;

			[[nodiscard]] constexpr T get() const noexcept
			{
				return static_cast<T>( base::get() );
			}
			constexpr void set( const T value ) noexcept
			{
				base::set( static_cast<underlying>( value ) );
			}
		};
	}

	template <typename T>
	class small_optional : private detail::small_optional_storage<T>
	{
		using base = detail::small_optional_storage<T>;

	public:
		constexpr small_optional() noexcept = default;

		constexpr small_optional( const small_optional& other ) noexcept = default;
		constexpr small_optional& operator=( const small_optional& other ) noexcept = default;

		constexpr small_optional( small_optional&& other ) noexcept
			: base{ other.m_value }
		{
			other.reset();
		}

		constexpr small_optional& operator=( small_optional&& other ) noexcept
		{
			base::m_value = other.m_value;
			other.reset();
			return *this;
		}

		constexpr small_optional( const T value ) noexcept
		{
			set( value );
		}
		constexpr small_optional& operator=( const T value ) noexcept
		{
			set( value );
			return *this;
		}

		constexpr small_optional( const std::nullopt_t ) noexcept
		{
		}
		constexpr small_optional& operator=( const std::nullopt_t ) noexcept
		{
			reset();
			return *this;
		}

		using base::has_value;

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return has_value();
		}

		[[nodiscard]] constexpr T value() const
		{
			if ( !has_value() )
			{
				throw std::bad_optional_access();
			}
			return base::get();
		}
		[[nodiscard]] T operator*() const noexcept
		{
			assert( has_value() );
			return base::get();
		}

		[[nodiscard]] T value_or( const T default_value ) const noexcept
		{
			return has_value() ? value() : default_value;
		}

		using base::reset;
		using base::set;

		void swap( small_optional& other ) noexcept
		{
			std::swap( base::m_value, other.m_value );
		}

		friend void swap( small_optional& lhs, small_optional& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		[[nodiscard]] T raw_value() const noexcept
		{
			return base::m_value;
		}
	};

	template <typename T, typename U>
	[[nodiscard]] constexpr bool operator==( const mclo::small_optional<T> lhs,
											 const mclo::small_optional<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs == *rhs;
		}

		return lhs_has_value == rhs_has_value;
	}

	template <typename T, typename U>
	[[nodiscard]] constexpr std::strong_ordering operator<=>( const mclo::small_optional<T> lhs,
															  const mclo::small_optional<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs <=> *rhs;
		}

		return lhs_has_value <=> rhs_has_value;
	}
}

template <typename T>
struct std::hash<mclo::small_optional<T>>
{
	[[nodiscard]] MCLO_STATIC_CALL_OPERATOR std::size_t operator()( const mclo::small_optional<T> value )
		MCLO_CONST_CALL_OPERATOR MCLO_NOEXCEPT_AND_BODY( std::hash<T>()( value.raw_value() ) )
};
