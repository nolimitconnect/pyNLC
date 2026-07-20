#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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


#include "windowing/VideoSync.h"
#include "guilib/DispResource.h"

class CVideoSyncNlc : public CVideoSync, IDispResource
{
public:
    CVideoSyncNlc( CVideoReferenceClock* clock ) : CVideoSync( clock ) {}


    // CVideoSync interface
    virtual bool Setup() override;
    virtual void Run( CEvent& stop ) override;
    virtual void Cleanup() override;
    virtual float GetFps() override;

    // IDispResource interface
    virtual void OnResetDisplay() override;

    // Choreographer callback
    void FrameCallback( int64_t frameTimeNanos );

private:
    int64_t m_LastVBlankTime = 0;  //timestamp of the last vblank, used for calculating how many vblanks happened
    CEvent m_abortEvent;
};

#endif // HAVE_NLC_GUI
