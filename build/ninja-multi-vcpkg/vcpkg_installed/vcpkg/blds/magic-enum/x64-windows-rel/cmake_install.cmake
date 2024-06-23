# Install script for directory: C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/pkgs/magic-enum_x64-windows")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "OFF")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_all.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_containers.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_flags.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_format.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_fuse.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_iostream.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_switch.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/include/magic_enum/magic_enum_utility.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/magic_enum/magic_enumConfig.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/magic_enum/magic_enumConfig.cmake"
         "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/x64-windows-rel/CMakeFiles/Export/107f9e75136fefeee8fdb6853b45270d/magic_enumConfig.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/magic_enum/magic_enumConfig-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/magic_enum/magic_enumConfig.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/magic_enum" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/x64-windows-rel/CMakeFiles/Export/107f9e75136fefeee8fdb6853b45270d/magic_enumConfig.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/magic_enum" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/x64-windows-rel/magic_enumConfigVersion.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/x64-windows-rel/magic_enum.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/magic_enum" TYPE FILE FILES "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/src/v0.9.5-591517a8a1.clean/package.xml")
endif()

if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
  file(WRITE "C:/Users/Matthew Clohessy/source/repos/mclo/build/ninja-multi-vcpkg/vcpkg_installed/vcpkg/blds/magic-enum/x64-windows-rel/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
