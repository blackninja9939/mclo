#pragma once

#include <exception>
#include <utility>

namespace mclo
{
	/// @brief RAII guard that unconditionally invokes a callable on destruction.
	/// @details Ensures cleanup code runs when leaving a scope, regardless of whether the scope exits
	/// normally or via an exception. The guard is non-moveable and non-copyable so destruction is
	/// guaranteed to happen exactly once with no branching overhead.
	/// @tparam F The callable type to invoke on destruction.
	/// @warning @p F must be nothrow move or copy constructible and nothrow invocable with no arguments.
	/// Violating these requirements is undefined behaviour.
	/// @see scope_fail
	/// @see scope_success
	template <typename F>
	class scope_exit
	{
	public:
		/// @brief Construct from an rvalue callable.
		/// @param func The callable to invoke on destruction. Moved into the guard.
		constexpr explicit scope_exit( F&& func ) noexcept
			: m_func( std::move( func ) )
		{
		}

		/// @brief Construct from an lvalue callable.
		/// @param func The callable to invoke on destruction. Copied into the guard.
		constexpr explicit scope_exit( const F& func ) noexcept
			: m_func( func )
		{
		}

		scope_exit( const scope_exit& ) = delete;
		scope_exit& operator=( const scope_exit& ) = delete;
		scope_exit( scope_exit&& ) = delete;
		scope_exit& operator=( scope_exit&& ) = delete;

		constexpr ~scope_exit() noexcept
		{
			m_func();
		}

	private:
		F m_func;
	};

	template <typename F>
	scope_exit( F ) -> scope_exit<F>;

	/// @brief RAII guard that invokes a callable on destruction only if the scope exits via an exception.
	/// @details Captures the @c std::uncaught_exceptions() count on construction and compares it on
	/// destruction. The callable is invoked only if the count has increased, indicating that the scope
	/// was exited by an exception being thrown. This correctly handles nested exception contexts.
	/// @tparam F The callable type to invoke on exception exit.
	/// @warning @p F must be nothrow move or copy constructible and nothrow invocable with no arguments.
	/// Violating these requirements is undefined behaviour.
	/// @see scope_exit
	/// @see scope_success
	template <typename F>
	class scope_fail
	{
	public:
		/// @brief Construct from an rvalue callable.
		/// @param func The callable to invoke if the scope exits via an exception. Moved into the guard.
		explicit scope_fail( F&& func ) noexcept
			: m_func( std::move( func ) )
		{
		}

		/// @brief Construct from an lvalue callable.
		/// @param func The callable to invoke if the scope exits via an exception. Copied into the guard.
		explicit scope_fail( const F& func ) noexcept
			: m_func( func )
		{
		}

		scope_fail( const scope_fail& ) = delete;
		scope_fail& operator=( const scope_fail& ) = delete;
		scope_fail( scope_fail&& ) = delete;
		scope_fail& operator=( scope_fail&& ) = delete;

		~scope_fail() noexcept
		{
			if ( std::uncaught_exceptions() > m_uncaught_exceptions )
			{
				m_func();
			}
		}

	private:
		F m_func;
		int m_uncaught_exceptions = std::uncaught_exceptions();
	};

	template <typename F>
	scope_fail( F ) -> scope_fail<F>;

	/// @brief RAII guard that invokes a callable on destruction only if the scope exits normally.
	/// @details Captures the @c std::uncaught_exceptions() count on construction and compares it on
	/// destruction. The callable is invoked only if the count has not increased, indicating that the
	/// scope was exited without an exception being thrown.
	/// @tparam F The callable type to invoke on normal exit.
	/// @warning @p F must be nothrow move or copy constructible and nothrow invocable with no arguments.
	/// Violating these requirements is undefined behaviour.
	/// @see scope_exit
	/// @see scope_fail
	template <typename F>
	class scope_success
	{
	public:
		/// @brief Construct from an rvalue callable.
		/// @param func The callable to invoke if the scope exits normally. Moved into the guard.
		explicit scope_success( F&& func ) noexcept
			: m_func( std::move( func ) )
		{
		}

		/// @brief Construct from an lvalue callable.
		/// @param func The callable to invoke if the scope exits normally. Copied into the guard.
		explicit scope_success( const F& func ) noexcept
			: m_func( func )
		{
		}

		scope_success( const scope_success& ) = delete;
		scope_success& operator=( const scope_success& ) = delete;
		scope_success( scope_success&& ) = delete;
		scope_success& operator=( scope_success&& ) = delete;

		~scope_success() noexcept
		{
			if ( std::uncaught_exceptions() <= m_uncaught_exceptions )
			{
				m_func();
			}
		}

	private:
		F m_func;
		int m_uncaught_exceptions = std::uncaught_exceptions();
	};

	template <typename F>
	scope_success( F ) -> scope_success<F>;
}
