//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "RenderGlLogic.h"
#include "RenderPlayerNlcThread.h"
#include "RenderGlOffScreenSurface.h"
#include "RenderGlWidget.h"

#include <QColorSpace>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxThread.h>
#include <CoreLib/VxElapseTimer.h>

# if defined(TARGET_OS_APPLE)
#  include <OpenGLES/ES2/gl.h>
# elif defined(TARGET_OS_ANDROID)
#  include <GLES2/gl2.h>
#  include <GLES2/gl2ext.h>
#  include <GLES3/gl3.h>
# elif defined(TARGET_OS_LINUX)
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/glext.h>
# elif defined(TARGET_OS_WINDOWS)
#  include <GL/gl.h>
#  include <GL/glu.h>
# endif // defined(TARGET_OS_ANDROID)

#include <QApplication>

#include <GuiInterface/IMediaPlayerRequests.h>
#include "MediaPlayerNlc.h"

//============================================================================
RenderGlLogic::RenderGlLogic( RenderGlWidget& renderWidget, QWidget* parent )
: QWidget( parent )
, m_RenderPlayerNlcThread( new RenderPlayerNlcThread( *this ) )
, m_RenderWidget( renderWidget )
, m_RenderGlShaders( *this )
{
}

//============================================================================
void RenderGlLogic::aboutToDestroy( void )
{
    if( m_AboutToDestroyCalled )
    {
        // already called
        return;
    }

    m_AboutToDestroyCalled = true;
    setRenderPlayerNlcThreadShouldRun( false );
}

//============================================================================
void RenderGlLogic::waitForPlayerExit( void )
{
    if( m_RenderPlayerNlcThread  )
    {
        LogModule( eLogVideoRender, LOG_VERBOSE, "RenderGlLogic::aboutToDestroy waiting for thread" );
        //IMediaPlayerRequests::getNlcPlayer().fromGuiStopModule( eMediaModulePlayerNlc );
        VxElapseTimer waitTimer;
        m_RenderPlayerNlcThread->quit(); // some platforms may not have windows to close so ensure quit()
        while( m_RenderPlayerNlcThread->isRunning() )
        {
            VxSleep( 500 );

            if( m_RenderPlayerNlcThread->isRunning() )
            {
                LogModule( eLogVideoRender, LOG_VERBOSE, "RenderGlLogic::aboutToDestroy still waiting for thread to exit %3.3f ms", waitTimer.elapsedMs() );
            }
        }

        //m_RenderPlayerNlcThread->wait();
        LogModule( eLogVideoRender, LOG_VERBOSE, "RenderGlLogic::aboutToDestroy waited for thread %3.3f ms", waitTimer.elapsedMs() );
        //delete m_RenderPlayerNlcThread;
        m_RenderPlayerNlcThread->deleteLater();
        m_RenderPlayerNlcThread = nullptr;
    }
}

//============================================================================
void RenderGlLogic::setRenderPlayerNlcThreadShouldRun( bool shouldRun )
{
    if( m_RenderPlayerNlcThread )
    {
        m_RenderPlayerNlcThread->setRenderThreadShouldRun( shouldRun );
    }
}

//============================================================================
bool RenderGlLogic::isRenderPlayerNlcThreadStarted()
{
    return m_RenderPlayerNlcThread && m_RenderPlayerNlcThread->isRenderThreadStarted();
}

//============================================================================
void RenderGlLogic::startRenderPlayerNlcThread()
{
    if( m_RenderPlayerNlcThread && !m_RenderPlayerNlcThread->isRenderThreadStarted() )
    {
        m_RenderPlayerNlcThread->startRenderThread();
    }
}

//============================================================================
void RenderGlLogic::stopRenderPlayerNlcThread()
{
    if( m_RenderPlayerNlcThread && m_RenderPlayerNlcThread->isRenderThreadStarted() )
    {
        m_RenderPlayerNlcThread->stopRenderThread();
    }
}

//============================================================================
//! must be called from render thread
void RenderGlLogic::initRenderGlContext()
{
    m_ThreadGlContext = new QOpenGLContext( this );
    QSurfaceFormat surfaceFormat = m_ThreadGlContext->format();

    // deprecated surfaceFormat.setColorSpace( QSurfaceFormat::ColorSpace::sRGBColorSpace );
    QColorSpace colorSpace(QColorSpace::NamedColorSpace::SRgb);
    surfaceFormat.setColorSpace( colorSpace );

#ifdef TARGET_OS_ANDROID
    surfaceFormat.setRenderableType( QSurfaceFormat::RenderableType::OpenGLES );
#else
    surfaceFormat.setRenderableType( QSurfaceFormat::RenderableType::OpenGL ); // QSurfaceFormat::RenderableType::OpenGLES );
#endif // TARGET_OS_ANDROID
    surfaceFormat.setSwapBehavior( QSurfaceFormat::SwapBehavior::DoubleBuffer );
    surfaceFormat.setAlphaBufferSize( 8 );
    surfaceFormat.setRedBufferSize( 8 );
    surfaceFormat.setGreenBufferSize( 8 );
    surfaceFormat.setBlueBufferSize( 8 );
    surfaceFormat.setDepthBufferSize( 24 );
    surfaceFormat.setStencilBufferSize( 8 );
    surfaceFormat.setSwapInterval( 1 );
    surfaceFormat.setSamples( 0 );

    m_ThreadGlContext->setFormat( surfaceFormat );
    m_ThreadGlContext->create();
    //m_ThreadGlContext->blockSignals( true );

    m_RenderThreadSurface = new RenderGlOffScreenSurface( this, &m_RenderWidget, m_ThreadGlContext ); // geometry not set at this point 
    m_RenderThreadSurface->setFormat( surfaceFormat );
    m_RenderThreadSurface->create();

    m_ThreadGlContext->makeCurrent( m_RenderThreadSurface );

    m_Glf = m_ThreadGlContext->functions();
    Q_ASSERT( m_Glf );
    m_Glf->initializeOpenGLFunctions();

    m_GlfExtra = m_ThreadGlContext->currentContext()->extraFunctions();
    Q_ASSERT( m_GlfExtra );
    if( m_GlfExtra )
    {
        m_GlfExtra->initializeOpenGLFunctions();
        m_Glf = m_GlfExtra;
    }

    m_RenderThreadSurface->setRenderFunctions( m_Glf, m_GlfExtra );

    glWidgetInitialized( m_ThreadGlContext, m_RenderThreadSurface );
    m_ThreadGlContext->doneCurrent();
}

//============================================================================
//! called from gui thread when ready for opengl rendering
void RenderGlLogic::glWidgetInitialized( QOpenGLContext * threadGlContext, RenderGlOffScreenSurface * renderSurface )
{
    m_ThreadGlContext = threadGlContext;
    m_RenderThreadSurface = renderSurface;

    // shader initialize must be done in gui thread
    m_RenderGlShaders.initShaders( m_GlfExtra );
    m_RenderSystemReady = true;
}

//============================================================================
void RenderGlLogic::setSurfaceSize( QSize surfaceSize )
{
    if( m_RenderThreadSurface  
        && surfaceSize.width() > 0
        && surfaceSize.height() > 0 )
    {
        m_RenderThreadSurface->setSurfaceSize( surfaceSize );
    }
    else
    {
        if( m_RenderThreadSurface )
        {
            LogModule( eLogVideoRender, LOG_ERROR, "%s no surface available", __func__ );
            vx_assert( false );
        }
        else
        {
            LogModule( eLogVideoRender, LOG_ERROR, "%s bad size param w%d h%d", __func__, surfaceSize.width(), surfaceSize.height() );
            vx_assert( false );
        }
    }
}

//============================================================================
bool RenderGlLogic::initRenderGlSystem()
{
    LogModule( eLogVideoRender, LOG_DEBUG, "%s thread %d", __func__, VxGetCurrentThreadId() );

    if( m_RenderThreadSurface )
    {
        m_RenderThreadSurface->initRenderGlSystem();
        VerifyGLStateQt();
        m_RenderRunning = true;
        LogModule( eLogVideoRender, LOG_DEBUG, "%s initialized", __func__ );
        return true;
    }
    else
    {
        LogModule( eLogVideoRender, LOG_ERROR, "%s no surface available", __func__ );
        vx_assert( false );
        return false;
    }
}

//============================================================================
bool RenderGlLogic::destroyRenderGlSystem()
{
    VerifyGLStateQt();
    m_RenderRunning = false;
    if( m_RenderThreadSurface )
    {
        m_RenderThreadSurface->destroyRenderGlSystem();
    }

    return true;
}

//============================================================================
bool RenderGlLogic::beginRenderGl()
{
    VerifyGLStateQt();
    if( m_RenderThreadSurface )
    {
        m_RenderThreadSurface->beginRenderGl();
    }

    VerifyGLStateQt();
    return true;
}

//============================================================================
bool RenderGlLogic::endRenderGl()
{
    VerifyGLStateQt();
    //render();

    if( m_RenderThreadSurface )
    {
        m_RenderThreadSurface->endRenderGl();
    }

    VerifyGLStateQt();
    return true;
}

//============================================================================
void RenderGlLogic::presentRenderGl( bool rendered, bool videoLayer )
{
    VerifyGLStateQt();
    if( m_RenderThreadSurface )
    {
        m_RenderThreadSurface->presentRenderGl( rendered, videoLayer );
        emit signalFrameRendered();
    }

    VerifyGLStateQt();
}

//============================================================================
#ifdef DEBUG
void  RenderGlLogic::VerifyGLStateQtDbg( const char* szfile, const char* szfunction, int lineno )
{
    GLenum err = glGetError();
    if( err == GL_NO_ERROR )
        return;
#ifdef TARGET_OS_ANDROID
    // no equivelent of gluErrorString on android platform
    LogModule( eLogVideoRender, LOG_ERROR, "GL ERROR: %d", err );
#else
    LogModule( eLogVideoRender, LOG_ERROR, "GL ERROR: %d %s", err, gluErrorString( err ) );
#endif // TARGET_OS_ANDROID
    //if( szfile && szfunction )
    //{
    //    LogMsg( LOG_ERROR, "In file:%s function:%s line:%d", szfile, szfunction, lineno );
    //}
}
#else
void RenderGlLogic::VerifyGLStateQt()
{
    GLenum err = glGetError();
    if( err == GL_NO_ERROR )
        return;
#ifdef TARGET_OS_ANDROID
    // no equivelent of gluErrorString on android platform
    LogModule( eLogVideoRender, LOG_ERROR, "GL ERROR: %d", err );
#else
    LogModule( eLogVideoRender, LOG_ERROR, "GL ERROR: %d %s", err, gluErrorString( err ) );
#endif // TARGET_OS_ANDROID
}
#endif

//============================================================================
void RenderGlLogic::verifyGlState( const char* msg ) // show gl error if any
{
#ifdef DEBUG_RENDER_THREADS
    if( msg && getRenderThreadId() && ( getRenderThreadId() != VxGetCurrentThreadId() ) )
    {
        LogModule( eLogVideoRender, LOG_ERROR, "ERROR %s render thread 0x%X != current thread %d ", msg, getRenderThreadId(), VxGetCurrentThreadId() );
    }
#endif // DEBUG_RENDER_THREADS

#ifdef DEBUG_LOG_RENDER_CALLS
    if( msg && getRenderThreadId() && ( getRenderThreadId() != VxGetCurrentThreadId() ) )
    {
        LogModule( eLogVideoRender, LOG_ERROR, "ERROR %s render thread 0x%X != thread %d ", msg, getRenderThreadId(), VxGetCurrentThreadId() );
    }
    else if( msg )
    {
        LogModule( eLogVideoRender, LOG_ERROR, "gl func %s render thread %d ", msg, getRenderThreadId() );
    }

#endif // DEBUG_RENDER_THREADS

    VerifyGLStateQt();
}
