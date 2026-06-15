cmake_minimum_required(VERSION 3.16)
project(pyNLC_Wrapper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Locate Python and pybind11 dependencies
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# 2. Define the compiled module output name
pybind11_add_module(nlc_engine nlc_wrapper.cpp)

# 3. Add global include directories for upstream libraries
target_include_directories(nlc_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/GuiInterface
    ${CMAKE_SOURCE_DIR}/libs/libplayer-nlc/xbmc
    ${CMAKE_SOURCE_DIR}/libs/ffmpeg
    ${CMAKE_SOURCE_DIR}/libs/libptopengine
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_STATIC_LIB_DIR}
)

# 4. Link the wrapper binary directly to the required static library objects
# common to all builds
target_link_libraries(nlc_engine PRIVATE
    player-nlc
    avdevice
    avfilter
    avformat
    avcodec
    postproc
    swresample
    swscale
    avutil
    ffmpegopus

    miniupnpc

    ptopengine
    netlib

    pktlib

    wav-decoder
    rnnoise
)

if(USE_AEC2)
    target_link_libraries(nlc_engine
        PRIVATE
        webrtc-aec2 # webrtc audio echo cancel
        #absl::strings # absl support for webrtc audio echo cancel
        #absl::base # absl support for webrtc audio echo cancel
    )
else()
    target_link_libraries(nlc_engine
        PRIVATE
        webrtc-aec # webrtc audio echo cancel
        absl::strings # absl support for webrtc audio echo cancel
        absl::base # absl support for webrtc audio echo cancel
    )
endif()


if (DEFINED TARGET_OS_WINDOWS)
    target_link_libraries(nlc_engine
        PRIVATE

        x264
        x265
        dav1d
        vpx

        pcre
        opencore-amr

        mediatools
        speex
        lame
        vorbis2

        gnu
        ass
        fribidi
        freetype

        iconv
        vxvideolib

        tag
        theora
        twolame

        tinyxml
        xml2

        lzma
        lzo
        zip
        CoreLib
        zlib
        crossguid
        pthread
        ogg

        opengl32
        glu32
        ole32
        winmm
        Rpcrt4
        ws2_32
        iphlpapi    # Fixes the unresolved symbols from miniupnpc
        dwmapi      # Fixes DwmIsCompositionEnabled from player-nlc
        cfgmgr32    # Fixes CM_Get_DevNode_Status from player-nlc
        Setupapi    # Fixes SetupDiGetClassDevsA and related functions from player-nlc
        glew        # Fixes glew32s static linking on Windows used by OverlayRendererQt.cpp from player-nlc
    )

endif()

if (DEFINED TARGET_OS_LINUX)
    if (DEFINED TARGET_CPU_ARM64)

        target_link_libraries(nlc_engine
            PRIVATE
            x264
            x265
            dav1d
            vpx
            pcre

            opencore-amr
            mediatools
            speex
            lame
            vorbis2
            ogg

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            tag
            theora
            twolame


            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            pthread
            dl
            GLU
            GL
            m
            uuid
            rt
        )
    else()
        target_link_libraries(nlc_engine
            PRIVATE
            x264
            x265
            dav1d
            vpx
            pcre

            opencore-amr
            mediatools
            speex
            lame
            vorbis2
            ogg

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            tag
            theora
            twolame

            freetype

            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            pthread
            dl
            GLU
            GL
            m
            uuid
            rt
        )
    endif()

elseif (DEFINED TARGET_OS_ANDROID)
    find_package(Qt6 COMPONENTS Gui REQUIRED)
    if (DEFINED TARGET_CPU_ARM64)
        target_link_libraries(nlc_engine
            PRIVATE
            # static nlc libraries

            x264
            x265
            dav1d
            vpx
            pcre

            opencore-amr
            mediatools
            speex
            lame
            vorbis2

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            speex
            tag
            theora
            twolame

            ogg
            netlib
            pktlib

            fribidi
            freetype

            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            dl
            m
            EGL
            GLESv2
            c
            stdc++
            jnigraphics
            android
        )
    elseif(DEFINED TARGET_CPU_ARM32)
        target_link_libraries(nlc_engine
            PRIVATE
            # static nlc libraries

            x264
            #x265 #cannot compile x265 on arm7a
            dav1d
            vpx
            pcre
            opencore-amr

            mediatools
            speex
            lame
            vorbis2

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            speex
            tag
            theora
            twolame


            freetype

            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            dl
            m
            EGL
            GLESv2
            c
            stdc++
            jnigraphics
            android
        )
    endif()

elseif (DEFINED TARGET_OS_WINDOWS)
    # do nothing .. was already linked
else()
    message(FATAL_ERROR "Unknown Target OS Defined for target_link_libraries")
endif()

if(USE_LIBJPEG_TURBO)
    target_link_libraries(nlc_engine
        PRIVATE
        jpegturbo
    )
else()
    target_link_libraries(nlc_engine
        PRIVATE
        jpg
    )
endif()
