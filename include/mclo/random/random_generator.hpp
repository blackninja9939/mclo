#pragma once

#include "mclo/concepts/arithmetic.hpp"
#include "mclo/debug/assert.hpp"

#include <concepts>
#include <random>
#include <ranges>

namespace mclo
{
	/// @brief A convenience wrapper around a random bit generation engine providing high level random utilities.
	/// @details Owns an instance of @p Engine and layers ergonomic helpers on top of it, such as drawing uniform
	/// values, filling ranges, picking random elements, and evaluating probabilities. The appropriate standard
	/// distribution is selected automatically based on the requested value type.
	/// @tparam Engine The underlying random bit generator, satisfying @c std::uniform_random_bit_generator.
	template <std::uniform_random_bit_generator Engine>
	class random_generator
	{
		template <typename T>
		using uniform_dist_for =
			std::conditional_t<std::integral<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;

	public:
		/// @brief The type of the underlying random bit generation engine.
		using engine_type = Engine;

		/// @brief Default constructs the generator, default constructing its engine.
		/// @details Only available when @p Engine is itself default constructible.
		random_generator() noexcept( std::is_nothrow_default_constructible_v<Engine> )
			requires( std::constructible_from<Engine> )
		= default;

		/// @brief Constructs the generator from an existing engine instance.
		/// @tparam U A type from which @p Engine can be constructed.
		/// @param engine The engine to copy or move into the generator.
		template <typename U>
			requires( std::constructible_from<Engine, U> )
		random_generator( U&& engine ) noexcept( std::is_nothrow_constructible_v<Engine, U> )
			: engine( std::forward<U>( engine ) )
		{
		}

		/// @brief Constructs the engine in place from the given arguments.
		/// @tparam Args The argument types forwarded to the engine's constructor.
		/// @param args The arguments forwarded to the engine's constructor.
		template <typename... Args>
			requires( std::constructible_from<Engine, Args...> )
		random_generator( std::in_place_t, Args&&... args ) noexcept( std::is_nothrow_constructible_v<Engine, Args...> )
			: engine( std::forward<Args>( args )... )
		{
		}

		/// @brief Returns a reference to the underlying engine.
		/// @return A reference to the owned engine, for use with standard distributions or direct generation.
		engine_type& get_engine() noexcept
		{
			return engine;
		}

		/// @brief Reseeds the underlying engine.
		/// @tparam Args The argument types forwarded to the engine's @c seed function.
		/// @param args The arguments forwarded to the engine's @c seed function.
		template <typename... Args>
		void seed( Args&&... args )
		{
			engine.seed( std::forward<Args>( args )... );
		}

		/// @brief Generates a single uniformly distributed value within an inclusive range.
		/// @details For integral @p T the range is @c [min, max]; for floating point @p T it is @c [min, max).
		/// @tparam T The arithmetic type of value to generate.
		/// @param min The lower bound of the range.
		/// @param max The upper bound of the range.
		/// @return A uniformly distributed value within the range.
		template <arithmetic T>
		T uniform( T min, T max )
		{
			uniform_dist_for<T> dist( min, max );
			return dist( engine );
		}

		/// @brief Fills an output range with values drawn from a given distribution.
		/// @tparam Distribution The distribution type used to produce each value.
		/// @tparam OutRng The output range type, whose value type matches the distribution's result type.
		/// @param dist The distribution to sample from.
		/// @param out_rng The range to fill with generated values.
		template <typename Distribution, std::ranges::output_range<typename Distribution::result_type> OutRng>
		void generate( Distribution dist, OutRng&& out_rng )
		{
			std::ranges::generate( std::forward<OutRng>( out_rng ), [ & ] { return dist( engine ); } );
		}

		/// @brief Fills an output range with uniformly distributed values of the range's value type.
		/// @tparam OutRng The output range type.
		/// @param out_rng The range to fill with generated values.
		template <std::ranges::range OutRng>
		void generate( OutRng&& out_rng )
		{
			using T = std::ranges::range_value_t<OutRng>;
			generate( uniform_dist_for<T>{}, std::forward<OutRng>( out_rng ) );
		}

		/// @brief Picks a random iterator into a range.
		/// @tparam Rng The input range type.
		/// @param rng The range to pick from.
		/// @return An iterator to a uniformly chosen element, or the end iterator if the range is empty.
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

		/// @brief Picks a random element from a range.
		/// @tparam Rng The input range type.
		/// @param rng The range to pick from.
		/// @return A reference to a uniformly chosen element.
		/// @pre The range must not be empty.
		template <std::ranges::input_range Rng>
		decltype( auto ) pick( Rng&& rng )
		{
			const auto size = std::ranges::distance( rng );
			DEBUG_ASSERT( size != 0, "Range must not be empty" );
			return *pick_it_size_checked( std::forward<Rng>( rng ), size );
		}

		/// @brief Evaluates a @p chance in @p out_of probability.
		/// @tparam T The integral type of the chance values.
		/// @param chance The number of favourable outcomes.
		/// @param out_of The total number of possible outcomes.
		/// @return @c true with probability @p chance / @p out_of. Always @c false if either value is non-positive,
		/// and always @c true if @p chance is at least @p out_of.
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

		/// @brief Evaluates a one in @p out_of probability.
		/// @tparam T The integral type of the chance value.
		/// @param out_of The total number of possible outcomes.
		/// @return @c true with probability 1 / @p out_of.
		template <std::integral T>
		bool chance_one_in( T out_of )
		{
			return chance( 1, out_of );
		}

		/// @brief Flips a fair coin.
		/// @return @c true or @c false, each with probability one half.
		bool coin_toss()
		{
			return chance_one_in( 2 );
		}

		/// @brief Evaluates a probability expressed as a fraction in the range @c [0.0, 1.0].
		/// @tparam T The floating point type of the chance value.
		/// @param chance The probability of returning @c true, where @c 0.0 is never and @c 1.0 is always.
		/// @return @c true with the given probability.
		/// @pre @p chance must be in the range @c [0.0, 1.0].
		template <std::floating_point T>
		bool percent_chance( T chance )
		{
			DEBUG_ASSERT( chance >= T( 0 ) && chance <= T( 1 ), "chance must be in range [0.0, 1.0]" );
			// Uniform real distribution is [0, 1), so for a chance of 1 aka 100% every possible
			// generated value is less than 1 so returns true.
			return uniform( T( 0 ), T( 1 ) ) < chance;
		}

	private:
		template <std::ranges::input_range Rng>
		auto pick_it_size_checked( Rng&& rng, std::ranges::range_difference_t<Rng> size )
		{
			uniform_dist_for<std::ranges::range_difference_t<Rng>> dist( 0, size - 1 );
			const auto index = dist( engine );
			return std::ranges::next( std::ranges::begin( std::forward<Rng>( rng ) ), index );
		}

		engine_type engine;
	};
}
