#include "mclo/utility/uuid.hpp"

#include <bit>
#include <random>

namespace
{
	std::array<std::byte, 16> generate_128_random_bits()
	{
		std::array<unsigned int, 4> bytes;
		std::random_device rd;
		std::ranges::generate( bytes, std::ref( rd ) );
		return std::bit_cast<std::array<std::byte, 16>>( bytes );
	}
}

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
		// Generate 128 random bits
		uuid new_uuid{ generate_128_random_bits() };
		// Set the version to 4 aka random UUID
		new_uuid.bytes[ 6 ] = ( new_uuid.bytes[ 6 ] & std::byte{ 0x0F } ) | std::byte{ 0x40 };
		// Set the variant to RFC 4122
		new_uuid.bytes[ 8 ] = ( new_uuid.bytes[ 8 ] & std::byte{ 0x3F } ) | std::byte{ 0x80 };
		return new_uuid;
	}

	std::string format_as( const uuid& u )
	{
		return u.to_string();
	}
}
