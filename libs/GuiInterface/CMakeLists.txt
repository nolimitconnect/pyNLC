
cmake_minimum_required (VERSION 3.22.1)

project(GuiInterface, LANGUAGES CXX)

set(GuiInterface_SOURCES)

list(APPEND GuiInterface_SOURCES 
    IAudioDefs.h
    IAudioInterface.h
    IDefs.cpp
    IDefs.h
    IFromGui.h
    IFromGuiDefs.h
    ILog.cpp
    ILog.h
    IMediaPlayerCallback.h
    IMediaPlayerRequests.h
    INlcEvents.h
    INlcGeometry.h
    INlcRender.h
    IScan.h
    ITimeDefs.h
    IToGui.h
    NlcRenderFrame.h
)

add_library(GuiInterface
    STATIC
    ${GuiInterface_SOURCES}
)

target_include_directories(GuiInterface 
        PRIVATE 
        ../
    ../libGLU/include
        ../libplayer-nlc/xbmc
        ../libptopengine
)

target_compile_definitions(GuiInterface
    PRIVATE
    ENABLE_VSTREAM
    ENABLE_NLC_PLAYER
)

