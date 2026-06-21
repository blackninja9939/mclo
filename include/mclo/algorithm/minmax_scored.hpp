#pragma once

#include <concepts>
#include <iterator>
#include <utility>

namespace mclo
{
	/// @brief Finds the element with the smallest score, as produced by a scoring function.
	/// @details Each element is scored once via @p score and the scores compared with @p compare.
	/// @tparam It Forward iterator type for the input range.
	/// @tparam Sentinel Sentinel type for @p It.
	/// @tparam Scorer Invocable mapping an element to a comparable score.
	/// @tparam Compare Binary predicate establishing a strict ordering of scores.
	/// @param first Iterator to the first element of the range.
	/// @param last Sentinel denoting the end of the range.
	/// @param score Functor producing the score for each element.
	/// @param compare Comparator returning true when the first score orders before the second.
	/// @return Iterator to the lowest-scored element, or @p last if the range is empty. The first
	/// such element is returned on ties.
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

	/// @brief Finds the element with the largest score, as produced by a scoring function.
	/// @details Each element is scored once via @p score and the scores compared with @p compare.
	/// @tparam It Forward iterator type for the input range.
	/// @tparam Sentinel Sentinel type for @p It.
	/// @tparam Scorer Invocable mapping an element to a comparable score.
	/// @tparam Compare Binary predicate establishing a strict ordering of scores.
	/// @param first Iterator to the first element of the range.
	/// @param last Sentinel denoting the end of the range.
	/// @param score Functor producing the score for each element.
	/// @param compare Comparator returning true when the first score orders before the second.
	/// @return Iterator to the highest-scored element, or @p last if the range is empty. The first
	/// such element is returned on ties.
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

	/// @brief Finds the elements with the smallest and largest scores in a single pass.
	/// @details Each element is scored once via @p score and the scores compared with @p compare.
	/// @tparam It Forward iterator type for the input range.
	/// @tparam Sentinel Sentinel type for @p It.
	/// @tparam Scorer Invocable mapping an element to a comparable score.
	/// @tparam Compare Binary predicate establishing a strict ordering of scores.
	/// @param first Iterator to the first element of the range.
	/// @param last Sentinel denoting the end of the range.
	/// @param score Functor producing the score for each element.
	/// @param compare Comparator returning true when the first score orders before the second.
	/// @return A pair of iterators to the lowest- and highest-scored elements respectively, or
	/// { last, last } if the range is empty.
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
