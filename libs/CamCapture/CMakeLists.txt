
cmake_minimum_required (VERSION 3.22.1)

project(CamCapture, LANGUAGES C CXX)



set(CamCapture_SOURCES)
list(APPEND CamCapture_SOURCES 
    CamCapture.cpp
    CamCapture.h
    CamProcessor.cpp
    CamProcessor.h
    CamRgbVideo.h
)

if(DEFINED TARGET_OS_WINDOWS) 
    list(APPEND CamCapture_SOURCES 
        CamWindows.cpp
        CamWindows.h
    )
endif()

if(DEFINED TARGET_OS_LINUX) 
    list(APPEND CamCapture_SOURCES 
        CamV4L2.cpp
        CamV4L2.h
    )
endif()

if(DEFINED TARGET_OS_ANDROID) 
    list(APPEND CamCapture_SOURCES 
        CamJavaClient.cpp
        CamJavaClient.h
    )
endif()

add_library(CamCapture STATIC 
    ${CamCapture_SOURCES}
)

target_compile_definitions(CamCapture
  PRIVATE

)

target_include_directories(CamCapture
  PRIVATE
    ../
    ../libptopengine
    ../ffmpeg
    ../ffmpeg/opus
)

