find_package( Catch2 3 REQUIRED )

add_executable( tests
	"consteval_check.hpp"
	"fancy_pointer.hpp"
	"assert_exception.hpp"
	"assert_macros.hpp"
	"assert_macros.cpp"
	"assert_listener.cpp"
	"assert_macro_tests.cpp"
	"array_tests.cpp"
	"bit_tests.cpp"
	"string_util_tests.cpp"
	"meta_tests.cpp"
	"mph_tests.cpp"
	"string_buffer_tests.cpp"
	"hash_tests.cpp"
	"tagged_ptr_tests.cpp"
	"small_optional_tests.cpp"
	"string_flyweight_tests.cpp"
	"math_tests.cpp"
	"slot_map_tests.cpp"
	"enum_map_tests.cpp"
	"enum_set_tests.cpp"
	"bitset_tests.cpp"
	"enum_range_tests.cpp"
	"new_type_tests.cpp"
	"small_vector_tests.cpp"
	"xoshiro256plusplus_tests.cpp"
	"splitmix64_tests.cpp"
	"random_generator_tests.cpp"
	"mutex_tests.cpp"
	"sem_version_tests.cpp"
	"try_tests.cpp"
	"not_null_tests.cpp"
	"intrusive_ptr_tests.cpp"
	"numeric_vector_tests.cpp"
	"hash_table_tests.cpp"
	"thread_local_key_tests.cpp"
	"instanced_thread_local_tests.cpp"
	"circular_buffer_tests.cpp"
	"intrusive_forward_list_tests.cpp"
	"pointer_variant_tests.cpp"
	"saturated_math_tests.cpp"
	"null_mutex_tests.cpp"
	"indirect_tests.cpp"
	"polymorphic_tests.cpp"
	"atomic_shared_ptr_tests.cpp"
)

target_compile_definitions( 
	tests 
	PUBLIC MCLO_CONFIG_ENABLE_TESTING
	PRIVATE CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
)
target_link_libraries( tests PRIVATE Catch2::Catch2WithMain mclo )

include(Catch)
catch_discover_tests(tests)
