# gappa - Genesis Applications for Phylogenetic Placement Analysis
# Copyright (C) 2017 Lucas Czech
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
# Lucas Czech <lucas.czech@h-its.org>
# Exelixis Lab, Heidelberg Institute for Theoretical Studies
# Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany

# --------------------------------------------------------------------------------------------------
#   CMake Init
# --------------------------------------------------------------------------------------------------

cmake_minimum_required (VERSION 2.8.7 FATAL_ERROR)

set (CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set (CMAKE_DISABLE_SOURCE_CHANGES  ON)

# --------------------------------------------------------------------------------------------------
#   Project Definition and Requirements
# --------------------------------------------------------------------------------------------------

project ( gappa CXX )

# --------------------------------------------------------------------------------------------------
#   Build Options
# --------------------------------------------------------------------------------------------------

set (CMAKE_BUILD_TYPE RELEASE)
set( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin )

# By default, we build a fully static version. This works with GCC, but might cause trouble with
# Clang, so we offer this option, which disables statical linking of system libraries.
# Genesis on the other hand is always linked statically, in order to be able to move around the
# final binary at least on the same system.
option ( BUILD_STATIC  "Build the static version of the executable." ON )

if( BUILD_STATIC AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    string(ASCII 27 Esc)
    message( STATUS "${Esc}[31m"
        "Compiler is not GCC. This might cause trouble with statically linking system libraries "
        "on Linux systems. If the compilation fails with some linking error, try a different compiler, "
        "or try to not build statically by deactivating the CMake option 'BUILD_STATIC'. "
    "${Esc}[0m" )
endif()

# --------------------------------------------------------------------------------------------------
#   Compiler and Linker Options
# --------------------------------------------------------------------------------------------------

# Set the warn flags to a very high level - except unknown pragmas, as this causes needless
# warnings with OpenMP and other pragma-based techniques.
set (WARN_FLAGS              "-Wall -Wextra -Wno-unknown-pragmas -pedantic -pedantic-errors")
set (CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -std=c++11 ${WARN_FLAGS}")

set (CMAKE_CXX_FLAGS_DEBUG   "-O2 -DDEBUG -g -ggdb3 -D_GLIBCXX_DEBUG" )
set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" )

IF(BUILD_STATIC)
    # Use different ways of telling CMake to build static, just to be sure.
    SET(BUILD_SHARED_LIBRARIES OFF)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    link_libraries("-static")
ENDIF()
message( STATUS "Static linking of system libraries: ${BUILD_STATIC}")

# --------------------------------------------------------------------------------------------------
#   Dependencies
# --------------------------------------------------------------------------------------------------

# Add Genesis as dependency.
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/genesis)

# Use everything that Genesis exports, just to be sure that we use the same setup.
add_definitions( ${GENESIS_DEFINITIONS} )
include_directories( ${GENESIS_INCLUDE_DIR} )
set( CMAKE_C_FLAGS          "${CMAKE_C_FLAGS}          ${GENESIS_C_FLAGS}")
set( CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        ${GENESIS_CXX_FLAGS}" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GENESIS_EXE_LINKER_FLAGS}" )

# Add CLI11 as dependency. It's header-only, so this is easy.
include_directories( "libs/CLI11/include" )

# Genesis exports the OpenMP variables. We use it to check and give advice in case it is not found.
if( NOT OPENMP_FOUND )
    string(ASCII 27 Esc)
    message( STATUS "${Esc}[31m"
        "Could not find OpenMP. This results in a considerably slower runtime for the program. Try "
        "to use a different compiler. Alternatively, if you are sure your compiler supports OpenMP, "
        "you can add the needed compiler flags to the CMake script. See BUILD.md for details."
    "${Esc}[0m" )
endif()

# ------------------------------------------------------------------------------
#   Sources
# ------------------------------------------------------------------------------

# Find all source files. The CMake documentation does not recommend this,
# but for our purposes, this is good enough for now.
file (GLOB_RECURSE gappa_sources   ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Add our own code. The main is only one cpp file, the rest are headers included from there.
include_directories( "src" )
add_executable( gappa ${gappa_sources} )

message( STATUS "CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS}" )
message( STATUS "GENESIS_LINK_LIBRARIES ${GENESIS_LINK_LIBRARIES}" )

# Link it against Genesis, and against all dependencies of Genesis.
target_link_libraries ( gappa ${GENESIS_LINK_LIBRARIES} )
