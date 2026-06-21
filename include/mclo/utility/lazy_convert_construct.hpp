#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace mclo
{
	/// @brief Wraps a factory callable and lazily produces its result only when converted to the result type.
	/// @details Stores a @p Factory and exposes an implicit conversion to its result type that invokes the
	/// factory on demand. This lets an expensive value be supplied where a conversion is expected, such as a
	/// container's @c try_emplace, without constructing it unless it is actually needed.
	/// @tparam Factory A callable taking no arguments that produces the value to construct.
	template <std::invocable<> Factory>
	class lazy_convert_construct
	{
	public:
		/// @brief The type produced by invoking the factory.
		using result_type = std::invoke_result_t<Factory>;

		/// @brief Constructs the wrapper from a factory callable.
		/// @param factory The callable that produces the value when needed.
		constexpr lazy_convert_construct( Factory&& factory ) noexcept( std::is_nothrow_move_constructible_v<Factory> )
			: m_factory( std::move( factory ) )
		{
		}

		/// @brief Invokes the factory and converts to its result type.
		/// @return The value produced by the factory.
		[[nodiscard]] constexpr operator result_type() const noexcept( std::is_nothrow_invocable_v<Factory&> )
		{
			return m_factory();
		}

	private:
		Factory m_factory;
	};
}
