find_package( benchmark REQUIRED )

add_executable( benchmarks
	# string_benchmarks.cpp
	# bitset_benchmark.cpp
	# enum_range_benchmarks.cpp
	enum_string_bi_map_benchmarks.cpp
)

target_link_libraries( benchmarks PRIVATE benchmark::benchmark benchmark::benchmark_main mclo )
