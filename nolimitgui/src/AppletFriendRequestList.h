#pragma once
//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletBase.h"

#include <CoreLib/VxElapseTimer.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class AppletFriendRequestListUi;
}
QT_END_NAMESPACE

class GuiFriendRequest;
class GuiFriendRequestListItem;

class AppletFriendRequestList : public AppletBase
{
	Q_OBJECT
public:
	AppletFriendRequestList( AppCommon& app, QWidget* parent = nullptr );
	virtual ~AppletFriendRequestList();



private slots:
    virtual void                slotAcceptButtonClicked( GuiFriendRequest* friendRequest, GuiFriendRequestListItem* hostItem );
    virtual void                slotDetailsButtonClicked( GuiFriendRequest* friendRequest, GuiFriendRequestListItem* hostItem );
    virtual void                slotRejectButtonClicked( GuiFriendRequest* friendRequest, GuiFriendRequestListItem* hostItem );

protected:
    void						showEvent( QShowEvent* ev ) override;
    void						hideEvent( QHideEvent* ev ) override;

    void                        updateRequestList( void );

    //=== vars ===//
    Ui::AppletFriendRequestListUi& ui;
};
