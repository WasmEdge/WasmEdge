# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# FindLLD.cmake - Custom find module for LLD
#
# LLD's installed LLDConfig.cmake uses find_package(LLVM ... EXACT ...) which
# fails when the installed LLVM has a different patch version than the one LLD
# was built against (e.g., LLD built with LLVM 22.1.0 but LLVM 22.1.1 is
# installed). This module bypasses LLDConfig.cmake entirely and directly
# includes LLDTargets.cmake, which is self-contained and does not enforce
# the EXACT version constraint.
#
# This module sets the same variables as LLDConfig.cmake:
#   LLD_FOUND, LLD_CMAKE_DIR, LLD_INCLUDE_DIRS, LLD_EXPORTED_TARGETS,
#   LLD_INSTALL_PREFIX

# Build list of directories to search for LLD cmake files.
set(_FindLLD_search_paths)
foreach(_dir IN ITEMS
    "${LLD_CMAKE_PATH}"
    "${LLD_DIR}"
    "${LLVM_LIBRARY_DIR}/cmake/lld"
    "${LLVM_DIR}/../lld")
  if(IS_DIRECTORY "${_dir}")
    list(APPEND _FindLLD_search_paths "${_dir}")
  endif()
endforeach()

# Locate LLDTargets.cmake in the search paths.
set(_FindLLD_targets_dir)
foreach(_dir IN LISTS _FindLLD_search_paths)
  if(EXISTS "${_dir}/LLDTargets.cmake")
    set(_FindLLD_targets_dir "${_dir}")
    break()
  endif()
endforeach()

if(NOT _FindLLD_targets_dir)
  if(LLD_FIND_REQUIRED)
    message(FATAL_ERROR "FindLLD: Could not find LLDTargets.cmake in: ${_FindLLD_search_paths}")
  endif()
  set(LLD_FOUND FALSE)
  return()
endif()

# Compute LLD install prefix from the cmake dir (go up 4 levels:
# <prefix>/lib/cmake/lld/ -> <prefix>/).
get_filename_component(_FindLLD_prefix "${_FindLLD_targets_dir}" REALPATH)
get_filename_component(_FindLLD_prefix "${_FindLLD_prefix}" DIRECTORY)
get_filename_component(_FindLLD_prefix "${_FindLLD_prefix}" DIRECTORY)
get_filename_component(_FindLLD_prefix "${_FindLLD_prefix}" DIRECTORY)

# Include LLDTargets.cmake directly from its original location.
# This bypasses LLDConfig.cmake (and its EXACT LLVM version check) while
# correctly resolving _IMPORT_PREFIX from CMAKE_CURRENT_LIST_FILE.
include("${_FindLLD_targets_dir}/LLDTargets.cmake")

# Set the same variables that LLDConfig.cmake would set.
set(LLD_INSTALL_PREFIX "${_FindLLD_prefix}")
set(LLD_CMAKE_DIR "${_FindLLD_targets_dir}")
set(LLD_INCLUDE_DIRS "${_FindLLD_prefix}/include")
set(LLD_EXPORTED_TARGETS "lldCommon;lld;lldCOFF;lldELF;lldMachO;lldMinGW;lldWasm")
set(LLD_FOUND TRUE)
set(LLD_DIR "${_FindLLD_targets_dir}" CACHE PATH "Path to LLD CMake config" FORCE)

message(STATUS "FindLLD: Found LLD at ${_FindLLD_prefix}")

# Cleanup temporary variables.
unset(_FindLLD_search_paths)
unset(_FindLLD_targets_dir)
unset(_FindLLD_prefix)
