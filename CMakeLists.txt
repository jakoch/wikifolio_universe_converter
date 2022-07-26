cmake_minimum_required(VERSION 3.19)

#-------------------------------------------------------------------
# Setup CMake
#-------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_FIND_LIBRARY_PREFIXES ";")
set(CMAKE_FIND_LIBRARY_SUFFIXES ";")

#-------------------------------------------------------------------
# Setup Compiler
#-------------------------------------------------------------------

include(SetupCompiler)

#-------------------------------------------------------------------
# Setup VCPKG
#-------------------------------------------------------------------

include(SetupVcpkg)

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
# Project
#-------------------------------------------------------------------

project(wiuc VERSION ${APP_VERSION} DESCRIPTION "wikifolio_investment_universe_converter" LANGUAGES C CXX)

#-------------------------------------------------------------------
# Versionize
#-------------------------------------------------------------------

configure_file("src/version.cpp.in" "version.cpp" @ONLY)

#-------------------------------------------------------------------
# Find Dependencies
#-------------------------------------------------------------------

list(APPEND CMAKE_PREFIX_PATH "${VCPKG_DIR}")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_DIR}/lib")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_DIR}/debug/lib")

find_package(unofficial-sqlite3 CONFIG REQUIRED)

find_package(CURL CONFIG REQUIRED)

if(NOT MSVC)
  find_package(fmt CONFIG REQUIRED)
endif()

find_package(expat CONFIG REQUIRED)

find_package(minizip CONFIG REQUIRED)

include(Findxlsxio_read)
find_package(xlsxio_read REQUIRED)

#-------------------------------------------------------------------
# Setup Sources
#-------------------------------------------------------------------

set(SRC
  src/main.h
  src/main.cpp
  src/version.h
  ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

#-------------------------------------------------------------------
# Setup Build Targets
#-------------------------------------------------------------------

#-------------------------------------------------------------------
# Build Target: wiuc                                    (executable)
#-------------------------------------------------------------------

add_executable(wiuc ${SRC})

if(MSVC)
  # Export symbols of xlsxio_read.
  target_compile_definitions(wiuc PRIVATE BUILD_XLSXIO_STATIC)
endif()

target_link_libraries(wiuc
    PRIVATE
      ${xlsxio_read_LIBRARY}
      minizip::minizip
      expat::expat
      unofficial::sqlite3::sqlite3
      CURL::libcurl
)

if(NOT MSVC)
  target_link_libraries(wiuc PRIVATE fmt::fmt)
endif()

target_include_directories(wiuc PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
)
target_include_directories(wiuc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
