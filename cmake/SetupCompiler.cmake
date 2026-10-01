
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

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "/NODEFAULTLIB:msvcrt.lib")
endif()

#-------------------------------------------------------------------
# Define C++ Standard to use
#-------------------------------------------------------------------

message("Using Compiler: ${CMAKE_CXX_COMPILER_ID}")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Both GCC and Clang understand these, and the project is warning-clean under them.
    # Without this, GCC builds compiled with no warnings enabled at all.
    add_compile_options(-Wall -Wextra -Werror)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # NOTE: this branch used to compare against "CLANG", but CMAKE_CXX_COMPILER_ID
        # is "Clang", and STREQUAL is case-sensitive, so the branch never ran.
        # It is kept disabled on purpose: enabling it would switch the standard
        # library from libstdc++ to libc++ for every Clang build. The presets
        # select the linker via CMAKE_LINKER_TYPE instead, and they do not request
        # libc++. Set USE_LIBCXX to ON to opt in.
        option(USE_LIBCXX "Link against libc++ instead of libstdc++" OFF)

        if(USE_LIBCXX)
            # enable incomplete features to get "std::format" support
            set(LIBCXX_ENABLE_INCOMPLETE_FEATURES ON)

            add_compile_options(-stdlib=libc++)
            # "-Wl" takes a single comma-separated argument. The previous
            # "-Wl -stdlib=libc++" passed "-stdlib=libc++" as a separate
            # argument, which the linker treated as a file name.
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-stdlib=libc++ -lc++ -lc++abi")
        else()
            add_compile_options(-fexec-charset=UTF-8)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fvisibility=hidden -pthread)
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -pthread") # -stdlib=libc++
        set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,--no-as-needed -ldl") # -stdlib=libc++ -lc++abi
    endif()
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

endif()


