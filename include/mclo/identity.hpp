#pragma once

#include "platform.hpp"

#include <utility>

namespace mclo
{
	struct identity
	{
		template <typename T>
		[[nodiscard]] [[msvc::intrinsic]] MCLO_STATIC_CALL_OPERATOR constexpr T&& operator()( T&& value )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return std::forward<T>( value );
		}
	};
}
