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

# The sources do not use threads directly, but libcurl and libssl link
# against pthreads. Linking the imported target "Threads::Threads" makes that
# dependency explicit, and it carries the correct flags per platform instead of
# hard-coding "-lpthread", which does not exist on Windows.
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

target_sources(wiuc
  PRIVATE
    src/main.h
    src/csv_format.h
    src/csv_format.cpp
    src/paths.h
    src/paths.cpp
    src/download.cpp
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
      Threads::Threads
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

#-------------------------------------------------------------------
# Build Target: csv_format_test                         (executable)
#-------------------------------------------------------------------

# The unit tests are opt-in, so that a release build does not pull in the
# Catch2 dependency. Enable them with:
#   cmake -B build -DVCPKG_MANIFEST_FEATURES=tests
# and run them with:
#   ctest --test-dir build --output-on-failure

option(BUILD_TESTS "Build the unit tests" OFF)

if(BUILD_TESTS)
  find_package(Catch2 CONFIG REQUIRED)

  include(CTest)
  enable_testing()

  add_executable(csv_format_test)

  target_sources(csv_format_test
    PRIVATE
      src/csv_format.h
      src/csv_format.cpp
      tests/csv_format_test.cpp
  )

  target_include_directories(csv_format_test
    PRIVATE
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
  )

  target_compile_features(csv_format_test
    PRIVATE
      cxx_std_20
      $<$<CXX_COMPILER_ID:MSVC>:cxx_std_23>
  )

  target_link_libraries(csv_format_test
    PRIVATE
      Catch2::Catch2WithMain
  )

  add_test(NAME csv_format COMMAND csv_format_test)

  set_tests_properties(csv_format PROPERTIES LABELS "unit")

  #-------------------------------------------------------------------
  # Build Target: paths_test                           (executable)
  #-------------------------------------------------------------------

  # Unit tests for the filesystem helpers. These are covered here rather than
  # through the wiuc binary, because a nested output folder is only created by
  # wiuc when it does not yet contain the .xlsx, which would make the end-to-end
  # test fall through to a real network download.

  add_executable(paths_test)

  target_sources(paths_test
    PRIVATE
      src/paths.h
      src/paths.cpp
      tests/paths_test.cpp
  )

  target_include_directories(paths_test
    PRIVATE
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
  )

  target_compile_features(paths_test
    PRIVATE
      cxx_std_20
      $<$<CXX_COMPILER_ID:MSVC>:cxx_std_23>
  )

  target_link_libraries(paths_test
    PRIVATE
      Catch2::Catch2WithMain
      fmt::fmt
  )

  add_test(NAME paths COMMAND paths_test)

  set_tests_properties(paths PROPERTIES LABELS "unit")

  #-------------------------------------------------------------------
  # Build Target: convert_fixture_test                  (executable)
  #-------------------------------------------------------------------

  # An end-to-end test. It drives the real wiuc binary against a local XLSX
  # fixture, so no network access is needed and the whole conversion path is
  # exercised. wiuc runs as a subprocess, which keeps its coverage counters in a
  # separate .profraw file, separate from this test binary's own counters.
  #
  # It does not use Catch2WithMain, because it needs its own main() to receive
  # the two paths from CTest, so it links the Catch2 library directly.

  add_executable(convert_fixture_test)

  target_sources(convert_fixture_test
    PRIVATE
      tests/convert_fixture_test.cpp
  )

  target_compile_features(convert_fixture_test
    PRIVATE
      cxx_std_20
      $<$<CXX_COMPILER_ID:MSVC>:cxx_std_23>
  )

  target_link_libraries(convert_fixture_test
    PRIVATE
      Catch2::Catch2
      unofficial::sqlite3::sqlite3
  )

  # The two paths are passed via the environment, because Catch2 treats positional
  # command line arguments as test-name filters.
  add_test(
    NAME convert_fixture
    COMMAND convert_fixture_test
  )

  set_tests_properties(convert_fixture PROPERTIES
    LABELS "e2e"
    # The e2e tests drive the wiuc binary, so it must be built first. The test
    # targets are not part of the "all" target, so this dependency is declared here
    # rather than relying on the build order of a separate invocation.
    DEPENDS wiuc
    ENVIRONMENT
      "WIUC_EXE=$<TARGET_FILE:wiuc>;WIUC_FIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/Investment_Universe.de.xlsx;WIUC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
  )

  #-------------------------------------------------------------------
  # Build Target: error_paths_test                     (executable)
  #-------------------------------------------------------------------

  # Error paths and CLI surface. Every case drives the real wiuc binary, so these
  # are end-to-end too. Like the other tests, it has its own main() to read the
  # paths from the environment, because Catch2 treats positional arguments as
  # test-name filters.
  #
  # download.cpp is deliberately not covered: reaching it needs a live network
  # fetch, or refactoring libcurl behind an injectable interface.

  add_executable(error_paths_test)

  target_sources(error_paths_test
    PRIVATE
      tests/error_paths_test.cpp
  )

  target_compile_features(error_paths_test
    PRIVATE
      cxx_std_20
      $<$<CXX_COMPILER_ID:MSVC>:cxx_std_23>
  )

  target_link_libraries(error_paths_test
    PRIVATE
      Catch2::Catch2
  )

  add_test(
    NAME error_paths
    COMMAND error_paths_test
  )

  set_tests_properties(error_paths PROPERTIES
    LABELS "e2e"
    DEPENDS wiuc
    ENVIRONMENT "WIUC_EXE=$<TARGET_FILE:wiuc>;WIUC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
  )
endif()
