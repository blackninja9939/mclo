#include <catch2/catch_template_test_macros.hpp>

#include "mclo/container/graph.hpp"

void foo()
{
	using graph = mclo::directed_acyclic_graph<int>;
	graph g;

	using vertex_handle = typename graph::vertex_handle;
	vertex_handle h = g.add_vertex( 1 );
	vertex_handle h2 = g.add_vertex( 2 );

	auto e = g.add_edge( h, h2 );
	// g.remove_edge( e );
	g.add_edge( h2, h2 );
}

TEST_CASE( "check", "[directed_graph]" )
{
	foo();
	SUCCEED();
}
