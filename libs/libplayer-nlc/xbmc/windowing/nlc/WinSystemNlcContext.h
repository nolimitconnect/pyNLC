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

#pragma once
#include "config_components_kodi.h"
#ifdef HAVE_NLC_GUI

//#include "utils/EGLUtils.h"
#include "rendering/qt/RenderSystemNlc.h"
#include "utils/GlobalsHandling.h"
#include "WinSystemNlc.h"

class CWinSystemNlcContext : public CWinSystemNlc, public CRenderSystemNlc
{
public:
    static void Register();
    static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  CWinSystemNlcContext() = default;
  virtual ~CWinSystemNlcContext() = default;

  // Implementation of CWinSystemBase via CWinSystemNlc
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool InitWindowSystem() override;
  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  virtual std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) override;

  //EGLDisplay GetEGLDisplay() const;
  //EGLSurface GetEGLSurface() const;
  //EGLContext GetEGLContext() const;
  //EGLConfig  GetEGLConfig() const;
protected:
  void SetVSyncImpl(bool enable) override;
  void PresentRenderImpl(bool rendered) override;

private:
//  CEGLContextUtils m_pGLContext;

};

#endif // HAVE_NLC_GUI
