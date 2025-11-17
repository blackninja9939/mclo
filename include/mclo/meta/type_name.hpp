#pragma once

#include "mclo/string/fixed_string.hpp"
#include "mclo/platform/compiler_detection.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <typename T>
		[[nodiscard]] constexpr auto extract_type_name()
		{
#if defined( MCLO_COMPILER_CLANG )
			constexpr auto prefix = std::string_view{ "[T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined( MCLO_COMPILER_GCC )
			constexpr auto prefix = std::string_view{ "with T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined( MCLO_COMPILER_MSVC )
			constexpr auto prefix = std::string_view{ "extract_type_name<" };
			constexpr auto suffix = std::string_view{ ">(void)" };
			constexpr auto function = std::string_view{ __FUNCSIG__ };
#else
#error Unsupported compiler
#endif

			constexpr auto start = function.find( prefix ) + prefix.size();
			constexpr auto end = function.rfind( suffix );

			static_assert( start < end );

			constexpr auto name = function.substr( start, ( end - start ) );
			return fixed_string<name.size()>( name );
		}

		template <typename T>
		constexpr fixed_string name_buffer = extract_type_name<T>();
	}

	template <typename T>
	constexpr std::string_view type_name_v = detail::name_buffer<T>;
}
