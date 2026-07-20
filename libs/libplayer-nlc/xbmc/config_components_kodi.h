
#pragma once

#if defined(TARGET_OS_WINDOWS)
#include <WinSock2.h>
#include <Windows.h>
#endif // defined(TARGET_OS_WINDOWS)

#ifndef NPT_CONFIG_ENABLE_LOGGING
# define NPT_CONFIG_ENABLE_LOGGING
#endif // NPT_CONFIG_ENABLE_LOGGING

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even with optimizations.
# define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
# define XBMC_FORCE_INLINE // donot use NLC_FORCE_INLINE.. __forceinline causes errors if not declared in correct place
#endif

# define HAVE_NLC_GUI            1 // define if have qt application interface and rendering

# define BUILD_KODI_MAIN        1

// #if defined(TARGET_OS_WINDOWS)
// # define HAS_MDNS 1 // ZeroConfig for microsoft
// #endif // defined(TARGET_OS_WINDOWS)
#ifdef HAVE_LIBNFS
# define HAS_FILESYSTEM_NFS
#endif // HAVE_LIBNFS


//#if defined(HAS_MDNS_EMBEDDED) && defined( TARGET_OS_WINDOWS)
//# define HAS_MDNS           1 
//#endif // HAS_MDNS_EMBEDDED

//#define HAS_MYSQL 1 // uses sqlite3 instead

// #define HAS_UPNP            1 
// #if !defined(TARGET_OS_ANDROID) && !defined(TARGET_OS_LINUX) // TODO implement existing zero config for android
// # define HAS_ZEROCONF        1
// #endif // TARGET_OS_ANDROID


#ifdef HAVE_LIBMICROHTTPD
# define HAS_WEB_SERVER     1
# define HAS_WEB_INTERFACE  1
#endif // HAVE_LIBMICROHTTPD

//#define HAS_JSONRPC

//#define HAS_FILESYSTEM    1
//#define HAS_FILESYSTEM_CDDA   1
//#define HAS_FILESYSTEM_SAP
//#define HAS_FILESYSTEM_SFTP 1 

#ifdef HAVE_LIBSMBCLIENT
# define HAS_FILESYSTEM_SMB
#endif // HAVE_LIBSMBCLIENT

#ifdef HAVE_MYSQL
# define HAS_MYSQL // we use sqlite3 instead for database
#endif // HAVE_MYSQL


// #define HAS_AIRPLAY          // uncomment for apple airplay support.. has dependencies on proprietary apple crap
// #define HAVE_LIBSHAIRPLAY    // cannot get to link without official apple crap ( sdp_get_connection(sdp_t *sdp) etc )
// #if defined(HAVE_LIBSHAIRPLAY)
// # define HAS_AIRTUNES        // air tunes requires libshareplay
// #endif

/*! @note Define "USE_DEMUX" at compile time if demuxing in the PVR add-on is used.
 *        Also XBMC's "DVDDemuxPacket.h" file must be in the include path of the add-on,
 *        and the add-on should set bHandlesDemuxing to true.
 */
//#define USE_DEMUX 1
#define HAS_DVDPLAYER 0         // define to 1 if have dvd player support
// #define HAS_OPTICAL_DRIVE        // uncomment to support dvd/bluray drives
// #define HAS_CDDA_RIPPER      // uncomment to play cd disks
// 
// #define HAVE_LIBNFS 
// 
// #define HAVE_LIBPULSE
// #define HAVE_LIBRTMP
// #define HAVE_LIBSOCKET
// #define HAVE_LIBSHAIRPLAY    // cannot get to link without official apple crap ( sdp_get_connection(sdp_t *sdp) etc )
// #define HAVE_LIBSMBCLIENT

#define HAS_ADSPADDONS          1
#define HAS_DVD_SWSCALE         1
#define HAS_EVENT_SERVER        1
//#define HAS_PYTHON              0
#define HAS_PVRCLIENTS          1
#define HAS_SCREENSAVER         1
#define HAS_VIDEOPLAYER         1
#define HAS_VIDEO_PLAYBACK      1
#define HAS_VISUALISATION       1

//#define HAS_SDL_JOYSTICK  // define for joystick control
# ifndef HAVE_NLC_GUI
#  ifndef HAS_DX
#   define HAS_DX 1
#  endif // HAS_DX
# endif // HAVE_NLC_GUI


#ifdef HAS_OPTICAL_DRIVE
# define HAS_CDDA_RIPPER
#endif

// #define HAS_LIB_OPENMAX 1
// #define HAS_LIBSTAGEFRIGHT 1     // android only
// #define HAS_MMAL 1               // hardware

// #define HAVE_LIBMICROHTTPD  1
#ifdef HAVE_LIBMICROHTTPD
# define HAS_WEB_SERVER     0
# define HAS_WEB_INTERFACE  0
#endif // HAVE_LIBMICROHTTPD

#if defined(HAVE_LIBMDNSEMBEDDED)
# define HAS_ZEROCONF
# define HAS_MDNS
# define HAS_MDNS_EMBEDDED
#endif // defined(HAVE_LIBMDNSEMBEDDED)

#if defined(HAVE_LIBGIF)
# define HAS_GIFLIB
#endif // defined(HAVE_LIBGIF)

#ifdef HAVE_LIBRTMP
# define HAS_LIBRTMP 1
#endif // HAVE_LIBRTMP

#ifdef HAVE_LIBPULSE
# define HAS_PULSEAUDIO
#endif // HAVE_LIBPULSE


#ifdef HAVE_LIBNFS
# define HAS_FILESYSTEM_NFS
#endif // HAVE_LIBNFS

#if defined(HAS_MDNS_EMBEDDED) && defined( TARGET_OS_WINDOWS)
# define HAS_MDNS           1 
#endif // HAS_MDNS_EMBEDDED

//#define HAS_MYSQL 1 // uses sqlite3 instead

//#if !defined(TARGET_OS_ANDROID) && !defined(TARGET_OS_LINUX) // TODO implement existing zero config for android
//# define HAS_ZEROCONF        1
//#endif // TARGET_OS_ANDROID

//#define HAS_JSONRPC

//#define HAS_FILESYSTEM    1
//#define HAS_FILESYSTEM_CDDA   1
//#define HAS_FILESYSTEM_SAP
//#define HAS_FILESYSTEM_SFTP 1 

#ifdef HAVE_LIBSMBCLIENT
# define HAS_FILESYSTEM_SMB
#endif // HAVE_LIBSMBCLIENT

#ifdef HAVE_MYSQL
# define HAS_MYSQL // we use sqlite3 instead for database
#endif // HAVE_MYSQL


/*! @note Define "USE_DEMUX" at compile time if demuxing in the PVR add-on is used.
 *        Also XBMC's "DVDDemuxPacket.h" file must be in the include path of the add-on,
 *        and the add-on should set bHandlesDemuxing to true.
 */
//#define USE_DEMUX 1
#define HAS_DVDPLAYER 0         // define to 1 if have dvd player support
// #define HAS_OPTICAL_DRIVE        // uncomment to support dvd/bluray drives
// #define HAS_CDDA_RIPPER      // uncomment to play cd disks

#define HAS_VIDEOPLAYER         1
#define HAS_VIDEO_PLAYBACK      1

#if defined (ENABLE_KODI)
# define HAS_ADSPADDONS          1
# define HAS_DVD_SWSCALE         1
# define HAS_EVENT_SERVER        1
# define HAS_PYTHON              1
# define HAS_PVRCLIENTS          1
# define HAS_SCREENSAVER         1
# define HAS_UPNP                1 
# define HAS_VISUALISATION       1
#endif // defined (ENABLE_KODI)


// #define HAS_SDL_JOYSTICK  // define for joystick control
# ifndef HAVE_NLC_GUI
#  ifndef HAS_DX
#   define HAS_DX 1
#  endif // HAS_DX
# endif // HAVE_NLC_GUI

#if defined(TARGET_OS_WINDOWS)
    #define THREADFUNC unsigned __stdcall
#else
    typedef int THREADFUNC;
#endif // defined(TARGET_OS_WINDOWS)

// #define HAS_AIRPLAY          // uncomment for apple airplay support.. has dependencies on proprietary apple crap
// #define HAVE_LIBSHAIRPLAY    // cannot get to link without official apple crap ( sdp_get_connection(sdp_t *sdp) etc )
// #if defined(HAVE_LIBSHAIRPLAY)
// # define HAS_AIRTUNES        // air tunes requires libshareplay
// #endif

//============================================================================
//=== Non-free Components ===//
//============================================================================

//#if defined(TARGET_OS_WINDOWS)
//# define HAS_FILESYSTEM_RAR
//#else
//# if defined(HAVE_XBMC_NONFREE)
//#  define HAS_FILESYSTEM_RAR
//# endif // defined(HAVE_XBMC_NONFREE)
//#endif


//============================================================================
//=== Apple Specific ===//
//============================================================================
#if defined(TARGET_OS_APPLE)
# if defined(TARGET_DARWIN)
#  define HAS_GL 1
#  define HAS_SDL 1
#  define HAS_SDL_WIN_EVENTS 1
# endif
# define HAS_ZEROCONF
# define HAS_LINUX_NETWORK
#endif

//============================================================================
//=== Linux Specific ===//
//============================================================================
#if defined(TARGET_OS_LINUX) || defined(TARGET_OS_FREEBSD)
# if defined(HAVE_LIBAVAHI_COMMON) && defined(HAVE_LIBAVAHI_CLIENT)
#  define HAS_ZEROCONF
#  define HAS_AVAHI
# endif
# ifdef HAVE_DBUS
#  define HAS_DBUS
# endif
# ifndef HAVE_NLC_GUI
#  define HAS_GL
# endif // HAVE_NLC_GUI
# ifdef HAVE_X11
#  define HAS_GLX
#  define HAS_X11_WIN_EVENTS
# endif
# ifdef HAVE_SDL
# define HAS_SDL
#  ifndef HAVE_X11
#   define HAS_SDL_WIN_EVENTS
#  endif
# else
#  ifndef HAVE_X11
#   define HAS_LINUX_EVENTS
#  endif
# endif
# define HAS_LINUX_NETWORK
# ifdef HAVE_LIRC
#  define HAS_LIRC
# endif
# ifdef HAVE_ALSA
#  define HAS_ALSA
# endif
#endif

//============================================================================
//=== Android specific ===//
//============================================================================
#if defined(TARGET_OS_ANDROID)
#undef HAS_LINUX_EVENTS
#undef HAS_LIRC
//#define HAS_ZEROCONF // Defined in NlcCompilerConfig.h
//# if defined(HAVE_NLC_GUI)
//#  define HAS_AVAHI
//# endif // defined(HAVE_NLC_GUI)
#endif

//============================================================================
//=== kodi components ===//
//============================================================================

#define NPT_CONFIG_ENABLE_LOGGING
#define PLT_HTTP_DEFAULT_SERVER "UPnP/1.0 DLNADOC/1.50 Kodi"
#define PLT_HTTP_DEFAULT_USER_AGENT "UPnP/1.0 DLNADOC/1.50 Kodi"

#define HAVE_DLL_LIB_ASS			0
#define HAVE_DLL_LIB_IMAGE			0
#define HAVE_DLL_LIB_SHAIRPLAY		0
#define HAVE_DLL_LIB_NFS			0
#define HAVE_DLL_LIB_PLIST			0
#define HAVE_DLL_LIB_CEC			0
#define HAVE_DLL_LIB_DVDNAV			0
#define HAVE_DLL_LIB_EXIF			0
#define HAVE_DLL_LIB_CPLUFF			0

#define HAVE_LIB_CDIO               0
#define HAVE_LIB_CURL			    0
#define HAVE_ADDONS			        0
#define HAVE_WEATHER			    0

#define ENABLE_ADDON_CODEC          0
#define ENABLE_ADDONS_BINARY        0
#define ENABLE_BLURAY               0
#define ENABLE_DVD_NAV              0
#define ENABLE_GAMES                0
#define ENABLE_JSON                 0
#define ENABLE_PERIPHERALS          0
#define ENABLE_POWER_HANDLING       0
#define ENABLE_PVR                  0
#define ENABLE_RADIO                0
#define ENABLE_RSS                  0
#define ENABLE_SHAREPLAY            0
#define ENABLE_SPEECH_RECOGNITION   0
#define ENABLE_UPNP                 0
#define ENABLE_WEBVTT               0
