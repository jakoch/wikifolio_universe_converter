cmake_minimum_required(VERSION 3.19)

#-------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  #
  # Settings for ALL build types
  #
  set(CMAKE_C_FLAGS_INIT             "-DWIN32 -D_WINDOWS -nologo")
  set(CMAKE_CXX_FLAGS_INIT           "-DWIN32 -D_WINDOWS -GR -EHsc -nologo")
  set(CMAKE_EXE_LINKER_FLAGS_INIT    "-machine:x64 -nologo")
  set(CMAKE_MODULE_LINKER_FLAGS_INIT "-machine:x64 -nologo")
  set(CMAKE_SHARED_LINKER_FLAGS_INIT "-machine:x64 -nologo")
  set(CMAKE_STATIC_LINKER_FLAGS_INIT "-machine:x64 -nologo")
  
  #
  # Debug
  #
  # Zi:     Produce a separate PDB file (debug symbols)
  # Ob0:    Disable inline expansions
  # Od:     Disable code movements for easier debugging (DEBUG)
  #
  set(CMAKE_CXX_FLAGS_DEBUG_INIT           "-Zi -Ob0 -Od")
  set(CMAKE_C_FLAGS_DEBUG_INIT             "-Zi -Ob0 -Od")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT    "-INCREMENTAL:NO -debug")
  set(CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT "-INCREMENTAL:NO -debug")
  set(CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT "-INCREMENTAL:NO -debug")

  #
  # Release
  #
  # O2:     Maximize Speed
  # Ob3:    Aggressive Inline Function Expansion
  # GL:     Whole Program Optimization
  # NDEBUG: Assertion checks turned off at compile time
  #
  set(CMAKE_CXX_FLAGS_RELEASE_INIT           "-O2 -Ob3 -GL -DNDEBUG")
  set(CMAKE_C_FLAGS_RELEASE_INIT             "-O2 -Ob3 -GL -DNDEBUG")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT    "-INCREMENTAL:NO -LTCG")
  set(CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT "-INCREMENTAL:NO -LTCG")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "-INCREMENTAL:NO -LTCG")

  # RelWithDebugInfo
  #
  # This build_type is important, because we need to step through the
  # assembly of the optimized release build, while having debug information.
  #
  # Zi:     Produce a separate PDB file (debug symbols)
  # O2:     Maximize Speed
  # Ob3:    Aggressive Inline Function Expansion
  # GL:     Whole Program Optimization
  # NDEBUG: Assertion checks turned off at compile time
  #
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT           "-Zi -O2 -Ob3 -GL -DNDEBUG")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT             "-Zi -O2 -Ob3 -GL -DNDEBUG")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT    "-INCREMENTAL:NO -LTCG -debug")
  set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT "-INCREMENTAL:NO -LTCG -debug")
  set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT "-INCREMENTAL:NO -LTCG -debug")

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  
endif()

if(WIN32)
    # Build for a Windows 10 host system.
    set(CMAKE_SYSTEM_VERSION 10.0)

    message(STATUS "[INFO] BUILD_SHARED_LIBS -> '${BUILD_SHARED_LIBS}'.")

    # When we build statically (MT):
    if(NOT BUILD_SHARED_LIBS)
        # Select MSVC runtime based on CMAKE_MSVC_RUNTIME_LIBRARY.
        # We switch from the multi-threaded dynamically-linked library (default)
        # to the multi-threaded statically-linked runtime library.
        cmake_policy(SET CMP0091 NEW)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif() 
    
endif()

#-------------------------------------------------------------------
# Setup CMake
#-------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#-------------------------------------------------------------------
# Setup VCPKG
#-------------------------------------------------------------------

include(SetupVcpkg)

#-------------------------------------------------------------------
# Build Options
#-------------------------------------------------------------------

option(BUILD_SHARED_LIBS "Build shared libraries"               OFF)

#-------------------------------------------------------------------
# Project
#-------------------------------------------------------------------

project(wiuc DESCRIPTION "wikifolio_investment_universe_converter")

#-------------------------------------------------------------------
# Define C++ Standard to use
#-------------------------------------------------------------------

set(CMAKE_CXX_STANDARD          23) # to get /std:c++latest on MSVC
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        ON)

#-------------------------------------------------------------------
# Find Dependencies
#-------------------------------------------------------------------

list(APPEND CMAKE_PREFIX_PATH "${VCPKG_DIR}")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_DIR}/lib")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_DIR}/debug/lib")

#find_package(SQLite3 REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

find_package(expat)
find_package(minizip CONFIG REQUIRED)

# TODO remove this. blocked by upstream (xlsxio) (no export config).
# hey upstream, can you please provide an export config?
set(xlsxio_read_DIR "${VCPKG_DIR}/include/xlsxio_read.h")
set(xlsxio_read_INCLUDE_DIRS "${VCPKG_DIR}/include")
find_library(xlsxio_read_LIBRARY
    NAMES xlsxio_read
    HINTS "${CMAKE_PREFIX_PATH}/lib"
)
SET(xlsxio_read_FOUND true)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(xlsxio_read DEFAULT_MSG xlsxio_read_LIBRARIES xlsxio_read_INCLUDE_DIRS)

find_package(xlsxio_read)

#-------------------------------------------------------------------
# Setup Sources
#-------------------------------------------------------------------

set(SRC
  src/main.h
  src/main.cpp
)

#-------------------------------------------------------------------
# Setup Build Targets
#-------------------------------------------------------------------

#-------------------------------------------------------------------
# Build Target: wiuc                                    (executable)
#-------------------------------------------------------------------

add_executable(wiuc ${SRC})

# Export symbols of xlsxio_read.
target_compile_definitions(wiuc PRIVATE BUILD_XLSXIO_STATIC)

target_link_libraries(wiuc
    PRIVATE
      EXPAT::EXPAT
      minizip::minizip
      unofficial::sqlite3::sqlite3
      CURL::libcurl
      "vcpkg_installed\\x64-windows-static\\lib\\xlsxio_read"
)
