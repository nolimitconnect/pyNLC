#pragma once
//============================================================================
// Copyright (C) 2020 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletBase.h"

#include "GuiUserUpdateCallback.h"

#include <CoreLib/MediaCallbackInterface.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class AppletCamSettingsUi;
}
QT_END_NAMESPACE

class ThumbnailViewWidget;
class AssetMgr;
class IVxVidCap;

class AppletCamSettings : public AppletBase, public MediaCallbackInterface, public GuiUserUpdateCallback
{
	Q_OBJECT
public:
	AppletCamSettings( AppCommon& app, QWidget* parent = nullptr );
	virtual ~AppletCamSettings() override;

    void						webCamSourceOffline();
    void                        startCamFeed( void );
    void                        stopCamFeed( void );
    void                        setIsCamFeedStarted( bool isStarted )   { m_CamFeedStarted = isStarted; }
    bool                        isCamFeedStarted( void )                { return m_CamFeedStarted; }
    bool                        isMyself( void )                        { return m_IsMyself; }

signals:
    void						signalPlayAudio( unsigned short * pu16PcmData, unsigned short u16PcmDataLen );
    void						signalSnapshotImage( QImage snapshotImage );

protected slots:
    void                        inDeviceChanged( int index );
    void                        updateInVideoDevices( void );
    void                        slotPollVideoDevices( void );

    void                        slotApplyInDeviceChange( void );

    void						slotToGuiRxedOfferReply( std::shared_ptr<GuiOfferSession> offerSession );
    void						slotToGuiSessionEnded( std::shared_ptr<GuiOfferSession> offerSession );

protected:
    virtual void				showEvent( QShowEvent* ev ) override;
    virtual void				hideEvent( QHideEvent* ev ) override;
    virtual void				closeEvent( QCloseEvent * ev ) override;

    void						setupCamFeed( VxNetIdent* feedNetIdent );
    void						resizeBitmapToFitScreen( QLabel * VideoScreen, QImage& oPicBitmap );

    void                        callbackOnlineStatusChange( GuiUser* guiUser, bool isOnline ) override;

    //=== vars ===//
    Ui::AppletCamSettingsUi&	ui;
    bool						m_IsMyself{ false };
    bool 					    m_CameraSourceAvail{ false };

    QImage	                    m_ImageBitmap;
    VxNetIdent*                 m_CamFeedIdent{ nullptr };
    VxGUID                      m_CamFeedId;
    bool                        m_CamFeedStarted{ false };
    QTimer*                     m_DevicePollTimer{ nullptr };
    int                         m_DevicePollAttempts{ 0 };
};
