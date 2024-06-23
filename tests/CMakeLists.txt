find_package( Catch2 3 REQUIRED )

add_executable( tests
	array_tests.cpp
	bit_tests.cpp
	string_util_tests.cpp
	meta_tests.cpp
	mph_tests.cpp
	string_buffer_tests.cpp
	hash_combine.cpp
	tagged_ptr_tests.cpp
)

target_link_libraries( tests PRIVATE Catch2::Catch2WithMain mclo )

include(Catch)
catch_discover_tests(tests)
