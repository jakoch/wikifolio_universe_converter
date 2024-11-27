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

if(DEFINED VCPKG_ROOT AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

#-------------------------------------------------------------------
# Project
#-------------------------------------------------------------------

project(wiuc VERSION ${APP_VERSION} DESCRIPTION "wikifolio_investment_universe_converter" LANGUAGES CXX)

#-------------------------------------------------------------------
# Setup Compiler & load additional includes
#-------------------------------------------------------------------

include(SetupCompiler)
include(ShowBuildTargetProperties)

#-------------------------------------------------------------------
# Versionize
#-------------------------------------------------------------------

string(TIMESTAMP CURRENT_YEAR "%Y")

configure_file("src/version.h.in" "version.h" @ONLY)

#-------------------------------------------------------------------
# Find Dependencies
#-------------------------------------------------------------------

#find_package(SQLite3 CONFIG REQUIRED)

# https://github.com/alicevision/geogram/issues/2#issuecomment-536835320
set(CMAKE_THREAD_LIBS_INIT "-lpthread")

# disable the win32 threads library, use POSIX threads instead
set(CMAKE_USE_WIN32_THREADS_INIT 0)

# enable the usage of pthreads
set(CMAKE_USE_PTHREADS_INIT 1)

# use pthreads instead of other threading libraries
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

find_package(unofficial-sqlite3 CONFIG REQUIRED)

find_package(CURL CONFIG REQUIRED)

find_package(fmt CONFIG REQUIRED)

find_package(expat CONFIG REQUIRED)

find_package(unofficial-minizip CONFIG REQUIRED)

include(Findxlsxio_read)
find_package(xlsxio_read REQUIRED)

#-------------------------------------------------------------------
# Setup Sources
#-------------------------------------------------------------------

set(SOURCES
  src/main.h
  src/main.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

#-------------------------------------------------------------------
# Setup Build Targets
#-------------------------------------------------------------------

#-------------------------------------------------------------------
# Build Target: wiuc                                    (executable)
#-------------------------------------------------------------------

add_executable(wiuc ${SOURCES})

if(MSVC)
  # Export symbols of xlsxio_read.
  target_compile_definitions(wiuc PRIVATE BUILD_XLSXIO_STATIC)

  target_compile_features(wiuc PUBLIC cxx_std_23)
endif()

target_compile_features(wiuc PUBLIC cxx_std_20)

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

target_include_directories(wiuc PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    # to include the generated "version.h"
    ${CMAKE_CURRENT_BINARY_DIR}
)

#-------------------------------------------------------------------
# FeatureSummary
#-------------------------------------------------------------------

include(FeatureSummary)
feature_summary(WHAT ALL DESCRIPTION "[Build Summary]\n" FATAL_ON_MISSING_REQUIRED_PACKAGES)

#-------------------------------------------------------------------
# Display Compiler and Linker properties of Build Targets
#-------------------------------------------------------------------

show_build_target_properties(wiuc)

#-------------------------------------------------------------------
# Install
#-------------------------------------------------------------------

# CMakePresets define CMAKE_INSTALL_PREFIX to /out/install
# We add a versionized folder as top-level dir for the executable.

install(TARGETS wiuc DESTINATION wiuc-${APP_VERSION})
