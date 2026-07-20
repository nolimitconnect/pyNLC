/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "config_components_kodi.h"
//#if defined(TARGET_OS_WINDOWS)
//# include <libglew/include/GL/glew.h>
//#endif // defined(TARGET_OS_WINDOWS)

#include "FrameBufferObject.h"
#include "ServiceBroker.h"
#include "rendering/RenderSystem.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#ifdef HAVE_NLC_GUI
#include <GuiInterface/INlcRender.h>
#endif // HAVE_NLC_GUI

 //////////////////////////////////////////////////////////////////////
 // CFrameBufferObject
 //////////////////////////////////////////////////////////////////////

CFrameBufferObject::CFrameBufferObject()
{
    m_fbo = 0;
    m_valid = false;
    m_supported = false;
    m_bound = false;
    m_texid = 0;
}

bool CFrameBufferObject::IsSupported()
{
    if( CServiceBroker::GetRenderSystem()->IsExtSupported( "GL_EXT_framebuffer_object" ) )
        m_supported = true;
    else
        m_supported = false;
    return m_supported;
}

bool CFrameBufferObject::Initialize()
{
    if( !IsSupported() )
        return false;

    Cleanup();
#ifdef HAVE_NLC_GUI
    INlcRender::getINlcRender().frameBufferGen( 1, &m_fbo );
#else
    glGenFramebuffers( 1, &m_fbo );
#endif // HAVE_NLC_GUI

    VerifyGLState();

    if( !m_fbo )
        return false;

    m_valid = true;
    return true;
}

void CFrameBufferObject::Cleanup()
{
    if( !IsValid() )
        return;

    if( m_fbo )
    {
#ifdef HAVE_NLC_GUI
        INlcRender::getINlcRender().frameBufferDelete( 1, &m_fbo );
#else
        glDeleteFramebuffers( 1, &m_fbo );
#endif // HAVE_NLC_GUI
    }

    if( m_texid )
        glDeleteTextures( 1, &m_texid );

    m_texid = 0;
    m_fbo = 0;
    m_valid = false;
    m_bound = false;
}

bool CFrameBufferObject::CreateAndBindToTexture( GLenum target, int width, int height, GLenum format, GLenum type,
                                                 GLenum filter, GLenum clampmode )
{
    if( !IsValid() )
        return false;

    INlcRender& nlcRender = INlcRender::getINlcRender();

    if( m_texid )
        nlcRender.glFuncDeleteTextures( 1, &m_texid );

    m_bound = false;

    nlcRender.glFuncGenTextures( 1, &m_texid );
    nlcRender.glFuncBindTexture( target, m_texid );
    nlcRender.glFuncTexImage2D( target, 0, format, width, height, 0, GL_RGBA, type, NULL );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_WRAP_S, clampmode );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_WRAP_T, clampmode );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_MAG_FILTER, filter );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_MIN_FILTER, filter );
    VerifyGLState();

#ifdef HAVE_NLC_GUI
    m_bound = false;

    nlcRender.glFuncGenTextures( 1, &m_texid );
    nlcRender.glFuncBindTexture( target, m_texid );
    nlcRender.glFuncTexImage2D( target, 0, format, width, height, 0, GL_RGBA, type, NULL );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_WRAP_S, clampmode );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_WRAP_T, clampmode );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_MAG_FILTER, filter );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_MIN_FILTER, filter );

    nlcRender.frameBufferBind( m_fbo );
    nlcRender.glFuncBindTexture( target, m_texid );
    nlcRender.frameBufferTexture2D( target, m_texid );
    VerifyGLState();
    bool status = nlcRender.frameBufferStatus( );
    nlcRender.frameBufferBind( 0 );
    if( !status )
    {
        VerifyGLState();
        return false;
    }
#else
    m_bound = false;

    glGenTextures( 1, &m_texid );
    glBindTexture( target, m_texid );
    glTexImage2D( target, 0, format, width, height, 0, GL_RGBA, type, NULL );
    glTexParameteri( target, GL_TEXTURE_WRAP_S, clampmode );
    glTexParameteri( target, GL_TEXTURE_WRAP_T, clampmode );
    glTexParameteri( target, GL_TEXTURE_MAG_FILTER, filter );
    glTexParameteri( target, GL_TEXTURE_MIN_FILTER, filter );

    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
    glBindTexture( target, m_texid );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_texid, 0 );
    VerifyGLState();
    GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    if( status != GL_FRAMEBUFFER_COMPLETE )
    {
        VerifyGLState();
        return false;
    }
#endif // HAVE_NLC_GUI

    m_bound = true;
    return true;
}

void CFrameBufferObject::SetFiltering( GLenum target, GLenum mode )
{
    INlcRender& nlcRender = INlcRender::getINlcRender();
    nlcRender.glFuncBindTexture( target, m_texid );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_MAG_FILTER, mode );
    nlcRender.glFuncTexParameteri( target, GL_TEXTURE_MIN_FILTER, mode );
}

// Begin rendering to FBO
bool CFrameBufferObject::BeginRender()
{
    if( IsValid() && IsBound() )
    {
#ifdef HAVE_NLC_GUI
        INlcRender::getINlcRender().frameBufferBind( m_fbo );
#else
       glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
#endif // HAVE_NLC_GUI
        return true;
    }
    return false;
}

// Finish rendering to FBO
void CFrameBufferObject::EndRender() const
{
    if( IsValid() )
    {
#ifdef HAVE_NLC_GUI
        INlcRender::getINlcRender().frameBufferBind( 0 );
#else
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
#endif // HAVE_NLC_GUI
    }
}
