# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required( VERSION 3.28 )

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

set( CMAKE_COMPILE_WARNING_AS_ERROR ON )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

project(
	"mclo"
	VERSION 0.1.0
	DESCRIPTION "C++ Utility Library"
	LANGUAGES CXX
)

if ( MSVC )
	add_compile_options( 
		/W4
		/Zc:__cplusplus
		/Zc:enumTypes
		/Zc:inline
		/Zc:preprocessor
		/Zc:templateScope
		/Zc:throwingNew
		/wd5030
	)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
	add_compile_options(
		-Wno-attributes
	)
elseif( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )	
	add_compile_options(
		-Wno-unknown-attributes
		-Wno-c++2b-extensions # They are guarded by the feature flag ifdef already
	)
endif()

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Include sub-projects.
add_subdirectory("source")

if(BUILD_BENCHMARKS)
	add_subdirectory("benchmarks")
endif()

if(BUILD_TESTS)
	enable_testing()
	add_subdirectory("tests")
endif()
