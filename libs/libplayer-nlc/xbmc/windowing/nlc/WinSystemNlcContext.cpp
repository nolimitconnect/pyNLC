/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "config_components_kodi.h"
#ifdef HAVE_NLC_GUI

#ifdef TARGET_OS_WINDOWS
HWND g_hWnd;
#endif

#include "VideoSyncNlc.h"
#include "WinSystemNlcContext.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "windowing/WindowSystemFactory.h"

void CWinSystemNlcContext::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem( CreateWinSystem, "qt" );
}

std::unique_ptr<CWinSystemBase> CWinSystemNlcContext::CreateWinSystem()
{
    std::unique_ptr<CWinSystemBase> winSystem( new CWinSystemNlcContext() );
    return winSystem;
}

bool CWinSystemNlcContext::InitWindowSystem()
{
    if( !CWinSystemNlc::InitWindowSystem() )
    {
        return false;
    }

    return true;
}

bool CWinSystemNlcContext::CreateNewWindow( const std::string& name,
                                           bool fullScreen,
                                           RESOLUTION_INFO& res )
{
    //m_pGLContext.Detach();

    //if (!CWinSystemNlc::CreateNewWindow(name, fullScreen, res))
    //{
    //  return false;
    //}

    //if (!m_pGLContext.CreateSurface(m_nativeWindow))
    //{
    //  return false;
    //}

    //const EGLint contextAttribs[] =
    //{
    //  EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    //};

    //if (!m_pGLContext.CreateContext(contextAttribs))
    //{
    //  return false;
    //}

    //if (!m_pGLContext.BindContext())
    //{
    //  return false;
    //}

    //if (!m_pGLContext.SurfaceAttrib())
    //{
    //  return false;
    //}

    if( !m_delayDispReset )
    {
        CSingleLock lock( m_resourceSection );
        // tell any shared resources
        for( std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i )
            ( *i )->OnResetDisplay();
    }

    return true;
}

bool CWinSystemNlcContext::ResizeWindow( int newWidth, int newHeight, int newLeft, int newTop )
{
    return CRenderSystemNlc::ResetRenderSystem( newWidth, newHeight );
}

bool CWinSystemNlcContext::SetFullScreen( bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays )
{
    CreateNewWindow( "", fullScreen, res );
    CRenderSystemNlc::ResetRenderSystem( res.iWidth, res.iHeight );
    return true;
}

void CWinSystemNlcContext::SetVSyncImpl( bool enable )
{
    m_iVSyncMode = enable ? 10 : 0;
    //if (!m_pGLContext.SetVSync(enable))
    //{
    //  m_iVSyncMode = 0;
    //  CLog::Log(LOGERROR, "%s,Could not set egl vsync", __FUNCTION__);
    //}
}

void CWinSystemNlcContext::PresentRenderImpl( bool rendered )
{
    if( m_delayDispReset && m_dispResetTimer.IsTimePast() )
    {
        m_delayDispReset = false;
        CSingleLock lock( m_resourceSection );
        // tell any shared resources
        for( std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i )
            ( *i )->OnResetDisplay();
    }
    if( !rendered )
        return;

//    m_pGLContext.SwapBuffers();
}


std::unique_ptr<CVideoSync> CWinSystemNlcContext::GetVideoSync(CVideoReferenceClock* clock)
{
    std::unique_ptr<CVideoSync> pVSync( new CVideoSyncNlc( clock ) );
    return pVSync;
}

#endif // HAVE_NLC_GUI
