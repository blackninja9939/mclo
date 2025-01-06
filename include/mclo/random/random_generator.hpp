#pragma once

#include "mclo/concepts/arithmetic.hpp"
#include "mclo/debug/assert.hpp"

#include <concepts>
#include <random>
#include <ranges>

namespace mclo
{
	template <std::uniform_random_bit_generator Engine>
	class random_generator
	{
		template <typename T>
		using uniform_dist_for =
			std::conditional_t<std::integral<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;

	public:
		using engine_type = Engine;

		random_generator() noexcept( std::is_nothrow_default_constructible_v<Engine> )
			requires( std::constructible_from<Engine> )
		= default;

		template <typename U>
			requires( std::constructible_from<Engine, U> )
		random_generator( U&& engine ) noexcept( std::is_nothrow_constructible_v<Engine, U> )
			: engine( std::forward<U>( engine ) )
		{
		}

		template <typename... Args>
			requires( std::constructible_from<Engine, Args...> )
		random_generator( std::in_place_t, Args&&... args ) noexcept( std::is_nothrow_constructible_v<Engine, Args...> )
			: engine( std::forward<Args>( args )... )
		{
		}

		engine_type& get_engine() noexcept
		{
			return engine;
		}

		template <typename... Args>
		void seed( Args&&... args )
		{
			engine.seed( std::forward<Args>( args )... );
		}

		template <arithmetic T>
		T uniform( T min, T max )
		{
			uniform_dist_for<T> dist( min, max );
			return dist( engine );
		}

		template <typename Distribution, std::ranges::output_range<typename Distribution::result_type> OutRng>
		void generate( Distribution dist, OutRng&& out_rng )
		{
			std::ranges::generate( std::forward<OutRng>( out_rng ), [ & ] { return dist( engine ); } );
		}

		template <std::ranges::range OutRng>
		void generate( OutRng&& out_rng )
		{
			using T = std::ranges::range_value_t<OutRng>;
			generate( uniform_dist_for<T>{}, std::forward<OutRng>( out_rng ) );
		}

		template <std::ranges::input_range Rng>
		auto pick_it( Rng&& rng )
		{
			const auto size = std::ranges::distance( rng );
			if ( size == 0 ) [[unlikely]]
			{
				return std::ranges::end( rng );
			}
			return pick_it_size_checked( std::forward<Rng>( rng ), size );
		}

		template <std::ranges::input_range Rng>
		decltype( auto ) pick( Rng&& rng )
		{
			const auto size = std::ranges::distance( rng );
			DEBUG_ASSERT( size != 0, "Range must not be empty" );
			return *pick_it_size_checked( std::forward<Rng>( rng ), size );
		}

		template <std::integral T>
		bool chance( T chance, T out_of )
		{
			if ( out_of <= 0 ) [[unlikely]]
			{
				return false;
			}
			if ( chance <= 0 ) [[unlikely]]
			{
				return false;
			}
			if ( chance >= out_of ) [[unlikely]]
			{
				return true;
			}
			return uniform( T( 0 ), out_of - 1 ) < chance;
		}

		template <std::integral T>
		bool chance_one_in( T out_of )
		{
			return chance( 1, out_of );
		}

		bool coin_toss()
		{
			return chance_one_in( 2 );
		}

		template <std::floating_point T>
		bool percent_chance( T chance )
		{
			DEBUG_ASSERT( chance >= T( 0 ) && chance <= T( 1 ), "chance must be in range [0.0, 1.0]" );
			// Uniform real distribution is [0, 1), so for a chance of 1 aka 100% every possible
			// generated value is less than 1 so returns true.
			return uniform( T( 0 ), T( 1 ) ) < chance;
		}

	private:
		template <std::ranges::input_range Rng, typename diff_t = std::ranges::range_difference_t<Rng>>
		auto pick_it_size_checked( Rng&& rng, diff_t size )
		{
			uniform_dist_for<diff_t> dist( 0, size - 1 );
			const auto index = dist( engine );
			return std::ranges::next( std::ranges::begin( std::forward<Rng>( rng ) ), index );
		}

		engine_type engine;
	};
}
