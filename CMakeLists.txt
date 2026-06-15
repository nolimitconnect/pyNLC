cmake_minimum_required(VERSION 3.22.1)

project(CppModules LANGUAGES C CXX)

set(PROJECT_SRC_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")


# Qt Creator can inject a per-build hook script through
# CMAKE_PROJECT_INCLUDE_BEFORE. If that generated file is removed, CMake fails
# inside project(). Clear stale paths so configure can continue.
if(DEFINED CMAKE_PROJECT_INCLUDE_BEFORE
   AND NOT CMAKE_PROJECT_INCLUDE_BEFORE STREQUAL ""
   AND NOT EXISTS "${CMAKE_PROJECT_INCLUDE_BEFORE}")
    message(WARNING "Ignoring missing CMAKE_PROJECT_INCLUDE_BEFORE: ${CMAKE_PROJECT_INCLUDE_BEFORE}")
    unset(CMAKE_PROJECT_INCLUDE_BEFORE CACHE)
    unset(CMAKE_PROJECT_INCLUDE_BEFORE)
endif()

if(WIN32)
    # CMake 4.2+: MSVC is no longer treated as an ASM compiler.
    # Keep Android on clang for ASM and use NASM for non-Android builds.
    if(POLICY CMP0194)
        cmake_policy(SET CMP0194 OLD)
    endif()

    # Keep MSVC runtime selection on legacy flag mode so ASM-based subprojects
    # are not validated against CMAKE_MSVC_RUNTIME_LIBRARY.
    if(POLICY CMP0091)
        cmake_policy(SET CMP0091 OLD)
    endif() 
endif()

project(MainSystem LANGUAGES C CXX)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(nlc_target_add_supported_warning_options target language)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "nlc_target_add_supported_warning_options: target '${target}' does not exist")
    endif()

    string(TOUPPER "${language}" _nlc_lang)
    if(NOT _nlc_lang STREQUAL "C" AND NOT _nlc_lang STREQUAL "CXX")
        message(FATAL_ERROR "nlc_target_add_supported_warning_options: language must be C or CXX")
    endif()

    foreach(_nlc_flag IN LISTS ARGN)
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" _nlc_flag_id "${_nlc_flag}")
        set(_nlc_cache_var "NLC_HAS_${_nlc_lang}_${_nlc_flag_id}")

        if(NOT DEFINED ${_nlc_cache_var})
            if(_nlc_lang STREQUAL "C")
                check_c_compiler_flag("${_nlc_flag}" ${_nlc_cache_var})
            else()
                check_cxx_compiler_flag("${_nlc_flag}" ${_nlc_cache_var})
            endif()
        endif()

        if(${_nlc_cache_var})
            target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:${_nlc_lang}>:${_nlc_flag}>)
        endif()
    endforeach()
endfunction()

# pybind11 requires C++14 or newer. Setting it to 17 globally across all targets.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# --------------------------
# Detect CPU architecture (only 64-bit allowed)
# --------------------------
set(TARGET_CPU_UNKNOWN TRUE)

if(MSVC)
    # only x64 supported on windows
    set(TARGET_CPU_X64 TRUE)
    set(TARGET_OS_WINDOWS TRUE)
    message(STATUS "*** TARGET_OS_WINDOWS ***")
    add_compile_definitions(TARGET_OS_WINDOWS TARGET_CPU_X64=1)

else()
    set(TARGET_POSIX true)
    add_compile_definitions(TARGET_POSIX=1)

    if(DEFINED ANDROID_ABI)
        # Android ABIs
        message(STATUS "*** TARGET_OS_ANDROID ***")
        if(ANDROID_ABI STREQUAL "arm64-v8a")
            set(TARGET_CPU_ARM64 TRUE)
            set(TARGET_OS_ANDROID TRUE)
            add_compile_definitions(TARGET_OS_ANDROID TARGET_CPU_ARM64=1)
        elseif(ANDROID_ABI STREQUAL "x86_64")
            set(TARGET_CPU_X64 TRUE)
            set(TARGET_OS_ANDROID TRUE)
            add_compile_definitions(TARGET_OS_ANDROID TARGET_CPU_X64=1)
        else()
            message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}. Only 64-bit CPUs allowed.")
        endif()

    else()
        # Non-Android or Windows platforms
        if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "")
            message(FATAL_ERROR "CMAKE_SYSTEM_PROCESSOR is empty. Unable to determine CPU architecture.")   
        endif()
        message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
        string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROC_LOWER)
        if(SYSTEM_PROC_LOWER MATCHES "amd64|x86_64|x64")
            set(TARGET_CPU_X64 TRUE)
            set(TARGET_OS_LINUX TRUE)
            add_compile_definitions(TARGET_OS_LINUX=1 TARGET_CPU_X64=1)
        elseif(SYSTEM_PROC_LOWER MATCHES "aarch64|arm64")
            set(TARGET_CPU_ARM64 TRUE)
            set(TARGET_OS_LINUX TRUE)
            add_compile_definitions(TARGET_OS_LINUX=1 TARGET_CPU_ARM64=1)
        else()
            message(FATAL_ERROR "Unsupported CPU: ${CMAKE_SYSTEM_PROCESSOR}. Only x64 or ARM64 allowed.")
        endif()

    endif()
endif()

# Global Include Directories for Core Engine Core Headers
include_directories("${CMAKE_CURRENT_LIST_DIR}/libs")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# --------------------------
# Print summary
# --------------------------
message(STATUS "**** Target Summary Begin ****")
if(TARGET_OS_WINDOWS)
    message(STATUS "OS: Windows")
endif()
if(TARGET_OS_LINUX)
    message(STATUS "OS: Linux")
endif()
if(TARGET_OS_ANDROID)
    message(STATUS "OS: Android")
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF) 
endif()
if(TARGET_OS_APPLE)
    message(STATUS "OS: Apple (macOS/iOS)")
endif()

if(TARGET_CPU_X64)
    message(STATUS "CPU: x64 (64-bit)")
elseif(TARGET_CPU_ARM64)
    message(STATUS "CPU: ARM64 (64-bit)")
else()
    message(FATAL_ERROR "Unknown CPU Type for ${CMAKE_SYSTEM_NAME}")
endif()
message(STATUS "**** Target Summary end ****")
message(STATUS "========================================")

# --------------------------
# Echo Canceller Setup
# --------------------------
# NOTE: AEC2 uses less than half the CPU processing that AEC3 requires.
# also AEC2 is less sensitive to timing
set(USE_AEC2 1)
if(USE_AEC2)
    add_compile_definitions(USE_AEC2=1)
    message(STATUS "** Echo canceller backend: AEC2 (libs/libwebrtc-aec2)")
    add_subdirectory(libs/libwebrtc-aec2)
else()
    add_compile_definitions(USE_AEC2=0)
    message(STATUS "** Echo canceller backend: AEC3 (libs/libwebrtc-aec)")
    add_subdirectory(libs/libwebrtc-aec)
endif()

unset(TARGET_CPU_X64)
unset(TARGET_CPU_ARM64)
unset(TARGET_OS_WINDOWS)
unset(TARGET_OS_ANDROID)
unset(TARGET_OS_LINUX)

message(STATUS "**** start pyNLC core library configuration ****")
message(STATUS "========================================")

# we only support 64-bit targets cpus althought 32-bit is theoretically possible
set(TARGET_CPU_64BIT true)
add_compile_definitions(TARGET_CPU_64BIT=1)
set(ARCH_32_BITS false)
add_compile_definitions(ARCH_32_BITS=0)
set(ARCH_64_BITS true)
add_compile_definitions(ARCH_64_BITS=1)

add_compile_definitions(NLC_ARCH_LITTLE_ENDIAN=1)
add_compile_definitions(HAVE_ALIGNED_STACK=1)
# always used floating point instead of FIXED_POINT dsp operations
add_compile_definitions(NLC_FLOATING_POINT=1)

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    if(MSVC)
        set(TARGET_OS_WINDOWS true)
        add_compile_definitions(TARGET_OS_WINDOWS=1)
        set(CMAKE_ASM_NASM_OBJECT_FORMAT win64)
        set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS nasm asm)
        
        set(X64 true)
        add_compile_definitions(X64=1)
        set(TARGET_CPU_X64 true)
        add_compile_definitions(TARGET_CPU_X64=1)

        string(REPLACE "/DUNICODE" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        string(REPLACE "/D_UNICODE" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        remove_definitions(-DUNICODE -D_UNICODE)
        add_definitions(-D_MBCS)
        add_compile_definitions(
                NOMINMAX
                WIN64
                _MBCS
                __STDC_CONSTANT_MACROS
                _CRT_SECURE_NO_WARNINGS
                _CRT_NONSTDC_NO_DEPRECATE
                _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        )
        
    else()
        # Cross compiling android on windows 
        message(STATUS "Build Target OS Android built on host OS Windows")
        set(TARGET_CPU_ARM64 TRUE)
        set(TARGET_OS_ANDROID TRUE)
        add_compile_definitions(TARGET_OS_ANDROID=1 TARGET_CPU_ARM64=1)
    endif()

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(TARGET_OS_LINUX true)
    add_compile_definitions(TARGET_OS_LINUX=1)
    add_compile_definitions(TARGET_POSIX=1)

    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "")
        message(FATAL_ERROR "CMAKE_SYSTEM_PROCESSOR is empty. Unable to determine CPU architecture.")   
    endif()
    message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROC_LOWER)
    if(SYSTEM_PROC_LOWER MATCHES "amd64|x86_64|x64")
        set(X64 true)
        add_compile_definitions(X64=1)
        set(TARGET_CPU_X64 true)
        add_compile_definitions(TARGET_CPU_X64=1)
    elseif(SYSTEM_PROC_LOWER MATCHES "aarch64|arm64")
        set(ARM64 true)
        add_compile_definitions(ARM64=1)
        set(TARGET_CPU_ARM64 true)
        add_compile_definitions(TARGET_CPU_ARM64=1)
    else()
        message(FATAL_ERROR "Unsupported CPU: ${CMAKE_SYSTEM_PROCESSOR}. Only x64 or ARM64 allowed.")
    endif()

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
    set(TARGET_OS_APPLE true)
    add_compile_definitions(TARGET_OS_APPLE=1)
    # TODO: Add Apple support
    message(FATAL_ERROR "Apple support not yet implemented: ${CMAKE_HOST_SYSTEM_NAME}")
else()
    message(FATAL_ERROR "Unsupported host OS: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

if(NOT TARGET_OS_ANDROID) # Android assembly compiled by clang, so NASM is not needed or supported
    enable_language(ASM_NASM)
    if(TARGET_OS_WINDOWS)
        set(ASM_NASM_FLAGS "-f win64")
    elseif(TARGET_OS_LINUX)
        set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)      
        if(TARGET_CPU_ARM64)
            set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS nasm S)
            set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -DPIC -F dwarf ")
        else()
            set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS nasm asm)
            set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -DPIC -F dwarf --DARCH_X86_64=1 ")
        endif()
        set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <DEFINES> <INCLUDES> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")
        set(CMAKE_DEPFILE_FLAGS_ASM_NASM "-MD <DEP_FILE> -MT <DEP_TARGET>")
    endif()   
endif()

#=== project folders setup ===#
if(DEFINED TARGET_OS_ANDROID)
    set(NLC_OS_TYPE android)
elseif(DEFINED TARGET_OS_LINUX)
    set(NLC_OS_TYPE linux)
elseif(DEFINED TARGET_OS_WINDOWS)
    set(NLC_OS_TYPE windows)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(NLC_BUILD_TYPE debug)
else()
    set(NLC_BUILD_TYPE release)
endif()

if(TARGET_CPU_X64)
    set(NLC_CPU_TYPE x64)
elseif(TARGET_CPU_ARM64)
    set(NLC_CPU_TYPE arm64)
else()
    set(NLC_CPU_TYPE unknown)
endif()

# set project static libs directory
file(MAKE_DIRECTORY "${PROJECT_SRC_ROOT_DIR}/build-staticlibs/${NLC_OS_TYPE}")
file(MAKE_DIRECTORY "${PROJECT_SRC_ROOT_DIR}/build-staticlibs/${NLC_OS_TYPE}/${NLC_BUILD_TYPE}")
file(MAKE_DIRECTORY "${PROJECT_SRC_ROOT_DIR}/build-staticlibs/${NLC_OS_TYPE}/${NLC_BUILD_TYPE}/${NLC_CPU_TYPE}")
set(PROJECT_STATIC_LIB_DIR "${PROJECT_SRC_ROOT_DIR}/build-staticlibs/${NLC_OS_TYPE}/${NLC_BUILD_TYPE}/${NLC_CPU_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_STATIC_LIB_DIR}")


# module path is needed for ConfigStaticDependLib.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/cmake_helpers")

# Libraries will still use qt to compile even though we are building a python extension module. This is because the libraries are shared between the C++ and Python targets, and we want to avoid duplicating the build logic for both. The Python extension module will link against the same libraries built with qt, but it will not be a qt application itself. This allows us to maintain a single set of libraries that can be used by both the C++ executable and the Python extension module, while still leveraging qt for the library builds.
# if 6.9.3 is not available then fallback to 6.7.3
set(QT_PATH_X64_NEW "$ENV{HOME}/Qt/6.9.3/gcc_64")
set(QT_PATH_X64_OLD "$ENV{HOME}/Qt/6.7.3/gcc_64")
set(QT_PATH_ARM_NEW "$ENV{HOME}/Qt/6.9.3/gcc_arm64")
set(QT_PATH_ARM_OLD "$ENV{HOME}/Qt/6.7.3/gcc_arm64")
# Only override Qt path if not set or invalid
if(NOT DEFINED CMAKE_PREFIX_PATH OR NOT EXISTS "${CMAKE_PREFIX_PATH}")
    if(NOT DEFINED FLATPAKBUILD AND TARGET_OS_LINUX)
        if(TARGET_CPU_ARM64)
            if(EXISTS "${QT_PATH_ARM_NEW}")
                set(CMAKE_PREFIX_PATH "${QT_PATH_ARM_NEW}")
            elseif(EXISTS "${QT_PATH_ARM_OLD}")
                set(CMAKE_PREFIX_PATH "${QT_PATH_ARM_OLD}")
            else()
                message(FATAL_ERROR "No suitable Qt installation found")
            endif()
        else()
            if(EXISTS "${QT_PATH_X64_NEW}")
                set(CMAKE_PREFIX_PATH "${QT_PATH_X64_NEW}")
            elseif(EXISTS "${QT_PATH_X64_OLD}")
                set(CMAKE_PREFIX_PATH "${QT_PATH_X64_OLD}")
            else()
                message(FATAL_ERROR "No suitable Qt installation found")
            endif()
        endif()
        message(STATUS "CMAKE_PREFIX_PATH is now ${CMAKE_PREFIX_PATH}")
    endif()
endif()

if(NOT TARGET_OS_WINDOWS)
    # libjpeg-turbo has speed advantage over libjpeg
    # set if want to use libturbo-jpeg instead of libjpeg
    set(USE_LIBJPEG_TURBO true)
    add_compile_definitions(USE_LIBJPEG_TURBO=1)
endif()


# Add upstream core logic libraries
add_subdirectory(libs)

# ----------------------------------------------------------------------
# Target Swap: Build Python Extension module instead of C++ executable
# ----------------------------------------------------------------------
message(STATUS "Configuring Python pybind11 extension modules...")

# Include local pybind11 checkout directory at root
add_subdirectory(pybind11)

# Build the custom Python binder bridge project
add_subdirectory(py_wrapper)

