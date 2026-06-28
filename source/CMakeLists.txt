# CMakeList.txt : CMake project for mclo, include source and define
# project specific logic here.
#

set(LIBRARY_TARGET_NAME ${PROJECT_NAME})

# Add interal source files and non exported headers here
add_library( ${LIBRARY_TARGET_NAME}
    "string/ascii_string_utils.cpp"
    "string/ascii_string_simd.hpp"
    "string/ascii_string_simd.cpp"
    "string/compare_ignore_case.cpp"
    "string/wide_convert.cpp"
    "hash/murmur_hash_3.cpp"
    "hash/rapidhash.cpp"
    "hash/xxhash.cpp"
    "random/xoshiro256plusplus.cpp"
    "random/splitmix64.cpp"
    "random/chacha.cpp"
    "threading/mutex.cpp"
    "threading/adaptive_waiter.cpp"
    "threading/condition_variable.cpp"
    "threading/thread_local_key.cpp"
    "threading/thread_properties.cpp"
    "platform/windows_wrapper.cpp"
    "platform/shared_library.cpp"
    "allocator/arena_allocator.cpp"
    "allocator/pool_allocator.cpp"
    "debug/assert.cpp"
    "debug/breakpoint.cpp"
    "debug/debugger_attached.cpp"
    "numeric/bit.cpp"
    "utility/sem_version.cpp"
    "utility/uuid.cpp"
)

set_target_properties(
    ${LIBRARY_TARGET_NAME}
    PROPERTIES VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Make alias
add_library(${PROJECT_NAME}::${LIBRARY_TARGET_NAME} ALIAS ${LIBRARY_TARGET_NAME})

# Link internal compile options (PRIVATE, BUILD_INTERFACE so they don't appear in exports)
target_link_libraries(${LIBRARY_TARGET_NAME} PRIVATE $<BUILD_INTERFACE:mclo_compile_options>)

# Find and link dependencies
find_package(xsimd CONFIG REQUIRED)
find_package(xxhash CONFIG REQUIRED)

target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC xsimd xxHash::xxhash)

# Link the platform dynamic loading library (dl) for shared_library on POSIX systems
target_link_libraries(${LIBRARY_TARGET_NAME} PRIVATE ${CMAKE_DL_LIBS})

# Require C++20 for consumers
target_compile_features(${LIBRARY_TARGET_NAME} PUBLIC cxx_std_20)

# Configurable default hasher (mclo::default_hasher) and random engine (mclo::default_random_generator).
# These are queried unconditionally by the headers; the cache variables below supply the defaults but a consuming
# project may override them, including pointing at a type/header that is not part of mclo.
set(MCLO_DEFAULT_HASHER "mclo::rapidhash" CACHE STRING
    "Fully qualified type name used by mclo::default_hasher")
set(MCLO_DEFAULT_HASHER_HEADER "mclo/hash/rapidhash.hpp" CACHE STRING
    "Include path of the header declaring MCLO_DEFAULT_HASHER")
set(MCLO_DEFAULT_RANDOM_ENGINE "mclo::xoshiro256plusplus" CACHE STRING
    "Fully qualified type name used by mclo::default_random_generator")
set(MCLO_DEFAULT_RANDOM_ENGINE_HEADER "mclo/random/xoshiro256plusplus.hpp" CACHE STRING
    "Include path of the header declaring MCLO_DEFAULT_RANDOM_ENGINE")

# PUBLIC so every consuming translation unit observes the same definitions, avoiding ODR mismatches.
target_compile_definitions(${LIBRARY_TARGET_NAME} PUBLIC
    MCLO_CONFIG_DEFAULT_HASHER=${MCLO_DEFAULT_HASHER}
    MCLO_CONFIG_DEFAULT_HASHER_HEADER="${MCLO_DEFAULT_HASHER_HEADER}"
    MCLO_CONFIG_DEFAULT_RANDOM_ENGINE=${MCLO_DEFAULT_RANDOM_ENGINE}
    MCLO_CONFIG_DEFAULT_RANDOM_ENGINE_HEADER="${MCLO_DEFAULT_RANDOM_ENGINE_HEADER}"
)

# Automatically include all public headers from the include folder
target_include_directories( ${LIBRARY_TARGET_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Export target
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    TARGETS ${LIBRARY_TARGET_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${LIBRARY_TARGET_NAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_TARGET_NAME}-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${LIBRARY_TARGET_NAME}
)

write_basic_package_version_file(
    "${LIBRARY_TARGET_NAME}-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    EXPORT ${LIBRARY_TARGET_NAME}-targets
    FILE ${LIBRARY_TARGET_NAME}-targets.cmake
    NAMESPACE ${LIBRARY_TARGET_NAME}::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${LIBRARY_TARGET_NAME}
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_TARGET_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_TARGET_NAME}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${LIBRARY_TARGET_NAME}
)

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
list(APPEND CPACK_SOURCE_IGNORE_FILES /.git/ /build/ .gitignore .DS_Store)

include(CPack)
