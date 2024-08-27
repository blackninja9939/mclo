find_package( benchmark REQUIRED )

add_executable( benchmarks
	string_benchmarks.cpp
	bitset_benchmark.cpp
)

target_link_libraries( benchmarks PRIVATE benchmark::benchmark benchmark::benchmark_main mclo )
