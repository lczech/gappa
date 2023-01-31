# gappa - Genesis Applications for Phylogenetic Placement Analysis
# Copyright (C) 2017-2022 Lucas Czech
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact:
# Lucas Czech <lczech@carnegiescience.edu>
# Department of Plant Biology, Carnegie Institution For Science
# 260 Panama Street, Stanford, CA 94305, USA

# --------------------------------------------------------------------------------------------------
#   CMake Init
# --------------------------------------------------------------------------------------------------

cmake_minimum_required (VERSION 2.8.7 FATAL_ERROR)

# Fun with colors!
if (NOT WIN32)
    string(ASCII 27 Esc)

    set(ColorBlue   "${Esc}[34m") # Build information.
    set(ColorGreen  "${Esc}[32m") # Optional additions.
    set(ColorYellow "${Esc}[33m") # Tipps for the user.
    set(ColorRed    "${Esc}[31m") # Warnings and Errors.
    set(ColorEnd    "${Esc}[0m")
endif()

set (CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set (CMAKE_DISABLE_SOURCE_CHANGES  ON)

# Need to make sure that AppleClang is actually recognized and not mistakingly identifyed
# as normal Clang. See https://cmake.org/cmake/help/v3.0/policy/CMP0025.html
# This is important for the test cmake script, where we need to set some warnings for clang,
# but not for apple clang.
if( NOT ( CMAKE_VERSION VERSION_LESS 3.0 ))
    # Appleclang compatibility
    cmake_policy(SET CMP0025 NEW)
    # Support for :: aliasing, used in new way to include OpenMP
    cmake_policy(SET CMP0028 NEW)
endif()

# --------------------------------------------------------------------------------------------------
#   Project Definition and Requirements
# --------------------------------------------------------------------------------------------------

project ( gappa CXX )

# --------------------------------------------------------------------------------------------------
#   Build Options
# --------------------------------------------------------------------------------------------------

# The build type is determined by the standard cmake variable CMAKE_BUILD_TYPE.
# Thus, if you want to change the build type, call `cmake .. -DCMAKE_BUILD_TYPE=DEBUG`
IF(CMAKE_BUILD_TYPE STREQUAL "")
    set (CMAKE_BUILD_TYPE RELEASE)
ENDIF()

# Print build type. Useful for debugging user issues.
message (STATUS "${ColorBlue}Gappa build type: ${CMAKE_BUILD_TYPE}${ColorEnd}")

# Store everything in `bin`.
set( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin )

# Option to enable building a fully static version, that is, statical linking of system libraries.
# This works with GCC, but might cause trouble with OpenMP and/or Clang, so it is disabled by
# default. Most users who compile on their own probaby won't need the fully portable version.
# Genesis on the other hand is always linked statically, in order to be able to move around the
# final binary at least on the same system.
option ( GAPPA_BUILD_STATIC  "Build the static version of the executable." OFF )

if( GAPPA_BUILD_STATIC AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    string(ASCII 27 Esc)
    message( STATUS "${Esc}[31m"
        "Compiler is not GCC. This might cause trouble with statically linking system libraries "
        "on Linux systems. If the compilation fails with some linking error, try a different compiler, "
        "or try to not build statically by deactivating the CMake option 'GAPPA_BUILD_STATIC'. "
    "${Esc}[0m" )
endif()

# --------------------------------------------------------------------------------------------------
#   Compiler and Linker Options
# --------------------------------------------------------------------------------------------------

if (MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
    # MinGW/MSVC do not support much. Also, the above string comparison in the condition does
    # not work there, hence the extra `MSVC` check...
    set (WARN_FLAGS "-Wall")
else ()
    # For other operating systems, set the warn flags to a very high level - except unknown pragmas,
    # as this causes needless warnings with OpenMP and other pragma-based techniques.
    set (WARN_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -pedantic -pedantic-errors")

    # Furthermore, in debug mode, we also want all warnings to be errors, so that we do not miss them
    # In release, this might hinder users to successfully build genesis if any warnings remain
    # or occur only on their compiler, so let's avoid trouble for them.
    IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        set (WARN_FLAGS "${WARN_FLAGS} -Werror")
    ENDIF()
endif ()

set (CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -std=c++11 ${WARN_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG   "-O2 -DDEBUG -g -ggdb3 -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC" )
set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" )

IF(GAPPA_BUILD_STATIC)
    # Use different ways of telling CMake to build static, just to be sure.
    SET(BUILD_SHARED_LIBRARIES OFF)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    link_libraries("-static")
ENDIF()
message( STATUS "Static linking of system libraries: ${GAPPA_BUILD_STATIC}")

# Genesis uses LTO, and so do we.
if( NOT ( CMAKE_VERSION VERSION_LESS 3.9 ))
    cmake_policy(SET CMP0069 NEW)
endif()

# --------------------------------------------------------------------------------------------------
#   Download Dependencies
# --------------------------------------------------------------------------------------------------

# Downloads dependencies if they are not there. We use a special script for this.
include( "${CMAKE_CURRENT_LIST_DIR}/tools/cmake/DownloadDependency.cmake" )

# Store the commit hashes of our dependencies.
# These are replaced by tools/cmake/update_dependencies.sh to the hashes that are currently checked out.
# Thus, do not replace the hashes manually!
SET( CLI11_COMMIT_HASH "13becaddb657eacd090537719a669d66d393b8b2" ) #CLI11_COMMIT_HASH#
SET( genesis_COMMIT_HASH "60a519711602f14d6c44765a477a104ac009e847" ) #genesis_COMMIT_HASH#
SET( sparsepp_COMMIT_HASH "6bfe3b4bdb364993e612d6bb729d680cf4c77649" ) #sparsepp_COMMIT_HASH#

# Call the github download function, which takes four arguments:
#  - LIBPATH    : Path to the libracy dir where dependencies are stored.
#  - LIBNAME    : Name of the dependency, that is, the name of its main directory within the ${LIBPATH}.
#  - TESTFILE   : A testfile to check if the dependency is already there.
#  - REPOSITORY : Github repository (user/reponame)
#  - COMMITHASH : Hash of the commit to check out
DOWNLOAD_GITHUB_DEPENDENCY( "${PROJECT_SOURCE_DIR}/libs" "CLI11"    "CMakeLists.txt" "CLIUtils/CLI11"    ${CLI11_COMMIT_HASH} )
DOWNLOAD_GITHUB_DEPENDENCY( "${PROJECT_SOURCE_DIR}/libs" "genesis"  "CMakeLists.txt" "lczech/genesis"    ${genesis_COMMIT_HASH} )
DOWNLOAD_GITHUB_DEPENDENCY( "${PROJECT_SOURCE_DIR}/libs" "sparsepp" "sparsepp/spp.h" "greg7mdp/sparsepp" ${sparsepp_COMMIT_HASH} )

# Policy CMP0042: Enable MACOSX_RPATH
if (APPLE)
   set(CMAKE_MACOSX_RPATH ON)
endif (APPLE)

# --------------------------------------------------------------------------------------------------
#   Dependencies Settings
# --------------------------------------------------------------------------------------------------

# Add Genesis as dependency, but without htslib, which we do not need in gappa.
option(GENESIS_USE_HTSLIB "Use htslib. See also GENESIS_SYSTEM_HTSLIB" OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/genesis)

# Use everything that Genesis exports, just to be sure that we use the same setup.
add_definitions( ${GENESIS_DEFINITIONS} )
include_directories( ${GENESIS_INCLUDE_DIR} )
set( CMAKE_C_FLAGS          "${CMAKE_C_FLAGS}          ${GENESIS_C_FLAGS}")
set( CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        ${GENESIS_CXX_FLAGS}" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GENESIS_EXE_LINKER_FLAGS}" )

# Genesis exports the OpenMP variables. We use it to check and give advice in case it is not found.
if( NOT OPENMP_FOUND )
    string(ASCII 27 Esc)
    message( STATUS "${Esc}[31m"
        "OpenMP was not found or has been deactivated. This results in a considerably slower "
        "runtime for the program. Try to use a different compiler. Alternatively, if you are sure "
        "your compiler supports OpenMP, you can add the needed compiler flags to the CMake script."
        # " See BUILD.md for details."
    "${Esc}[0m" )
endif()

# Special treatment of static OpenMP linking, see comment here
# https://stackoverflow.com/questions/74569611/cannot-build-statically-linked-binary-c-debian-ubuntu#comment131650188_74569611
if( GAPPA_BUILD_STATIC AND OPENMP_FOUND )
    message( STATUS "Attempting static linking of OpenMP")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl")
endif()

# Add CLI11 as dependency. It's header-only, so this is easy.
include_directories( SYSTEM "libs/CLI11/include" )

# Same for sparse++
include_directories( SYSTEM "libs/sparsepp" )

# ------------------------------------------------------------------------------
#   Sources
# ------------------------------------------------------------------------------

# Find all source files. The CMake documentation does not recommend this,
# but for our purposes, this is good enough for now.
file (GLOB_RECURSE gappa_sources ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Add our own code. The main is only one cpp file, the rest are headers included from there.
include_directories( "src" )
add_executable( gappa ${gappa_sources} )

# Link it against Genesis, and against all dependencies of Genesis.
target_link_libraries ( gappa ${GENESIS_LINK_LIBRARIES} )

# We have to use LTO for gappa as well for some compilers to properly link, I think.
if(GENESIS_HAS_LTO)
    message(STATUS "Building with LTO/IPO support")
    set_property(TARGET gappa PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "Building without LTO/IPO support")
endif()

message( STATUS "CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS}" )
message( STATUS "GENESIS_LINK_LIBRARIES ${GENESIS_LINK_LIBRARIES}" )
