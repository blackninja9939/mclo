#include "mclo/utility/uuid.hpp"

#include <random>

namespace mclo
{
	std::string uuid::to_string() const
	{
		char result[ 36 ];
		std::size_t pos = 0;

		for ( std::size_t i = 0; i < bytes.size(); ++i )
		{
			const auto byte = std::to_integer<std::uint8_t>( bytes[ i ] );
			result[ pos++ ] = to_hex( byte >> 4 );
			result[ pos++ ] = to_hex( byte );

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
