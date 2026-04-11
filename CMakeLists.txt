# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required( VERSION 3.28 )

option(BUILD_BENCHMARKS "Build benchmarks" OFF)

project(
	"mclo"
	VERSION 0.1.0
	DESCRIPTION "C++ Utility Library"
	LANGUAGES CXX
)

set( CMAKE_COMPILE_WARNING_AS_ERROR ON )
set( CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>" )

# Internal compile options shared across all targets (not exported to consumers)
add_library( mclo_compile_options INTERFACE )
if ( MSVC )
	target_compile_options( mclo_compile_options INTERFACE
		/W4
		/Zc:__cplusplus
		/Zc:enumTypes
		/Zc:inline
		/Zc:preprocessor
		/Zc:templateScope
		/Zc:throwingNew
		/wd5030
		/w44062
	)
	target_compile_definitions( mclo_compile_options INTERFACE _CRT_SECURE_NO_WARNINGS )
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
	target_compile_options( mclo_compile_options INTERFACE -Wno-attributes )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
	target_compile_options( mclo_compile_options INTERFACE -Wno-unknown-attributes )
endif()

# Include sub-projects.
add_subdirectory("source")

if(BUILD_BENCHMARKS)
	add_subdirectory("benchmarks")
endif()

include(CTest)
if(BUILD_TESTING)
	add_subdirectory("tests")
endif()
