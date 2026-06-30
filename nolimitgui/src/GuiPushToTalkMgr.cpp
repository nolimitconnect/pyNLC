//============================================================================
// Copyright (C) 2024 Brett R. Jones
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

#include "GuiPushToTalkMgr.h"

#include "AppCommon.h"
#include "GuiUserMgr.h"

#include "GuiAudioMgr.h"
#include "SoundFxMgr.h"

#include <PushToTalk/PushToTalkMgr.h>

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

//============================================================================
GuiPushToTalkMgr::GuiPushToTalkMgr()
	: QObject()
{
}

//============================================================================
void GuiPushToTalkMgr::onSystemReady( void )
{
	connect( this, SIGNAL(signalInternalPushToTalkStatus(VxGUID,EPushToTalkStatus) ), this, SLOT(slotInternalPushToTalkStatus(VxGUID,EPushToTalkStatus)), Qt::QueuedConnection );
	GetAppInstance().getEngine().getPushToTalkMgr().wantPushToTalkCallbacks(this, true);
	GetAppInstance().getUserMgr().wantGuiUserUpdateCallbacks( this, true );
}

//============================================================================
void GuiPushToTalkMgr::callbackPushToTalkStatus( VxGUID& onlineId, enum EPushToTalkStatus pushToTalkStatus )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	emit signalInternalPushToTalkStatus( onlineId, pushToTalkStatus );
}

//============================================================================
void GuiPushToTalkMgr::slotInternalPushToTalkStatus( VxGUID onlineId, EPushToTalkStatus pushToTalkStatus )
{
	if( VxIsAppShuttingDown() )
	{
		return;
	}

	setPushToTalkStatus( onlineId, pushToTalkStatus );

	for( auto& client : m_PushToTalkClients )
	{
		client->callbackPushToTalkStatus( onlineId, pushToTalkStatus );
	}
}

//========================================================================
void GuiPushToTalkMgr::callbackOnlineStatusChange( GuiUser* guiUser, bool isOnline )
{
	if( !isOnline )
	{
		VxGUID onlineId = guiUser->getMyOnlineId();
		setUserOffline( onlineId );
	}
}

//========================================================================
void GuiPushToTalkMgr::setUserOffline( VxGUID& onlineId )
{
	auto iter = m_PushToTalkStatusMap.find( onlineId );
	if( iter != m_PushToTalkStatusMap.end() )
	{
		iter->second = ePushToTalStatusNoConnection;
	}
}

//========================================================================
void GuiPushToTalkMgr::wantGuiPushToTalkCallbacks( GuiPushToTalkCallback* clientInterface, bool wantCallbacks )
{
	for( auto iter = m_PushToTalkClients.begin(); iter != m_PushToTalkClients.end(); ++iter )
	{
		GuiPushToTalkCallback* offerInterface = (*iter);
		if( offerInterface == clientInterface )
		{
			if( wantCallbacks )
			{
				// already in list
				return;
			}
			else
			{
				// remove from list
				m_PushToTalkClients.erase( iter );
				return;
			}
		}
	}

	if( wantCallbacks )
	{
		m_PushToTalkClients.emplace_back( clientInterface );
	}
}

//============================================================================
void GuiPushToTalkMgr::setPushToTalkStatus( VxGUID& onlineId, EPushToTalkStatus pushToTalkStatus )
{
    if( onlineId.isValid() )
    {
        m_PushToTalkStatusMap[onlineId] = pushToTalkStatus;
    }
}

//============================================================================
EPushToTalkStatus GuiPushToTalkMgr::getPushToTalkStatus( VxGUID& onlineId )
{
	EPushToTalkStatus pushToTalkStatus{ ePushToTalkStatusNotActive };
	auto iter = m_PushToTalkStatusMap.find( onlineId );
	if( iter != m_PushToTalkStatusMap.end() )
	{
		pushToTalkStatus = iter->second;
	}

	return pushToTalkStatus;
};

//============================================================================
void GuiPushToTalkMgr::togglePushToTalk( VxGUID& onlineId )
{
	if( onlineId.isValid() )
	{
		if( GetAppInstance().getUserMgr().isUserOnline( onlineId ) )
		{
			EPushToTalkStatus status = getPushToTalkStatus( onlineId );
			if( status == ePushToTalkStatusTxEnabled || status == ePushToTalkStatusDuplexEnabled )
			{
				if( !GetAppInstance().getFromGuiInterface().fromGuiPushToTalk( onlineId, false ) )
				{
					GetAppInstance().getSoundFxMgr().playSnd( eSndDefBusy );
				}
			}
			else
			{
				if( !GetAppInstance().getFromGuiInterface().fromGuiPushToTalk( onlineId, true ) )
				{
					GetAppInstance().getSoundFxMgr().playSnd( eSndDefBusy );
				}
			}
		}
		else
		{
			setUserOffline( onlineId );
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "GuiPushToTalkMgr::%s invalid online id", __func__ );
	}
}
