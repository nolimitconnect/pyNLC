#pragma once
//============================================================================
// Copyright (C) 2025 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "ListWidgetBase.h"
#include "GuiUserUpdateCallback.h"

class GuiFriendRequest;
class GuiFriendRequestListItem;
class VxNetIdent;

class GuiFriendRequestListWidget : public ListWidgetBase, public GuiUserUpdateCallback
{
	Q_OBJECT

public:
    GuiFriendRequestListWidget( QWidget* parent );
    virtual ~GuiFriendRequestListWidget();

    void                        clearRequestList( void );
    void                        addFriendRequest( GuiFriendRequest* friendRequest );
    void                        removeFriendRequest( VxGUID& requestId );

    GuiFriendRequest*           findFriendRequest( VxGUID& requestId );

    GuiFriendRequestListItem*   findListItemWidgetByRequestId( VxGUID& requestId );
    GuiFriendRequestListItem*   findListItemWidgetByOnlineId( VxGUID& onlineId );

    // user manager update callbacks
    virtual void				callbackUserUpdated( GuiUser* guiUser ) override;

    void                        updateUser( GuiUser* guiUser );

signals:
    void                        signalAcceptButtonClicked( GuiFriendRequest* friendRequest, GuiFriendRequestListItem* hostItem );
    void                        signalDetailsButtonClicked( GuiFriendRequest* friendRequest, GuiFriendRequestListItem* hostItem );
    void                        signalFriendshipButtonClicked( GuiFriendRequest* friendRequest, GuiFriendRequestListItem* hostItem );
    void                        signalRejectButtonClicked( GuiFriendRequest* friendRequest, GuiFriendRequestListItem* hostItem );

protected slots:
    void                        slotAcceptButtonClicked( GuiFriendRequestListItem* hostItem );
    void                        slotDetailsButtonClicked( GuiFriendRequestListItem* hostItem );
    void                        slotFriendshipButtonClicked( GuiFriendRequestListItem* hostItem );
    void                        slotRejectButtonClicked( GuiFriendRequestListItem* hostItem );

protected:
    GuiFriendRequestListItem*   friendRequestToWidget( GuiFriendRequest* friendRequest );
    GuiFriendRequest*           widgetToFriendRequest( GuiFriendRequestListItem* hostItem );

	//=== vars ===//
    VxElapseTimer						m_ClickEventTimer; // avoid duplicate clicks
};

