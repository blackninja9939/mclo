#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace mclo
{
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

	/// @brief Binds @p args to the front of the compile-time callable @p ConstFunc.
	/// @details Like std::bind_front but the callable is supplied as a non-type template parameter, so it is baked
	/// into the type and stored with no runtime footprint. The returned object invokes @p ConstFunc with the bound
	/// arguments followed by any arguments passed at the call site.
	/// @tparam ConstFunc The callable to invoke, supplied as a non-type template parameter.
	/// @tparam Args The types of the arguments to bind to the front.
	/// @param args The arguments to bind to the front of each call.
	/// @return A callable that invokes @p ConstFunc with the bound arguments prepended to its call arguments.
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

	/// @brief Binds @p args to the back of the compile-time callable @p ConstFunc.
	/// @details Like std::bind_back but the callable is supplied as a non-type template parameter, so it is baked
	/// into the type and stored with no runtime footprint. The returned object invokes @p ConstFunc with any
	/// arguments passed at the call site followed by the bound arguments.
	/// @tparam ConstFunc The callable to invoke, supplied as a non-type template parameter.
	/// @tparam Args The types of the arguments to bind to the back.
	/// @param args The arguments to bind to the back of each call.
	/// @return A callable that invokes @p ConstFunc with the bound arguments appended to its call arguments.
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
