
cmake_minimum_required (VERSION 3.22.1)

project(VirtStream, LANGUAGES C CXX)



set(VirtStream_SOURCES)
list(APPEND VirtStream_SOURCES 
    VirtProviderFile.cpp
    VirtProviderFile.h
    VirtStorageProvider.cpp
    VirtStorageProvider.h
    VirtStreamCache.cpp
    VirtStreamCache.h
    VirtStreamFile.cpp
    VirtStreamFile.h
    VirtStreamMgr.cpp
    VirtStreamMgr.h
    VirtStreamMgrFileIo.cpp
    VirtStreamMgrProvider.cpp
)

if(DEFINED TARGET_OS_WINDOWS) 
    list(APPEND VirtStream_SOURCES 

    )
endif()

if(DEFINED TARGET_OS_LINUX) 
    list(APPEND VirtStream_SOURCES 

    )
endif()

if(DEFINED TARGET_OS_ANDROID) 
    list(APPEND VirtStream_SOURCES 

    )
endif()

add_library(VirtStream STATIC 
    ${VirtStream_SOURCES}
)

target_compile_definitions(VirtStream
  PRIVATE

)

target_include_directories(VirtStream
  PRIVATE
    ../
        ../libptopengine
)

