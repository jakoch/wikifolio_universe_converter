
#-------------------------------------------------------------------
# Compiler Setup
#-------------------------------------------------------------------

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
# Define C++ Standard to use
#-------------------------------------------------------------------

if(MSVC)
  set(CMAKE_CXX_STANDARD          23) # to get /std:c++latest on MSVC
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS        ON)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "CLANG")
  set(CMAKE_CXX_STANDARD          20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS        ON)

  # enable incomplete features to get "std::format" support
  set(LIBCXX_ENABLE_INCOMPLETE_FEATURES ON)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2b -stdlib=libc++ -Wall -Wextra -Werror -fexec-charset=UTF-8 -mtune=skylake")
  #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} some other flags")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

#-------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------

if (MSVC)
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
