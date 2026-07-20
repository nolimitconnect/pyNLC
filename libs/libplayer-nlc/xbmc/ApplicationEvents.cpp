/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "threads/SystemClock.h"
#include "Application.h"

#include "events/EventLog.h"
#include "events/NotificationEvent.h"

#include "utils/JobManager.h"
#include "utils/Variant.h"

#include "utils/Screenshot.h"
#include "NlcCoreUtil.h"
#include "NlcUrl.h"
#include "guilib/GUIComponent.h"
#include "guilib/TextureManager.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"

#include "video/Bookmark.h"
#include "video/VideoLibraryQueue.h"
#include "music/MusicLibraryQueue.h"
#include "guilib/GUIControlProfiler.h"
#include "utils/LangCodeExpander.h"

#include "playlists/PlayListFactory.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIColorManager.h"
#include "guilib/StereoscopicsManager.h"


#include "guilib/GUIAudioManager.h"

#include "messaging/ThreadMessage.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "SectionLoader.h"

#include "GUIUserMessages.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/PluginDirectory.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "GUILargeTextureManager.h"
#include "TextureCache.h"
#include "playlists/SmartPlayList.h"
#include "playlists/PlayList.h"
#include "profiles/ProfileManager.h"
#include "windowing/WinSystem.h"

#include "guilib/LocalizeStrings.h"
#include "utils/CPUInfo.h"
#include "utils/FileExtensionProvider.h"
#include "utils/log.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"

#include "music/infoscanner/MusicInfoScanner.h"
#include "music/MusicUtils.h"
#include "music/MusicThumbLoader.h"

 // Windows includes
#include "guilib/GUIWindowManager.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "windows/GUIWindowScreensaver.h"
#include "video/PlayerController.h"

// Dialog includes
#include "video/dialogs/GUIDialogVideoBookmarks.h"


#include "video/dialogs/GUIDialogFullScreenInfo.h"
#include "guilib/GUIControlFactory.h"

#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "CompileInfo.h"

#ifdef TARGET_WINDOWS
#include "platform/win32/win32util.h"
#endif

#ifdef TARGET_DARWIN_OSX
#include "platform/darwin/osx/CocoaInterface.h"
#include "platform/darwin/osx/XBMCHelper.h"
#endif
#ifdef TARGET_DARWIN
#include "platform/darwin/DarwinUtils.h"
#endif

//BRJ #ifdef HAS_OPTICAL_DRIVE
//#include <cdio/logging.h>
//#endif

#include "storage/MediaManager.h"
#include "utils/SaveFileStateJob.h"
#include "utils/AlarmClock.h"
#include "utils/StringUtils.h"
#include "DatabaseManager.h"


#ifdef TARGET_POSIX
#include "XHandle.h"
#include "XTimeUtils.h"
#include "platform/posix/filesystem/PosixDirectory.h"
#endif

#if defined(TARGET_ANDROID) && !defined(HAVE_NLC_GUI)
# include <android/jni/Build.h>
# include "platform/android/activity/XBMCApp.h"
# include "platform/android/activity/AndroidFeatures.h"
#elif defined(TARGET_ANDROID) && defined(HAVE_NLC_GUI)
# include <platform/nlc/android/jni/Build.h>
# include "platform/nlc/KodiNlcApp.h"
# include "platform/nlc/android/activity/AndroidFeatures.h"
#endif

#ifdef TARGET_WINDOWS
#include "platform/Environment.h"
#endif

#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif

//TODO: XInitThreads
#ifdef HAVE_X11
#include "X11/Xlib.h"
#endif

#include "cores/FFmpeg.h"
#include "utils/CharsetConverter.h"
#include "pictures/GUIWindowSlideShow.h"

#include <GuiInterface/IToGui.h>

#include "windowing/nlc/WinSystemNlc.h"

#include <CoreLib/VxDebug.h>

using namespace ADDON;
using namespace XFILE;

using namespace PLAYLIST;
//using namespace VIDEO;
using namespace MUSIC_INFO;

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace ActiveAE;

using namespace XbmcThreads;

#define MAX_FFWD_SPEED 5

//============================================================================
void CApplication::fromGuiKeyPressEvent( EMediaModule mediaModule, int key, int mod )
{
    if( CServiceBroker::GetWinSystem() )
    {
        XBMC_Event msg{ XBMC_KEYDOWN };
        msg.key.keysym.scancode = 0;
        msg.key.keysym.sym = ( XBMCKey )key;
        msg.key.keysym.unicode = 0;
        msg.key.keysym.mod = ( XBMCMod )mod;

        CWinSystemNlc* winSys = dynamic_cast< CWinSystemNlc* >( CServiceBroker::GetWinSystem() );
        if( winSys )
        {
            winSys->MessagePush( &msg );
        }
    }
}

//============================================================================
void CApplication::fromGuiKeyReleaseEvent( EMediaModule mediaModule, int key, int mod )
{
    if( CServiceBroker::GetWinSystem() )
    {
        XBMC_Event msg{ XBMC_KEYUP };
        msg.key.keysym.scancode = 0;
        msg.key.keysym.sym = ( XBMCKey )key;
        msg.key.keysym.unicode = 0;
        msg.key.keysym.mod = ( XBMCMod )mod;
        CWinSystemNlc* winSys = dynamic_cast< CWinSystemNlc* >( CServiceBroker::GetWinSystem() );
        if( winSys )
        {
            winSys->MessagePush( &msg );
        }
    }
}

//============================================================================
void CApplication::fromGuiMousePressEvent( EMediaModule mediaModule, int mouseXPos, int mouseYPos, int mouseButton )
{
    if( CServiceBroker::GetWinSystem() )
    {
        XBMC_Event msg{ XBMC_MOUSEBUTTONDOWN };
        msg.button = { ( unsigned char )mouseButton, ( uint16_t )mouseXPos, ( uint16_t )mouseYPos };
        CWinSystemNlc* winSys = dynamic_cast< CWinSystemNlc* >( CServiceBroker::GetWinSystem() );
        if( winSys )
        {
            LogMsg( LOG_VERBOSE, "CApplication::fromGuiMousePressEvent XBMC_MOUSEBUTTONDOWN" );
            winSys->MessagePush( &msg );
        }
    }
}

//============================================================================
void CApplication::fromGuiMouseReleaseEvent( EMediaModule mediaModule, int mouseXPos, int mouseYPos, int mouseButton )
{
    if( CServiceBroker::GetWinSystem() )
    {
        XBMC_Event msg{ XBMC_MOUSEBUTTONUP };
        msg.button = { (unsigned char)mouseButton, (uint16_t)mouseXPos, (uint16_t)mouseYPos };
        CWinSystemNlc* winSys = dynamic_cast< CWinSystemNlc* >( CServiceBroker::GetWinSystem() );
        if( winSys )
        {
             LogMsg( LOG_VERBOSE, "CApplication::fromGuiMousePressEvent XBMC_MOUSEBUTTONUP" );
            winSys->MessagePush( &msg );
        }
    }
}

//============================================================================
void CApplication::fromGuiMouseMoveEvent( EMediaModule mediaModule, int mouseXPos, int mouseYPos )
{
    XBMC_Event msg{ XBMC_MOUSEMOTION };
    msg.motion.x = (uint16_t)mouseXPos;
    msg.motion.y = (uint16_t)mouseYPos;
    CWinSystemNlc* winSys = dynamic_cast< CWinSystemNlc* >( CServiceBroker::GetWinSystem() );
    if( winSys )
    {
        winSys->MessagePush( &msg );
    }
}

//============================================================================
void CApplication::fromGuiRenderWindowResize( EMediaModule mediaModule, int winWidth, int winHeight )
{
    setRenderWindowSize( winWidth, winHeight );
	if( CServiceBroker::GetWinSystem() )
	{
		XBMC_Event msg{ XBMC_VIDEORESIZE };
		msg.resize = { winWidth, winHeight };
        //CWinSystemNlc* winSys = dynamic_cast< CWinSystemNlc* >( CServiceBroker::GetWinSystem() );
        //if( winSys )
        //{
        //    winSys->MessagePush( &msg );
        //}
        OnEvent( msg );
	}
}

//============================================================================
void CApplication::fromGuiCloseEvent( EMediaModule mediaModule )
{
    if( CServiceBroker::GetWinSystem() )
    {
        XBMC_Event msg{ XBMC_QUIT };
        CWinSystemNlc* winSys = dynamic_cast< CWinSystemNlc* >( CServiceBroker::GetWinSystem() );
        if( winSys )
        {
            winSys->MessagePush( &msg );
        }
    }
}

//============================================================================
void CApplication::fromGuiVisibleEvent( EMediaModule mediaModule, bool isVisible )
{

}

//============================================================================
bool CApplication::toGuiMediaAction( EMediaPlayerAction playerAction, int actionVal, const char* fileName )
{
    return IToGui::getIToGui().toGuiMediaAction( eMediaModulePlayerNlc, playerAction, actionVal, fileName );
}

//============================================================================
void CApplication::toGuiMediaError( EMediaError mediaError, const char* msg )
{
    IToGui::getIToGui().toGuiMediaError( eMediaModulePlayerNlc, mediaError, msg );
}

//============================================================================
void CApplication::setRenderWindowSize( int winWidth, int winHeight )
{
    bool wasResized{ false };
    // scope for lock
    {
        std::unique_lock<CCriticalSection> lock(m_critical);
        if( m_RenderWindowWidth != winWidth || m_RenderWindowHeight != winHeight )
        {
            wasResized = true;
            LogModule( eLogVideoIo, LOG_DEBUG, "setRenderWindowSize from %dx%d to %dx%d",
                       m_RenderWindowWidth, m_RenderWindowHeight, winWidth, winHeight );
            m_RenderWindowWidth = winWidth;
            m_RenderWindowHeight = winHeight;
        }
    }

    if( wasResized )
    {
        onRenderWindowResized( winWidth, winHeight );
    }
}

//============================================================================
void CApplication::getRenderWindowSize( int& winWidth, int& winHeight )
{
    std::unique_lock<CCriticalSection> lock(m_critical);
    winWidth = m_RenderWindowWidth;
    winHeight = m_RenderWindowHeight;
}