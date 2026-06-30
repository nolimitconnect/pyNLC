#pragma once
//============================================================================
// Copyright (C) 2016 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <QWidget> // must be declared first or linux Qt will error in qmetatype.h 2167:23: array subscript value 53 is outside the bounds

#include "GuiUserUpdateCallback.h"
#include "GuiOfferSession.h"
#include "PhoneRinger.h"

#include <OfferBase/OfferCallback.h>

#include <CoreLib/VxMutex.h>
#include <CoreLib/VxElapseTimer.h>

class GuiOfferCallback;
class GuiOfferSession;
class GuiOfferSession;
class GuiUser;
class QListWidgetItem;
class QTimer;
class VxGUID;

class GuiOfferMgrBase : public QWidget, public GuiUserUpdateCallback, public OfferCallback
{
	Q_OBJECT

	const int MAX_OFFER_HISTORY_ENTRIES = 20;
public:
	GuiOfferMgrBase( AppCommon& myApp );

    virtual std::vector<std::shared_ptr<GuiOfferSession>>& getOfferList( void )			{ return m_OfferList; }
	virtual std::vector<std::shared_ptr<GuiOfferSession>>& getOfferHistoryList( void )	{ return m_OfferHistory; }

	virtual void                onAppCommonCreated( void ) = 0;
	virtual void                onMessengerReady( bool ready )				{ }
	virtual bool                isMessengerReady( void );
	virtual void                onSystemReady( bool ready ) { }

	virtual int					getActiveOfferCount( void )					{ return (int)m_LastActiveOfferCount; }
	virtual int					getHistoryOfferCount( void )				{ return (int)m_OfferHistory.size(); }

	virtual std::shared_ptr<GuiOfferSession>	getTopGuiOfferSession( void ); // returns null if no session offers

	virtual void                toGuiRxedPluginOffer( VxGUID& onlineId, OfferBaseInfo& offerInfo );
	virtual void                toGuiRxedOfferReply( VxGUID& onlineId, OfferBaseInfo& offerInfo );
	virtual void                toGuiRxedOfferUpdated( OfferBaseInfo* offerInfo );

	virtual void				toGuiPluginSessionStarted( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId );
	virtual void				toGuiPluginSessionEnded( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId );

	virtual bool				fromGuiMakePluginOffer( QWidget* parent, EPluginType pluginType, GuiUser* guiUser, FileInfo& fileInfo );
	virtual bool				fromGuiMakePluginOffer( QWidget* parent, EPluginType pluginType, GuiUser* guiUser, OfferBaseInfo& offerInfo );
	virtual bool				fromGuiToPluginOfferReply( EPluginType pluginType, GuiUser* guiUser, OfferBaseInfo& offerInfo, VxGUID& offerSessionId, EOfferResponse offerResponse );

	void						onIsInSession( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser, bool isInSession );
	void						onSessionExit( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser );
	void						startedSessionInReply( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser );

	void						acceptOfferButtonClicked( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser );
	void						rejectOfferButtonClicked( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser );

	// called if starting new session to know if responding to existing offer
	std::shared_ptr<GuiOfferSession>	findActiveAndAvailableOffer( GuiUser* guiUser, EPluginType pluginType );
	void						sentOffer( std::shared_ptr<GuiOfferSession>& offerSession );
	void						sentOfferReply( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser, EOfferResponse offerResponse );
	void						removePluginSessionOffer( EPluginType pluginType, GuiUser* guiUser );
	void						removePluginSessionOffer( VxGUID& offerSessionId );

	virtual void				showOfferInfo( GuiOfferSession* offerSessionIn, QWidget* contentFrame );
	virtual void				viewOffer( GuiOfferSession* offerSession, QWidget* contentFrame );
	virtual bool				acceptOffer( GuiOfferSession* offerSession, QWidget* contentFrame );
	virtual bool				rejectOffer( GuiOfferSession* offerSession, QWidget* contentFrame );

	std::shared_ptr<GuiOfferSession> findOffer( GuiOfferSession* offerSession );

	virtual bool				validateOffer( std::shared_ptr<GuiOfferSession>& offerSession, QWidget* contentFrame, bool showErrorMsg = true );

	void						phoneRingTimeout( std::shared_ptr<GuiOfferSession>& offerSession );

	void						updateRxedOffer( GuiUser* guiUser, std::shared_ptr<GuiOfferSession>& offerSession, OfferBaseInfo& offerInfo );

	bool						haveActiveOffer( VxGUID& onlineId, EPluginType pluginType );

signals:
	void						signalCallbackFileWasShredded( QString fileName );

	void						signalCallbackOfferSendState( VxGUID assetOfferId, EOfferSendState assetSendState, int param );
    void						signalCallbackOfferAction( VxGUID assetOfferId, EOfferAction offerAction, int param );

    void						signalCallbackOfferAdded( OfferBaseInfo* assetInfo );
    void						signalCallbackOfferUpdated( OfferBaseInfo* assetInfo );
    void						signalCallbackOfferRemoved( VxGUID offerId );

protected slots:
	void						slotCallbackFileWasShredded( QString fileName );

	void						slotCallbackOfferSendState( VxGUID assetOfferId, EOfferSendState assetSendState, int param );
    void						slotCallbackOfferAction( VxGUID assetOfferId, EOfferAction offerAction, int param );

    void						slotCallbackOfferAdded( OfferBaseInfo* assetInfo );
    void						slotCallbackOfferUpdated( OfferBaseInfo* assetInfo );
    void						slotCallbackOfferRemoved( VxGUID offerId );

protected:
	virtual void				callbackOnlineStatusChange( GuiUser* guiUser, bool isOnline ) override;

	void						recievedOffer( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser );
	void						recievedOfferReply( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser, EOfferResponse offerResponse );
	void						recievedSessionEnd( EPluginType pluginType, VxGUID offerSessionId, GuiUser* guiUser, EOfferResponse offerResponse );

	void						changeOfferState( std::shared_ptr<GuiOfferSession>& offerSession, EOfferState newOfferState );

	std::shared_ptr<GuiOfferSession>	createOfferSession( GuiUser* guiUser, OfferBaseInfo& offerInfo );
    std::shared_ptr<GuiOfferSession>	findOfferSession( EPluginType pluginType, VxGUID sessionId, GuiUser* guiUser, bool activeOffer, bool historyOffer, bool logNotFound = true );

	void						checkAndUpdateIfEmptyOfferList( void );
	void						updateActiveOfferCount( void );

	// callbacks
	void						connectCallbackSignalsAndSlots( void );

	void						callbackFileWasShredded( std::string& fileName ) override;

	void						callbackOfferSendState( VxGUID& assetOfferId, EOfferSendState assetSendState, int param ) override;
    void						callbackOfferAction( VxGUID& assetOfferId, EOfferAction offerAction, int param ) override;

    void						callbackOfferAdded( OfferBaseInfo* assetInfo ) override;
    void						callbackOfferUpdated( OfferBaseInfo* assetInfo ) override;
    void						callbackOfferRemoved( VxGUID& offerId ) override;

	virtual void				onNewOfferSession( std::shared_ptr<GuiOfferSession> offerSession );

	bool                        launchOfferResponseAccept( GuiOfferSession* offerSession, QWidget* contentFrame );
	bool                        launchOfferResponseAccept( std::shared_ptr<GuiOfferSession>& offerSession, QWidget* contentFrame );

    bool                        sendResponse( GuiUser* guiUser, OfferBaseInfo& offerInfo, EOfferState offerState );

    void                        announceOfferMsg( GuiUser* guiUser, EPluginType pluginType, VxGUID& offerId, std::string& offerMsg );

	void						moveToHistory( VxGUID& offerId );
	void						removeOffer( VxGUID& offerId );

	void						checkMaxHistory( void );

	//=== vars ===//
	AppCommon& 					m_MyApp;

	bool						m_UserIsInSession{ false };
    int							m_LastActiveOfferCount{ 0 };

	std::vector<std::shared_ptr<GuiOfferSession>> m_OfferList;
	std::vector<std::shared_ptr<GuiOfferSession>> m_OfferHistory;

	std::vector<GuiOfferCallback*>	m_OfferCallbackList;

	PhoneRinger					m_PhoneRinger;
};
