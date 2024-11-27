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

		template <std::unsigned_integral T>
		struct small_optional_storage<T>
		{
			static constexpr T max_value = std::numeric_limits<T>::max() - 1;

			[[nodiscard]] constexpr bool has_value() const noexcept
			{
				return m_value != 0;
			}

			[[nodiscard]] constexpr T get() const noexcept
			{
				return m_value - 1;
			}
			constexpr void set( const T value ) noexcept
			{
				assert( value <= max_value );
				m_value = value + 1;
			}

			T m_value = 0;
		};

		template <std::signed_integral T>
		struct small_optional_storage<T>
		{
			static constexpr T max_value = std::numeric_limits<T>::max() - 1;

			[[nodiscard]] constexpr bool has_value() const noexcept
			{
				return m_value != 0;
			}

			[[nodiscard]] constexpr T get() const noexcept
			{
				return m_value - static_cast<int>( m_value >= 0 );
			}
			constexpr void set( const T value ) noexcept
			{
				assert( value <= max_value );
				m_value = value + static_cast<int>( value >= 0 );
			}

			T m_value = 0;
		};

		template <>
		struct small_optional_storage<bool>
		{
			static constexpr bool max_value = std::numeric_limits<bool>::max();

			[[nodiscard]] constexpr bool has_value() const noexcept
			{
				return m_value != 0;
			}

			[[nodiscard]] constexpr bool get() const noexcept
			{
				return m_value - 1;
			}
			constexpr void set( const bool value ) noexcept
			{
				assert( value <= max_value );
				m_value = value + 1;
			}

			std::uint8_t m_value = 0;
		};

		template <std::floating_point Float, std::integral IntRep, IntRep QuietNaN>
		struct small_optional_float_storage
		{
			static_assert( sizeof( Float ) == sizeof( IntRep ) );
			static_assert( std::numeric_limits<Float>::is_iec559 );

			static constexpr IntRep invalid = QuietNaN;

			[[nodiscard]] constexpr bool has_value() const noexcept
			{
				return m_value != invalid;
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

			union
			{
				IntRep m_value = invalid;
				Float m_debugger_value;
			};
		};

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
		struct small_optional_storage<T>
		{
			static constexpr bool is_pointer_type = true; // For natvis
			static constexpr std::uintptr_t invalid = std::numeric_limits<std::uintptr_t>::max();

			[[nodiscard]] constexpr bool has_value() const noexcept
			{
				return m_value != invalid;
			}

			[[nodiscard]] T get() const noexcept
			{
				return reinterpret_cast<T>( m_value );
			}
			void set( const T value ) noexcept
			{
				m_value = reinterpret_cast<std::uintptr_t>( value );
			}

			std::uintptr_t m_value = invalid;
		};

		template <typename T>
			requires( std::is_enum_v<T> )
		struct small_optional_storage<T> : small_optional_storage<std::underlying_type_t<T>>
		{
			using underlying = std::underlying_type_t<T>;
			using base = small_optional_storage<underlying>;

			[[nodiscard]] T get() const noexcept
			{
				return static_cast<T>( base::get() );
			}
			void set( const T value ) noexcept
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
			: base{ std::exchange( other.m_value, 0 ) }
		{
		}
		constexpr small_optional& operator=( small_optional&& other ) noexcept
		{
			base::m_value = std::exchange( other.m_value, 0 );
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

		void reset() noexcept
		{
			base::m_value = 0;
		}
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
		return lhs.raw_value() == rhs.raw_value();
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
