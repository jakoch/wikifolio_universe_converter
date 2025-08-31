cmake_minimum_required(VERSION 3.20)

#-------------------------------------------------------------------
# Setup CMake
#-------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_FIND_LIBRARY_PREFIXES ";")
set(CMAKE_FIND_LIBRARY_SUFFIXES ";")

# Generate CompilationDatabase in `compile_commands.json`.
# The file is used by build tools and IntelliSense (VSCode, YouCompleteMe, etc).
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

#-------------------------------------------------------------------
# Build Options
#-------------------------------------------------------------------

option(BUILD_SHARED_LIBS "Build shared libraries"               OFF)

#-------------------------------------------------------------------
# Get Versioning Data
#-------------------------------------------------------------------

# vcpkg.json is the primary source for versioning data
file(READ ${CMAKE_SOURCE_DIR}/vcpkg.json VCPKG_JSON_STRING)
string(JSON APP_NAME     GET ${VCPKG_JSON_STRING} "name")
string(JSON APP_VERSION  GET ${VCPKG_JSON_STRING} "version")
string(JSON APP_LICENSE  GET ${VCPKG_JSON_STRING} "license")
string(JSON APP_DESC     GET ${VCPKG_JSON_STRING} "description")
string(JSON APP_HOMEPAGE GET ${VCPKG_JSON_STRING} "homepage")

#-------------------------------------------------------------------
# Setup VCPKG
#-------------------------------------------------------------------

include(SetupVcpkg)

#-------------------------------------------------------------------
# Project
#-------------------------------------------------------------------

project(wiuc VERSION ${APP_VERSION} DESCRIPTION "wikifolio_investment_universe_converter" LANGUAGES CXX)

#-------------------------------------------------------------------
# Versioning
#-------------------------------------------------------------------

# Additional placeholders used in the version template
string(TIMESTAMP CURRENT_YEAR "%Y")

configure_file("src/version.h.in" "version.h" @ONLY)

#-------------------------------------------------------------------
# Setup Compiler & load additional includes
#-------------------------------------------------------------------

include(SetupCompiler)
include(ShowBuildTargetProperties)

#-------------------------------------------------------------------
# Settings for package Threads
#-------------------------------------------------------------------

# Source: https://github.com/alicevision/geogram/issues/2#issuecomment-536835320
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_USE_WIN32_THREADS_INIT 0)     # disable the win32 threads library, use POSIX threads instead
set(CMAKE_USE_PTHREADS_INIT 1)          # enable the usage of pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)     # use pthreads instead of other threading libraries

#-------------------------------------------------------------------
# Find Dependencies
#-------------------------------------------------------------------

find_package(Threads REQUIRED) # CONFIG not used, because system package

#find_package(SQLite3 CONFIG REQUIRED) # they don't publish?

find_package(unofficial-sqlite3 CONFIG REQUIRED)

find_package(CURL CONFIG REQUIRED)

find_package(fmt CONFIG REQUIRED)

find_package(expat CONFIG REQUIRED)

find_package(unofficial-minizip CONFIG REQUIRED)

include(Findxlsxio_read)
find_package(xlsxio_read REQUIRED)

#-------------------------------------------------------------------
# Setup Build Targets
#-------------------------------------------------------------------

#-------------------------------------------------------------------
# Definition Order                        (for all executables/libs)
#-------------------------------------------------------------------
# 1.  add_executable() / add_library()
# 2.  target_sources()
# 3.  target_include_directories()
# 4.  target_compile_definitions()
# 5.  target_compile_features()
# 6.  target_compile_options()           [if needed]
# 7.  target_link_libraries()
# 8.  show feature_summary()             [if needed]
# 9.  show_build_target_properties()     [if needed]
# 10. install()                          [if needed]

#-------------------------------------------------------------------
# Build Target: wiuc                                    (executable)
#-------------------------------------------------------------------

add_executable(wiuc)

# TODO: use FILE_SET, but this doesnt exist yet in CMake 4 (08-2025)
# target_sources(wiuc
#   PRIVATE
#     FILE_SET SOURCES
#     FILES
#       src/main.h
#       src/main.cpp
#       ${CMAKE_CURRENT_BINARY_DIR}/version.h
# )

target_sources(wiuc
  PRIVATE
    src/main.h
    src/main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_include_directories(wiuc
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    ${CMAKE_CURRENT_BINARY_DIR} # to include the generated "version.h"
)

target_compile_definitions(wiuc
  PRIVATE
    # Export symbols of xlsxio_read.
    $<$<CXX_COMPILER_ID:MSVC>:BUILD_XLSXIO_STATIC>
)

target_compile_features(wiuc
  PUBLIC
    cxx_std_20
    $<$<CXX_COMPILER_ID:MSVC>:cxx_std_23>
)

target_link_libraries(wiuc
    PRIVATE
      ${xlsxio_read_LIBRARY}
      unofficial::minizip::minizip
      expat::expat
      #SQLite::SQLite3
      unofficial::sqlite3::sqlite3
      CURL::libcurl
      fmt::fmt
)

#-------------------------------------------------------------------
# Display Feature Summary
#-------------------------------------------------------------------

include(FeatureSummary)

feature_summary(
  WHAT ALL
  DESCRIPTION "[Build Summary]\n"
  FATAL_ON_MISSING_REQUIRED_PACKAGES
)

#-------------------------------------------------------------------
# Display Target Properties
#-------------------------------------------------------------------

show_build_target_properties(wiuc)

#-------------------------------------------------------------------
# Install
#-------------------------------------------------------------------

# WARNING: we alter the default DESTINATION path!
# CMakePresets sets CMAKE_INSTALL_PREFIX to `/out/install`.
# We add a versioned folder as top-level dir for the executable.

install(
  TARGETS wiuc
  DESTINATION wiuc-${APP_VERSION}
)
