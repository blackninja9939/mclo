﻿# CMakeList.txt : CMake project for mclo, include source and define
# project specific logic here.
#

set(LIBRARY_TARGET_NAME ${PROJECT_NAME})

# Add interal source files and non exported headers here
add_library( ${LIBRARY_TARGET_NAME}
    "string/ascii_string_utils.cpp"
    "string/ascii_string_simd.cpp"
    "string/string_utils.cpp"
)

# Make alias
add_library(${PROJECT_NAME}::${LIBRARY_TARGET_NAME} ALIAS ${LIBRARY_TARGET_NAME})

# Find and link dependencies
find_package(xsimd CONFIG REQUIRED)

target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC xsimd)

# Automatically include all public headers from the include folder
target_include_directories( ${LIBRARY_TARGET_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
