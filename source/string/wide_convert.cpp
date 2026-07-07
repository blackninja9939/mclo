#include "mclo/string/wide_convert.hpp"

#include "mclo/platform/os_detection.hpp"

#ifdef MCLO_OS_WINDOWS
#include "mclo/platform/windows_wrapper.hpp"
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

#ifdef MCLO_OS_WINDOWS
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
	std::mbsrtowcs( result.data(), &str.data(), size, &state );
	return result;
#endif
}

std::string mclo::from_wstring( const std::wstring_view str )
{
	if ( str.empty() )
	{
		return {};
	}

#ifdef MCLO_OS_WINDOWS
	if ( str.size() > std::numeric_limits<int>::max() )
	{
		throw std::length_error( "String size exceeds maximum length for conversion to narrow string" );
	}

	const int size_needed =
		WideCharToMultiByte( CP_UTF8, 0, str.data(), static_cast<int>( str.size() ), nullptr, 0, nullptr, nullptr );
	if ( size_needed <= 0 )
	{
		throw std::system_error( last_error_code(), "Failed to convert wide string to string" );
	}

	std::string result( size_needed, '\0' );
	WideCharToMultiByte(
		CP_UTF8, 0, str.data(), static_cast<int>( str.size() ), result.data(), size_needed, nullptr, nullptr );
	return result;
#else
	std::mbstate_t state{};
	const wchar_t* src = str.data();
	const int size = std::wcsrtombs( nullptr, &src, 0, &state );
	if ( size < 0 )
	{
		throw std::system_error( std::make_error_code( errno ),
								 "Failed to determine size for conversion to narrow string" );
	}

	std::string result( size, '\0' );
	std::wcsrtombs( result.data(), &src, size, &state );
	return result;
#endif
}
