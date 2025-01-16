#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace mclo
{ // These are not implemented as constexpr because Android does not support constexpr invoke and Sony does not detect
  // these as being constexpr safe literal types
#if defined( __cpp_lib_bind_front ) && ( __cpp_lib_bind_front >= 202306L )
	using std::bind_front;
#else
	template <typename Func, typename... Args>
	constexpr auto bind_front( Func&& func, Args&&... args )
	{
		return [ func = std::forward<Func>( func ),
				 ... captured = std::forward<Args>( args ) ]( auto&&... args ) -> decltype( auto ) {
			return std::invoke( func, captured..., std::forward<decltype( args )>( args )... );
		};
	}
	template <auto ConstFunc, typename... Args>
	constexpr auto bind_front( Args&&... args )
	{
		using Func = decltype( ConstFunc );
		if constexpr ( std::is_pointer_v<Func> || std::is_member_pointer_v<Func> )
		{
			static_assert( ConstFunc != nullptr );
		}
		return [... captured = std::forward<Args>( args ) ]( auto&&... args ) -> decltype( auto ) {
			return std::invoke( ConstFunc, captured..., std::forward<decltype( args )>( args )... );
		};
	}
#endif
#if defined( __cpp_lib_bind_back ) && ( __cpp_lib_bind_back >= 202306L )
	using std::bind_back;
#else
	template <typename Func, typename... Args>
	constexpr auto bind_back( Func&& func, Args&&... args )
	{
		return [ func = std::forward<Func>( func ),
				 ... captured = std::forward<Args>( args ) ]( auto&&... args ) -> decltype( auto ) {
			return std::invoke( func, std::forward<decltype( args )>( args )..., captured... );
		};
	}
	template <auto ConstFunc, typename... Args>
	constexpr auto bind_back( Args&&... args )
	{
		using Func = decltype( ConstFunc );
		if constexpr ( std::is_pointer_v<Func> || std::is_member_pointer_v<Func> )
		{
			static_assert( ConstFunc != nullptr );
		}
		return [... captured = std::forward<Args>( args ) ]( auto&&... args ) -> decltype( auto ) {
			return std::invoke( ConstFunc, std::forward<decltype( args )>( args )..., captured... );
		};
	}
#endif
}
