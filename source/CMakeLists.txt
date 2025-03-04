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
    "hash/murmur_hash_3.cpp"
    "hash/rapidhash.cpp"
    "random/xoshiro256plusplus.cpp"
    "random/splitmix64.cpp"
    "threading/mutex.cpp"
    "threading/condition_variable.cpp"
    "threading/thread_local_key.cpp"
    "threading/thread_properties.cpp"
    "threading/windows_wrapper.h"
    "allocator/default_resource.cpp"
    "allocator/linear_allocator.cpp"
)

set_target_properties(
    ${LIBRARY_TARGET_NAME}
    PROPERTIES VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Make alias
add_library(${PROJECT_NAME}::${LIBRARY_TARGET_NAME} ALIAS ${LIBRARY_TARGET_NAME})

# Find and link dependencies
find_package(fmt CONFIG REQUIRED)
find_package(xsimd CONFIG REQUIRED)
find_package(libassert CONFIG REQUIRED)

target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC fmt::fmt xsimd libassert::assert)

# Add compile definitions
target_compile_definitions( 
    ${LIBRARY_TARGET_NAME}
    PUBLIC
        LIBASSERT_USE_FMT
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
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY_TARGET_NAME}
)

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
list(APPEND CPACK_SOURCE_IGNORE_FILES /.git/ /build/ .gitignore .DS_Store)

include(CPack)
