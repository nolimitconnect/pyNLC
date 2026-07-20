/*
 *      Initial code sponsored by: Voddler Inc (voddler.com)
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

#pragma once
#include "config_components_kodi.h"
#include "OverlayRenderer.h"
#include "system_gl.h"

class CDVDOverlay;
class CDVDOverlayImage;
class CDVDOverlaySpu;
class CDVDOverlaySSA;
typedef struct ass_image ASS_Image;

namespace OVERLAY {

    class COverlayTextureNlc : public COverlay
    {
    public:
        explicit COverlayTextureNlc( const CDVDOverlayImage& o, CRect& rSource );
        explicit COverlayTextureNlc( const CDVDOverlaySpu& o );
        ~COverlayTextureNlc() override;

        void Render( SRenderState& state ) override;

    	GLuint m_texture = 0;
        float  m_u;
        float  m_v;
        bool   m_pma; /*< is alpha in texture premultiplied in the values */
    };

    class COverlayGlyphNlc : public COverlay
    {
    public:
        COverlayGlyphNlc( ASS_Image* images, float width, float height );

        ~COverlayGlyphNlc() override;

        void Render( SRenderState& state ) override;

        struct VERTEX
        {
            GLfloat u, v;
            GLubyte r, g, b, a;
            GLfloat x, y, z;
        };

    	std::vector<VERTEX> m_vertex;
    	GLuint m_texture = 0;

        float  m_u;
        float  m_v;
    };

}
