#pragma once
//============================================================================
// Created by Brett R. Jones in 2018 and issued to public domain
//============================================================================

#ifdef DEBUG
# define CMAKE_INTDIR            "Debug"
#else
# define CMAKE_INTDIR            "Release"
#endif

# define HAVE_NLC_GUI            1 // define if have qt application interface and rendering
// define USE_STATIC_LIBS if want to create/link static libraries where possible
#define USE_STATIC_LIBS 1

#ifdef USE_STATIC_LIBS
# define TAGLIB_STATIC
# define FREETYPE2_STATIC           1
#endif // TARGET_OS_WINDOWS

// uncomment to enable ffmpeg log
//#define ENABLE_FFMPEG_LOG

// define if linking in CoreLib/VxFunctionsMissingInWindows.cpp so standard functions like strptime are not double linked
#define HAVE_WINDOWS_MISSING_FUNCTIONS 1

