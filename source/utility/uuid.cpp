#include "mclo/utility/uuid.hpp"

#include <random>

namespace mclo
{
	std::string uuid::to_string() const
	{
		static constexpr char hex_chars[] = "0123456789abcdef";
		char result[ 36 ];
		std::size_t pos = 0;

		for ( std::size_t i = 0; i < bytes.size(); ++i )
		{
			const auto byte = std::to_integer<std::uint8_t>( bytes[ i ] );
			result[ pos++ ] = hex_chars[ ( byte >> 4 ) & 0x0F ];
			result[ pos++ ] = hex_chars[ byte & 0x0F ];

			if ( i == 3 || i == 5 || i == 7 || i == 9 )
			{
				result[ pos++ ] = '-';
			}
		}

		return { result, std::size( result ) };
	}

	uuid uuid::generate()
	{
		std::random_device rd;
		return generate( rd );
	}

	std::string format_as( const uuid& u )
	{
		return u.to_string();
	}
}
