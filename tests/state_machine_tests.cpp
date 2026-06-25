#include <catch2/catch_test_macros.hpp>

#include "mclo/utility/overloaded.hpp"
#include "mclo/utility/state_machine.hpp"
#include "mclo/utility/state_machine_graph.hpp"

#include <string>
#include <string_view>
#include <variant>

namespace
{
	// A simple traffic light style machine demonstrating the core mechanics.
	struct red;
	struct green;
	struct yellow;

	// Transitions: temporary objects that consume one state and return the next.
	struct go
	{
		static constexpr std::string_view transition_name = "Go!";
		green operator()( red&& ) const noexcept;
	};

	struct caution
	{
		yellow operator()( green&& ) const noexcept;
	};

	struct stop
	{
		red operator()( yellow&& ) const noexcept;
	};

	// A self-loop transition: consumes red and returns red unchanged (a "stay" edge in the graph).
	struct hold
	{
		red operator()( red&& state ) const noexcept;
	};

	struct red
	{
		static constexpr std::string_view state_name = "Red";
		// red may stay red (hold) or turn green (go).
		using transitions = std::variant<hold, go>;
	};

	struct green
	{
		// green must always change; it cannot stay green.
		using transitions = std::variant<caution>;
	};

	struct yellow
	{
		using transitions = std::variant<stop>;
	};

	green go::operator()( red&& ) const noexcept
	{
		return green{};
	}
	yellow caution::operator()( green&& ) const noexcept
	{
		return yellow{};
	}
	red stop::operator()( yellow&& ) const noexcept
	{
		return red{};
	}
	red hold::operator()( red&& state ) const noexcept
	{
		return std::move( state );
	}

	using traffic_light = mclo::state_machine<red, green, yellow>;

	// A machine carrying data, with a self-loop transition that mutates the value.
	struct counting;
	struct done;

	struct increment
	{
		int by = 1;
		counting operator()( counting&& state ) const noexcept;
	};

	struct finish
	{
		done operator()( counting&& ) const noexcept;
	};

	// done is terminal but must still author a transition; it stays via a self-loop.
	struct rest
	{
		done operator()( done&& state ) const noexcept;
	};

	struct counting
	{
		int value = 0;
		using transitions = std::variant<increment, finish>;
	};

	struct done
	{
		int total = 0;
		using transitions = std::variant<rest>;
	};

	counting increment::operator()( counting&& state ) const noexcept
	{
		state.value += by;
		return std::move( state );
	}
	done finish::operator()( counting&& state ) const noexcept
	{
		return done{ state.value };
	}
	done rest::operator()( done&& state ) const noexcept
	{
		return std::move( state );
	}

	using counter = mclo::state_machine<counting, done>;

	// Plain state structs with no nested transitions; bound externally via the trait below.
	struct plain_a
	{
	};
	struct plain_b
	{
	};
	struct a_to_b
	{
		plain_b operator()( plain_a&& ) const noexcept
		{
			return {};
		}
	};
	struct b_stay
	{
		plain_b operator()( plain_b&& state ) const noexcept
		{
			return std::move( state );
		}
	};
}

// Bind transitions to the plain structs externally, like specialising std::hash.
template <>
struct mclo::state_transitions<plain_a>
{
	using type = std::variant<a_to_b>;
};
template <>
struct mclo::state_transitions<plain_b>
{
	using type = std::variant<b_stay>;
};

namespace
{
	using external_machine = mclo::state_machine<plain_a, plain_b>;
}

TEST_CASE( "state_machine default constructs to first state", "[state_machine]" )
{
	traffic_light machine;
	CHECK( machine.is_state<red>() );
}

TEST_CASE( "state_machine constructs from an explicit state", "[state_machine]" )
{
	traffic_light machine{ green{} };
	CHECK( machine.is_state<green>() );
}

TEST_CASE( "state_machine self-loop keeps the state and reports no change", "[state_machine]" )
{
	traffic_light machine; // red

	const bool changed = machine.step( mclo::overloaded{
		[]( const red& ) -> red::transitions { return hold{}; },
		[]( const green& ) -> green::transitions { return caution{}; },
		[]( const yellow& ) -> yellow::transitions { return stop{}; },
	} );

	CHECK_FALSE( changed );
	CHECK( machine.is_state<red>() );
}

TEST_CASE( "state_machine applies a transition and reports a state change", "[state_machine]" )
{
	traffic_light machine; // red

	const auto selector = mclo::overloaded{
		[]( const red& ) -> red::transitions { return go{}; },
		[]( const green& ) -> green::transitions { return caution{}; },
		[]( const yellow& ) -> yellow::transitions { return stop{}; },
	};

	CHECK( machine.step( selector ) );
	CHECK( machine.is_state<green>() );

	CHECK( machine.step( selector ) );
	CHECK( machine.is_state<yellow>() );

	CHECK( machine.step( selector ) );
	CHECK( machine.is_state<red>() );
}

TEST_CASE( "state_machine self-loop mutates value but reports no type change", "[state_machine]" )
{
	counter machine{ counting{ 0 } };

	const auto add_two = mclo::overloaded{
		[]( const counting& ) -> counting::transitions { return increment{ 2 }; },
		[]( const done& ) -> done::transitions { return rest{}; },
	};

	CHECK_FALSE( machine.step( add_two ) ); // same type, value changed
	CHECK( machine.get<counting>().value == 2 );

	CHECK_FALSE( machine.step( add_two ) );
	CHECK( machine.get<counting>().value == 4 );
}

TEST_CASE( "state_machine transition moves data into the next state", "[state_machine]" )
{
	counter machine{ counting{ 7 } };

	const bool changed = machine.step( mclo::overloaded{
		[]( const counting& ) -> counting::transitions { return finish{}; },
		[]( const done& ) -> done::transitions { return rest{}; },
	} );

	REQUIRE( changed );
	REQUIRE( machine.is_state<done>() );
	CHECK( machine.get<done>().total == 7 );
}

TEST_CASE( "state_machine visit observes the active state", "[state_machine]" )
{
	traffic_light machine{ yellow{} };

	const std::string_view name = machine.visit( mclo::overloaded{
		[]( const red& ) { return std::string_view{ "red" }; },
		[]( const green& ) { return std::string_view{ "green" }; },
		[]( const yellow& ) { return std::string_view{ "yellow" }; },
	} );

	CHECK( name == "yellow" );
}

TEST_CASE( "state_machine try_get returns the active state or nullptr", "[state_machine]" )
{
	traffic_light machine; // red

	CHECK( machine.try_get<red>() != nullptr );
	CHECK( machine.try_get<green>() == nullptr );
	CHECK( machine.try_get<yellow>() == nullptr );

	const traffic_light& const_machine = machine;
	CHECK( const_machine.try_get<red>() != nullptr );
	CHECK( const_machine.try_get<green>() == nullptr );
}

TEST_CASE( "state_machine binds transitions via external trait specialization", "[state_machine]" )
{
	external_machine machine; // plain_a
	CHECK( machine.is_state<plain_a>() );

	const bool changed = machine.step( mclo::overloaded{
		[]( const plain_a& ) -> mclo::state_transitions_t<plain_a> { return a_to_b{}; },
		[]( const plain_b& ) -> mclo::state_transitions_t<plain_b> { return b_stay{}; },
	} );

	CHECK( changed );
	CHECK( machine.is_state<plain_b>() );
}

TEST_CASE( "state_machine_to_dot emits an edge per transition including self-loops", "[state_machine]" )
{
	const std::string dot = mclo::state_machine_to_dot<traffic_light>();

	CHECK( dot.starts_with( "digraph state_machine {\n" ) );
	CHECK( dot.ends_with( "}\n" ) );

	CHECK( dot.find( "\" -> \"" ) != std::string::npos );
	CHECK( dot.find( "[label=\"" ) != std::string::npos );

	// red and go provide custom names; the rest fall back to their type names.
	CHECK( dot.find( "\"Red\" -> " ) != std::string::npos );
	CHECK( dot.find( "[label=\"Go!\"]" ) != std::string::npos );

	// red has two transitions (hold self-loop + go), green one (caution), yellow one (stop) = 4 edges.
	std::size_t edges = 0;
	std::size_t pos = 0;
	while ( ( pos = dot.find( "->", pos ) ) != std::string::npos )
	{
		++edges;
		pos += 2;
	}
	CHECK( edges == 4 );
}
