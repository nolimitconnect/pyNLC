#pragma once
//============================================================================
// Copyright (C) 2009 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "IDefs.h"
#include <GuiInterface/IAudioInterface.h>

#include <CoreLib/AppErr.h>
#include <CoreLib/AssetDefs.h>
#include <CoreLib/VxSha1Hash.h>
#include <PktLib/VxCommon.h>

class AssetBaseInfo;
class BlobInfo;
class CamJpgVideo;
class FileInfo;
class GroupieId;
class GroupieInfo;
class HostedInfo;
class OfferBaseInfo;
class OfferClientInfo;
class OfferHostInfo;
class ThumbInfo;
class VxGUID;
class VxNetIdent;

//! IToGui is an abstract interface for calls to GUI from native C++/C code
class IToGui
{
public:
	static IToGui&				getIToGui( void );

	virtual bool                toGuiMediaAction( EMediaModule mediaModule, EMediaPlayerAction playerAction, int actionVal = 0, const char* fileName = "" ) = 0;
	virtual void                toGuiMediaError( EMediaModule mediaModule, EMediaError mediaError, const char* msg ) = 0;

	virtual void                toGuiSetIsAppModuleRunning( EMediaModule mediaModule, bool isRunning ) = 0;
	virtual bool                toGuiGetIsAppModuleRunning( EMediaModule mediaModule ) = 0;

	virtual bool                toGuiRunModule( EMediaModule mediaModule ) = 0;
	virtual bool                toGuiStopModule( EMediaModule mediaModule ) = 0;

    virtual void				toGuiPlayNlcMedia( AssetBaseInfo* assetInfo ) = 0;

	/// Send log message to GUI
	virtual void				toGuiLog( int logFlags, const char* pMsg ) = 0;
	/// Send error occurred message to GUI
	virtual void				toGuiAppErr( EAppErr eAppErr, const char* errMsg = "" ) = 0;
	/// Send error occurred message to GUI (Will popup error msg box)
	virtual void				toGuiAppPopupErr( EAppErr eAppErr, const char* errMsg ) = 0;
	/// Send status bar message to GUI
	virtual void				toGuiStatusMessage( const char* errMsg ) = 0;
	virtual void				toGuiPluginMsg( EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, const char* paramMsg = "" ) = 0;
	virtual void				toGuiPluginCommError( EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, ECommErr commErr ) {};
    /// a module has changed state
    virtual void				toGuiModuleState( EMediaModule moduleNum, EModuleState moduleState ) = 0;

	/// Start/Stop camera capture
	virtual void				toGuiWantVideoCapture( EMediaModule mediaModule, bool wantVidCapture ) = 0;
	/// Send video feed frame to GUI for playback.. includes amount of motion detected
	virtual void				toGuiPlayJpgVideo( VxGUID& onlineId, std::shared_ptr<CamJpgVideo>& jpgVideo ) = 0;

    /// Send host status to GUI for display
    virtual void				toGuiHostAnnounceStatus( EHostType hostType, VxGUID& sessionId, EHostAnnounceStatus joinStatus, const char* msg = "" ) = 0;
    virtual void				toGuiHostJoinStatus( EHostType hostType, VxGUID& sessionId, EHostJoinStatus joinStatus, const char* msg = "" ) = 0;

    virtual void				toGuiHostSearchStatus( EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr = eCommErrNone, const char* msg = "" ) = 0;
    virtual void				toGuiHostSearchResult( EHostType hostType, VxGUID& sessionId, HostedInfo& hostedInfo ) = 0;
	virtual void				toGuiHostSearchComplete( EHostType hostType, VxGUID& sessionId ) = 0;

	virtual void				toGuiGroupieSearchStatus( EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr = eCommErrNone, const char* msg = "" ) = 0;
	virtual void				toGuiGroupieSearchResult( EHostType hostType, VxGUID& sessionId, GroupieInfo& groupieInfo ) = 0;
	virtual void				toGuiGroupieSearchComplete( EHostType hostType, VxGUID& sessionId ) = 0;

    /// Send is port open test state/status to GUI
    virtual void				toGuiIsPortOpenStatus( EIsPortOpenStatus eIsPortOpenStatus, const char* msg = "" ) = 0;
    /// Send Network available status to GUI for display
    virtual void				toGuiNetAvailableStatus( ENetAvailStatus eNetAvailStatus ) = 0;
	/// Send Network state to GUI for display
	virtual void				toGuiNetworkState( ENetworkStateType eNetworkState, const char* stateMsg = "" ) = 0;
	/// Send connect by phone shake status to GUI
	virtual void				toGuiRandomConnectStatus( ERandomConnectStatus eRandomConnectStatus, const char* msg = "" ) = 0;
    /// Send state/status to GUI (currently just query host id)
    virtual void				toGuiRunTestStatus( const char*testName, ERunTestStatus eRunTestStatus, const char* msg = "" ) = 0;

	virtual void				toGuiIndentListUpdate( EUserViewType listType, VxGUID& onlineId, uint64_t timestamp ) = 0;
	virtual void				toGuiIndentListRemove( EUserViewType listType, VxGUID& onlineId ) = 0;

    /// contact added to engine
    virtual void				toGuiContactAdded( VxNetIdent* netIdent ) = 0;
    /// contact removed from engine
    virtual void				toGuiContactRemoved( VxGUID& onlineId ) = 0;

	/// Update contact to online state GUI
	virtual void				toGuiContactOnline( VxNetIdent* netIdent ) = 0;

	/// Notify GUI that contact info of any type changed
	virtual void				toGuiContactAnythingChange( VxNetIdent* netIdent ) = 0;

	/// Notify GUI that the last time contact was in session with you changed
	virtual void				toGuiContactLastSessionTimeChange( VxNetIdent* netIdent ) = 0;

	//! Called from engine when need to update my online identity
	virtual void				toGuiUpdateMyIdent( VxNetIdent* netIdent ) = 0;
	//! called from engine when identity changes need saved for next bootup
	virtual void				toGuiSaveMyIdent( VxNetIdent* netIdent ) = 0;

	//! Contact has sent a offer
	virtual void				toGuiRxedPluginOffer( VxGUID onlineId, OfferBaseInfo& offerInfo ) = 0;
	//! Contact has responded to offer
	virtual void				toGuiRxedOfferReply( VxGUID onlineId, OfferBaseInfo& offerInfo ) = 0;
	//! Plugin session has started
	virtual void				toGuiPluginSessionStarted( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId ) = 0;
	//! Plugin session has stopped
	virtual void				toGuiPluginSessionEnded( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId ) = 0;
	//! Plugin session status or variable value
	virtual void				toGuiPluginStatus(	EPluginType		pluginType,
													int				statusType,
													int				statusValue ) = 0;
	//! Received text message from contact
	virtual void				toGuiInstMsg( VxGUID& onlineId, EPluginType	pluginType, const char* pMsg ) = 0;

	//! Send list of contacts shared files to GUI
	virtual void				toGuiFileListReply( VxGUID& onlineId, EPluginType pluginType, FileInfo& fileInfo ) = 0;
	//! Send list of files to GUI.. used to send directory listing or shared files or files that are in library
	virtual void				toGuiFileList( VxGUID& appInstId, FileInfo& fileInfo ) = 0;
	virtual void				toGuiFileListCompleted( VxGUID& appInstId ) = 0;
	//! Send scan of folder media files to GUI
	virtual void				toGuiFolderScan( VxGUID& appInstId, FileInfo& fileInfo ) = 0;
	virtual void				toGuiFolderScanCompleted( VxGUID& appInstId, bool wasCanceled ) = 0;
	//! Upload a file started
	virtual void				toGuiFileUploadStart( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId, FileInfo& fileInfo ) = 0;
	/// Upload a file completed
	virtual void				toGuiFileUploadComplete( EPluginType pluginType, VxGUID& lclSessionId, std::string& fileName, EXferError xferError ) = 0;
	/// Download a file started
	virtual void				toGuiFileDownloadStart( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId, FileInfo& fileInfo ) = 0;
	/// Download a file completed
	virtual void				toGuiFileDownloadComplete( EPluginType pluginType, VxGUID& lclSessionId, std::string& fileName, EXferError xferError ) = 0;
	/// File transfer progress and/or state
	virtual void				toGuiFileXferState( EPluginType pluginType, VxGUID& lclSessionId, EXferDirection xferDir, EXferState xferState, EXferError xferErr, int param1 ) = 0;
	/// File has been deleted from storage
	virtual void				toGuiFileDeleted( std::string& fileName ) = 0;


	/// Video file or audio file or emote icon or text message has been added to Text Chat session
    virtual void				toGuiAssetAdded( AssetBaseInfo* assetInfo ) = 0;
	virtual void				toGuiAssetUpdated( AssetBaseInfo* assetInfo ) = 0;
	virtual void				toGuiAssetRemoved( AssetBaseInfo* assetInfo ) = 0;

	virtual void				toGuiAssetXferState( VxGUID& assetUniqueId, EAssetSendState assetSendState, int param ) = 0;

	/// Video file or audio file or emote icon or text message asset info in result of request to get Text Chat session assets
	virtual void				toGuiAssetSessionHistory( AssetBaseInfo* assetInfo ) = 0;
	/// Asset state has changed ( like transfer has failed )
	virtual void				toGuiAssetAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 ) = 0;
	/// Text Chat session action like video chat session requested
	virtual void				toGuiMultiSessionAction( EMSessionAction mSessionAction, VxGUID onlineId, int pos0to100000 ) = 0;

    //=== to gui host list ===//
    virtual void				toGuiBlobAdded( BlobInfo * assetInfo ) = 0;
    virtual void				toGuiBlobAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 ) = 0;
    virtual void				toGuiBlobSessionHistory( BlobInfo * assetInfo ) = 0;

	/// Game action ( Used by Truth Or Dare video chat game )
	virtual void				toGuiTodGameAction( EPluginType	pluginType, VxGUID& onlineId, ETodGameAction todGameAction ) = 0;

	///============================================================================
	/// Scan network for user shared resources
	///============================================================================

	/// Search has found a matching file
	virtual void				toGuiSearchResultFileSearch( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId, FileInfo& fileInfo ) = 0;

	virtual void				toGuiNetworkIsTested( bool requiresRelay, std::string& ipAddr, uint16_t ipPort ) = 0;

	virtual void				toGuiAdminAvail( GroupieId& adminGroupieId, bool adminAvail ) = 0;

	///============================================================================
	/// Additional gui updates
	///============================================================================
    
    virtual void                toGuiUpdateWantMicrophoneCount( int wantMicCnt ) = 0;
    virtual void                toGuiUpdateWantSpeakerCount( int wantSpeakerCnt ) = 0;
};
