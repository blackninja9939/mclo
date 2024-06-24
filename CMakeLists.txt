# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required( VERSION 3.28 )

set( CMAKE_COMPILE_WARNING_AS_ERROR ON )
set( CMAKE_CXX_STANDARD 17 )

project ("mclo")

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
elseif( CMAKE_CXX_COMILER_ID STREQUAL "GNU" )
	add_compile_options(
		-Wno-attributes=msvc::
	)
elseif( CMAKE_CXX_COMILER_ID MATCHES "Clang" )	
	add_compile_options(
		-Wno-unknown-attributes
	)
endif()

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Include sub-projects.
add_subdirectory ("source")

enable_testing()
add_subdirectory ("tests")
