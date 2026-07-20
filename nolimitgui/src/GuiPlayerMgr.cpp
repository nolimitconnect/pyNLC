//============================================================================
// Copyright (C) 2022 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "GuiPlayerMgr.h"

#include "ActivityBase.h"
#include "AppCommon.h"
#include "AppletPlayerNlc.h"
#include "AppSettings.h"
#include "GuiHelpers.h"
#include "GuiPlayerCallback.h"
#include "GuiVideoTitleBarCallback.h"
#include "HomeWindow.h"

#include <AssetMgr/AssetInfo.h>
#include <MediaProcessor/CamJpgVideo.h>
#include <P2PEngine/P2PEngine.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>

#include <QDesktopServices>
#include <QUrl>
#include <QPainter>

//============================================================================
GuiPlayerMgr::GuiPlayerMgr()
	: QObject()
{
	m_MediaSessionId.initializeWithNewVxGUID();

    QObject::connect(this, &GuiPlayerMgr::signalInternalPlayCamJpg, this, &GuiPlayerMgr::slotInternalPlayCamJpg, Qt::QueuedConnection );
}

//============================================================================
void GuiPlayerMgr::playerMgrStartup( void )
{
	// tell engine to send all video jpeg inputs
	VxGUID nullGuid;
	GetPtoPEngine().fromGuiWantMediaInput( nullGuid, eMediaInputVideoJpg, this, eMediaModuleMediaPlayer, m_MediaSessionId, true );
}

//============================================================================
void GuiPlayerMgr::wantPlayVideoCallbacks( VxGUID& feedOnlineId, GuiPlayerCallback* client, bool enable )
{
	if( m_VideoPlayClientsBusy )
	{
		LogMsg( LOG_ERROR, "GuiPlayerMgr::wantPlayVideoCallbacks do NOT call while busy" );
	}

	for( auto iter = m_VideoPlayClients.begin(); iter != m_VideoPlayClients.end(); ++iter )
	{
		if( iter->second == client &&  iter->first == feedOnlineId )
		{
			m_VideoPlayClients.erase( iter );
			break;
		}
	}

	if( enable )
	{
		m_VideoPlayClients.emplace_back( std::make_pair( feedOnlineId, client ) );
	}
}

//============================================================================
void GuiPlayerMgr::toGuiPlayJpgVideo( VxGUID& vidFeedId, std::shared_ptr<CamJpgVideo>& camJpg )
{
	int inSignal = m_JpgCntInSignal.fetch_add( 1, std::memory_order_relaxed );

	if( inSignal >= 1 )
	{
		m_JpgCntInSignal.fetch_sub( 1, std::memory_order_relaxed );
		if( LogEnabled( eLogWebCam ) )LogModule( eLogWebCam, LOG_ERROR, "GuiPlayerMgr::%s too many in signal/slots tag=%llu src=%d(%s)",
			__func__,
			camJpg ? (unsigned long long)camJpg->m_FrameTag : 0ULL,
			camJpg ? (int)camJpg->m_SourceModule : (int)eMediaModuleInvalid,
			DescribeMediaModule( camJpg ? camJpg->m_SourceModule : eMediaModuleInvalid ) );
		return;
	}

    emit signalInternalPlayCamJpg( vidFeedId, camJpg );
}

//============================================================================
void GuiPlayerMgr::slotInternalPlayCamJpg( VxGUID feedOnlineId, std::shared_ptr<CamJpgVideo> camJpg )
{
	int inSignal = m_JpgCntInSignal.fetch_sub( 1, std::memory_order_relaxed ) - 1;
	if( inSignal < 0 )
	{
		m_JpgCntInSignal.store( 0, std::memory_order_relaxed );
		inSignal = 0;
	}

	if( inSignal )
    {
		if( LogEnabled( eLogWebCam ))LogModule( eLogWebCam, LOG_ERROR, "GuiPlayerMgr::%s now %d in signal/slots", __func__, inSignal );
    }

    QImage vidFrame;
    if( !vidFrame.loadFromData( camJpg->m_VidData.get(), camJpg->m_VidDataLen, "JPG") )
    {
        LogMsg( LOG_WARNING, "GuiPlayerMgr::%s failed to load JPG", __func__ );
        return;
    }

	int motion0To100000 = camJpg->m_Motion;

	vx_assert( !m_VideoPlayClientsBusy );

	m_VideoPlayClientsBusy = true;
	for( auto clientPair : m_VideoPlayClients )
	{
		if( !clientPair.first.isValid() || clientPair.first == feedOnlineId )
		{
			clientPair.second->callbackGuiPlayMotionVideoFrame( feedOnlineId, vidFrame, motion0To100000 );
		}
	}

	m_VideoPlayClientsBusy = false;

	if( m_VideoTitleBarClients.size() && feedOnlineId == GetAppInstance().getMyOnlineId() && m_TitleBarImageSize.width() )
	{
		QPixmap titleBarPixmap = QPixmap::fromImage( vidFrame.scaled( m_TitleBarImageSize ) );
		if( !titleBarPixmap.isNull() )
		{
			for( auto client : m_VideoTitleBarClients )
			{
				client->callbackGuiVideoTitleBarPixmap( titleBarPixmap );
			}
		}
	}
}

//============================================================================
void GuiPlayerMgr::callbackVideoJpg( VxGUID& vidFeedId, std::shared_ptr<CamJpgVideo>& camJpg )
{
	int inSignal = m_JpgCntInSignal.fetch_add( 1, std::memory_order_relaxed );

	if( inSignal >= 1 )
	{
		m_JpgCntInSignal.fetch_sub( 1, std::memory_order_relaxed );
		if( LogEnabled( eLogWebCam ) )LogModule( eLogWebCam, LOG_ERROR, "GuiPlayerMgr::%s too many in signal/slots tag=%llu src=%d(%s)",
			__func__,
			camJpg ? (unsigned long long)camJpg->m_FrameTag : 0ULL,
			camJpg ? (int)camJpg->m_SourceModule : (int)eMediaModuleInvalid,
			DescribeMediaModule( camJpg ? camJpg->m_SourceModule : eMediaModuleInvalid ) );
		return;
	}

	emit signalInternalPlayCamJpg( vidFeedId, camJpg );
}

//============================================================================
void GuiPlayerMgr::wantVideoTitleBarCallbacks( GuiVideoTitleBarCallback* client, bool enable )
{
	for( auto iter = m_VideoTitleBarClients.begin(); iter != m_VideoTitleBarClients.end(); ++iter )
	{
		if( *iter == client )
		{
			m_VideoTitleBarClients.erase( iter );
			break;
		}
	}

	if( enable )
	{
		m_VideoTitleBarClients.emplace_back( client );
	}
}

//============================================================================
bool GuiPlayerMgr::playFile( QString fileNameAndPath, int pos0to100000, bool isStream, bool useExternPlayer, QWidget* launchParent )
{
    if( fileNameAndPath.isEmpty() )
	{
		LogMsg( LOG_WARNING, "GuiPlayerMgr::playFile Empty File Name" );
		return false;
	}

    VxFileInfoBase fileInfo;
    if( !VxFileUtil::getFileInfo( fileNameAndPath.toUtf8().constData(), fileInfo ) )
	{
        LogMsg( LOG_WARNING, "File no longer available %s", fileNameAndPath.toUtf8().constData() );
		return false;
	}

    LogMsg( LOG_VERBOSE, "%s file name %s file %s pos %d", __func__, fileInfo.getFileName().c_str(), fileInfo.getFileNameAndPath().c_str(), pos0to100000 );

    AssetInfo newAsset( fileInfo );
	newAsset.setIsStream( isStream );

	return playMedia( newAsset, useExternPlayer, pos0to100000, launchParent );
}

//============================================================================
bool GuiPlayerMgr::playStream( AssetBaseInfo& assetInfo, VxGUID lclSessionId, int pos0to100000, QWidget* launchParent )
{
	QWidget* parentWidget = launchParent ? launchParent : &GetAppInstance().getHomeWindow();
	// launch the applet that plays this file
	ActivityBase* applet = GetAppInstance().launchApplet( eAppletPlayerNlc, parentWidget, "", assetInfo.getAssetUniqueId() );
	if( applet )
	{
		AppletPlayerNlc* player = dynamic_cast<AppletPlayerNlc*>(applet);
		if( player )
		{
			AssetPlaySession playSession( assetInfo, lclSessionId, pos0to100000 );
			return player->playMedia( playSession, false );
		}		
	}

	return false;
}

//============================================================================
bool GuiPlayerMgr::playMedia( AssetBaseInfo& assetInfo, bool useExternPlayer, int pos0to100000, QWidget* launchParent )
{
	QWidget* parentWidget = launchParent ? launchParent : &GetAppInstance().getHomeWindow();

	if( eAssetTypeExe == assetInfo.getAssetType() )
	{
		QMessageBox::warning( &GetAppInstance().getHomeWindow(), QObject::tr("Attempted to play an executable which is not allowed"), QString(assetInfo.getAssetName().c_str()));
		return false;
	}

	if( eAssetTypeArchives == assetInfo.getAssetType() )
	{
		QMessageBox::warning( &GetAppInstance().getHomeWindow(), QObject::tr( "Attempted to open an archive file which is not allowed" ), QString( assetInfo.getAssetName().c_str() ) );
		return false;
	}

    if( assetInfo.isStream() || !useExternPlayer )
	{
        if( assetInfo.isPhotoAsset() || assetInfo.isStream() || !GetAppInstance().getAppSettings().getUseSystemMediaPlayer() )
		{
			EApplet appletType = GuiHelpers::getAppletThatPlaysFile( GetAppInstance(), assetInfo );
			if( appletType != eAppletUnknown )
			{
				// launch the applet that plays this file
				ActivityBase* applet = GetAppInstance().launchApplet( appletType, parentWidget, "", assetInfo.getAssetUniqueId() );
				if( applet )
				{
					AssetPlaySession assetPlaySession( assetInfo );
					assetPlaySession.setPlayPosition( pos0to100000 );
					return applet->playMedia( assetPlaySession, false );
				}
			}
		}
	}

    if( !assetInfo.isStream() )
	{
// #ifdef TARGET_OS_WINDOWS
// 		ShellExecuteA( 0, 0, assetInfo.getAssetNameAndPath().c_str(), 0, 0, SW_SHOW );
// #else
		QDesktopServices::openUrl( QUrl::fromLocalFile( assetInfo.getAssetNameAndPath().c_str() ) );
// #endif // TARGET_OS_WINDOWS
	}
	return true;
}

