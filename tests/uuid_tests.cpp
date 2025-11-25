#include <catch2/catch_test_macros.hpp>

#include <mclo/utility/uuid.hpp>

TEST_CASE( "uuid to_string correct format", "[uuid]" )
{
	static constexpr mclo::uuid u{
		std::array<std::byte, 16>{ std::byte{ 0x12 },
                                  std::byte{ 0x34 },
                                  std::byte{ 0x56 },
                                  std::byte{ 0x78 },
                                  std::byte{ 0x9A },
                                  std::byte{ 0xBC },
                                  std::byte{ 0xDE },
                                  std::byte{ 0xF0 },
                                  std::byte{ 0x12 },
                                  std::byte{ 0x34 },
                                  std::byte{ 0x56 },
                                  std::byte{ 0x78 },
                                  std::byte{ 0x9A },
                                  std::byte{ 0xBC },
                                  std::byte{ 0xDE },
                                  std::byte{ 0xF0 } }
    };

	const std::string str = u.to_string();

	CHECK( str == "12345678-9abc-def0-1234-56789abcdef0" );
}

TEST_CASE( "uuid generate produces different uuids", "[uuid]" )
{
    const mclo::uuid uuid1 = mclo::uuid::generate();
    const mclo::uuid uuid2 = mclo::uuid::generate();
    CHECK( uuid1 != uuid2 );
}
