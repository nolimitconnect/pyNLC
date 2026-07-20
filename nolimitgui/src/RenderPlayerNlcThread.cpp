
//============================================================================
// Copyright (C) 2023 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <QWidget> // must be declared first or Qt 6.2.4 will error in qmetatype.h 2167:23: array subscript value 53 is outside the bounds

#include <GuiInterface/IToGui.h>
#include <P2PEngine/P2PEngine.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

#include "RenderPlayerNlcThread.h"
#include "RenderGlOffScreenSurface.h"
#include "RenderGlWidget.h"

#include <GuiInterface/IMediaPlayerRequests.h>
#include "MediaPlayerNlc.h"

//============================================================================
RenderPlayerNlcThread::RenderPlayerNlcThread( RenderGlLogic& renderLogic )
: QThread()
, m_RenderLogic( renderLogic )
{
    //connect( this, SIGNAL( finished() ), this, SLOT( deleteLater() ) );
}

//============================================================================
RenderPlayerNlcThread::~RenderPlayerNlcThread()
{
}

//============================================================================
void RenderPlayerNlcThread::run()
{
    static bool isKodiRunning = false;
    if( !isKodiRunning )
    {
        LogModule( eLogVideoRender, LOG_ERROR, "RenderPlayerNlcThread %d", VxGetCurrentThreadId() );
        isKodiRunning = true;
#ifdef RENDER_LOGO_INSTEAD_OF_KODI
        qDebug() << "hello from worker thread " << VxGetCurrentThreadId();
        m_RenderLogic.initRenderGlSystem();
        while( m_ShouldRun )
        {
            if( !m_RenderLogic.beginRenderGl() )
            {
                qDebug() << "thread beginRender failed";
                break;
            }

            if( !m_RenderLogic.endRenderGl() )
            {
                qDebug() << "thread endRender failed";
                break;
            }

            m_RenderLogic.presentRenderGl( true, true );
            msleep( 30 );
        }

        m_RenderLogic.destroyRenderGlSystem();
        qDebug() << "worker thread done " << VxGetCurrentThreadId();
#else
        // will not return from doRun until kodi is shutdown
        IMediaPlayerRequests::getNlcPlayer().fromThreadStartModule( eMediaModulePlayerNlc );

#endif // RENDER_LOGO_INSTEAD_OF_KODI
        isKodiRunning = false;
        m_IsThreadStarted = false;
    }
    else
    {
        LogModule( eLogVideoRender, LOG_ERROR, "Tried to run kodi twice" );
        m_IsThreadStarted = false;
    }
}

//============================================================================
void RenderPlayerNlcThread::startRenderThread()
{
    if( !isRenderThreadStarted() )
    {
        m_ShouldRun = true;
        m_IsThreadStarted = true;
        start();
    }
}

//============================================================================
void RenderPlayerNlcThread::stopRenderThread()
{
    if( isRenderThreadStarted() )
    {
        m_IsThreadStarted = false;
        //stop();
    }
}
