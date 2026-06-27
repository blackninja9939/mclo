#pragma once

#include <version>

#ifdef __cpp_lib_expected

#include <expected>

namespace mclo
{
	/// @brief Exception thrown when the value or error of an expected is accessed incorrectly.
	using std::bad_expected_access;

	/// @brief A type that holds either an expected value of type @c T or an unexpected error of type @c E.
	using std::expected;

	/// @brief Tag value selecting in-place construction of the unexpected error of an expected.
	using std::unexpect;

	/// @brief Tag type of @c unexpect.
	using std::unexpect_t;

	/// @brief A wrapper representing an unexpected error value of type @c E.
	using std::unexpected;
}

#else

#include "mclo/concepts/specialization_of.hpp"
#include "mclo/debug/assert.hpp"

#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename E>
	class [[nodiscard]] unexpected;

	template <typename T, typename E>
	class [[nodiscard]] expected;

	namespace detail
	{
		template <typename T>
		constexpr bool is_unexpected_specialization = mclo::is_specialization_of_v<T, unexpected>;

		template <typename T>
		constexpr bool is_expected_specialization = mclo::is_specialization_of_v<T, expected>;

		template <typename T>
		concept trivially_copy_assignable_expected =
			std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T> &&
			std::is_trivially_destructible_v<T>;

		template <typename T>
		concept trivially_move_assignable_expected =
			std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T> &&
			std::is_trivially_destructible_v<T>;

		template <typename T, typename E>
		concept copy_assignable_expected =
			std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T> && std::is_copy_assignable_v<E> &&
			std::is_copy_constructible_v<E> &&
			( std::is_nothrow_move_constructible_v<T> || std::is_nothrow_move_constructible_v<E> );

		template <typename E>
		concept copy_assignable_expected_void = std::is_copy_assignable_v<E> && std::is_copy_constructible_v<E>;

		template <typename T, typename E>
		concept move_assignable_expected =
			std::is_move_assignable_v<T> && std::is_move_constructible_v<T> && std::is_move_assignable_v<E> &&
			std::is_move_constructible_v<E> &&
			( std::is_nothrow_move_constructible_v<T> || std::is_nothrow_move_constructible_v<E> );

		template <typename E>
		concept move_assignable_expected_void = std::is_move_assignable_v<E> && std::is_move_constructible_v<E>;

		template <typename NewType, typename OldType, typename... Args>
		constexpr void reinit_expected( NewType& new_val, OldType& old_val, Args&&... args )
		{
			if constexpr ( std::is_nothrow_constructible_v<NewType, Args...> )
			{
				std::destroy_at( std::addressof( old_val ) );
				std::construct_at( std::addressof( new_val ), std::forward<Args>( args )... );
			}
			else if constexpr ( std::is_nothrow_move_constructible_v<NewType> )
			{
				NewType temp( std::forward<Args>( args )... );
				std::destroy_at( std::addressof( old_val ) );
				std::construct_at( std::addressof( new_val ), std::move( temp ) );
			}
			else
			{
				OldType temp( std::move( old_val ) );
				std::destroy_at( std::addressof( old_val ) );
				try
				{
					std::construct_at( std::addressof( new_val ), std::forward<Args>( args )... );
				}
				catch ( ... )
				{
					std::construct_at( std::addressof( old_val ), std::move( temp ) );
					throw;
				}
			}
		}
	}

	template <typename E>
	class [[nodiscard]] bad_expected_access;

	/// @brief Base of the @ref bad_expected_access hierarchy providing the common exception interface. Mirrors
	/// @c std::bad_expected_access<void>.
	template <>
	class [[nodiscard]] bad_expected_access<void> : public std::exception
	{
	public:
		[[nodiscard]] const char* what() const noexcept override
		{
			return "bad expected access";
		}

	protected:
		bad_expected_access() = default;
		bad_expected_access( const bad_expected_access& ) = default;
		bad_expected_access( bad_expected_access&& ) = default;
		bad_expected_access& operator=( const bad_expected_access& ) = default;
		bad_expected_access& operator=( bad_expected_access&& ) = default;
	};

	/// @brief Exception thrown when the value or error of an @ref expected is accessed incorrectly, carrying the error.
	/// @details A polyfill for C++23 @c std::bad_expected_access; the interface mirrors the standard, so refer to
	/// @c std::bad_expected_access for the full API description.
	/// @tparam E The unexpected error type carried by the exception.
	template <typename E>
	class [[nodiscard]] bad_expected_access : public bad_expected_access<void>
	{
	public:
		explicit bad_expected_access( E error ) noexcept( std::is_nothrow_move_constructible_v<E> )
			: m_error( std::move( error ) )
		{
		}

		[[nodiscard]] const E& error() const& noexcept
		{
			return m_error;
		}

		[[nodiscard]] E& error() & noexcept
		{
			return m_error;
		}

		[[nodiscard]] const E&& error() const&& noexcept
		{
			return std::move( m_error );
		}

		[[nodiscard]] E&& error() && noexcept
		{
			return std::move( m_error );
		}

	private:
		E m_error;
	};

	/// @brief Tag type used to select in-place construction of the unexpected error of an @ref expected. Mirrors
	/// @c std::unexpect_t.
	struct unexpect_t
	{
		explicit unexpect_t() = default;
	};

	/// @brief Tag value of type @ref unexpect_t. Mirrors @c std::unexpect.
	inline constexpr unexpect_t unexpect{};

	/// @brief A wrapper representing an unexpected error value of type @p E, used to initialise the error state of an
	/// @ref expected.
	/// @details A polyfill for C++23 @c std::unexpected; the interface mirrors the standard, so refer to
	/// @c std::unexpected for the full API description.
	/// @tparam E The unexpected error type.
	template <typename E>
	class [[nodiscard]] unexpected
	{
	public:
		constexpr explicit unexpected( E error ) noexcept( std::is_nothrow_move_constructible_v<E> )
			: m_error( std::move( error ) )
		{
		}

		template <typename Err = E>
			requires( !std::is_same_v<std::remove_cvref_t<Err>, unexpected> &&
					  !std::is_same_v<std::remove_cvref_t<Err>, std::in_place_t> && std::is_constructible_v<E, Err> )
		constexpr explicit unexpected( Err&& error ) noexcept( std::is_nothrow_constructible_v<E, Err> )
			: m_error( std::forward<Err>( error ) )
		{
		}

		template <typename... Args>
			requires std::is_constructible_v<E, Args...>
		constexpr explicit unexpected( std::in_place_t,
									   Args&&... args ) noexcept( std::is_nothrow_constructible_v<E, Args...> )
			: m_error( std::forward<Args>( args )... )
		{
		}

		template <typename U, typename... Args>
			requires( std::is_constructible_v<E, std::initializer_list<U>&, Args...> )
		constexpr explicit unexpected( std::in_place_t, std::initializer_list<U> ilist, Args&&... args ) noexcept(
			std::is_nothrow_constructible_v<E, std::initializer_list<U>&, Args...> )
			: m_error( ilist, std::forward<Args>( args )... )
		{
		}

		constexpr unexpected( const unexpected& other ) = default;
		constexpr unexpected( unexpected&& other ) noexcept( std::is_nothrow_move_constructible_v<E> ) = default;

		[[nodiscard]] constexpr const E& error() const& noexcept
		{
			return m_error;
		}

		[[nodiscard]] constexpr E& error() & noexcept
		{
			return m_error;
		}

		[[nodiscard]] constexpr const E&& error() const&& noexcept
		{
			return std::move( m_error );
		}

		[[nodiscard]] constexpr E&& error() && noexcept
		{
			return std::move( m_error );
		}

		constexpr void swap( unexpected& other ) noexcept( std::is_nothrow_swappable_v<E> )
		{
			static_assert( std::is_swappable_v<E>, "Error type must be swappable" );
			using std::swap;
			swap( m_error, other.m_error );
		}

		friend constexpr void swap( unexpected& lhs, unexpected& rhs ) noexcept( std::is_nothrow_swappable_v<E> )
			requires std::is_swappable_v<E>
		{
			lhs.swap( rhs );
		}

		template <typename E2>
		friend constexpr bool operator==( const unexpected& lhs,
										  const unexpected<E2>& rhs ) noexcept( noexcept( lhs.error() == rhs.error() ) )
		{
			return lhs.error() == rhs.error();
		}

	private:
		E m_error;
	};

	template <typename E>
	unexpected( E ) -> unexpected<E>;

	/// @brief A type that holds either an expected value of type @p T or an unexpected error of type @p E.
	/// @details A polyfill for C++23 @c std::expected, used when the standard library does not provide it. The
	/// interface mirrors the standard exactly, so refer to @c std::expected (the [expected] clause of the C++ standard,
	/// or https://en.cppreference.com/w/cpp/utility/expected) for the full API description.
	/// @tparam T The expected value type.
	/// @tparam E The unexpected error type.
	template <typename T, typename E>
	class [[nodiscard]] expected
	{
	public:
		using value_type = T;
		using error_type = E;
		using unexpected_type = unexpected<E>;

		template <typename U>
		using rebind = expected<U, error_type>;

		// Constructors
		constexpr expected()
			requires std::is_default_constructible_v<T>
			: m_value()
			, m_has_value( true )
		{
		}

		constexpr expected( const expected& other )
			requires( std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_constructible_v<E> )
		= default;

		constexpr expected( const expected& other )
			requires( std::is_copy_constructible_v<T> && std::is_copy_constructible_v<E> &&
					  !( std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_constructible_v<E> ))
			: m_has_value( other.m_has_value )
		{
			if ( m_has_value )
			{
				std::construct_at( std::addressof( m_value ), other.m_value );
			}
			else
			{
				std::construct_at( std::addressof( m_error ), other.m_error );
			}
		}

		constexpr expected( expected&& other )
			requires( std::is_trivially_move_constructible_v<T> && std::is_trivially_move_constructible_v<E> )
		= default;

		constexpr expected( expected&& other ) noexcept( std::is_nothrow_move_constructible_v<T> &&
														 std::is_nothrow_move_constructible_v<E> )
			requires( std::is_move_constructible_v<T> && std::is_move_constructible_v<E> &&
					  !( std::is_trivially_move_constructible_v<T> && std::is_trivially_move_constructible_v<E> ))
			: m_has_value( other.m_has_value )
		{
			if ( m_has_value )
			{
				std::construct_at( std::addressof( m_value ), std::move( other.m_value ) );
			}
			else
			{
				std::construct_at( std::addressof( m_error ), std::move( other.m_error ) );
			}
		}

		template <typename U, typename G>
			requires( std::is_constructible_v<T, const U&> && std::is_constructible_v<E, const G&> &&
					  converting_constructor_constraint<U, G>() )
		constexpr explicit( !std::is_convertible_v<const U&, T> || !std::is_convertible_v<const G&, E> )
			expected( const expected<U, G>& other )
			: m_has_value( other.has_value() )
		{
			if ( m_has_value )
			{
				std::construct_at( std::addressof( m_value ), *other );
			}
			else
			{
				std::construct_at( std::addressof( m_error ), other.error() );
			}
		}

		template <typename U, typename G>
			requires( std::is_constructible_v<T, U> && std::is_constructible_v<E, G> &&
					  converting_constructor_constraint<U, G>() )
		constexpr explicit( !std::is_convertible_v<U, T> || !std::is_convertible_v<G, E> )
			expected( expected<U, G>&& other )
			: m_has_value( other.has_value() )
		{
			if ( m_has_value )
			{
				std::construct_at( std::addressof( m_value ), *std::move( other ) );
			}
			else
			{
				std::construct_at( std::addressof( m_error ), std::move( other ).error() );
			}
		}

		template <typename U = std::remove_cv_t<T>>
			requires( !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
					  !std::is_same_v<expected, std::remove_cvref_t<U>> &&
					  !detail::is_unexpected_specialization<std::remove_cvref_t<U>> && std::is_constructible_v<T, U> &&
					  ( !std::is_same_v<std::remove_cv_t<T>, bool> ||
						!detail::is_expected_specialization<std::remove_cvref_t<U>> ))
		constexpr explicit( !std::is_convertible_v<U, T> )
			expected( U&& v ) noexcept( std::is_nothrow_constructible_v<T, U> )
			: m_value( std::forward<U>( v ) )
			, m_has_value( true )
		{
		}

		template <typename G>
			requires std::is_constructible_v<E, const G&>
		constexpr explicit( !std::is_convertible_v<const G&, E> )
			expected( const unexpected<G>& e ) noexcept( std::is_nothrow_constructible_v<E, const G&> )
			: m_error( e.error() )
			, m_has_value( false )
		{
		}

		template <typename G>
			requires std::is_constructible_v<E, G>
		constexpr explicit( !std::is_convertible_v<G, E> )
			expected( unexpected<G>&& e ) noexcept( std::is_nothrow_constructible_v<E, G> )
			: m_error( std::move( e ).error() )
			, m_has_value( false )
		{
		}

		template <typename... Args>
			requires std::is_constructible_v<T, Args...>
		constexpr explicit expected( std::in_place_t,
									 Args&&... args ) noexcept( std::is_nothrow_constructible_v<T, Args...> )
			: m_value( std::forward<Args>( args )... )
			, m_has_value( true )
		{
		}

		template <typename U, typename... Args>
			requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
		constexpr explicit expected( std::in_place_t, std::initializer_list<U> il, Args&&... args ) noexcept(
			std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...> )
			: m_value( il, std::forward<Args>( args )... )
			, m_has_value( true )
		{
		}

		template <typename... Args>
			requires std::is_constructible_v<E, Args...>
		constexpr explicit expected( unexpect_t,
									 Args&&... args ) noexcept( std::is_nothrow_constructible_v<E, Args...> )
			: m_error( std::forward<Args>( args )... )
			, m_has_value( false )
		{
		}

		template <typename U, typename... Args>
			requires std::is_constructible_v<E, std::initializer_list<U>&, Args...>
		constexpr explicit expected( unexpect_t, std::initializer_list<U> il, Args&&... args ) noexcept(
			std::is_nothrow_constructible_v<E, std::initializer_list<U>&, Args...> )
			: m_error( il, std::forward<Args>( args )... )
			, m_has_value( false )
		{
		}

		// Destructor
		constexpr ~expected() noexcept
		{
			if ( has_value() )
			{
				std::destroy_at( std::addressof( m_value ) );
			}
			else
			{
				std::destroy_at( std::addressof( m_error ) );
			}
		}

		constexpr ~expected()
			requires( std::is_trivially_destructible_v<T> && std::is_trivially_destructible_v<E> )
		= default;

		// Assignment
		expected& operator=( const expected& ) = delete;

		constexpr expected& operator=( const expected& )
			requires( detail::copy_assignable_expected<T, E> && detail::trivially_copy_assignable_expected<T> &&
					  detail::trivially_copy_assignable_expected<E> )
		= default;

		constexpr expected& operator=( const expected& other )
			requires detail::copy_assignable_expected<T, E>
		{
			if ( m_has_value && other.m_has_value )
			{
				m_value = other.m_value;
			}
			else if ( m_has_value )
			{
				detail::reinit_expected( m_error, m_value, other.m_error );
				m_has_value = false;
			}
			else if ( other.m_has_value )
			{
				detail::reinit_expected( m_value, m_error, other.m_value );
				m_has_value = true;
			}
			else
			{
				m_error = other.m_error;
			}
			return *this;
		}

		expected& operator=( expected&& ) = delete;

		constexpr expected& operator=( expected&& )
			requires( detail::move_assignable_expected<T, E> && detail::trivially_move_assignable_expected<T> &&
					  detail::trivially_move_assignable_expected<E> )
		= default;

		constexpr expected& operator=( expected&& other ) noexcept( std::is_nothrow_move_constructible_v<T> &&
																	std::is_nothrow_move_assignable_v<T> &&
																	std::is_nothrow_move_constructible_v<E> &&
																	std::is_nothrow_move_assignable_v<E> )
			requires detail::move_assignable_expected<T, E>
		{
			if ( m_has_value && other.m_has_value )
			{
				m_value = std::move( other.m_value );
			}
			else if ( m_has_value )
			{
				detail::reinit_expected( m_error, m_value, std::move( other.m_error ) );
				m_has_value = false;
			}
			else if ( other.m_has_value )
			{
				detail::reinit_expected( m_value, m_error, std::move( other.m_value ) );
				m_has_value = true;
			}
			else
			{
				m_error = std::move( other.m_error );
			}
			return *this;
		}

		template <typename U = std::remove_cv_t<T>>
			requires( !std::is_same_v<expected, std::remove_cvref_t<U>> &&
					  !detail::is_unexpected_specialization<std::remove_cvref_t<U>> && std::is_constructible_v<T, U> &&
					  std::is_assignable_v<T&, U> &&
					  ( std::is_nothrow_constructible_v<T, U> || std::is_nothrow_move_constructible_v<T> ||
						std::is_nothrow_move_constructible_v<E> ))
		constexpr expected& operator=( U&& v ) noexcept( std::is_nothrow_constructible_v<T, U> &&
														 std::is_nothrow_assignable_v<T&, U> )
		{
			if ( m_has_value )
			{
				m_value = std::forward<U>( v );
			}
			else
			{
				detail::reinit_expected( m_value, m_error, std::forward<U>( v ) );
				m_has_value = true;
			}
			return *this;
		}

		template <typename G>
			requires( std::is_constructible_v<E, const G&> && std::is_assignable_v<E&, const G&> &&
					  ( std::is_nothrow_constructible_v<E, const G&> || std::is_nothrow_move_constructible_v<T> ||
						std::is_nothrow_move_constructible_v<E> ))
		constexpr expected& operator=( const unexpected<G>& e )
		{
			if ( m_has_value )
			{
				detail::reinit_expected( m_error, m_value, e.error() );
				m_has_value = false;
			}
			else
			{
				m_error = e.error();
			}
			return *this;
		}

		template <typename G>
			requires( std::is_constructible_v<E, G> && std::is_assignable_v<E&, G> &&
					  ( std::is_nothrow_constructible_v<E, G> || std::is_nothrow_move_constructible_v<T> ||
						std::is_nothrow_move_constructible_v<E> ))
		constexpr expected& operator=( unexpected<G>&& e )
		{
			if ( m_has_value )
			{
				detail::reinit_expected( m_error, m_value, std::move( e ).error() );
				m_has_value = false;
			}
			else
			{
				m_error = std::move( e ).error();
			}
			return *this;
		}

		// Observers
		[[nodiscard]] constexpr const T* operator->() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( has_value(), "Attempted to access value of an expected that does not contain a value" );
			return std::addressof( m_value );
		}

		[[nodiscard]] [[nodiscard]] constexpr T* operator->() MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( has_value(), "Attempted to access value of an expected that does not contain a value" );
			return std::addressof( m_value );
		}

		[[nodiscard]] constexpr const T& operator*() const& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( has_value(), "Attempted to access value of an expected that does not contain a value" );
			return m_value;
		}

		[[nodiscard]] constexpr T& operator*() & MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( has_value(), "Attempted to access value of an expected that does not contain a value" );
			return m_value;
		}

		[[nodiscard]] constexpr const T&& operator*() const&& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( has_value(), "Attempted to access value of an expected that does not contain a value" );
			return std::move( m_value );
		}

		[[nodiscard]] constexpr T&& operator*() && MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( has_value(), "Attempted to access value of an expected that does not contain a value" );
			return std::move( m_value );
		}

		[[nodiscard]] constexpr bool has_value() const noexcept
		{
			return m_has_value;
		}

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return has_value();
		}

		[[nodiscard]] constexpr T& value() &
		{
			if ( !has_value() )
			{
				throw_bad_expected_const();
			}
			return m_value;
		}

		[[nodiscard]] constexpr const T& value() const&
		{
			if ( !has_value() )
			{
				throw_bad_expected_const();
			}
			return m_value;
		}

		[[nodiscard]] constexpr T&& value() &&
		{
			if ( !has_value() )
			{
				throw_bad_expected_move();
			}
			return std::move( m_value );
		}

		[[nodiscard]] constexpr const T&& value() const&&
		{
			if ( !has_value() )
			{
				throw_bad_expected_move();
			}
			return std::move( m_value );
		}

		[[nodiscard]] constexpr const E& error() const& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return m_error;
		}

		[[nodiscard]] constexpr E& error() & MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return m_error;
		}

		[[nodiscard]] constexpr const E&& error() const&& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return std::move( m_error );
		}

		[[nodiscard]] constexpr E&& error() && MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return std::move( m_error );
		}

		template <typename U = std::remove_cv_t<T>>
		constexpr T value_or( U&& default_value ) const& noexcept( std::is_nothrow_copy_constructible_v<T> &&
																   std::is_nothrow_convertible_v<U, T> )
		{
			static_assert( std::is_copy_constructible_v<T> && std::is_convertible_v<U, T>,
						   "Value type must be copy constructible and constructible from the default value type" );
			if ( has_value() )
			{
				return m_value;
			}
			else
			{
				return static_cast<T>( std::forward<U>( default_value ) );
			}
		}

		template <typename U = std::remove_cv_t<T>>
		constexpr T value_or( U&& default_value ) && noexcept( std::is_nothrow_move_constructible_v<T> &&
															   std::is_nothrow_convertible_v<U, T> )
		{
			static_assert( std::is_move_constructible_v<T> && std::is_convertible_v<U, T>,
						   "Value type must be move constructible and constructible from the default value type" );
			if ( has_value() )
			{
				return std::move( m_value );
			}
			else
			{
				return static_cast<T>( std::forward<U>( default_value ) );
			}
		}

		template <typename G = E>
		constexpr G error_or( G&& default_value ) const& noexcept( std::is_nothrow_copy_constructible_v<E> &&
																   std::is_nothrow_convertible_v<G, E> )
		{
			static_assert( std::is_copy_constructible_v<E> && std::is_convertible_v<G, E>,
						   "Error type must be copy constructible and constructible from the default error type" );
			if ( has_value() )
			{
				return std::forward<G>( default_value );
			}
			else
			{
				return m_error;
			}
		}

		template <typename G = E>
		constexpr G error_or( G&& default_value ) && noexcept( std::is_nothrow_move_constructible_v<E> &&
															   std::is_nothrow_convertible_v<G, E> )
		{
			static_assert( std::is_move_constructible_v<E> && std::is_convertible_v<G, E>,
						   "Error type must be move constructible and constructible from the default error type" );
			if ( has_value() )
			{
				return std::forward<G>( default_value );
			}
			else
			{
				return std::move( m_error );
			}
		}

		// Monadic operations
		template <typename F>
			requires std::is_constructible_v<E, E&>
		constexpr auto and_then( F&& f ) &
		{
			return and_then_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&>
		constexpr auto and_then( F&& f ) const&
		{
			return and_then_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, E&&>
		constexpr auto and_then( F&& f ) &&
		{
			return and_then_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&&>
		constexpr auto and_then( F&& f ) const&&
		{
			return and_then_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, T&>
		constexpr auto or_else( F&& f ) &
		{
			return or_else_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, const T&>
		constexpr auto or_else( F&& f ) const&
		{
			return or_else_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, T&&>
		constexpr auto or_else( F&& f ) &&
		{
			return or_else_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, const T&&>
		constexpr auto or_else( F&& f ) const&&
		{
			return or_else_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, E&>
		constexpr auto transform( F&& f ) &
		{
			return transform_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&>
		constexpr auto transform( F&& f ) const&
		{
			return transform_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, E&&>
		constexpr auto transform( F&& f ) &&
		{
			return transform_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&&>
		constexpr auto transform( F&& f ) const&&
		{
			return transform_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, T&>
		constexpr auto transform_error( F&& f ) &
		{
			return transform_error_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, const T&>
		constexpr auto transform_error( F&& f ) const&
		{
			return transform_error_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, T&&>
		constexpr auto transform_error( F&& f ) &&
		{
			return transform_error_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<T, const T&&>
		constexpr auto transform_error( F&& f ) const&&
		{
			return transform_error_impl( std::move( *this ), std::forward<F>( f ) );
		}

		// Comparison
		template <typename T2, typename E2>
			requires( !std::is_void_v<T2> )
		friend constexpr bool operator==( const expected& lhs, const expected<T2, E2>& rhs )
		{
			if ( lhs.has_value() != rhs.has_value() )
			{
				return false;
			}
			return lhs.has_value() ? static_cast<bool>( *lhs == *rhs )
								   : static_cast<bool>( lhs.error() == rhs.error() );
		}

		template <typename E2>
		friend constexpr bool operator==( const expected& lhs, const unexpected<E2>& unex )
		{
			return !lhs.has_value() && static_cast<bool>( lhs.error() == unex.error() );
		}

		template <typename T2>
		friend constexpr bool operator==( const expected& lhs, const T2& val )
		{
			return lhs.has_value() && static_cast<bool>( *lhs == val );
		}

		// Modifiers
		constexpr void swap( expected& other ) noexcept( std::is_nothrow_move_constructible_v<T> &&
														 std::is_nothrow_swappable_v<T> &&
														 std::is_nothrow_move_constructible_v<E> &&
														 std::is_nothrow_swappable_v<E> )
			requires( std::is_swappable_v<T> && std::is_swappable_v<E> && std::is_move_constructible_v<T> &&
					  std::is_move_constructible_v<E> &&
					  ( std::is_nothrow_move_constructible_v<T> || std::is_nothrow_move_constructible_v<E> ))
		{
			if ( m_has_value && other.m_has_value )
			{
				using std::swap;
				swap( m_value, other.m_value );
			}
			else if ( !m_has_value && !other.m_has_value )
			{
				using std::swap;
				swap( m_error, other.m_error );
			}
			else if ( m_has_value )
			{
				swap_value_with_error( other );
			}
			else
			{
				other.swap_value_with_error( *this );
			}
		}

		friend constexpr void swap( expected& lhs, expected& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
			requires requires { lhs.swap( rhs ); }
		{
			lhs.swap( rhs );
		}

		template <typename... Args>
			requires std::is_nothrow_constructible_v<T, Args...>
		constexpr T& emplace( Args&&... args ) noexcept
		{
			if ( has_value() )
			{
				if constexpr ( !std::is_trivially_destructible_v<T> )
				{
					std::destroy_at( std::addressof( m_value ) );
				}
			}
			else
			{
				if constexpr ( !std::is_trivially_destructible_v<E> )
				{
					std::destroy_at( std::addressof( m_error ) );
				}
				m_has_value = true;
			}
			return *std::construct_at( std::addressof( m_value ), std::forward<Args>( args )... );
		}

		template <typename U, typename... Args>
			requires std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>
		constexpr T& emplace( std::initializer_list<U> il, Args&&... args ) noexcept
		{
			if ( has_value() )
			{
				if constexpr ( !std::is_trivially_destructible_v<T> )
				{
					std::destroy_at( std::addressof( m_value ) );
				}
			}
			else
			{
				if constexpr ( !std::is_trivially_destructible_v<E> )
				{
					std::destroy_at( std::addressof( m_error ) );
				}
				m_has_value = true;
			}
			return *std::construct_at( std::addressof( m_value ), il, std::forward<Args>( args )... );
		}

	private:
		[[noreturn]] void throw_bad_expected_const() const
		{
			static_assert( std::is_copy_constructible_v<E>,
						   "Error type must be copy constructible to throw bad_expected_access" );
			throw bad_expected_access<std::decay_t<E>>( std::as_const( m_error ) );
		}

		[[noreturn]] void throw_bad_expected_move()
		{
			static_assert( std::is_copy_constructible_v<E> ||
							   std::is_constructible_v<E, decltype( std::move( m_error ) )>,
						   "Error type must be copy constructible or move constructible to throw bad_expected_access" );
			throw bad_expected_access<std::decay_t<E>>( std::move( m_error ) );
		}

		template <typename U, typename G>
		[[nodiscard]] static constexpr bool converting_constructor_constraint() noexcept
		{
			constexpr bool value_constructible =
				std::is_same_v<std::remove_cv_t<T>, bool> ||
				( !std::is_constructible_v<T, expected<U, G>&> && !std::is_constructible_v<T, expected<U, G>> &&
				  !std::is_constructible_v<T, const expected<U, G>&> &&
				  !std::is_constructible_v<T, const expected<U, G>> && !std::is_convertible_v<expected<U, G>&, T> &&
				  !std::is_convertible_v<expected<U, G>, T> && !std::is_convertible_v<const expected<U, G>&, T> &&
				  !std::is_convertible_v<const expected<U, G>, T> );
			constexpr bool unexpected_not_constructible =
				!std::is_constructible_v<unexpected<E>, expected<U, G>&> &&
				!std::is_constructible_v<unexpected<E>, expected<U, G>> &&
				!std::is_constructible_v<unexpected<E>, const expected<U, G>&> &&
				!std::is_constructible_v<unexpected<E>, const expected<U, G>>;
			return value_constructible && unexpected_not_constructible;
		}

		constexpr void swap_value_with_error( expected& other )
		{
			// *this contains a value, other contains an error
			if constexpr ( std::is_nothrow_move_constructible_v<E> )
			{
				E temp( std::move( other.m_error ) );
				std::destroy_at( std::addressof( other.m_error ) );
				try
				{
					std::construct_at( std::addressof( other.m_value ), std::move( m_value ) );
					std::destroy_at( std::addressof( m_value ) );
					std::construct_at( std::addressof( m_error ), std::move( temp ) );
				}
				catch ( ... )
				{
					std::construct_at( std::addressof( other.m_error ), std::move( temp ) );
					throw;
				}
			}
			else
			{
				std::remove_cv_t<T> temp( std::move( m_value ) );
				std::destroy_at( std::addressof( m_value ) );
				try
				{
					std::construct_at( std::addressof( m_error ), std::move( other.m_error ) );
					std::destroy_at( std::addressof( other.m_error ) );
					std::construct_at( std::addressof( other.m_value ), std::move( temp ) );
				}
				catch ( ... )
				{
					std::construct_at( std::addressof( m_value ), std::move( temp ) );
					throw;
				}
			}
			m_has_value = false;
			other.m_has_value = true;
		}

		template <typename Self, typename F>
		static constexpr auto and_then_impl( Self&& self, F&& f )
		{
			using U = std::remove_cvref_t<std::invoke_result_t<F, decltype( ( std::forward<Self>( self ).m_value ) )>>;
			static_assert( detail::is_expected_specialization<U>,
						   "The function passed to and_then must return a specialization of expected" );
			static_assert( std::is_same_v<typename U::error_type, E>,
						   "The function passed to and_then must return an expected with the same error type" );
			if ( self.has_value() )
			{
				return std::invoke( std::forward<F>( f ), std::forward<Self>( self ).m_value );
			}
			else
			{
				return U( unexpect, std::forward<Self>( self ).m_error );
			}
		}

		template <typename Self, typename F>
		static constexpr auto or_else_impl( Self&& self, F&& f )
		{
			using G = std::remove_cvref_t<std::invoke_result_t<F, decltype( ( std::forward<Self>( self ).m_error ) )>>;
			static_assert( detail::is_expected_specialization<G>,
						   "The function passed to or_else must return a specialization of expected" );
			static_assert( std::is_same_v<typename G::value_type, T>,
						   "The function passed to or_else must return an expected with the same value type" );
			if ( self.has_value() )
			{
				return G( std::in_place, std::forward<Self>( self ).m_value );
			}
			else
			{
				return std::invoke( std::forward<F>( f ), std::forward<Self>( self ).m_error );
			}
		}

		template <typename Self, typename F>
		static constexpr auto transform_impl( Self&& self, F&& f )
		{
			using U = std::remove_cv_t<std::invoke_result_t<F, decltype( ( std::forward<Self>( self ).m_value ) )>>;
			using result_type = expected<U, E>;
			if ( self.has_value() )
			{
				if constexpr ( std::is_void_v<U> )
				{
					std::invoke( std::forward<F>( f ), std::forward<Self>( self ).m_value );
					return result_type();
				}
				else
				{
					return result_type( std::in_place,
										std::invoke( std::forward<F>( f ), std::forward<Self>( self ).m_value ) );
				}
			}
			else
			{
				return result_type( unexpect, std::forward<Self>( self ).m_error );
			}
		}

		template <typename Self, typename F>
		static constexpr auto transform_error_impl( Self&& self, F&& f )
		{
			using G = std::remove_cv_t<std::invoke_result_t<F, decltype( ( std::forward<Self>( self ).m_error ) )>>;
			using result_type = expected<T, G>;
			if ( self.has_value() )
			{
				return result_type( std::in_place, std::forward<Self>( self ).m_value );
			}
			else
			{
				return result_type( unexpect, std::invoke( std::forward<F>( f ), std::forward<Self>( self ).m_error ) );
			}
		}

		union
		{
			std::remove_cv_t<T> m_value;
			E m_error;
		};

		bool m_has_value = false;
	};

	template <typename T, typename E>
		requires std::is_void_v<T>
	class [[nodiscard]] expected<T, E>
	{
	public:
		using value_type = T;
		using error_type = E;
		using unexpected_type = unexpected<E>;

		template <typename U>
		using rebind = expected<U, error_type>;

		// Constructors
		constexpr expected() noexcept
			: m_has_value( true )
		{
		}

		constexpr expected( const expected& other )
			requires std::is_trivially_copy_constructible_v<E>
		= default;

		constexpr expected( const expected& other )
			requires( std::is_copy_constructible_v<E> && !std::is_trivially_copy_constructible_v<E> )
			: m_has_value( other.m_has_value )
		{
			if ( !m_has_value )
			{
				std::construct_at( std::addressof( m_error ), other.m_error );
			}
		}

		constexpr expected( expected&& other )
			requires std::is_trivially_move_constructible_v<E>
		= default;

		constexpr expected( expected&& other ) noexcept( std::is_nothrow_move_constructible_v<E> )
			requires( std::is_move_constructible_v<E> && !std::is_trivially_move_constructible_v<E> )
			: m_has_value( other.m_has_value )
		{
			if ( !m_has_value )
			{
				std::construct_at( std::addressof( m_error ), std::move( other.m_error ) );
			}
		}

		template <typename U, typename G>
			requires( std::is_void_v<U> && std::is_constructible_v<E, const G&> &&
					  converting_constructor_constraint<U, G>() )
		constexpr explicit( !std::is_convertible_v<const G&, E> ) expected( const expected<U, G>& other )
			: m_has_value( other.has_value() )
		{
			if ( !m_has_value )
			{
				std::construct_at( std::addressof( m_error ), other.error() );
			}
		}

		template <typename U, typename G>
			requires( std::is_void_v<U> && std::is_constructible_v<E, G> && converting_constructor_constraint<U, G>() )
		constexpr explicit( !std::is_convertible_v<G, E> ) expected( expected<U, G>&& other )
			: m_has_value( other.has_value() )
		{
			if ( !m_has_value )
			{
				std::construct_at( std::addressof( m_error ), std::move( other ).error() );
			}
		}

		template <typename G>
			requires std::is_constructible_v<E, const G&>
		constexpr explicit( !std::is_convertible_v<const G&, E> )
			expected( const unexpected<G>& e ) noexcept( std::is_nothrow_constructible_v<E, const G&> )
			: m_error( e.error() )
			, m_has_value( false )
		{
		}

		template <typename G>
			requires std::is_constructible_v<E, G>
		constexpr explicit( !std::is_convertible_v<G, E> )
			expected( unexpected<G>&& e ) noexcept( std::is_nothrow_constructible_v<E, G> )
			: m_error( std::move( e ).error() )
			, m_has_value( false )
		{
		}

		constexpr explicit expected( std::in_place_t ) noexcept
			: m_has_value( true )
		{
		}

		template <typename... Args>
			requires std::is_constructible_v<E, Args...>
		constexpr explicit expected( unexpect_t,
									 Args&&... args ) noexcept( std::is_nothrow_constructible_v<E, Args...> )
			: m_error( std::forward<Args>( args )... )
			, m_has_value( false )
		{
		}

		template <typename U, typename... Args>
			requires std::is_constructible_v<E, std::initializer_list<U>&, Args...>
		constexpr explicit expected( unexpect_t, std::initializer_list<U> il, Args&&... args ) noexcept(
			std::is_nothrow_constructible_v<E, std::initializer_list<U>&, Args...> )
			: m_error( il, std::forward<Args>( args )... )
			, m_has_value( false )
		{
		}

		// Destructor
		constexpr ~expected() noexcept
		{
			if ( !has_value() )
			{
				std::destroy_at( std::addressof( m_error ) );
			}
		}

		constexpr ~expected()
			requires std::is_trivially_destructible_v<E>
		= default;

		// Assignment
		expected& operator=( const expected& ) = delete;

		constexpr expected& operator=( const expected& )
			requires( detail::copy_assignable_expected_void<E> && detail::trivially_copy_assignable_expected<E> )
		= default;

		constexpr expected& operator=( const expected& other )
			requires detail::copy_assignable_expected_void<E>
		{
			if ( m_has_value && other.m_has_value )
			{
				// both contain a value, nothing to do
			}
			else if ( m_has_value )
			{
				std::construct_at( std::addressof( m_error ), other.m_error );
				m_has_value = false;
			}
			else if ( other.m_has_value )
			{
				std::destroy_at( std::addressof( m_error ) );
				m_has_value = true;
			}
			else
			{
				m_error = other.m_error;
			}
			return *this;
		}

		expected& operator=( expected&& ) = delete;

		constexpr expected& operator=( expected&& )
			requires( detail::move_assignable_expected_void<E> && detail::trivially_move_assignable_expected<E> )
		= default;

		constexpr expected& operator=( expected&& other ) noexcept( std::is_nothrow_move_constructible_v<E> &&
																	std::is_nothrow_move_assignable_v<E> )
			requires detail::move_assignable_expected_void<E>
		{
			if ( m_has_value && other.m_has_value )
			{
				// both contain a value, nothing to do
			}
			else if ( m_has_value )
			{
				std::construct_at( std::addressof( m_error ), std::move( other.m_error ) );
				m_has_value = false;
			}
			else if ( other.m_has_value )
			{
				std::destroy_at( std::addressof( m_error ) );
				m_has_value = true;
			}
			else
			{
				m_error = std::move( other.m_error );
			}
			return *this;
		}

		template <typename G>
			requires( std::is_constructible_v<E, const G&> && std::is_assignable_v<E&, const G&> )
		constexpr expected& operator=( const unexpected<G>& e )
		{
			if ( m_has_value )
			{
				std::construct_at( std::addressof( m_error ), e.error() );
				m_has_value = false;
			}
			else
			{
				m_error = e.error();
			}
			return *this;
		}

		template <typename G>
			requires( std::is_constructible_v<E, G> && std::is_assignable_v<E&, G> )
		constexpr expected& operator=( unexpected<G>&& e )
		{
			if ( m_has_value )
			{
				std::construct_at( std::addressof( m_error ), std::move( e ).error() );
				m_has_value = false;
			}
			else
			{
				m_error = std::move( e ).error();
			}
			return *this;
		}

		// Observers
		constexpr void operator*() const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( has_value(), "Attempted to access value of an expected that does not contain a value" );
		}

		[[nodiscard]] constexpr bool has_value() const noexcept
		{
			return m_has_value;
		}

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return has_value();
		}

		[[nodiscard]] constexpr void value() const&
		{
			if ( !has_value() )
			{
				throw_bad_expected_const();
			}
		}

		[[nodiscard]] constexpr void value() &&
		{
			if ( !has_value() )
			{
				throw_bad_expected_move();
			}
		}

		[[nodiscard]] constexpr const E& error() const& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return m_error;
		}

		[[nodiscard]] constexpr E& error() & MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return m_error;
		}

		[[nodiscard]] constexpr const E&& error() const&& MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return std::move( m_error );
		}

		[[nodiscard]] constexpr E&& error() && MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( !has_value(), "Attempted to access error of an expected that contains a value" );
			return std::move( m_error );
		}

		template <typename G = E>
		constexpr G error_or( G&& default_value ) const& noexcept( std::is_nothrow_copy_constructible_v<E> &&
																   std::is_nothrow_convertible_v<G, E> )
		{
			static_assert( std::is_copy_constructible_v<E> && std::is_convertible_v<G, E>,
						   "Error type must be copy constructible and constructible from the default error type" );
			if ( has_value() )
			{
				return std::forward<G>( default_value );
			}
			else
			{
				return m_error;
			}
		}

		template <typename G = E>
		constexpr G error_or( G&& default_value ) && noexcept( std::is_nothrow_move_constructible_v<E> &&
															   std::is_nothrow_convertible_v<G, E> )
		{
			static_assert( std::is_move_constructible_v<E> && std::is_convertible_v<G, E>,
						   "Error type must be move constructible and constructible from the default error type" );
			if ( has_value() )
			{
				return std::forward<G>( default_value );
			}
			else
			{
				return std::move( m_error );
			}
		}

		// Monadic operations
		template <typename F>
			requires std::is_constructible_v<E, E&>
		constexpr auto and_then( F&& f ) &
		{
			return and_then_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&>
		constexpr auto and_then( F&& f ) const&
		{
			return and_then_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, E&&>
		constexpr auto and_then( F&& f ) &&
		{
			return and_then_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&&>
		constexpr auto and_then( F&& f ) const&&
		{
			return and_then_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto or_else( F&& f ) &
		{
			return or_else_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto or_else( F&& f ) const&
		{
			return or_else_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto or_else( F&& f ) &&
		{
			return or_else_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto or_else( F&& f ) const&&
		{
			return or_else_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, E&>
		constexpr auto transform( F&& f ) &
		{
			return transform_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&>
		constexpr auto transform( F&& f ) const&
		{
			return transform_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, E&&>
		constexpr auto transform( F&& f ) &&
		{
			return transform_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
			requires std::is_constructible_v<E, const E&&>
		constexpr auto transform( F&& f ) const&&
		{
			return transform_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto transform_error( F&& f ) &
		{
			return transform_error_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto transform_error( F&& f ) const&
		{
			return transform_error_impl( *this, std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto transform_error( F&& f ) &&
		{
			return transform_error_impl( std::move( *this ), std::forward<F>( f ) );
		}

		template <typename F>
		constexpr auto transform_error( F&& f ) const&&
		{
			return transform_error_impl( std::move( *this ), std::forward<F>( f ) );
		}

		// Comparison
		template <typename T2, typename E2>
			requires std::is_void_v<T2>
		friend constexpr bool operator==( const expected& lhs, const expected<T2, E2>& rhs )
		{
			if ( lhs.has_value() != rhs.has_value() )
			{
				return false;
			}
			return lhs.has_value() || static_cast<bool>( lhs.error() == rhs.error() );
		}

		template <typename E2>
		friend constexpr bool operator==( const expected& lhs, const unexpected<E2>& unex )
		{
			return !lhs.has_value() && static_cast<bool>( lhs.error() == unex.error() );
		}

		// Modifiers
		constexpr void swap( expected& other ) noexcept( std::is_nothrow_move_constructible_v<E> &&
														 std::is_nothrow_swappable_v<E> )
			requires( std::is_swappable_v<E> && std::is_move_constructible_v<E> )
		{
			if ( m_has_value && other.m_has_value )
			{
				// both contain a value, nothing to do
			}
			else if ( !m_has_value && !other.m_has_value )
			{
				using std::swap;
				swap( m_error, other.m_error );
			}
			else if ( m_has_value )
			{
				std::construct_at( std::addressof( m_error ), std::move( other.m_error ) );
				std::destroy_at( std::addressof( other.m_error ) );
				m_has_value = false;
				other.m_has_value = true;
			}
			else
			{
				other.swap( *this );
			}
		}

		friend constexpr void swap( expected& lhs, expected& rhs ) noexcept( noexcept( lhs.swap( rhs ) ) )
			requires requires { lhs.swap( rhs ); }
		{
			lhs.swap( rhs );
		}

		constexpr void emplace() noexcept
		{
			if ( !has_value() )
			{
				if constexpr ( !std::is_trivially_destructible_v<E> )
				{
					std::destroy_at( std::addressof( m_error ) );
				}
				m_has_value = true;
			}
		}

	private:
		[[noreturn]] void throw_bad_expected_const() const
		{
			static_assert( std::is_copy_constructible_v<E>,
						   "Error type must be copy constructible to throw bad_expected_access" );
			throw bad_expected_access<std::decay_t<E>>( std::as_const( m_error ) );
		}

		[[noreturn]] void throw_bad_expected_move()
		{
			static_assert( std::is_move_constructible_v<E>,
						   "Error type must be move constructible to throw bad_expected_access" );
			throw bad_expected_access<std::decay_t<E>>( std::move( m_error ) );
		}

		template <typename U, typename G>
		[[nodiscard]] static constexpr bool converting_constructor_constraint() noexcept
		{
			return !std::is_constructible_v<unexpected<E>, expected<U, G>&> &&
				   !std::is_constructible_v<unexpected<E>, expected<U, G>> &&
				   !std::is_constructible_v<unexpected<E>, const expected<U, G>&> &&
				   !std::is_constructible_v<unexpected<E>, const expected<U, G>>;
		}

		template <typename Self, typename F>
		static constexpr auto and_then_impl( Self&& self, F&& f )
		{
			using U = std::remove_cvref_t<std::invoke_result_t<F>>;
			static_assert( detail::is_expected_specialization<U>,
						   "The function passed to and_then must return a specialization of expected" );
			static_assert( std::is_same_v<typename U::error_type, E>,
						   "The function passed to and_then must return an expected with the same error type" );
			if ( self.has_value() )
			{
				return std::invoke( std::forward<F>( f ) );
			}
			else
			{
				return U( unexpect, std::forward<Self>( self ).m_error );
			}
		}

		template <typename Self, typename F>
		static constexpr auto or_else_impl( Self&& self, F&& f )
		{
			using G = std::remove_cvref_t<std::invoke_result_t<F, decltype( ( std::forward<Self>( self ).m_error ) )>>;
			static_assert( detail::is_expected_specialization<G>,
						   "The function passed to or_else must return a specialization of expected" );
			static_assert( std::is_same_v<typename G::value_type, T>,
						   "The function passed to or_else must return an expected with the same value type" );
			if ( self.has_value() )
			{
				return G();
			}
			else
			{
				return std::invoke( std::forward<F>( f ), std::forward<Self>( self ).m_error );
			}
		}

		template <typename Self, typename F>
		static constexpr auto transform_impl( Self&& self, F&& f )
		{
			using U = std::remove_cv_t<std::invoke_result_t<F>>;
			using result_type = expected<U, E>;
			if ( self.has_value() )
			{
				if constexpr ( std::is_void_v<U> )
				{
					std::invoke( std::forward<F>( f ) );
					return result_type();
				}
				else
				{
					return result_type( std::in_place, std::invoke( std::forward<F>( f ) ) );
				}
			}
			else
			{
				return result_type( unexpect, std::forward<Self>( self ).m_error );
			}
		}

		template <typename Self, typename F>
		static constexpr auto transform_error_impl( Self&& self, F&& f )
		{
			using G = std::remove_cv_t<std::invoke_result_t<F, decltype( ( std::forward<Self>( self ).m_error ) )>>;
			using result_type = expected<T, G>;
			if ( self.has_value() )
			{
				return result_type();
			}
			else
			{
				return result_type( unexpect, std::invoke( std::forward<F>( f ), std::forward<Self>( self ).m_error ) );
			}
		}

		union
		{
			E m_error;
		};

		bool m_has_value = false;
	};
}

#endif
