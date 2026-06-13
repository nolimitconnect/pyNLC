cmake_minimum_required(VERSION 3.16)
project(pyNLC_Wrapper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Locate Python and pybind11 dependencies
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/pybind11)

# 2. Define the compiled module output name
pybind11_add_module(nlc_engine nlc_wrapper.cpp)

# 3. Add global include directories for upstream libraries
target_include_directories(nlc_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/GuiInterface
    ${CMAKE_SOURCE_DIR}/libs/CoreLib
    ${CMAKE_SOURCE_DIR}/libs/NetLib
    ${CMAKE_SOURCE_DIR}/libs/PktLib
    ${CMAKE_SOURCE_DIR}/libs/HostListMgr
)

# 4. Link the wrapper binary directly to the required static library objects
# Adjust these names or paths to match your project's primary CMake targets
target_link_libraries(nlc_engine PRIVATE
    CoreLib
    NetLib
    PktLib
    HostListMgr
)
