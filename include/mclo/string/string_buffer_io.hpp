#pragma once

#include "string_buffer.hpp"

#include <iosfwd>
#include <locale>

namespace mclo
{
	template <typename CharT, std::size_t N>
	std::basic_ostream<CharT, std::char_traits<CharT>>& operator<<(
		std::basic_ostream<CharT, std::char_traits<CharT>>& os, const basic_string_buffer<CharT, N>& str )
	{
		using view = typename basic_string_buffer<CharT, N>::view_type;
		return os << view( str );
	}

	template <typename CharT, std::size_t N>
	std::basic_istream<CharT, std::char_traits<CharT>>& operator>>(
		std::basic_istream<CharT, std::char_traits<CharT>>& is, basic_string_buffer<CharT, N>& str )
	{
		using traits = std::char_traits<CharT>;
		using istream_type = std::basic_istream<CharT, traits>;
		using char_ctype = std::ctype<CharT>;
		using string_type = basic_string_buffer<CharT, N>;
		using size_type = typename string_type::size_type;

		typename istream_type::iostate state = istream_type::goodbit;
		bool changed = false;
		const typename istream_type::sentry ok( is );

		if ( ok )
		{
			const char_ctype& ctype_facet = std::use_facet<char_ctype>( is.getloc() );
			str.clear();

			try
			{
				size_type size;
				if ( 0 < is.width() && static_cast<size_type>( is.width() ) < str.max_size() )
				{
					size = static_cast<size_type>( is.width() );
				}
				else
				{
					size = str.max_size();
				}

				typename traits::int_type meta = is.rdbuf()->sgetc();

				for ( ; 0 < size; --size, meta = is.rdbuf()->snextc() )
				{
					if ( traits::eq_int_type( traits::eof(), meta ) )
					{
						state |= istream_type::eofbit;
						break;
					}
					else if ( ctype_facet.is( char_ctype::space, traits::to_char_type( meta ) ) )
					{
						break;
					}
					else
					{
						str.push_back( traits::to_char_type( meta ) );
						changed = true;
					}
				}
			}
			catch ( ... )
			{
				try
				{
					is.setstate( istream_type::badbit );
				}
				catch ( ... )
				{
					// Discard the exception this sets to re-raise our more meaningful one
				}
				throw;
			}
		}

		is.width( 0 );
		if ( !changed )
		{
			state |= istream_type::failbit;
		}

		is.setstate( state );
		return is;
	}
}
