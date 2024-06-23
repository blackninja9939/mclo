# CMakeList.txt : CMake project for mclo, include source and define
# project specific logic here.
#

set(LIBRARY_TARGET_NAME ${PROJECT_NAME})

# Add interal source files and non exported headers here
add_library( ${LIBRARY_TARGET_NAME}
    string_utils.cpp
)

# Make alias
add_library(${PROJECT_NAME}::${LIBRARY_TARGET_NAME} ALIAS ${LIBRARY_TARGET_NAME})

# Find and link dependencies
find_package(magic_enum 0.9.5 REQUIRED)

target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC magic_enum::magic_enum)

# Add compile options
target_compile_definitions(${LIBRARY_TARGET_NAME} PUBLIC MAGIC_ENUM_ENABLE_HASH MAGIC_ENUM_ENABLE_HASH_SWITCH )

# Automatically include all public headers from the include folder
target_include_directories( ${LIBRARY_TARGET_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
