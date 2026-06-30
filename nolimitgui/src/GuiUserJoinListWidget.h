#pragma once
//============================================================================
// Copyright (C) 2021 Brett R. Jones 
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

#include "GuiThumbCallback.h"
#include "GuiUserUpdateCallback.h"
#include <GuiInterface/IDefs.h>

#include <CoreLib/VxElapseTimer.h>
#include <CoreLib/GroupieId.h>

#include <QListWidget>

enum EUserJoinViewType
{
    eUserJoinViewTypeNone,
    eUserJoinViewTypeFriends,
    eUserJoinViewTypeGroup,
    eUserJoinViewTypeChatRoom,
    eUserJoinViewTypeRandomConnect,
    eUserJoinViewTypeEverybody,
    eUserJoinViewTypeIgnored,

    eMaxUserJoinViewTypeNone,
};

class GuiUserJoinListItem;
class GuiUserJoin;
class AppCommon;
class MyIcons;
class P2PEngine;
class GuiUser;
class GuiUserJoinSession;
class GuiUserJoinMgr;
class GuiThumb;
class GuiThumbMgr;

class GuiUserJoinListWidget : public QListWidget, public GuiUserUpdateCallback, public GuiThumbCallback
{
	Q_OBJECT

public:
	GuiUserJoinListWidget( QWidget* parent );
    virtual ~GuiUserJoinListWidget();

	AppCommon&					getMyApp( void ) { return m_MyApp; }

    void                        clearUserJoinList( void );

    GuiUserJoinListItem*        addOrUpdateUserJoinSession( GuiUserJoinSession* hostSession );

    GuiUserJoinListItem*        findListEntryWidgetBySessionId( VxGUID& sessionId );
    GuiUserJoinListItem*        findListEntryWidgetByGroupieId( GroupieId& groupieId );

    void                        updateUserJoin( GuiUserJoin* guiUserJoin );
    void                        removeUserJoin( GroupieId& groupieId );

    void                        setUserJoinViewType( EUserJoinViewType viewType );
    EUserJoinViewType           getUserJoinViewType( void )                     { return m_UserJoinViewType; }

    void                        updateUser( GuiUser* guiUser );

signals:
    void                        signalAvatarButtonClicked( GuiUserJoinSession* hostSession, GuiUserJoinListItem* hostItem );
    void                        signalMenuButtonClicked( GuiUserJoinSession* hostSession, GuiUserJoinListItem* hostItem );

protected slots:
    void				        slotUserJoinAdded( GuiUserJoin* guiUserJoin );
    void				        slotUserJoinRemoved( GroupieId& groupieId );
    void                        slotUserJoinUpdated( GuiUserJoin* guiUserJoin );
    void                        slotUserJoinOnlineStatus( GuiUserJoin* guiUserJoin, bool isOnline );

    void                        slotAvatarButtonClicked( GuiUserJoinListItem* hostItem );
    void                        slotMenuButtonClicked( GuiUserJoinListItem* hostItem );

protected:
    void				        callbackThumbAdded( GuiThumb* guiThumb ) override;
    void                        callbackThumbUpdated( GuiThumb* guiThumb ) override;
    void				        callbackThumbRemoved( VxGUID& thumbId ) override;

    GuiUserJoinListItem*        sessionToWidget( GuiUserJoinSession* hostSession );

    virtual void                onListItemAdded( GuiUserJoinSession* userSession, GuiUserJoinListItem* userItem );
    virtual void                onListItemUpdated( GuiUserJoinSession* userSession, GuiUserJoinListItem* userItem );

    virtual void                onAvatarButtonClicked( GuiUserJoinListItem* hostItem );
    virtual void                onMenuButtonClicked( GuiUserJoinListItem* hostItem );

    void                        refreshList( void );
    bool                        isListViewMatch( GuiUser* guiUser );

    void                        updateThumb( GuiThumb* guiThumb );
 
	//=== vars ===//
	AppCommon&					m_MyApp;
    P2PEngine&					m_Engine;
    GuiUserJoinMgr&				m_UserJoinMgr;
    GuiThumbMgr&				m_ThumbMgr;
	VxElapseTimer						m_ClickEventTimer; // avoid duplicate clicks
    std::map<GroupieId, GuiUserJoinSession*> m_UserJoinCache;

    EUserJoinViewType           m_UserJoinViewType{ eUserJoinViewTypeNone };
};

