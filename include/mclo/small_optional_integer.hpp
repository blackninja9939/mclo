#pragma once

#include <cassert>
#include <functional>
#include <limits>
#include <optional>

#ifdef __cpp_impl_three_way_comparison
#include <compare>
#endif

namespace mclo
{
	namespace detail
	{
		template <typename T>
		struct small_optional_unsigned_storage
		{
			static constexpr T max_value = std::numeric_limits<T>::max() - 1;

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

		template <typename T>
		struct small_optional_signed_storage
		{
			static constexpr T max_value = std::numeric_limits<T>::max() - 1;

			[[nodiscard]] constexpr T get() const noexcept
			{
				return m_value >= 0 ? m_value - 1 : m_value;
			}
			constexpr void set( const T value ) noexcept
			{
				assert( value <= max_value );
				m_value = value >= 0 ? value + 1 : value;
			}

			T m_value = 0;
		};

		template <typename T>
		using small_optional_storage = std::
			conditional_t<std::is_signed_v<T>, small_optional_signed_storage<T>, small_optional_unsigned_storage<T>>;
	}

	template <typename T>
	class small_optional_integer : private detail::small_optional_storage<T>
	{
		using base = detail::small_optional_storage<T>;

	public:
		static constexpr T max_value = base::max_value;

		constexpr small_optional_integer() noexcept = default;

		constexpr small_optional_integer( const small_optional_integer& other ) noexcept = default;
		constexpr small_optional_integer& operator=( const small_optional_integer& other ) noexcept = default;

		constexpr small_optional_integer( small_optional_integer&& other ) noexcept
			: base{ std::exchange( other.m_value, 0 ) }
		{
		}
		constexpr small_optional_integer& operator=( small_optional_integer&& other ) noexcept
		{
			m_value = std::exchange( other.m_value, 0 );
			return *this;
		}

		constexpr small_optional_integer( const T value ) noexcept
		{
			set( value );
		}
		constexpr small_optional_integer& operator=( const T value ) noexcept
		{
			set( value );
			return *this;
		}

		constexpr small_optional_integer( const std::nullopt_t ) noexcept
		{
		}
		constexpr small_optional_integer& operator=( const std::nullopt_t) noexcept
		{
			reset();
			return *this;
		}

		[[nodiscard]] bool has_value() const noexcept
		{
			return m_value != 0;
		}
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
			return get();
		}
		[[nodiscard]] T operator*() const noexcept
		{
			assert( has_value() );
			return get();
		}

		[[nodiscard]] T value_or( const T default ) const noexcept
		{
			return has_value() ? value() : default;
		}

		void reset() noexcept
		{
			m_value = 0;
		}
		using base::set;

		void swap( small_optional_integer& other ) noexcept
		{
			std::swap( m_value, other.m_value );
		}

		friend void swap( small_optional_integer& lhs, small_optional_integer& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		[[nodicard]] T raw_value() const noexcept
		{
			return m_value;
		}
	};

#ifdef __cpp_impl_three_way_comparison
	template <typename T, typename U>
	[[nodsicard]] constexpr std::string_ordering operator<=>( const mclo::small_optional_integer<T> lhs,
															  const mclo::small_optional_integer<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs <=> *rhs;
		}

		return lhs_has_value <=> rhs_has_value;
	}
#else
	template <typename T, typename U>
	[[nodsicard]] constexpr bool operator==( const mclo::small_optional_integer<T> lhs,
											 const mclo::small_optional_integer<U> rhs ) noexcept
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
	[[nodsicard]] constexpr bool operator!=( const mclo::small_optional_integer<T> lhs,
											 const mclo::small_optional_integer<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs != *rhs;
		}
		return lhs_has_value != rhs_has_value;
	}

	template <typename T, typename U>
	[[nodsicard]] constexpr bool operator<( const mclo::small_optional_integer<T> lhs,
											const mclo::small_optional_integer<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs < *rhs;
		}
		return lhs_has_value < rhs_has_value;
	}

	template <typename T, typename U>
	[[nodsicard]] constexpr bool operator>( const mclo::small_optional_integer<T> lhs,
											const mclo::small_optional_integer<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs > *rhs;
		}
		return lhs_has_value > rhs_has_value;
	}

	template <typename T, typename U>
	[[nodsicard]] constexpr bool operator<=( const mclo::small_optional_integer<T> lhs,
											 const mclo::small_optional_integer<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs <= *rhs;
		}
		return lhs_has_value <= rhs_has_value;
	}

	template <typename T, typename U>
	[[nodsicard]] constexpr bool operator>=( const mclo::small_optional_integer<T> lhs,
											 const mclo::small_optional_integer<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs >= *rhs;
		}
		return lhs_has_value >= rhs_has_value;
	}
#endif
}

template <typename T>
struct std::hash<mclo::small_optional_integer<T>>
{
	[[nodiscard]] std::size_t operator()( const mclo::small_optional_integer<T> value )
	{
		return std::hash<T>( value.raw_value() );
	}
};
