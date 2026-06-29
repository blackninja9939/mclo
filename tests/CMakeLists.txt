find_package( Catch2 3 REQUIRED )

add_executable( tests
	"consteval_check.hpp"
	"fancy_pointer.hpp"
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
	"strong_type_tests.cpp"
	"small_vector_tests.cpp"
	"xoshiro256plusplus_tests.cpp"
	"splitmix64_tests.cpp"
	"seed_mixing_tests.cpp"
	"chacha_tests.cpp"
	"random_generator_tests.cpp"
	"weighted_random_tests.cpp"
	"weighted_vector_tests.cpp"
	"mutex_tests.cpp"
	"sem_version_tests.cpp"
	"synchronized_tests.cpp"
	"try_tests.cpp"
	"intrusive_ptr_tests.cpp"
	"thread_local_key_tests.cpp"
	"instanced_thread_local_tests.cpp"
	"circular_buffer_tests.cpp"
	"intrusive_forward_list_tests.cpp"
	"pointer_variant_tests.cpp"
	"null_mutex_tests.cpp"
	"indirect_tests.cpp"
	"polymorphic_tests.cpp"
	"copy_on_write_tests.cpp"
	"checked_math_tests.cpp"
	"overflowing_math_tests.cpp"
	"saturated_math_tests.cpp"
	"normalized_float_tests.cpp"
	"fixed_point_tests.cpp"
	"type_id_set_tests.cpp"
	"lazy_convert_construct_tests.cpp"
	"wide_convert_tests.cpp"
	"minmax_scored_tests.cpp"
	"loop_unroll_tests.cpp"
	"morton_index_tests.cpp"
	"timer_tests.cpp"
	"uuid_tests.cpp"
	"spin_mutex_tests.cpp"
	"allocate_unique_tests.cpp"
	"work_stealing_deque_tests.cpp"
	"static_string_tests.cpp"
	"flexible_array_tests.cpp"
	"scope_guard_tests.cpp"
	"hex_convert_tests.cpp"
	"packed_int_vector_tests.cpp"
	"packed_int_array_tests.cpp"
	"radix_sort_tests.cpp"
	"triangular_layout_tests.cpp"
	"shared_library_tests.cpp"
	"map_lookup_tests.cpp"
	"atomic128_tests.cpp"
	"atomic_shared_ptr_tests.cpp"
	"state_machine_tests.cpp"
)

target_compile_definitions( 
	tests 
	PRIVATE
		CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
)
target_link_libraries( tests PRIVATE Catch2::Catch2WithMain mclo mclo_compile_options )

# A small shared library loaded at runtime by the shared_library tests
add_library( test_module SHARED "test_module/test_module.cpp" )
target_link_libraries( test_module PRIVATE mclo )
add_dependencies( tests test_module )
target_compile_definitions( tests PRIVATE TEST_MODULE_PATH="$<TARGET_FILE:test_module>" )

include(Catch)
catch_discover_tests(tests)
