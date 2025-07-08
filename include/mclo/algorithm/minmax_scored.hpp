#pragma once

#include <concepts>
#include <iterator>
#include <utility>

namespace mclo
{
	template <std::forward_iterator It,
			  std::sentinel_for<It> Sentinel,
			  std::invocable<std::iter_reference_t<It>> Scorer,
			  typename Compare = std::less<>>
	[[nodiscard]] constexpr It min_scored( It first, Sentinel last, Scorer score, Compare compare = {} )
	{
		if ( first == last )
		{
			return first;
		}

		It min_it = first;
		auto min_score = std::invoke( score, *min_it );

		++first;

		for ( ; first != last; ++first )
		{
			auto next_score = std::invoke( score, *first );
			if ( std::invoke( compare, next_score, min_score ) )
			{
				min_it = first;
				min_score = std::move( next_score );
			}
		}

		return min_it;
	}

	template <std::forward_iterator It,
			  std::sentinel_for<It> Sentinel,
			  std::invocable<std::iter_reference_t<It>> Scorer,
			  typename Compare = std::less<>>
	[[nodiscard]] constexpr It max_scored( It first, Sentinel last, Scorer score, Compare compare = {} )
	{
		if ( first == last )
		{
			return first;
		}

		It max_it = first;
		auto max_score = std::invoke( score, *max_it );

		++first;

		for ( ; first != last; ++first )
		{
			auto next_score = std::invoke( score, *first );
			if ( std::invoke( compare, max_score, next_score ) )
			{
				max_it = first;
				max_score = std::move( next_score );
			}
		}

		return max_it;
	}

	template <std::forward_iterator It,
			  std::sentinel_for<It> Sentinel,
			  std::invocable<std::iter_reference_t<It>> Scorer,
			  typename Compare = std::less<>>
	[[nodiscard]] constexpr std::pair<It, It> minmax_scored( It first,
															 Sentinel last,
															 Scorer score,
															 Compare compare = {} )
	{
		if ( first == last )
		{
			return { first, first };
		}

		It min_it = first;
		It max_it = min_it;
		auto min_score = std::invoke( score, *min_it );
		auto max_score = min_score;

		++first;

		for ( ; first != last; ++first )
		{
			auto next_score = std::invoke( score, *first );
			if ( std::invoke( compare, next_score, min_score ) )
			{
				min_it = first;
				min_score = std::move( next_score );
			}
			else if ( std::invoke( compare, max_score, next_score ) )
			{
				max_it = first;
				max_score = std::move( next_score );
			}
		}

		return { min_it, max_it };
	}
}
