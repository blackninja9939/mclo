find_package( Catch2 3 REQUIRED )

add_executable( tests
	consteval_check.h
	array_tests.cpp
	bit_tests.cpp
	string_util_tests.cpp
	meta_tests.cpp
	mph_tests.cpp
	string_buffer_tests.cpp
	hash_tests.cpp
	tagged_ptr_tests.cpp
	small_optional_tests.cpp
	string_table_tests.cpp
	math_tests.cpp
	slot_map_tests.cpp
	enum_map_tests.cpp
	bitset_tests.cpp
	enum_range_tests.cpp
	new_type_tests.cpp
	small_vector_tests.cpp
	xoshiro256plusplus_tests.cpp
	splitmix64_tests.cpp
	random_generator_tests.cpp
)

target_compile_definitions( tests PRIVATE CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS )
target_link_libraries( tests PRIVATE Catch2::Catch2WithMain mclo )

include(Catch)
catch_discover_tests(tests)
