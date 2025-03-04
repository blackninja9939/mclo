#include "mclo/string/string_utils.hpp"

#include <cwctype>

namespace mclo
{
	void to_upper( std::wstring& string ) noexcept
	{
		for ( wchar_t& c : string )
		{
			c = std::towupper( c );
		}
	}

	void to_lower( std::wstring& string ) noexcept
	{
		for ( wchar_t& c : string )
		{
			c = std::towlower( c );
		}
	}
}
