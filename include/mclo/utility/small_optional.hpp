#pragma once

#include "mclo/preprocessor/platform.hpp"

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
		enum class small_optional_natvis_type : std::uint8_t
		{
			unknown,
			integer,
			floating_point,
		};

		template <typename T>
		constexpr small_optional_natvis_type natvis_type_for = small_optional_natvis_type::unknown;

		template <typename T>
			requires requires { T::natvis_type; }
		constexpr small_optional_natvis_type natvis_type_for<T> = T::natvis_type;

		template <typename Storage, typename T>
		concept small_optional_storage_type = requires( Storage& storage, const Storage& c_storage, T value ) {
			{
				c_storage.has_value()
			} noexcept -> std::same_as<bool>;
			{
				storage.reset()
			} noexcept -> std::same_as<void>;
			{
				c_storage.get()
			} noexcept -> std::same_as<T>;
			{
				storage.set( value )
			} noexcept -> std::same_as<void>;
			requires std::copyable<Storage>;
		};
	}

	template <typename T>
	struct small_optional_storage;

	template <std::integral T>
	struct small_optional_storage<T>
	{
		static constexpr auto natvis_type = detail::small_optional_natvis_type::integer;

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

		[[nodiscard]] constexpr bool get() const noexcept
		{
			return m_value;
		}
		constexpr void set( const bool value ) noexcept
		{
			base::set( value );
		}
	};

	template <std::floating_point Float, std::integral IntRep, IntRep QuietNaN>
	struct small_optional_float_storage
	{
		static_assert( sizeof( Float ) == sizeof( IntRep ) );
		static_assert( std::numeric_limits<Float>::is_iec559 );

		static constexpr auto natvis_type = detail::small_optional_natvis_type::floating_point;
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

	template <typename CharT, typename Traits>
	struct small_optional_storage<std::basic_string_view<CharT, Traits>> : small_optional_storage<const CharT*>
	{
		using base = small_optional_storage<const CharT*>;
		using view = std::basic_string_view<CharT, Traits>;

		[[nodiscard]] view get() const noexcept
		{
			return { base::get(), m_size };
		}
		void set( const view value ) noexcept
		{
			base::set( value.data() );
			m_size = value.size();
		}

		typename view::size_type m_size = 0;
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

	template <typename T>
	class small_optional : private small_optional_storage<T>
	{
		using base = small_optional_storage<T>;
		static_assert( detail::small_optional_storage_type<base, T>,
					   "The specialization of small_optional_storage for T should model the requirements of "
					   "small_optional_storage_type" );

		static constexpr auto natvis_type = detail::natvis_type_for<base>;

		base& as_base() noexcept
		{
			return *this;
		}

	public:
		constexpr small_optional() noexcept = default;

		constexpr small_optional( const small_optional& other ) noexcept = default;
		constexpr small_optional& operator=( const small_optional& other ) noexcept = default;

		constexpr small_optional( small_optional&& other ) noexcept
			: base{ other.as_base() }
		{
			other.reset();
		}

		constexpr small_optional& operator=( small_optional&& other ) noexcept
		{
			as_base() = std::move( other.as_base() );
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
			std::swap( as_base(), other.as_base() );
		}

		friend void swap( small_optional& lhs, small_optional& rhs ) noexcept
		{
			lhs.swap( rhs );
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
		MCLO_CONST_CALL_OPERATOR MCLO_NOEXCEPT_AND_BODY( value ? std::hash<T>()( *value ) : 0 )
};
