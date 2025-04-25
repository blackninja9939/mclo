#pragma once

#include "mclo/container/small_vector.hpp"
#include "mclo/container/span.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/utility/new_type.hpp"

#include <algorithm>
#include <queue>
#include <vector>

namespace mclo
{
	namespace detail
	{
		template <typename T>
		struct access_trait
		{
			using type = std::remove_reference_t<T>;
			static constexpr bool is_const = std::is_const_v<type>;
			static constexpr meta::type_id_t id = meta::type_id<type>;
		};

		template <typename T>
		void detect_single_access( [[maybe_unused]] mclo::small_flat_set<meta::type_id_t>& reads,
								   [[maybe_unused]] mclo::small_flat_set<meta::type_id_t>& writes )
		{
			using access = access_trait<T>;
			if constexpr ( access::is_const )
			{
				[[maybe_unused]] const auto it = reads.insert( access::id );
				DEBUG_ASSERT( it.second, "Access type already exists in reads set" );
			}
			else
			{
				[[maybe_unused]] const auto it = writes.insert( access::id );
				DEBUG_ASSERT( it.second, "Access type already exists in writes set" );
			}
		}

		template <typename... Ts>
		void detect_access( mclo::small_flat_set<meta::type_id_t>& reads,
							mclo::small_flat_set<meta::type_id_t>& writes )
		{
			( detect_single_access<Ts>( reads, writes ), ... );
		}
	} // namespace detail

	class async_executor
	{
	public:
		using handle = std::future<void>;

		template <typename Func>
		handle queue( Func&& func )
		{
			return std::async( std::launch::async | std::launch::deferred, std::forward<Func>( func ) );
		}

		void wait( const handle& h )
		{
			h.wait();
		}
	};

	namespace detail
	{
		template <typename T, typename Handle>
		concept schedule_executor_functions = requires( T t, Handle h ) {
			{ t.queue( std::declval<void ( * )()>() ) } -> std::same_as<Handle>;
			{ t.wait( h ) };
		};
	}

	template <typename T>
	concept schedule_executor = requires( T t ) {
		typename T::handle;
		requires detail::schedule_executor_functions<T, typename T::handle>;
	};

	class scheduler
	{
	public:
		using system_handle = mclo::new_type<std::size_t, struct system_handle_tag>;

		template <typename... SystemArgs>
		system_handle register_system( std::string name,
									   void ( *func )( SystemArgs... ),
									   mclo::span<const system_handle> dependencies = {} )
		{
			system_handle handle{ m_systems.size() };

			system& sys = m_systems.emplace_back( system{ std::move( name ), make_system_func( func ) } );
			detail::detect_access<SystemArgs...>( sys.m_reads, sys.m_writes );

			for ( const system_handle dependency : dependencies )
			{
				DEBUG_ASSERT( dependency < m_systems.size(), "Dependency handle is out of range" );
				sys.m_dependencies.insert( dependency );
			}

			m_changed = true;
			return handle;
		}

		template <schedule_executor executor_type>
		void run( entt::registry& registry, executor_type& executor )
		{
			if ( m_changed )
			{
				update_graph();
				m_changed = false;
			}

			std::vector<typename executor_type::handle> tasks;

			for ( const std::vector<system_handle>& layer : m_layered_toplogy )
			{
				tasks.clear();
				tasks.reserve( layer.size() );

				for ( const system_handle handle : layer )
				{
					const system& sys = m_systems[ handle ];
					tasks.push_back( executor.queue( [ &registry, &sys ] { sys.m_func( registry ); } ) );
				}

				for ( auto& task : tasks )
				{
					executor.wait( task );
				}
			}
		}

	private:
		using system_func = std::function<void( entt::registry& )>;

		template <typename... SystemArgs>
		static system_func make_system_func( void ( *func )( SystemArgs... ) )
		{
			return [ func ]( entt::registry& registry ) {
				registry.view<std::remove_reference_t<SystemArgs>...>().each(
					[ func ]( SystemArgs... args ) { func( args... ); } );
			};
		}

		void update_graph()
		{
			// Build graph of all dependencies both detected and explicit
			std::vector<mclo::flat_set<system_handle>> graph;
			graph.resize( m_systems.size() );

			const std::size_t num_systems = m_systems.size();
			for ( std::size_t a = 0; a < num_systems; ++a )
			{
				const system& system_a = m_systems[ a ];
				for ( std::size_t b = 0; b < num_systems; ++b )
				{
					// Skip self
					if ( a == b )
					{
						continue;
					}

					const system& system_b = m_systems[ b ];
					bool needs_dependency = false;

					// Check if system there are reads/writes conflicts
					for ( const meta::type_id_t read : system_a.m_reads )
					{
						if ( system_b.m_writes.contains( read ) )
						{
							needs_dependency = true;
							break;
						}
					}

					if ( !needs_dependency )
					{
						for ( const meta::type_id_t write : system_a.m_writes )
						{
							if ( system_b.m_writes.contains( write ) || system_b.m_reads.contains( write ) )
							{
								needs_dependency = true;
								break;
							}
						}
					}

					if ( needs_dependency )
					{
						graph[ b ].insert( system_handle( a ) );
					}
				}

				graph[ a ].insert( system_a.m_dependencies.begin(), system_a.m_dependencies.end() );
			}

			// Calculate in-degrees
			std::vector<std::size_t> in_degrees;
			in_degrees.resize( num_systems );

			for ( std::size_t i = 0; i < num_systems; ++i )
			{
				for ( const system_handle dependency : graph[ i ] )
				{
					++in_degrees[ dependency ];
				}
			}

			// Topological sort
			std::queue<system_handle> queue;

			// Push all systems with no dependencies
			for ( std::size_t i = 0; i < num_systems; ++i )
			{
				if ( in_degrees[ i ] == 0 )
				{
					queue.push( system_handle( i ) );
				}
			}

			std::size_t processed_systems = 0;
			m_layered_toplogy.clear();
			while ( !queue.empty() )
			{
				std::vector<system_handle>& current_layer = m_layered_toplogy.emplace_back();

				// Pop all systems with no dependencies into current layer
				while ( !queue.empty() )
				{
					current_layer.push_back( queue.front() );
					queue.pop();
					++processed_systems;
				}

				// For each system in the current layer, remove it from the dependency graph and push any systems that
				// have no dependencies
				for ( const system_handle current : current_layer )
				{
					for ( const system_handle dependency : graph[ current ] )
					{
						--in_degrees[ dependency ];
						if ( in_degrees[ dependency ] == 0 )
						{
							queue.push( dependency );
						}
					}
				}
			}

			DEBUG_ASSERT( processed_systems == num_systems, "Graph is not a DAG, cycle detected" );
		}

		struct system
		{
			std::string m_name;
			system_func m_func;
			mclo::small_flat_set<meta::type_id_t> m_reads;
			mclo::small_flat_set<meta::type_id_t> m_writes;
			mclo::small_flat_set<system_handle, 2> m_dependencies;
		};

		std::vector<system> m_systems;

		std::vector<std::vector<system_handle>> m_layered_toplogy;

		bool m_changed = false; // todo(mc) separate scheduler registering systems from the built grpah to execute on?
	};

	void foo( const id_component& id, const rotation_component& rotation )
	{
		( void )id;
		( void )rotation;
	}

	void check()
	{
		scheduler schedule;
		scheduler::system_handle h1 = schedule.register_system( "foo", foo );
		scheduler::system_handle h2 = schedule.register_system(
			"bar",
			+[]( const id_component& id, const rotation_component& rotation ) {
				( void )id;
				( void )rotation;
			},
			std::array{ h1 } );

		async_executor executor;

		entt::registry reg;
		schedule.run( reg, executor );
	}
} // namespace mclo
