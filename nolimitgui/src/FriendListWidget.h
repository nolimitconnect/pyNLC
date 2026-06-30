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

#include "FriendList.h"

#include <CoreLib/AssetDefs.h>
#include <CoreLib/VxElapseTimer.h>

#include <QListWidget>

class FriendListEntryWidget;
class GuiUser;
class AppCommon;
class MyIcons;
class P2PEngine;

class FriendListWidget : public QListWidget
{
	Q_OBJECT

public:
	FriendListWidget( QWidget* parent );

	AppCommon&					getMyApp( void ) { return m_MyApp; }
	MyIcons&					getMyIcons( void );

	void						setFriendViewType( EFriendViewType eWhichFriendsToShow );
	EFriendViewType				getFriendViewType( void );

	void						setFriendHasUnviewedTextMessages( VxGUID& onlineId, bool hasTextMsgs );
	//! update friend in list
	void						updateFriend( GuiUser* guiUser, bool sessionTimeChange = false );
	bool						isUserInList( GuiUser* guiUser );

	void						removeFriend( GuiUser* guiUser );
	void						removeFriend( VxGUID& onlineId );

	void						refreshFriendList( EFriendViewType eWhichFriendsToShow );

	int							updateHostServerMembers( EHostType hostType );

signals:
	void						signalUpdateFriend( GuiUser* guiUser, bool sessionTimeChange );
	void						signalFriendClicked( GuiUser* guiUser );
	void						signalRefreshFriendList( EFriendViewType eWhichFriendsToShow );

private slots:
	void						slotUpdateFriend( GuiUser* guiUser, bool sessionTimeChange );
	void						slotRefreshFriend( VxGUID friendId );
	void						slotAssetViewMsgAction( EAssetAction eAssetAction, VxGUID onlineId, int pos );
	void						slotItemClicked(QListWidgetItem*);
	void						slotRefreshFriendList( EFriendViewType eWhichFriendsToShow );
	void						slotFriendListItemClicked( FriendListEntryWidget* widget );
	void						slotFriendMenuButtonClicked( FriendListEntryWidget* widget );

protected:
	//!	fill friend into new QListWidgetItem*
	FriendListEntryWidget *		friendToWidget( GuiUser* poFriend );
	//!	get friend from QListWidgetItem data
    GuiUser*					widgetToFriend( FriendListEntryWidget * item );

	void						updateListEntryWidget( FriendListEntryWidget * item, GuiUser* guiUser );

	FriendListEntryWidget *		findListEntryWidget( GuiUser* guiUser );

	//=== vars ===//
	AppCommon&					m_MyApp;
	P2PEngine&					m_Engine;
	EFriendViewType				m_eFriendViewType;
	GuiUser*					m_SelectedFriend{ nullptr };
	VxElapseTimer						m_ClickEventTimer; // avoid duplicate clicks
	VxGUID						m_ViewingOnlineId;
	bool						m_IsCurrentlyViewing;
};

