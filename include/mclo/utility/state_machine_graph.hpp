#pragma once

#include "mclo/meta/type_name.hpp"
#include "mclo/utility/state_machine.hpp"

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

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

		template <typename State, typename Transitions>
		struct dump_state_edges;

		template <typename State, typename... Transitions>
		struct dump_state_edges<State, std::variant<Transitions...>>
		{
			static void append( std::string& out )
			{
				(
					[ &out ] {
						using next_state = std::remove_cvref_t<std::invoke_result_t<const Transitions&, State&&>>;
						append_dot_edge( out,
										 state_display_name<State>(),
										 transition_display_name<Transitions>(),
										 state_display_name<next_state>() );
					}(),
					... );
			}
		};

		template <typename Machine>
		struct dump_machine_graph;

		template <typename... States>
		struct dump_machine_graph<state_machine<States...>>
		{
			static std::string dump()
			{
				std::string out = "digraph state_machine {\n";
				( dump_state_edges<States, state_transitions_t<States>>::append( out ), ... );
				out += "}\n";
				return out;
			}
		};
	}

	/// @brief Produces a Graphviz DOT description of every state and transition in @p Machine.
	/// @details Walks each state's authored transitions and emits a directed edge from the state to the state the
	/// transition returns, labelled with the transition. Self-loops (transitions that return their own state type)
	/// are included as edges from a state to itself. Each state and transition is labelled with its @c state_name or
	/// @c transition_name @c static @c constexpr member if it declares one, otherwise its type name. The result can
	/// be rendered with Graphviz (e.g. @c dot) to visualise the machine.
	/// @tparam Machine A @ref state_machine specialisation to describe.
	/// @return The DOT digraph as a string.
	template <typename Machine>
	[[nodiscard]] std::string state_machine_to_dot()
	{
		return detail::dump_machine_graph<Machine>::dump();
	}
}
