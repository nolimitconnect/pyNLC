#pragma once
//============================================================================
// Copyright (C) 2022 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#if defined(TARGET_OS_LINUX)
#include <QWidget> // must be declared first or linux Qt will error in qmetatype.h 2167:23: array subscript value 53 is outside the bounds
#endif // defined(TARGET_OS_LINUX)

#include <CoreLib/MediaCallbackInterface.h>
#include <CoreLib/VxGUID.h>

#include <QObject>
#include <QSharedPointer>
#include <QSize>

#include <atomic>

class AppCommon;
class AssetBaseInfo;
class CamJpgVideo;
class GuiPlayerCallback;
class GuiVideoTitleBarCallback;
class QImage;

class GuiPlayerMgr : public QObject, public MediaCallbackInterface
{
	Q_OBJECT;
public:
	GuiPlayerMgr();
	GuiPlayerMgr( const GuiPlayerMgr& rhs ) = delete;
	virtual ~GuiPlayerMgr() = default;

	void						playerMgrStartup( void );

	bool						playFile( QString fileNameAndPath, int pos0to100000, bool isStream, bool useExternPlayer, QWidget* launchParent = nullptr );
	bool						playMedia( AssetBaseInfo& assetInfo, bool useExternPlayer, int pos0to100000 = 0, QWidget* launchParent = nullptr );
	bool						playStream( AssetBaseInfo& assetInfo, VxGUID lclSessionId, int pos0to100000 = 0, QWidget* launchParent = nullptr );

	// if feedOnlineId is empty then want all callbacks
	void                        wantPlayVideoCallbacks( VxGUID& feedOnlineId, GuiPlayerCallback* client, bool enable );
	void                        setTitleBarVideoImageSize( QSize imageSize ) { if( imageSize.width() > 4 && imageSize.height() > 4 ) m_TitleBarImageSize = imageSize; }
	void                        wantVideoTitleBarCallbacks( GuiVideoTitleBarCallback* client, bool enable );

    void						callbackVideoJpg( VxGUID& vidFeedId, std::shared_ptr<CamJpgVideo>& camJpg ) override;

	void						toGuiPlayJpgVideo( VxGUID& feedOnlineId, std::shared_ptr<CamJpgVideo>& camJpg );

signals:
    void						signalInternalPlayCamJpg( VxGUID feedOnlineId, std::shared_ptr<CamJpgVideo> camJpg );

protected slots:
    void						slotInternalPlayCamJpg( VxGUID feedOnlineId, std::shared_ptr<CamJpgVideo> camJpg );

protected:
	bool						m_VideoPlayClientsBusy{ false };
	std::vector<std::pair<VxGUID,GuiPlayerCallback*>>  m_VideoPlayClients;
	std::vector<GuiVideoTitleBarCallback*>  m_VideoTitleBarClients;
	QSize						m_TitleBarImageSize;

	QAtomicInt					m_BehindFeedFrameCnt;
	QAtomicInt					m_BehindMotionFrameCnt;
	VxGUID						m_MediaSessionId;

	std::atomic<int>			m_JpgCntInSignal{ 0 };
};
