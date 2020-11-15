cmake_minimum_required(VERSION 3.10)
project(Atmos)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# If the build type hasn't been specified, defaulting it to Release
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

######################
# Atmos - Executable #
######################

add_executable(Atmos)

# Using C++17
target_compile_features(Atmos PRIVATE cxx_std_17)

############################
# Atmos - Useful variables #
############################

# Detect whether Emscripten is being used
if (CMAKE_CXX_COMPILER MATCHES "/em\\+\\+.*$")
    set(ATMOS_USE_EMSCRIPTEN ON)
    set(CMAKE_AR "${CMAKE_C_COMPILER}")
    set(CMAKE_STATIC_LIBRARY_SUFFIX ".bc")
    set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> -o <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> -o <TARGET> <LINK_FLAGS> <OBJECTS>")
else ()
    set(ATMOS_USE_EMSCRIPTEN OFF)
endif ()

if (MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang") # Finding exclusively MSVC, not clang-cl
    set(ATMOS_COMPILER "MSVC")
    set(ATMOS_COMPILER_MSVC ON)
    target_compile_definitions(Atmos PUBLIC ATMOS_COMPILER_MSVC)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (MSVC)
        # Using clang-cl, for which both MSVC & Clang are found
        set(ATMOS_COMPILER "Clang-cl")
        set(ATMOS_COMPILER_CLANG_CL ON)
    else ()
        set(ATMOS_COMPILER "Clang")
    endif ()

    set(ATMOS_COMPILER_CLANG ON)
    target_compile_definitions(Atmos PUBLIC ATMOS_COMPILER_CLANG)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(ATMOS_COMPILER "GCC")
    set(ATMOS_COMPILER_GCC ON)
    target_compile_definitions(Atmos PUBLIC ATMOS_COMPILER_GCC)
endif ()

if (WIN32 OR CYGWIN)
    set(ATMOS_PLATFORM "Windows")
    set(ATMOS_PLATFORM_WINDOWS ON)
    target_compile_definitions(Atmos PUBLIC ATMOS_PLATFORM_WINDOWS)

    if (CYGWIN)
        set(ATMOS_PLATFORM "${ATMOS_PLATFORM} (Cygwin)")
        set(ATMOS_PLATFORM_CYGWIN ON)
        target_compile_definitions(Atmos PUBLIC ATMOS_PLATFORM_CYGWIN)
    endif ()
elseif (APPLE)
    set(ATMOS_PLATFORM "macOS")
    set(ATMOS_PLATFORM_MAC ON)
    target_compile_definitions(Atmos PUBLIC ATMOS_PLATFORM_MAC)
elseif (ATMOS_USE_EMSCRIPTEN)
    set(ATMOS_PLATFORM "Emscripten")
    set(ATMOS_PLATFORM_EMSCRIPTEN ON)
    target_compile_definitions(Atmos PUBLIC ATMOS_PLATFORM_EMSCRIPTEN USE_OPENGL_ES)
elseif (UNIX)
    set(ATMOS_PLATFORM "Linux")
    set(ATMOS_PLATFORM_LINUX ON)
    target_compile_definitions(Atmos PUBLIC ATMOS_PLATFORM_LINUX)
endif ()

if (ATMOS_COMPILER_MSVC)
    set(ATMOS_CONFIG_DEBUG   "$<IF:$<CONFIG:Debug>,ON,OFF>")
    set(ATMOS_CONFIG_RELEASE "$<IF:$<CONFIG:Debug>,OFF,ON>")
    set(ATMOS_CONFIG_SHORT   "$<IF:$<CONFIG:Debug>,Debug,Release>")

    target_compile_definitions(Atmos PUBLIC $<IF:$<CONFIG:Debug>,ATMOS_CONFIG_DEBUG,ATMOS_CONFIG_RELEASE>)

    set(CONFIG_STR_UPPER $<UPPER_CASE:$<CONFIG>>)
else ()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(ATMOS_CONFIG_DEBUG   ON)
        set(ATMOS_CONFIG_RELEASE OFF)
        set(ATMOS_CONFIG_SHORT   "Debug")

        target_compile_definitions(Atmos PUBLIC ATMOS_CONFIG_DEBUG)
    else ()
        set(ATMOS_CONFIG_DEBUG   OFF)
        set(ATMOS_CONFIG_RELEASE ON)
        set(ATMOS_CONFIG_SHORT   "Release")

        target_compile_definitions(Atmos PUBLIC ATMOS_CONFIG_RELEASE)
    endif ()

    string(TOUPPER ${CMAKE_BUILD_TYPE} CONFIG_STR_UPPER)
endif ()

set(ATMOS_BUILD_FLAGS "${CMAKE_CXX_FLAGS_${CONFIG_STR_UPPER}}")

option(ENABLE_DEBUG_INFO "Creates a debug target which prints useful values" OFF)

if (ENABLE_DEBUG_INFO)
    cmake_host_system_information(RESULT PLATFORM_RELEASE QUERY OS_RELEASE)
    cmake_host_system_information(RESULT PLATFORM_VERSION QUERY OS_VERSION)

    # Trimming potential leading & trailing space characters (happens for Windows' values)
    string(STRIP ${PLATFORM_RELEASE} PLATFORM_RELEASE)
    string(STRIP ${PLATFORM_VERSION} PLATFORM_VERSION)

    add_custom_target(
        Atmos_PrintDebugInfo

        ${CMAKE_COMMAND} -E echo "" &&
        ${CMAKE_COMMAND} -E echo "######################" &&
        ${CMAKE_COMMAND} -E echo "# [Atmos] Debug info #" &&
        ${CMAKE_COMMAND} -E echo "######################" &&
        ${CMAKE_COMMAND} -E echo "" &&

        ${CMAKE_COMMAND} -E echo "--- Platform (found ${ATMOS_PLATFORM})" &&
        ${CMAKE_COMMAND} -E echo "  Name:     ${CMAKE_SYSTEM_NAME}" &&
        ${CMAKE_COMMAND} -E echo "  Release:  ${PLATFORM_RELEASE}" &&
        ${CMAKE_COMMAND} -E echo "  Version:  ${PLATFORM_VERSION}" &&
        ${CMAKE_COMMAND} -E echo "" &&

        ${CMAKE_COMMAND} -E echo "--- Compiler (found ${ATMOS_COMPILER})" &&
        ${CMAKE_COMMAND} -E echo "  ID:       ${CMAKE_CXX_COMPILER_ID}" &&
        ${CMAKE_COMMAND} -E echo "  Version:  ${CMAKE_CXX_COMPILER_VERSION}" &&
        ${CMAKE_COMMAND} -E echo "  Path:     ${CMAKE_CXX_COMPILER}" &&
        ${CMAKE_COMMAND} -E echo "" &&

        ${CMAKE_COMMAND} -E echo "--- Configuration" &&
        ${CMAKE_COMMAND} -E echo "  Name:                 $<CONFIG>" &&
        ${CMAKE_COMMAND} -E echo "  ATMOS_CONFIG_DEBUG:   ${ATMOS_CONFIG_DEBUG}" &&
        ${CMAKE_COMMAND} -E echo "  ATMOS_CONFIG_RELEASE: ${ATMOS_CONFIG_RELEASE}" &&
        ${CMAKE_COMMAND} -E echo "  ATMOS_CONFIG_SHORT:   ${ATMOS_CONFIG_SHORT}" &&
        ${CMAKE_COMMAND} -E echo "" &&

        ${CMAKE_COMMAND} -E echo "--- Build flags: '${ATMOS_BUILD_FLAGS}'" &&
        ${CMAKE_COMMAND} -E echo ""
    )
endif ()

if (ATMOS_USE_EMSCRIPTEN)
    target_compile_definitions(Atmos PUBLIC ATMOS_ROOT="/")
else ()
    target_compile_definitions(Atmos PUBLIC ATMOS_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/")
endif ()

##########################
# Atmos - Compiler flags #
##########################

if (ATMOS_COMPILER_GCC)
    set(
        ATMOS_COMPILER_FLAGS

        -pedantic
        -pedantic-errors
        -Wall
        -Wextra

        -Warray-bounds
        -Wcast-align
        -Wcast-qual
        -Wconditionally-supported
        -Wconversion
        -Wdisabled-optimization
        -Wdouble-promotion
        -Wfloat-conversion
        -Wformat=2
        -Wformat-security
        -Wlogical-op
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wnoexcept
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wopenmp-simd
        -Woverloaded-virtual
        -Wpacked
        -Wredundant-decls
        -Wstrict-aliasing
        -Wstrict-null-sentinel
        #-Wsuggest-final-methods
        #-Wsuggest-final-types
        -Wtrampolines
        -Wundef
        -Wuninitialized
        -Wunused-macros
        -Wuseless-cast
        -Wvector-operation-performance
        -Wvla
        -Wzero-as-null-pointer-constant

        -Wno-comment
        -Wno-format-nonliteral
    )

    # Enabling some other warnings available since GCC 5
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5)
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            -fsized-deallocation
            -Warray-bounds=2
            -Wformat-signedness
            -Wsized-deallocation
        )
    endif ()

    # Enabling some other warnings available since GCC 6
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 6)
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            -Wduplicated-cond
            -Wnull-dereference
        )
    endif ()

    # Enabling some other warnings available since GCC 7
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7)
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            -Waligned-new
            -Walloca
            -Walloc-zero
            -Wformat-overflow
            -Wshadow
        )
    endif ()

    # Enabling code coverage
    option(ATMOS_ENABLE_COVERAGE "Enable code coverage (GCC only)" OFF)

    if (ATMOS_CONFIG_DEBUG AND ATMOS_ENABLE_COVERAGE)
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            -g
            -O0
            -fno-inline
            -fno-inline-small-functions
            -fno-default-inline
            -fprofile-arcs
            -ftest-coverage
        )

        set(
            ATMOS_LINKER_FLAGS

            gcov
        )
    endif ()
elseif (ATMOS_COMPILER_CLANG)
    set(
        ATMOS_COMPILER_FLAGS

        -Weverything

        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-covered-switch-default
        -Wno-documentation
        -Wno-documentation-unknown-command
        -Wno-exit-time-destructors
        -Wno-float-equal
        -Wno-format-nonliteral
        -Wno-global-constructors
        -Wno-mismatched-tags
        -Wno-missing-braces
        -Wno-padded
        -Wno-reserved-id-macro
        -Wno-sign-conversion
        -Wno-switch-enum
        -Wno-weak-vtables
    )

    if (ATMOS_COMPILER_CLANG_CL)
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            # Disabling warnings triggered in externals
            -Wno-language-extension-token
            -Wno-nonportable-system-include-path
            -Wno-zero-as-null-pointer-constant
        )
    else ()
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            # Other warning flags not recognized by clang-cl
            -pedantic
            -pedantic-errors
        )
    endif ()

    if (ATMOS_USE_EMSCRIPTEN)
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            "SHELL:-s USE_GLFW=3"
            "SHELL:-s USE_LIBPNG=1"
        )

        set(
            ATMOS_LINKER_FLAGS

            ${ATMOS_LINKER_FLAGS}
            "-s USE_LIBPNG=1"
            "-s USE_GLFW=3"
            "-s USE_WEBGL2=1"
            "-s OFFSCREEN_FRAMEBUFFER=1"
            "-s OFFSCREENCANVAS_SUPPORT=1"
            "-s DOUBLE_MODE=0"
            glfw
        )
    endif ()

    # Disabling some warnings available since Clang 5
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5)
        set(
            ATMOS_COMPILER_FLAGS

            ${ATMOS_COMPILER_FLAGS}
            -Wno-unused-template
        )
    endif ()
elseif (ATMOS_COMPILER_MSVC)
    set(
        ATMOS_COMPILER_FLAGS

        /Wall

        /wd4061 # Enum value in a switch not explicitly handled by a case label
        /wd4571 # SEH exceptions aren't caught since Visual C++ 7.1
        /wd5045 # Spectre mitigation

        # Warnings triggered by the FBX SDK
        /wd4266 # No override available (function is hidden)
        /wd4619 # Unknown warning number
        /wd4625 # Copy constructor implicitly deleted
        /wd4626 # Copy assignment operator implicitly deleted

        # Warnings triggered by Catch
        /wd4365 # Signed/unsigned mismatch (implicit conversion)
        /wd4388 # Signed/unsigned mismatch (equality comparison)
        /wd4583 # Destructor not implicitly called
        /wd4623 # Default constructor implicitly deleted
        /wd4868 # Evaluation order not guaranteed in braced initializing list
        /wd5204 # Class with virtual functions but no virtual destructor

        # Warnings triggered by MSVC's standard library
        /wd4355 # 'this' used in base member initializing list
        /wd4514 # Unreferenced inline function has been removed
        /wd4548 # Expression before comma has no effect
        /wd4668 # Preprocessor macro not defined
        /wd4710 # Function not inlined
        /wd4711 # Function inlined
        /wd4774 # Format string is not a string literal
        /wd4820 # Added padding to members
        /wd5026 # Move constructor implicitly deleted
        /wd5027 # Move assignment operator implicitly deleted
        /wd5039 # Pointer/ref to a potentially throwing function passed to an 'extern "C"' function (with -EHc)
    )

    # To automatically export all the classes & functions
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

    # CMake automatically appends /W3 to the standard flags, which produces a warning with MSVC when adding another level; this has to be removed
    # TODO: if possible, this should be done per target, not globally
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif ()

if (ATMOS_COMPILER_MSVC OR ATMOS_COMPILER_CLANG_CL)
    set(
        ATMOS_COMPILER_FLAGS

        ${ATMOS_COMPILER_FLAGS}
        /permissive- # Improving standard compliance
        /EHsc # Enabling exceptions
        /utf-8 # Forcing MSVC to actually handle files as UTF-8
    )

    target_compile_definitions(
        Atmos

        PRIVATE

        NOMINMAX # Preventing definitions of min & max macros
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING # Ignoring std::codecvt deprecation warnings
    )
endif ()

########################
# Atmos - Source files #
########################

set(
    ATMOS_SRC

    main.cpp
    src/Atmos/*.cpp

    include/Atmos/*.hpp
    include/Atmos/*.inl
)

# Adding every file to be compiled
file(
    GLOB
    ATMOS_FILES

    ${ATMOS_SRC}
)

#####################
# Atmos - RaZ usage #
#####################

# Finding RaZ
option(ATMOS_BUILD_RAZ "Build RaZ alongside Atmos (requires downloading the submodule)" OFF)

if (NOT ATMOS_BUILD_RAZ)
    set(RAZ_ROOT "C:/RaZ" CACHE PATH "Path to the RaZ installation")

    set(RAZ_LIB_DIR "${RAZ_ROOT}/lib")

    # Visual Studio having all the configurations from within the project, CMAKE_BUILD_TYPE is unknown at generation time
    # Adding a link directory automatically creates anoter path to which is appended the $(Configuration) macro, which contains the build type
    if (NOT ATMOS_COMPILER_MSVC)
        set(RAZ_LIB_DIR "${RAZ_LIB_DIR}/${CMAKE_BUILD_TYPE}")
    endif ()

    target_link_directories(Atmos PRIVATE "${RAZ_LIB_DIR}")
    target_include_directories(Atmos PUBLIC "${RAZ_ROOT}/include")
    target_compile_definitions(Atmos PRIVATE RAZ_USE_WINDOW) # Raz::Window is needed

    # Additional linking flags
    if (ATMOS_PLATFORM_WINDOWS)
        set(
            ATMOS_LINKER_FLAGS

            ${ATMOS_LINKER_FLAGS}
            opengl32
        )
    elseif (ATMOS_PLATFORM_LINUX)
        set(
            ATMOS_LINKER_FLAGS

            ${ATMOS_LINKER_FLAGS}
            dl
            pthread
            GL
            X11
            Xrandr
            Xcursor
            Xinerama
            Xxf86vm
        )
    elseif (ATMOS_PLATFORM_MAC)
        find_package(OpenGL REQUIRED)

        set(
            ATMOS_LINKER_FLAGS

            ${ATMOS_LINKER_FLAGS}
            OpenGL::GL
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
        )
    endif ()
else ()
    set(RAZ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/RaZ")

    if (EXISTS "${RAZ_ROOT}")
        # No need to keep the examples, unit tests & documentation generation
        set(RAZ_BUILD_EXAMPLES OFF)
        set(RAZ_BUILD_TESTS OFF)
        set(RAZ_GEN_DOC OFF)

        add_subdirectory("${RAZ_ROOT}")
    else ()
        message(FATAL_ERROR "Failed to find RaZ; the submodule must be downloaded")
    endif ()
endif ()

target_compile_definitions(Atmos PRIVATE RAZ_ROOT="${RAZ_ROOT}/")
target_link_libraries(Atmos PUBLIC RaZ)

#################
# Atmos - Build #
#################

target_include_directories(Atmos PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (NOT ATMOS_COMPILER_MSVC)
    # Defining the compiler flags only for C++; this doesn't work with MSVC
    set(ATMOS_COMPILER_FLAGS $<$<COMPILE_LANGUAGE:CXX>:${ATMOS_COMPILER_FLAGS}>)
endif ()

target_compile_options(Atmos PRIVATE ${ATMOS_COMPILER_FLAGS})
target_link_libraries(Atmos PRIVATE ${ATMOS_LINKER_FLAGS})

# Cygwin's Clang needs to use GCC's standard library
if (CYGWIN AND ATMOS_COMPILER_CLANG)
    target_compile_options(Atmos PRIVATE -stdlib=libstdc++)
    target_link_libraries(Atmos PRIVATE stdc++)
endif ()

# Compiling Atmos' sources
target_sources(Atmos PRIVATE ${ATMOS_FILES})
