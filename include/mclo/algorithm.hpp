#pragma once

#include "constant_evaluated.hpp"

#include <algorithm>

namespace mclo
{
#ifdef __cpp_lib_constexpr_algorithms
	using std::copy;
	using std::fill;
	using std::fill_n;
	using std::iter_swap;
	using std::reverse;
#else
	namespace detail
	{
		template <typename T>
		constexpr void swap( T& lhs, T& rhs )
		{
			T tmp = std::move( lhs );
			lhs = std::move( rhs );
			rhs = std::move( tmp );
		}
	}

	template <typename ForwardIt1, typename ForwardIt2>
	constexpr void iter_swap( ForwardIt1 a, ForwardIt2 b )
	{
		detail::swap( *a, *b );
	}

	template <typename InputIt, typename OutputIt>
	constexpr OutputIt copy( InputIt first, InputIt last, OutputIt out )
	{
		if ( mclo::is_constant_evaluated() )
		{
			while ( first != last )
			{
				*out++ = *first++;
			}
			return out;
		}
		else
		{
			return std::copy( first, last, out );
		}
	}

	template <typename BirdirIt>
	constexpr void reverse( BirdirIt first, BirdirIt last )
	{
		if ( mclo::is_constant_evaluated() )
		{
			while ( first != last && first != --last )
			{
				mclo::iter_swap( first++, last );
			}
		}
		else
		{
			std::reverse( first, last );
		}
	}

	template <typename ForwardIt, typename T>
	constexpr void fill( ForwardIt first, ForwardIt last, const T& value )
	{
		if ( mclo::is_constant_evaluated() )
		{
			while ( first != last )
			{
				*first++ = value;
			}
		}
		else
		{
			std::fill( first, last, value );
		}
	}

	template <typename OutputIt, typename Size, typename T>
	constexpr void fill_n( OutputIt it, Size size, const T& value )
	{
		if ( mclo::is_constant_evaluated() )
		{
			while ( size-- > 0 )
			{
				*it++ = value;
			}
		}
		else
		{
			std::fill_n( it, size, value );
		}
	}
#endif
}
