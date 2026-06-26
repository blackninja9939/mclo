#pragma once

#include "mclo/meta/type_name.hpp"
#include "mclo/utility/state_machine.hpp"

#include <concepts>
#include <string>
#include <string_view>

namespace mclo
{
	namespace detail
	{
		// The display name for a state: its state_name member if it provides one, else its type name.
		template <typename State>
		[[nodiscard]] constexpr std::string_view state_display_name() noexcept
		{
			if constexpr ( requires {
							   { State::state_name } -> std::convertible_to<std::string_view>;
						   } )
			{
				return State::state_name;
			}
			else
			{
				return meta::type_name_v<State>;
			}
		}

		// The display name for a transition: its transition_name member if it provides one, else its type name.
		template <typename Transition>
		[[nodiscard]] constexpr std::string_view transition_display_name() noexcept
		{
			if constexpr ( requires {
							   { Transition::transition_name } -> std::convertible_to<std::string_view>;
						   } )
			{
				return Transition::transition_name;
			}
			else
			{
				return meta::type_name_v<Transition>;
			}
		}

		inline void append_dot_edge( std::string& out,
									 const std::string_view from,
									 const std::string_view label,
									 const std::string_view to )
		{
			out += "    \"";
			out += from;
			out += "\" -> \"";
			out += to;
			out += "\" [label=\"";
			out += label;
			out += "\"];\n";
		}
	}

	/// @brief Produces a Graphviz DOT description of every state and transition in @p Machine.
	/// @details Walks the machine's transition graph via @ref visit_state_machine_transitions and emits a directed
	/// edge from each state to the state its transition returns, labelled with the transition. Self-loops (transitions
	/// that return their own state type) are included as edges from a state to itself. Each state and transition is
	/// labelled with its @c state_name or @c transition_name @c static @c constexpr member if it declares one,
	/// otherwise its type name. The result can be rendered with Graphviz (e.g. @c dot) to visualise the machine.
	/// @tparam Machine A @ref state_machine specialisation to describe.
	/// @return The DOT digraph as a string.
	template <typename Machine>
	[[nodiscard]] std::string state_machine_to_dot()
	{
		std::string out = "digraph state_machine {\n";
		visit_state_machine_transitions<Machine>( [ &out ]<typename Edge>( Edge ) {
			detail::append_dot_edge( out,
									 detail::state_display_name<typename Edge::from_state>(),
									 detail::transition_display_name<typename Edge::transition>(),
									 detail::state_display_name<typename Edge::to_state>() );
		} );
		out += "}\n";
		return out;
	}
}
