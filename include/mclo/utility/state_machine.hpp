#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace mclo
{
	/// @brief Customization point declaring the transitions a @p State permits.
	/// @details Resolves to a @c std::variant of transition function objects describing the only ways @p State is
	/// allowed to change. The primary template reads the state's nested @c transitions alias, so a state may simply
	/// author @code using transitions = std::variant<...>; @endcode. Specialise this trait (in namespace @c mclo,
	/// like @c std::hash) to bind transitions externally instead, which lets you declare all states and
	/// transitions first and couple them afterwards, and to attach transitions to state types you do not own.
	/// @tparam State The state type whose permitted transitions are declared.
	template <typename State>
	struct state_transitions
	{
		using type = typename State::transitions;
	};

	/// @brief The @c std::variant of transitions a @p State permits; see @ref state_transitions.
	/// @tparam State The state type whose permitted transitions are queried.
	template <typename State>
	using state_transitions_t = typename state_transitions<State>::type;

	namespace detail
	{
		template <typename T, typename Variant>
		constexpr bool is_state_alternative = false;

		template <typename T, typename... States>
		constexpr bool is_state_alternative<T, std::variant<States...>> = ( std::is_same_v<T, States> || ... );

		// Validate every transition a single state authors against the full set of states.
		template <typename State, typename StateVariant, typename Transitions>
		struct validate_state_transitions;

		template <typename State, typename StateVariant, typename... Transitions>
		struct validate_state_transitions<State, StateVariant, std::variant<Transitions...>>
		{
			static_assert( ( std::is_invocable_v<const Transitions&, State&&> && ... ),
						   "Every transition a state authors must be invocable with that state as an rvalue." );

			static_assert(
				( is_state_alternative<std::remove_cvref_t<std::invoke_result_t<const Transitions&, State&&>>,
									   StateVariant> &&
				  ... ),
				"Every transition must return one of the state machine's state types." );

			static constexpr bool value = true;
		};
	}

	/// @brief A strongly typed finite state machine over a fixed set of @p States.
	/// @details Exactly one state is active at a time, stored in a @c std::variant. Each state type authors a
	/// nested @c transitions alias (a @c std::variant of transition function objects) describing the only ways it
	/// is allowed to change; a state can therefore never transition to anything it did not declare. A transition
	/// consumes the current state by value/rvalue and returns a new state by value, so transitions are temporary
	/// objects that move the machine forward. A state that wishes to remain (optionally mutating its value) simply
	/// authors a transition that returns its own type, which appears as a self-loop in the transition graph.
	/// Driving the machine is done through @ref step, which is given a selector (typically an @ref overloaded
	/// visitor) that inspects the active state and returns the transition to perform.
	/// @tparam States The complete set of state types the machine can hold.
	template <typename... States>
	class state_machine
	{
		static_assert( sizeof...( States ) > 0, "A state machine must have at least one state." );

		using variant_type = std::variant<States...>;

		static_assert( ( detail::validate_state_transitions<States, variant_type, state_transitions_t<States>>::value &&
						 ... ) );

	public:
		/// @brief Constructs the machine in its first state, value-initialised.
		constexpr state_machine() noexcept( std::is_nothrow_default_constructible_v<variant_type> ) = default;

		/// @brief Constructs the machine holding @p state.
		/// @tparam State One of the machine's state types.
		/// @param state The initial state to hold.
		template <typename State>
			requires( detail::is_state_alternative<std::remove_cvref_t<State>, variant_type> )
		constexpr state_machine( State&& state ) noexcept( std::is_nothrow_constructible_v<variant_type, State> )
			: m_state( std::forward<State>( state ) )
		{
		}

		/// @brief Advances the machine by asking @p selector which transition to perform from the active state.
		/// @details The selector inspects the active state by @c const reference — it does not mutate it — and returns
		/// the transition to perform (an alternative of that state's @c transitions variant). The chosen transition
		/// consumes the active state and produces the next state, which is emplaced as the new active state. The new
		/// value is fully constructed before the old state is replaced, so a transition may safely mutate and return
		/// the same object for a self-loop.
		/// @tparam Selector A callable accepting every state by @c const reference and returning that state's
		/// permitted transitions variant.
		/// @param selector The visitor selecting the transition to apply.
		/// @return @c true if the active state changed type (a different variant alternative), @c false for a
		/// self-loop that kept the same state type.
		template <typename Selector>
		constexpr bool step( Selector&& selector )
		{
			const std::size_t old_index = m_state.index();
			std::visit(
				[ this, &selector ]( auto& state ) {
					auto transition = std::forward<Selector>( selector )( std::as_const( state ) );
					std::visit(
						[ this, &state ]( auto&& chosen ) {
							using next_state = std::remove_cvref_t<
								std::invoke_result_t<decltype( chosen ), std::remove_reference_t<decltype( state )>>>;
							m_state.template emplace<next_state>(
								std::forward<decltype( chosen )>( chosen )( std::move( state ) ) );
						},
						std::move( transition ) );
				},
				m_state );
			return old_index != m_state.index();
		}

		/// @brief Whether the active state is @p State.
		/// @tparam State One of the machine's state types.
		template <typename State>
		[[nodiscard]] constexpr bool is_state() const noexcept
		{
			return std::holds_alternative<State>( m_state );
		}

		/// @brief Access the active state as @p State.
		/// @tparam State The expected active state type.
		template <typename State>
		[[nodiscard]] constexpr State& get()
		{
			return std::get<State>( m_state );
		}

		/// @brief Access the active state as @p State.
		/// @tparam State The expected active state type.
		template <typename State>
		[[nodiscard]] constexpr const State& get() const
		{
			return std::get<State>( m_state );
		}

		/// @brief A pointer to the active state if it is @p State, otherwise @c nullptr.
		/// @tparam State The state type to query.
		template <typename State>
		[[nodiscard]] constexpr State* try_get() noexcept
		{
			return std::get_if<State>( &m_state );
		}

		/// @brief A pointer to the active state if it is @p State, otherwise @c nullptr.
		/// @tparam State The state type to query.
		template <typename State>
		[[nodiscard]] constexpr const State* try_get() const noexcept
		{
			return std::get_if<State>( &m_state );
		}

		/// @brief Visit the active state with @p visitor.
		/// @tparam Visitor A callable accepting every state type.
		/// @param visitor The visitor to invoke with the active state.
		template <typename Visitor>
		constexpr decltype( auto ) visit( Visitor&& visitor )
		{
			return std::visit( std::forward<Visitor>( visitor ), m_state );
		}

		/// @brief Visit the active state with @p visitor.
		/// @tparam Visitor A callable accepting every state type.
		/// @param visitor The visitor to invoke with the active state.
		template <typename Visitor>
		constexpr decltype( auto ) visit( Visitor&& visitor ) const
		{
			return std::visit( std::forward<Visitor>( visitor ), m_state );
		}

	private:
		variant_type m_state;
	};

	/// @brief A node in a @ref state_machine's transition graph, identifying one state type.
	/// @details A purely type-level descriptor passed to a @ref visit_state_machine_states visitor. It carries no
	/// runtime data; consumers read the @c state alias (e.g. to resolve a display name or build a node).
	/// @tparam State The state type this node represents.
	template <typename State>
	struct state_machine_node
	{
		/// @brief The state type this node represents.
		using state = State;
	};

	/// @brief An edge in a @ref state_machine's transition graph: a transition from one state to another.
	/// @details A purely type-level descriptor passed to a @ref visit_state_machine_transitions visitor. It carries
	/// no runtime data; consumers read the type aliases to resolve names or build their own graph representation. A
	/// self-loop is represented by an edge whose @c from_state and @c to_state are the same type.
	/// @tparam From The state type the transition consumes.
	/// @tparam Transition The transition function object type.
	/// @tparam To The state type the transition produces.
	template <typename From, typename Transition, typename To>
	struct state_machine_edge
	{
		/// @brief The state type the transition consumes.
		using from_state = From;
		/// @brief The transition function object type.
		using transition = Transition;
		/// @brief The state type the transition produces.
		using to_state = To;
	};

	namespace detail
	{
		template <typename State, typename Transitions>
		struct state_machine_edge_emitter;

		template <typename State, typename... Transitions>
		struct state_machine_edge_emitter<State, std::variant<Transitions...>>
		{
			template <typename Visitor>
			static constexpr void emit( Visitor& visitor )
			{
				( visitor(
					  state_machine_edge<State,
										 Transitions,
										 std::remove_cvref_t<std::invoke_result_t<const Transitions&, State&&>>>{} ),
				  ... );
			}
		};

		template <typename Machine>
		struct state_machine_walker;

		template <typename... States>
		struct state_machine_walker<state_machine<States...>>
		{
			template <typename Visitor>
			static constexpr void visit_states( Visitor& visitor )
			{
				( visitor( state_machine_node<States>{} ), ... );
			}

			template <typename Visitor>
			static constexpr void visit_transitions( Visitor& visitor )
			{
				( state_machine_edge_emitter<States, state_transitions_t<States>>::emit( visitor ), ... );
			}
		};
	}

	/// @brief Invokes @p visitor once per state of @p Machine, walking its transition graph's nodes.
	/// @details The visitor is called with a @ref state_machine_node for each state type, in declaration order. The
	/// walk is purely type-level (no machine instance is required), so this is the building block for tooling such as
	/// graph exporters or editors. Use an explicitly templated lambda (e.g. @c []<typename Node>(Node){}) to read the
	/// node's @c state type.
	/// @tparam Machine A @ref state_machine specialisation to walk.
	/// @tparam Visitor A callable accepting a @ref state_machine_node for every state.
	/// @param visitor The visitor invoked for each state node.
	template <typename Machine, typename Visitor>
	constexpr void visit_state_machine_states( Visitor&& visitor )
	{
		detail::state_machine_walker<Machine>::visit_states( visitor );
	}

	/// @brief Invokes @p visitor once per transition of @p Machine, walking its transition graph's edges.
	/// @details The visitor is called with a @ref state_machine_edge for every transition each state authors, in
	/// declaration order. Self-loops (transitions returning their own state type) are included. The walk is purely
	/// type-level (no machine instance is required). Use an explicitly templated lambda (e.g.
	/// @c []<typename Edge>(Edge){}) to read the edge's @c from_state, @c transition and @c to_state types.
	/// @tparam Machine A @ref state_machine specialisation to walk.
	/// @tparam Visitor A callable accepting a @ref state_machine_edge for every transition.
	/// @param visitor The visitor invoked for each transition edge.
	template <typename Machine, typename Visitor>
	constexpr void visit_state_machine_transitions( Visitor&& visitor )
	{
		detail::state_machine_walker<Machine>::visit_transitions( visitor );
	}
}
