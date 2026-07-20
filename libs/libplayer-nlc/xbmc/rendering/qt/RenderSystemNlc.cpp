/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "config_components_kodi.h"
#ifdef HAVE_NLC_GUI

#include "RenderSystemNlc.h"

#include "guilib/DirtyRegion.h"


#include "windowing/GraphicContext.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"
#include "utils/MathUtils.h"
#ifdef TARGET_POSIX
#include "XTimeUtils.h"
#endif

#include <GuiInterface/INlcRender.h>

#include <CoreLib/VxDebug.h>

//============================================================================
CRenderSystemNlc::CRenderSystemNlc()
    : CRenderSystemBase()
    , m_INlc( INlcRender::getINlcRender() )
{
}

//============================================================================
bool CRenderSystemNlc::InitRenderSystem()
{
    m_maxTextureSize = 1024;
    m_bVSync = false;
    m_iVSyncMode = 0;
    m_bVsyncInit = false;
    // Get the version number
    m_RenderVersionMajor = 1;
    m_RenderVersionMinor = 0;

    // Get our driver vendor and renderer
    m_RenderVendor = "Nlc";
    m_RenderExtensions = " ";

    //LogGraphicsInfo();

    m_INlc.initRenderSystem();
    m_bRenderCreated = true;

    return true;
}

//============================================================================
bool CRenderSystemNlc::ResetRenderSystem( int width, int height )
{
    int maxTextureSize = m_INlc.getMaxTextureSize( );
    if( maxTextureSize )
    {
        m_maxTextureSize = maxTextureSize;
    }

    return m_INlc.resetRenderSystem(  width, height );
}

//============================================================================
bool CRenderSystemNlc::DestroyRenderSystem()
{
    ResetScissors();
    CDirtyRegionList dirtyRegions;
    CDirtyRegion dirtyWindow( CServiceBroker::GetWinSystem()->GetGfxContext().GetViewWindow() );
    dirtyRegions.push_back( dirtyWindow );

    ClearBuffers( 0 );
    PresentRenderImpl( true );

    m_bRenderCreated = false;
    INlcRender::getINlcRender().destroyRenderSystem( );

    return true;
}

//============================================================================
bool CRenderSystemNlc::BeginRender()
{
    if( !m_bRenderCreated )
        return false;

    m_limitedColorRange = CServiceBroker::GetWinSystem()->UseLimitedColor();

    INlcRender::getINlcRender().beginRender( );
    return true;
}

//============================================================================
bool CRenderSystemNlc::EndRender()
{
    if( !m_bRenderCreated )
        return false;
    INlcRender::getINlcRender().endRender( );
    return true;
}

//============================================================================
bool CRenderSystemNlc::ClearBuffers( UTILS::COLOR::Color color )
{
    return INlcRender::getINlcRender().clearBuffers( (NlcColor)color );
}

//============================================================================
bool CRenderSystemNlc::IsExtSupported( const char* extension ) const
{
    return INlcRender::getINlcRender().isExtSupported( extension );
}

//============================================================================
void CRenderSystemNlc::PresentRender( bool rendered, bool videoLayer )
{
    SetVSync( true );

    if( !m_bRenderCreated )
        return;

    if( videoLayer )
    {
        LogModule( eLogVideoIo, LOG_VERBOSE, "CRenderSystemNlc::PresentRender videoLayer" );
    }

    PresentRenderImpl( rendered );

     m_INlc.presentRender(  rendered, videoLayer );
    // if video is rendered to a separate layer, we should not block this thread
    if( !rendered && !videoLayer )
        Sleep( 40 );
}

//============================================================================
void CRenderSystemNlc::SetVSync( bool enable )
{
    if( m_bVSync == enable && m_bVsyncInit == true )
        return;

    if( !m_bRenderCreated )
        return;

    if( enable )
        CLog::Log( LOGINFO, "Nlc: Enabling VSYNC" );
    else
        CLog::Log( LOGINFO, "Nlc: Disabling VSYNC" );

    m_iVSyncMode = 0;
    m_iVSyncErrors = 0;
    m_bVSync = enable;
    m_bVsyncInit = true;

    m_INlc.verifyGlState( "SetVSync Begin" );
    SetVSyncImpl( enable );
    m_INlc.verifyGlState( "SetVSync End" );

    if( !enable )
        return;

    if( !m_iVSyncMode )
        CLog::Log( LOGERROR, "Nlc: Vertical Blank Syncing unsupported" );
    else
        CLog::Log( LOGINFO, "Nlc: Selected vsync mode %d", m_iVSyncMode );
}

//============================================================================
void CRenderSystemNlc::CaptureStateBlock()
{
    if( !m_bRenderCreated )
        return;
    m_INlc.captureStateBlock( );
}

//============================================================================
void CRenderSystemNlc::ApplyStateBlock()
{
    if( !m_bRenderCreated )
        return;
    m_INlc.applyStateBlock( );
}

//============================================================================
void CRenderSystemNlc::SetCameraPosition( const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor )
{
    if( !m_bRenderCreated )
        return;

    m_INlc.setCameraPosition( (const NlcPoint &)camera, screenWidth, screenHeight, stereoFactor );
}

//============================================================================
void CRenderSystemNlc::Project( float &x, float &y, float &z )
{
    m_INlc.project( x, y, z );
}

//============================================================================
bool CRenderSystemNlc::TestRender()
{
    return  m_INlc.testRender( );
}

//============================================================================
void CRenderSystemNlc::InitialiseShaders()
{
    m_INlc.initializeShaders( );
}

//============================================================================
void CRenderSystemNlc::ReleaseShaders()
{
    m_INlc.releaseShaders();
}

//============================================================================
void CRenderSystemNlc::EnableGUIShader( ESHADERMETHOD method )
{
    m_INlc.enableShader( method );
}

//============================================================================
void CRenderSystemNlc::DisableGUIShader()
{
    return  m_INlc.disableGUIShader();
}

//============================================================================
int CRenderSystemNlc::GUIShaderGetPos()
{
    return  m_INlc.shaderGetPos();
}

//============================================================================
int CRenderSystemNlc::GUIShaderGetCol()
{
    return  m_INlc.shaderGetCol();
}

//============================================================================
int CRenderSystemNlc::GUIShaderGetModel()
{
    return  m_INlc.shaderGetModel();
}

//============================================================================
int CRenderSystemNlc::GUIShaderGetCoord0()
{
    return  m_INlc.shaderGetCoord0();
}

//============================================================================
int CRenderSystemNlc::GUIShaderGetCoord1()
{
    return  m_INlc.shaderGetCoord1();
}

//============================================================================
int CRenderSystemNlc::GUIShaderGetUniCol()
{
    return  m_INlc.shaderGetUniCol();
}

//============================================================================
void CRenderSystemNlc::CalculateMaxTexturesize()
{
    // GLES cannot do PROXY textures to determine maximum size,
    CLog::Log( LOGINFO, "Nlc: Maximum texture width: %u", m_maxTextureSize );
}

//============================================================================
void CRenderSystemNlc::GetViewPort( CRect& viewPort )
{
    if( !m_bRenderCreated )
        return;

    return  m_INlc.getViewPort( (NlcRect&) viewPort );
}

//============================================================================
void CRenderSystemNlc::SetViewPort( const CRect& viewPort )
{
    if( !m_bRenderCreated )
        return;

    m_INlc.setViewPort( ( NlcRect& )viewPort );
}

//============================================================================
bool CRenderSystemNlc::ScissorsCanEffectClipping()
{

    return false;
}

//============================================================================
CRect CRenderSystemNlc::ClipRectToScissorRect( const CRect &rect )
{
    return rect;
}

//============================================================================
void CRenderSystemNlc::SetScissors( const CRect &rect )
{
    if( !m_bRenderCreated )
        return;
}

//============================================================================
void CRenderSystemNlc::ResetScissors()
{
    SetScissors( CRect( 0, 0, ( float )m_width, ( float )m_height ) );
}

#endif // HAVE_NLC_GUI
