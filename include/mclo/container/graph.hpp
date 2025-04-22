#pragma once

#include "mclo/container/dynamic_bitset.hpp"
#include "mclo/container/small_vector.hpp"
#include "mclo/debug/assert.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace mclo
{
	namespace detail
	{
		struct undirected_graph_policy
		{
			static constexpr bool is_directed = false;
			static constexpr bool allow_cycles = true;
		};

		struct directed_graph_policy
		{
			static constexpr bool is_directed = true;
			static constexpr bool allow_cycles = true;
		};

		struct directed_acyclic_graph_policy
		{
			static constexpr bool is_directed = true;
			static constexpr bool allow_cycles = false;
		};

		template <typename Graph, typename Tag>
		class graph_handle
		{
			friend Graph;

		public:
			[[nodiscard]] constexpr bool operator==( const graph_handle& other ) const noexcept = default;

			constexpr friend void swap( graph_handle& lhs, graph_handle& rhs ) noexcept
			{
				std::swap( lhs.m_index, rhs.m_index );
			}

		private:
			constexpr graph_handle( const std::size_t index ) noexcept
				: m_index( index )
			{
			}

			std::size_t m_index = 0;
		};

		template <typename VertexHandle>
		class graph_edge_base
		{
		public:
			constexpr graph_edge_base( const VertexHandle from, const VertexHandle to ) noexcept
				: m_from( from )
				, m_to( to )
			{
			}

			constexpr friend void swap( graph_edge_base& lhs, graph_edge_base& rhs ) noexcept
			{
				using std::swap;
				swap( lhs.m_from, rhs.m_from );
				swap( lhs.m_to, rhs.m_to );
			}

			[[nodiscard]] constexpr VertexHandle from() const noexcept
			{
				return m_from;
			}

			[[nodiscard]] constexpr VertexHandle to() const noexcept
			{
				return m_to;
			}

		private:
			VertexHandle m_from;
			VertexHandle m_to;
		};

		template <typename VertexHandle, typename EdgeData>
		class graph_edge : public graph_edge_base<VertexHandle>
		{
			using base = graph_edge_base<VertexHandle>;

		public:
			template <typename... Args>
			constexpr graph_edge( const VertexHandle from,
								  const VertexHandle to,
								  Args&&... args ) noexcept( std::is_nothrow_constructible_v<EdgeData, Args...> )
				: base( from, to )
				, m_data( std::forward<Args>( args )... )
			{
			}

			[[nodiscard]] constexpr EdgeData& data() noexcept
			{
				return m_data;
			}
			[[nodiscard]] constexpr const EdgeData& data() const noexcept
			{
				return m_data;
			}

		private:
			EdgeData m_data;
		};

		template <typename VertexHandle>
		class graph_edge<VertexHandle, void> : public graph_edge_base<VertexHandle>
		{
			using base = graph_edge_base<VertexHandle>;

		public:
			using base::base;
		};
	}

	template <typename VertexData, typename EdgeData, typename Policy>
	class graph
	{
	public:
		using vertex_data = VertexData;
		using edge_data = EdgeData;

		using vertex_handle = detail::graph_handle<graph, vertex_data>;
		using edge_handle = detail::graph_handle<graph, edge_data>;

		class vertex
		{
			friend graph;

		public:
			template <typename... Args>
			constexpr vertex( std::in_place_t,
							  Args&&... args ) noexcept( std::is_nothrow_constructible_v<vertex_data, Args...> )
				: m_data( std::forward<Args>( args )... )
			{
			}

			constexpr friend void swap( vertex& lhs, vertex& rhs ) noexcept
			{
				using std::swap;
				swap( lhs.m_data, rhs.m_data );
				swap( lhs.m_edges, rhs.m_edges );
			}

			[[nodiscard]] constexpr vertex_data& data() noexcept
			{
				return m_data;
			}
			[[nodiscard]] constexpr const vertex_data& data() const noexcept
			{
				return m_data;
			}

			[[nodiscard]] constexpr std::span<edge_handle> edges() const noexcept
			{
				return m_edges;
			}

		private:
			vertex_data m_data;
			mclo::small_vector<edge_handle, 2> m_edges;
		};

		using edge = detail::graph_edge<vertex_handle, edge_data>;

		graph() = default;

		template <typename... Args>
		[[nodiscard]] constexpr vertex_handle add_vertex( Args&&... args )
		{
			vertex_handle handle( m_vertices.size() );
			m_vertices.emplace_back( std::in_place, std::forward<Args>( args )... );
			return handle;
		}

		template <typename... Args>
		constexpr edge_handle add_edge( const vertex_handle& from, const vertex_handle& to, Args&&... args )
		{
			if constexpr ( !Policy::allow_cycles )
			{
				DEBUG_ASSERT( !creates_cycle_dfs( from, to ), "Adding this edge would create a cycle in the graph" );
			}

			edge_handle handle( m_edges.size() );
			m_edges.emplace_back( from, to, std::forward<Args>( args )... );
			m_vertices[ from.m_index ].m_edges.push_back( handle );

			if constexpr ( !Policy::is_directed )
			{
				m_vertices[ to.m_index ].m_edges.push_back( handle );
			}

			return handle;
		}

		// void remove_edge( const edge_handle handle )
		//{
		//	const auto& edge = m_edges[ handle.m_index ];
		//	// Remove edge from the source vertex
		//	auto& from_vertex = m_vertices[ edge.from().m_index ];
		//	std::erase( from_vertex.m_edges, handle );

		//	// Remove edge from the target vertex (if undirected)
		//	if constexpr ( !Policy::is_directed )
		//	{
		//		auto& to_vertex = m_vertices[ edge.to().m_index ];
		//		std::erase( to_vertex.m_edges, handle );
		//	}

		//	// Finally, remove the edge itself from the edges list
		//	m_edges.erase( m_edges.begin() + handle.m_index );

		//	// Update subsequent edge indices in the graph
		//	for ( std::size_t i = handle.m_index; i < m_edges.size(); ++i )
		//	{
		//		m_edges[ i ].m_index = i;
		//	}
		//}

		// void remove_vertex( const vertex_handle handle )
		//{
		//	// First, remove all edges associated with this vertex
		//	auto& vertex = m_vertices[ handle.m_index ];
		//	mclo::small_vector<edge_handle, 32> edges_to_remove( vertex.m_edges() );

		//	for ( const auto& edge_handle : edges_to_remove )
		//	{
		//		remove_edge( edge_handle );
		//	}

		//	// Remove the vertex itself from the vertices list
		//	m_vertices.erase( m_vertices.begin() + handle.m_index );

		//	// Update subsequent vertex indices in the graph
		//	for ( std::size_t i = handle.m_index; i < m_vertices.size(); ++i )
		//	{
		//		std::erase_if( m_edges, [ i ]( const edge& e ) { return e.from.m_index == i || e.to.m_index == i; } );
		//		for ( auto& edge : m_vertices[ i ].edges )
		//		{
		//			edge.m_index = i;
		//		}
		//	}
		//}

		[[nodiscard]] constexpr vertex& get_vertex( const vertex_handle& handle ) noexcept
		{
			return m_vertices[ handle.m_index ];
		}
		[[nodiscard]] constexpr const vertex& get_vertex( const vertex_handle& handle ) const noexcept
		{
			return m_vertices[ handle.m_index ];
		}

		[[nodiscard]] constexpr edge& get_edge( const edge_handle& handle ) noexcept
		{
			return m_edges[ handle.m_index ];
		}
		[[nodiscard]] constexpr const edge& get_edge( const edge_handle& handle ) const noexcept
		{
			return m_edges[ handle.m_index ];
		}

		[[nodiscard]] constexpr std::span<vertex> vertices() noexcept
		{
			return m_vertices;
		}
		[[nodiscard]] constexpr std::span<const vertex> vertices() const noexcept
		{
			return m_vertices;
		}

		[[nodiscard]] constexpr std::span<edge> edges() noexcept
		{
			return m_edges;
		}
		[[nodiscard]] constexpr std::span<const edge> edges() const noexcept
		{
			return m_edges;
		}

		/*
		* todo(mc)
		* - Add a function to remove an edge
		* - Add a function to remove a vertex
		* - Add a function to check if an edge exists, or is this doable via the public exposed data?
		* - Add a visitor support, depth first, breadth first, etc. must start from a vertex
		* - From the visitor can we implement the edge detection? Its just visit until you hit that node then stop
		* - topological sort but will that break the indicies, maybe just a toplogical visitor?
		*/

		template<typename Func>
		constexpr void visit_depth_first( const vertex_handle& start, Func func ) const
		{
			mclo::dynamic_bitset<> visited( m_vertices.size() );
			visit_depth_first_recursive( start, visited, func );
		}

		template <typename Func>
		constexpr void visit_topological( Func func ) const
			requires ( Policy::is_directed && !Policy::allow_cycles )
		{
			std::vector<std::size_t> in_degree( m_vertices.size(), 0 );

			// Compute in-degrees
			for ( const auto& edge : m_edges )
			{
				in_degree[ edge.to().m_index ]++;
			}

			// Initialize a queue with vertices that have zero in-degree
			std::vector<vertex_handle> queue;
			for ( std::size_t i = 0; i < m_vertices.size(); ++i )
			{
				if ( in_degree[ i ] == 0 )
				{
					queue.push_back( vertex_handle( i ) );
				}
			}

			// Perform topological sort
			while ( !queue.empty() )
			{
				vertex_handle v = queue.back();
				queue.pop_back();
				func( v );

				// Decrease the in-degree of all outgoing vertices
				for ( const edge_handle& e_handle : m_vertices[ v.m_index ].m_edges )
				{
					const edge& e = m_edges[ e_handle.m_index ];
					if ( --in_degree[ e.to().m_index ] == 0 )
					{
						queue.push_back( e.to() );
					}
				}
			}
		}

	private:
		template <typename Func>
		constexpr bool visit_depth_first_recursive( const vertex_handle& vertex,
													mclo::dynamic_bitset<>& visited,
													Func func ) const
		{
			if ( visited.test_set( vertex.m_index ) )
			{
				return;
			}

			func( vertex );

			for ( const auto& e_handle : m_vertices[ vertex.m_index ].m_edges )
			{
				const edge& e = m_edges[ e_handle.m_index ];
				if constexpr ( Policy::is_directed )
				{
					if ( e.from() == vertex )
					{
						visit_depth_first_recursive( e.to(), visited, func );
					}
				}
				else
				{
					const vertex_handle next = ( e.from() == vertex ) ? e.to() : e.from();
					visit_depth_first_recursive( next, visited, func );
				}
			}
		}

		[[nodiscard]] bool creates_cycle_dfs( vertex_handle from, vertex_handle to ) const
		{
			mclo::dynamic_bitset<> visited( m_vertices.size() );
			return creates_cycle_dfs_recursive( to, from, visited );
		}

		[[nodiscard]] bool creates_cycle_dfs_recursive( vertex_handle from,
														vertex_handle to,
														mclo::dynamic_bitset<>& visited ) const
		{
			if ( from == to )
			{
				return true;
			}

			const bool already_visited = visited.test_set( to.m_index );
			if ( already_visited )
			{
				return false;
			}

			for ( const auto& e_handle : m_vertices[ from.m_index ].m_edges )
			{
				const edge& e = m_edges[ e_handle.m_index ];
				if constexpr ( Policy::is_directed )
				{
					if ( e.from() == from )
					{
						if ( creates_cycle_dfs_recursive( e.to(), to, visited ) )
						{
							return true;
						}
					}
				}
				else
				{
					const vertex_handle next = ( e.from() == from ) ? e.to() : e.from();
					if ( creates_cycle_dfs_recursive( next, to, visited ) )
					{
						return true;
					}
				}
			}

			return false;
		}

		std::vector<vertex> m_vertices;
		std::vector<edge> m_edges;
	};

	template <typename VertexData, typename EdgeData = void>
	using undirected_graph = graph<VertexData, EdgeData, detail::undirected_graph_policy>;

	template <typename VertexData, typename EdgeData = void>
	using directed_graph = graph<VertexData, EdgeData, detail::directed_graph_policy>;

	template <typename VertexData, typename EdgeData = void>
	using directed_acyclic_graph = graph<VertexData, EdgeData, detail::directed_acyclic_graph_policy>;
}
