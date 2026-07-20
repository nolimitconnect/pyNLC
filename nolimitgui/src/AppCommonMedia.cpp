//============================================================================
// Copyright (C) 2009 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppCommon.h"

#include "AppModuleState.h"
#include "AppSettings.h"
#include "GuiPlayerMgr.h"
#include "HomeWindow.h"

#include "ToGuiActivityInterface.h"
#include "ToGuiHardwareControlInterface.h"

#include <AssetBase/AssetPlaySession.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <VxVideoLib/VxVideoLib.h>

#include <QTimer>
#include <QMessageBox>

//============================================================================
bool AppCommon::toGuiMediaAction( EMediaModule mediaModule, EMediaPlayerAction playerAction, int actionVal, const char* fileName )
{
	if( VxIsAppShuttingDown() )
	{
        return false;
	}

	emit signalInternalMediaAction( mediaModule, playerAction, actionVal, fileName );
	return true;
}

//============================================================================
void AppCommon::toGuiMediaError( EMediaModule mediaModule, EMediaError mediaError, const char* msg )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalMediaError( mediaModule, mediaError, msg );
}

//============================================================================
void AppCommon::toGuiSetIsAppModuleRunning( EMediaModule mediaModule, bool isRunning )
{
	m_AppModuleState.toGuiSetIsAppModuleRunning( mediaModule, isRunning );
}

//============================================================================
bool AppCommon::toGuiGetIsAppModuleRunning( EMediaModule mediaModule )
{
	return m_AppModuleState.toGuiGetIsAppModuleRunning( mediaModule );
}

//============================================================================
bool AppCommon::toGuiRunModule( EMediaModule mediaModule )
{
	return m_AppModuleState.toGuiRunModule( mediaModule );
}

//============================================================================
bool AppCommon::toGuiStopModule( EMediaModule mediaModule )
{
    return m_AppModuleState.toGuiStopModule( mediaModule );
}

//============================================================================
void AppCommon::toGuiPlayNlcMedia( AssetBaseInfo* assetInfo )
{
	LogMsg( LOG_INFO, "#### AppCommon::toGuiPlayNlcMedia %s", assetInfo->getAssetName().c_str() );
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalPlayNlcMedia( *assetInfo );
}

//============================================================================
void AppCommon::slotInternalPlayNlcMedia( AssetBaseInfo assetInfo )
{
	AssetPlaySession playSession( assetInfo );
	m_PlayerMgr.playMedia( playSession, false );
}

//============================================================================
void AppCommon::toGuiPlayJpgVideo( VxGUID& feedOnlineId, std::shared_ptr<CamJpgVideo>& jpgVideo )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	m_PlayerMgr.toGuiPlayJpgVideo( feedOnlineId, jpgVideo );
}

//============================================================================
void AppCommon::slotInternalMediaAction( EMediaModule mediaModule, EMediaPlayerAction playerAction, int actionVal, QString fileName )
{
	LogMsg( LOG_VERBOSE, "Media Action %d val %d fileName %s", playerAction, actionVal, fileName.toUtf8().constData() );
}

//============================================================================
void AppCommon::slotInternalMediaError( EMediaModule mediaModule, EMediaError mediaError, QString msg )
{
    static bool isBusy{false};
	LogMsg( LOG_ERROR, "Media Error %d %s", mediaError, msg.toUtf8().constData() );

	if( isBusy )
	{
		// just log instead of show message box if user has not acked the previous message	
		return;
	}

	//isBusy = true;
	//QMessageBox::warning(&getHomeWindow(), QObject::tr("Media Error"), msg);
	//isBusy = false;
}
