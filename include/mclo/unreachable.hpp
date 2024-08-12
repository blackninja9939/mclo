#pragma once

#include "platform.hpp"

#include <cassert>
#include <utility>

namespace mclo
{
#ifdef __cpp_lib_unreachable
	using std::unreachable;
#else
	[[noreturn]] void unreachable()
	{
#ifndef NDEBUG
		assert( false );
#endif
#if defined( _MSC_VER ) && !defined( __clang__ ) // MSVC
		__assume( false );
#elif MCLO_HAS_BUILTIN( __builtin_unreachable )  // GCC, Clang & Others
		__builtin_unreachable();
#endif
	}
#endif
}
