#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "benchmark::benchmark" for configuration "Release"
set_property(TARGET benchmark::benchmark APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(benchmark::benchmark PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/benchmark.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/benchmark.dll"
  )

list(APPEND _cmake_import_check_targets benchmark::benchmark )
list(APPEND _cmake_import_check_files_for_benchmark::benchmark "${_IMPORT_PREFIX}/lib/benchmark.lib" "${_IMPORT_PREFIX}/bin/benchmark.dll" )

# Import target "benchmark::benchmark_main" for configuration "Release"
set_property(TARGET benchmark::benchmark_main APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(benchmark::benchmark_main PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/benchmark_main.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/benchmark_main.dll"
  )

list(APPEND _cmake_import_check_targets benchmark::benchmark_main )
list(APPEND _cmake_import_check_files_for_benchmark::benchmark_main "${_IMPORT_PREFIX}/lib/benchmark_main.lib" "${_IMPORT_PREFIX}/bin/benchmark_main.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
