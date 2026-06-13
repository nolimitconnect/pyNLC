cmake_minimum_required(VERSION 3.22.1)

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

# we only support 64-bit targetsx
set(TARGET_CPU_64BIT true)
add_compile_definitions(TARGET_CPU_64BIT=1)

if(MSVC)
    string(REPLACE "/DUNICODE" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/D_UNICODE" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    remove_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-D_MBCS)
    add_compile_definitions(
            __STDC_CONSTANT_MACROS
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_DEPRECATE
    )

    set(TARGET_OS_WINDOWS true)
    add_compile_definitions(TARGET_OS_WINDOWS)

    set(TARGET_CPU_X64 true)
    add_compile_definitions(TARGET_CPU_X64=1)
    set(ARCH_X86_64 true)
    add_compile_definitions(ARCH_X86_64=1)
    
endif()


# Add upstream core logic libraries (Modify or add remaining static engine lib subdirs below)
add_subdirectory(libs)

# ----------------------------------------------------------------------
# Target Swap: Build Python Extension module instead of C++ executable
# ----------------------------------------------------------------------
message(STATUS "Configuring Python pybind11 extension modules...")

# Include local pybind11 checkout directory at root
add_subdirectory(pybind11)

# Build the custom Python binder bridge project
add_subdirectory(py_wrapper)
