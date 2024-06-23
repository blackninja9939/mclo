#pragma once

#include "constant_evaluated.hpp"

#include <numeric>

namespace mclo
{
#ifdef __cpp_lib_constexpr_numeric
	using std::accumulate;
	using std::iota;
#else
	template <typename InputIt, typename T, typename Func>
	[[nodiscard]] constexpr T accumulate( InputIt first, InputIt last, T starting_value, Func func )
	{
		while ( first != last )
		{
			starting_value = func( starting_value, *first++ );
		}
		return starting_value;
	}

	template <typename ForwardIt, typename T>
	constexpr void iota( ForwardIt first, ForwardIt last, T value )
	{
		while ( first != last )
		{
			*first++ = value++;
		}
	}
#endif
}
