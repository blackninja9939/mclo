#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace mclo
{
	struct uuid
	{
		std::array<std::byte, 16> bytes{};

		[[nodiscard]] constexpr auto operator<=>( const uuid& other ) const noexcept = default;

		[[nodiscard]] std::string to_string() const;

		[[nodiscard]] static uuid generate();
	};

	std::string format_as( const uuid& u );
}
