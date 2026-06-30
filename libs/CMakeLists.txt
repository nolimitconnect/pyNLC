
# CMakeLists.txt for nolimitapp libraries

# all library project defines
add_compile_definitions(HAVE_CONFIG_H LIB_STATIC ENABLE_NLC_PLAYER)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
# Mirror source include path in binary tree so generated headers (e.g. CoreLib/AppVersion.h)
# are found with the same #include <CoreLib/...> pattern used across all libraries.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(DEFINED TARGET_OS_WINDOWS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sysheaders)
endif()

include(ConfigStaticDependLib)

#=== project subdirectories ===#

if(BUILD_SYSTEM_FLATPAK)
    # FlatPak 6.7 works but 6.9 fails

    # with Raspberry Pi 5:
    # bwrap: execvp appstream-compose: No such file or directory

    # with Ubuntu 24.04:
    # /usr/lib/gcc/x86_64-unknown-linux-gnu/14.3.0/../../../../x86_64-unknown-linux-gnu/bin/ld: cannot find -lGLU: No such file or directory
    # even though modules has
    # - shared-modules/glu/glu-9.json

    # so build a static libGLU
    set(BUILD_LIB_GLU true)
endif()

if(BUILD_LIB_GLU)
    add_subdirectory(libGLU)
endif()

add_subdirectory(libplayer-nlc)
add_subdirectory(libaudio-nlc)

add_subdirectory(libvpx)

add_subdirectory(ffmpeg)

add_subdirectory(CoreLib)
add_subdirectory(MediaToolsLib)
add_subdirectory(NetLib)
add_subdirectory(PktLib)
add_subdirectory(VxVideoLib)
#add_subdirectory(androidNdk) # not used as library but used in android builds

#add_subdirectory(flatbuffers) # not used directly but headers are used

add_subdirectory(libasprintf)
add_subdirectory(libass)

add_subdirectory(libcrossguid)
add_subdirectory(libdav1d)

add_subdirectory(libfreetype)
add_subdirectory(libfribidi)

add_subdirectory(libgnu)
add_subdirectory(libiconv)

if(USE_LIBJPEG_TURBO)
    message(STATUS "adding subdir libjpeg-turbo")
    add_subdirectory(libjpeg-turbo)
else()
     message(STATUS "adding subdir libjpg")
    add_subdirectory(libjpg)
endif()

add_subdirectory(liblame)
#add_subdirectory(liblcms2) # not used but headers needed for avcodec
add_subdirectory(liblzma)
add_subdirectory(liblzo)
add_subdirectory(libmfx)

add_subdirectory(libminiupnpc)

add_subdirectory(libogg)
add_subdirectory(libopencore-amr)

add_subdirectory(libpcre)

add_subdirectory(librnnoise)

#add_subdirectory(libplist) # not used ?? pretty sure it is needed

if(DEFINED TARGET_OS_WINDOWS)
    add_subdirectory(libpthread) # only needed on windows
endif()

add_subdirectory(libptopengine)

add_subdirectory(libspeex)
add_subdirectory(libtag)
add_subdirectory(libtheora)
add_subdirectory(libtinyxml)
add_subdirectory(libtwolame)
add_subdirectory(libvorbis)
add_subdirectory(libwav-decoder)

#libx265 does not compile for arm7a also kodi does not include in arm builds
if(NOT DEFINED TARGET_CPU_ARM32)
 add_subdirectory(libx265)
endif()

add_subdirectory(libx264)

#add_subdirectory(libxavs) # not used (video codec)
add_subdirectory(libxml2)
#add_subdirectory(libxvid) # not used (video codec)
add_subdirectory(libzip)
add_subdirectory(libzlib)

if(DEFINED TARGET_OS_WINDOWS)    
    # glew is no longer supported on windows so build it ourselves. It is only used for the OpenGL renderer in the Qt overlay and is not needed on other platforms where we use the system's OpenGL implementation.
    add_subdirectory(libglew) 
endif()


if(DEFINED TARGET_OS_WINDOWS)
    function(nlc_strip_gcc_warning_options_from_dir dirPath)
        get_property(dirTargets DIRECTORY "${dirPath}" PROPERTY BUILDSYSTEM_TARGETS)
        foreach(targetName IN LISTS dirTargets)
            get_target_property(targetOptions "${targetName}" COMPILE_OPTIONS)
            if(targetOptions)
                set(filteredTargetOptions "")
                foreach(opt IN LISTS targetOptions)
                    if(opt MATCHES "(^|:)\\-W" OR opt MATCHES "(^| )\\-W")
                    else()
                        list(APPEND filteredTargetOptions "${opt}")
                    endif()
                endforeach()
                set_target_properties("${targetName}" PROPERTIES COMPILE_OPTIONS "${filteredTargetOptions}")
            endif()
        endforeach()

        get_property(childDirs DIRECTORY "${dirPath}" PROPERTY SUBDIRECTORIES)
        foreach(childDir IN LISTS childDirs)
            nlc_strip_gcc_warning_options_from_dir("${childDir}")
        endforeach()
    endfunction()

    nlc_strip_gcc_warning_options_from_dir("${CMAKE_CURRENT_SOURCE_DIR}")
endif()




