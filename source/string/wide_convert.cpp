#include "mclo/string/wide_convert.hpp"

#ifdef _WIN32
#include "mclo/platform/windows_wrapper.h"
#else
#include <cwchar>
#endif

#include <stdexcept>
#include <system_error>

std::wstring mclo::to_wstring( const std::string_view str )
{
	if ( str.empty() )
	{
		return {};
	}

#ifdef _WIN32
	if ( str.size() > std::numeric_limits<int>::max() )
	{
		throw std::length_error( "String size exceeds maximum length for conversion to wide string" );
	}

	const int size_needed = MultiByteToWideChar( CP_UTF8, 0, str.data(), static_cast<int>( str.size() ), nullptr, 0 );
	if ( size_needed <= 0 )
	{
		throw std::system_error( last_error_code(), "Failed to convert string to wide string" );
	}

	std::wstring result( size_needed, L'\0' );
	MultiByteToWideChar( CP_UTF8, 0, str.data(), static_cast<int>( str.size() ), result.data(), size_needed );
	return result;
#else
	std::mbstate_t state{};
	const int size = std::mbsrtowcs( nullptr, &str.data(), 0, &state );
	if ( size < 0 )
	{
		throw std::system_error( std::make_error_code( errno ),
								 "Failed to determine size for conversion to wide string" );
	}

	std::wstring result( size, L'\0' );
	std::mbsrtowcs( result.data(), &str.data(), str.size(), &state );
	return result;
#endif
}
