#pragma once

#include "mclo/concepts/derived_from.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash.hpp"
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
			{ c_storage.has_value() } noexcept -> std::same_as<bool>;
			{ storage.reset() } noexcept -> std::same_as<void>;
			{ c_storage.get() } noexcept -> std::same_as<T>;
			{ storage.set( value ) } noexcept -> std::same_as<void>;
			requires std::copyable<Storage>;
		};
	}

	/// @brief Customization point defining how a value type is stored inside a @ref small_optional.
	/// @details A specialisation must reserve one bit pattern of @p T as the empty sentinel and provide the storage
	/// interface used by @ref small_optional: a @c has_value query, a @c reset that sets the sentinel, a @c get that
	/// returns the stored value, and a @c set that stores a value. Specialisations are provided for integral, boolean,
	/// floating point, pointer, enum and @c std::string_view types; specialise it for your own type to make it usable
	/// with @ref small_optional.
	/// @tparam T The value type to store.
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
			DEBUG_ASSERT( value != invalid, "Value is invalid sentinel" );
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
			DEBUG_ASSERT( m_value != invalid, "Value is invalid sentinel" );
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

	/// @brief A space efficient optional that encodes its empty state inside the value using a sentinel.
	/// @details Unlike @c std::optional, this stores no separate boolean flag. Instead it reserves an otherwise
	/// unused bit pattern of the value type as the empty sentinel, so @c sizeof(small_optional<T>) equals
	/// @c sizeof(T). How the sentinel is chosen is controlled by @ref small_optional_storage, which can be
	/// specialised for custom types. The interface mirrors @c std::optional but the value is held by value and
	/// returned by value rather than by reference.
	/// @tparam T The value type held by the optional.
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
		/// @brief Constructs an empty optional.
		constexpr small_optional() noexcept = default;

		constexpr small_optional( const small_optional& other ) noexcept = default;
		constexpr small_optional& operator=( const small_optional& other ) noexcept = default;

		/// @brief Move constructs from another optional, leaving the source empty.
		/// @param other The optional to move from.
		constexpr small_optional( small_optional&& other ) noexcept
			: base{ other.as_base() }
		{
			other.reset();
		}

		/// @brief Move assigns from another optional, leaving the source empty.
		/// @param other The optional to move from.
		/// @return A reference to this optional.
		constexpr small_optional& operator=( small_optional&& other ) noexcept
		{
			as_base() = std::move( other.as_base() );
			other.reset();
			return *this;
		}

		/// @brief Constructs an optional holding a value.
		/// @param value The value to store. Must not equal the sentinel bit pattern.
		constexpr small_optional( const T value ) noexcept
		{
			set( value );
		}

		/// @brief Assigns a value to the optional.
		/// @param value The value to store. Must not equal the sentinel bit pattern.
		/// @return A reference to this optional.
		constexpr small_optional& operator=( const T value ) noexcept
		{
			set( value );
			return *this;
		}

		/// @brief Constructs an empty optional from @c std::nullopt.
		constexpr small_optional( const std::nullopt_t ) noexcept
		{
		}

		/// @brief Resets the optional to empty.
		/// @return A reference to this optional.
		constexpr small_optional& operator=( const std::nullopt_t ) noexcept
		{
			reset();
			return *this;
		}

		using base::has_value;

		/// @brief Checks whether the optional holds a value.
		/// @return @c true if a value is held, otherwise @c false.
		[[nodiscard]] explicit operator bool() const noexcept
		{
			return has_value();
		}

		/// @brief Returns the held value, throwing if empty.
		/// @return The held value.
		/// @throws std::bad_optional_access If the optional is empty.
		[[nodiscard]] constexpr T value() const
		{
			if ( !has_value() )
			{
				throw std::bad_optional_access();
			}
			return base::get();
		}

		/// @brief Returns the held value without checking.
		/// @return The held value.
		/// @pre The optional must hold a value.
		[[nodiscard]] T operator*() const noexcept
		{
			DEBUG_ASSERT( has_value(), "Optional has no value" );
			return base::get();
		}

		/// @brief Returns the held value, or a default if empty.
		/// @param default_value The value to return if the optional is empty.
		/// @return The held value if present, otherwise @p default_value.
		[[nodiscard]] T value_or( const T default_value ) const noexcept
		{
			return has_value() ? value() : default_value;
		}

		using base::reset;
		using base::set;

		/// @brief Swaps the contents of two optionals.
		/// @param other The optional to swap with.
		void swap( small_optional& other ) noexcept
		{
			std::swap( as_base(), other.as_base() );
		}

		/// @brief Swaps the contents of two optionals.
		/// @param lhs The first optional.
		/// @param rhs The second optional.
		friend void swap( small_optional& lhs, small_optional& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

		/// @brief Appends the optional to a hasher, hashing the value if present.
		/// @param hasher The hasher to append to.
		/// @param value The optional to hash.
		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const small_optional& value ) noexcept
		{
			if ( value )
			{
				hash_append( hasher, *value );
			}
		}
	};

	/// @brief Compares two optionals for equality.
	/// @details Two empty optionals are equal, an empty and a non-empty optional are unequal, and two non-empty
	/// optionals are equal if their values are equal.
	/// @return @c true if the optionals are equal.
	template <typename T, std::equality_comparable_with<T> U>
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

	/// @brief Three way compares two optionals.
	/// @details An empty optional orders before any non-empty optional; two non-empty optionals compare by value.
	/// @return The ordering of @p lhs relative to @p rhs.
	template <typename T, std::three_way_comparable_with<T> U>
	[[nodiscard]] constexpr std::compare_three_way_result_t<T, U> operator<=>(
		const mclo::small_optional<T> lhs, const mclo::small_optional<U> rhs ) noexcept
	{
		const bool lhs_has_value = lhs.has_value();
		const bool rhs_has_value = rhs.has_value();
		if ( lhs_has_value && rhs_has_value )
		{
			return *lhs <=> *rhs;
		}

		return lhs_has_value <=> rhs_has_value;
	}

	/// @brief Checks whether an optional is empty by comparing against @c std::nullopt.
	/// @return @c true if the optional is empty.
	template <typename T>
	[[nodiscard]] constexpr bool operator==( const mclo::small_optional<T> lhs, const std::nullopt_t ) noexcept
	{
		return !lhs.has_value();
	}

	/// @brief Three way compares an optional against @c std::nullopt.
	/// @details An empty optional is equivalent to @c std::nullopt; a non-empty optional orders after it.
	/// @return The ordering of @p lhs relative to @c std::nullopt.
	template <typename T>
	[[nodiscard]] constexpr std::strong_ordering operator<=>( const mclo::small_optional<T> lhs,
															  const std::nullopt_t rhs ) noexcept
	{
		return lhs.has_value() <=> false;
	}

	/// @brief Compares an optional against a bare value for equality.
	/// @details An empty optional is never equal to a value; a non-empty optional is equal if its value compares
	/// equal to @p rhs.
	/// @return @c true if the optional holds a value equal to @p rhs.
	template <typename T, typename U>
		requires( !mclo::derived_from_specialization<U, mclo::small_optional> && std::equality_comparable_with<T, U> )
	[[nodiscard]] constexpr bool operator==( const mclo::small_optional<T> lhs, const U& rhs ) noexcept
	{
		if ( lhs )
		{
			return *lhs == rhs;
		}
		return false;
	}

	/// @brief Three way compares an optional against a bare value.
	/// @details An empty optional orders before any value; a non-empty optional compares its value against @p rhs.
	/// @return The ordering of @p lhs relative to @p rhs.
	template <typename T, typename U>
		requires( !mclo::derived_from_specialization<U, mclo::small_optional> && std::three_way_comparable_with<T, U> )
	[[nodiscard]] constexpr std::compare_three_way_result_t<T, U> operator<=>( const mclo::small_optional<T> lhs,
																			   const U& rhs ) noexcept
	{
		if ( lhs )
		{
			return *lhs == rhs;
		}
		return std::strong_ordering::less;
	}
}

template <typename T>
struct std::hash<mclo::small_optional<T>> : mclo::hash<mclo::small_optional<T>>
{
};
