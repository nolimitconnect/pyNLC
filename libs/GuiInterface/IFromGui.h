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

#include "IFromGuiDefs.h"

#include <HostListMgr/HostedInfo.h>

#include <NetLib/NetSettings.h>

#include <PktLib/VxCommon.h>

#include <CoreLib/VxKeyDefs.h>
#include <CoreLib/VxFileTypeMasks.h>
#include <CoreLib/VxGUID.h>
#include <CoreLib/AssetDefs.h>
#include <CoreLib/MediaCallbackInterface.h>

class AssetBaseInfo;
class FileShareSettings;
class InetAddress;
class VxSha1Hash;

class FileInfo;
class FriendRequestInfo;
class GroupieInfo;
class OfferBaseInfo;
class SearchParams;
class VxPtopUrl;

/// IFromGui is an abstract interface for from GUI to P2PEngine calls
class IFromGui
{
public:
    static IFromGui&			getIFromGui( void );

	/// First call to engine should send path to assets ( game and app resources ) and path to root of where to write application data
	virtual void				fromGuiAppStartup( std::string assetsDir, std::string rootDataDir, bool fromThread = false ) = 0;
	/// Second call to engine should send path where to write login name specific application data 
	virtual void				fromGuiSetUserSpecificDir( std::string userSpecificDir, bool fromThread = false ) = 0;
	/// Third call to engine should send path where to put downloads from other users
	virtual void				fromGuiSetUserXferDir( std::string userDownloadDir, bool fromThread = false ) = 0;
	/// Called with identity of user that logged on
	virtual void				fromGuiUserLoggedOn( VxNetIdent* netIdent, bool fromThread = false ) = 0;

	/// Call to engine when application is about to exit
	virtual void				fromGuiAppShutdown( void ) = 0;

	/// delete user from database.. change does not take effect until next reboot
	virtual bool				fromGuiDeleteUser( VxGUID& onlineId ) = 0;

	/// Returns disk space available in incomplete downloads directory
	virtual uint64_t			fromGuiGetDiskFreeSpace( const char* dir = nullptr ) = 0; 
	/// deletes cached files and returns amount of disk space deleted
	virtual uint64_t			fromGuiClearCache( ECacheType cacheType ) = 0;

	/// Axis orientation of device changed 
	virtual bool				fromGuiOrientationEvent( float f32RotX, float f32RotY, float f32RotZ ) = 0;
	/// Mouse or touch event
	virtual bool				fromGuiMouseEvent( EMouseButtonType eMouseButType, EMouseEventType eMouseEventType, int iMouseXPos, int iMouseYPos  ) = 0;
	/// Mouse wheel event
	virtual bool				fromGuiMouseWheel( float f32MouseWheelDist ) = 0;
	/// Key was pressed event
	virtual bool				fromGuiKeyEvent( EKeyEventType eKeyEventType, EKeyCode eKey, int iFlags = 0 ) = 0;

	/// Not currently used ( for opengl games )
	virtual void				fromGuiNativeGlInit( void ) = 0;
	/// Not currently used ( for opengl games )
	virtual void				fromGuiNativeGlResize( int w, int h ) = 0;
	/// Not currently used ( for opengl games )
	virtual int					fromGuiNativeGlRender( void ) = 0;
	/// Not currently used ( for opengl games )
	virtual void				fromGuiNativeGlPauseRender( void ) = 0;
	/// Not currently used ( for opengl games )
	virtual void				fromGuiNativeGlResumeRender( void ) = 0;
	/// Not currently used ( for opengl games )
	virtual void				fromGuiNativeGlDestroy( void ) = 0;

	/// Record audio control
	virtual bool				fromGuiSndRecord( ESndRecordState eRecState, VxGUID& feedId, const char* fileName ) = 0;
	/// Record video control
	virtual bool				fromGuiVideoRecord( EVideoRecordState eRecState, VxGUID& feedId, const char* fileName  ) = 0;

    //! play video or audio file
	virtual bool				fromGuiPlayLocalMedia( const char*  fileName, const char* fileNameAndPath, uint64_t fileLen, uint8_t fileType, int pos ) = 0;

	//! play video or audio file
	virtual bool				fromGuiPlayLocalMedia( const char* fileName, const char* fileNameAndPath, uint64_t fileLen, uint8_t fileType, VxGUID assetId, int pos ) = 0;

	/// Add/Remove callback from MediaProcessor when given media type is processed and available
	virtual void				fromGuiWantMediaInput( VxGUID& onlineId, EMediaInputType mediaType, MediaCallbackInterface * callback, EMediaModule mediaModule, VxGUID& mediaSessionId, bool wantInput ) = 0;
	/// Add/Remove callback from MediaProcessor when given media type is processed and available from specific user
	virtual void				fromGuiWantMediaInput( VxGUID& onlineId, EMediaInputType mediaType, EMediaModule mediaModule, VxGUID& mediaSessionId, bool wantInput ) = 0;

	/// Called when user changes his/her online name
	virtual void				fromGuiOnlineNameChanged( const char* newOnlineName ) = 0;
	/// Called when user changes his/her mood message
	virtual void				fromGuiMoodMessageChanged( const char* newMoodMessage ) = 0;
    /// Called when user changes his/her personal info
    virtual void				fromGuiIdentPersonalInfoChanged( int age, int gender, int language, int preferredContent ) = 0;

	/// Called when user changes his/her About Me web page picture.. sets flag that user has About Me Picture for scanning
	virtual void				fromGuiSetUserHasProfilePicture( bool haveAboutMePicture ) = 0;
	/// Called when user changes permission level of plugin
	virtual void				fromGuiUpdateMyIdent( VxNetIdent* netIdent, bool permissionAndStatsOnly = false ) = 0;
	/// Can be called to get users identity from engine.. usually for current node url or network info
	virtual void				fromGuiQueryMyIdent( VxNetIdent* poRetIdent ) = 0;
	/// Can tell engine to set has offers flag to be restored if application is restarted
	virtual void				fromGuiSetIdentHasTextOffers( VxGUID& onlineId, bool hasTextOffers ) = 0;
	/// Change permission level granted to a contact 
	virtual bool				fromGuiChangeMyFriendshipToHim(	VxGUID&			onlineId, 
																EFriendState	eMyFriendshipToHim,
																EFriendState	eHisFriendshipToMe ) = 0;
    /// Set network settings
    virtual void				fromGuiApplyNetHostSettings( NetHostSetting& netSettings ) = 0;
	/// Set network settings
	virtual void				fromGuiSetNetSettings( NetSettings& netSettings ) = 0;
	/// Get network settings
	virtual void				fromGuiGetNetSettings( NetSettings& netSettings ) = 0;

	/// Set number of users that can use device as relay
	virtual void				fromGuiSetRelaySettings( int userRelayMaxCnt, int systemRelayMaxCnt ) = 0;

	/// Run test to see if TCP port is open and what the external IP Address is
	virtual void				fromGuiRunIsPortOpenTest( uint16_t port ) = 0;
    /// Run test on the given url
    virtual void				fromGuiRunUrlAction( VxGUID& sessionId, const char* myUrl, const char* ptopUrl, ENetCmdType testType ) = 0;

    virtual void				fromGuiAnnounceHost( HostedId& adminId, VxGUID& sessionId, std::string& hostUrl, bool fromThread = false ) = 0;
    virtual void				fromGuiJoinHost( HostedId& adminId, VxGUID& sessionId, std::string& hostUrl, bool fromThread = false ) = 0;
	virtual void				fromGuiLeaveHost( HostedId& adminId, bool fromThread = false ) = 0;
	virtual void				fromGuiUnJoinHost( HostedId& adminId, bool fromThread = false ) = 0;

    virtual void				fromGuiSearchHost( EHostType hostType, SearchParams& searchParams, bool enable, bool fromThread = false ) = 0;

	virtual void                fromGuiBlockUser( VxGUID& onlineId, bool fromThread = false ) = 0;

	virtual void				fromGuiSendAnnouncedList( EHostType hostType, VxGUID& sessionId ) = 0;

	virtual void 				fromGuiDisconnectFromUser( VxGUID& onlineId ) = 0;

	/// Set file share settings
	virtual void				fromGuiSetFileShareSettings( FileShareSettings& fileShareSettings ) = 0;
	/// Get file share settings
	virtual void				fromGuiGetFileShareSettings( FileShareSettings& fileShareSettings ) = 0;
	/// Update About Me Web page
	virtual void				fromGuiUpdateWebPageProfile(	const char*	pProfileDir,	// directory containing user profile
																const char*	strGreeting,	// greeting text
																const char*	aboutMe,		// about me text
																const char*	url1,			// favorite url 1
																const char*	url2,			// favorite url 2
                                                                const char*	url3,           // favorite url 3
                                                                const char*	donation ) = 0;	// donation		
	/// Set permission level required to access a plugin
	virtual void				fromGuiSetPluginPermission( EPluginType pluginType, EFriendState eFriendState ) = 0;
	/// Get permission level required to access a plugin
	virtual int					fromGuiGetPluginPermission( EPluginType pluginType ) = 0;
	/// Get server state of specific plugin ( disabled or in session or not in session )
	virtual EPluginServerState	fromGuiGetPluginServerState( EPluginType pluginType ) = 0;

	/// Start plugin session or server
	virtual bool                fromGuiStartPluginSession( EPluginType pluginType, VxGUID onlineId, VxGUID lclSessionId = VxGUID::nullVxGUID() ) = 0;
	/// Stop plugin session or server
	virtual void				fromGuiStopPluginSession( EPluginType pluginType, VxGUID onlineId, VxGUID lclSessionId = VxGUID::nullVxGUID()  ) = 0;
	/// Return true if plugin is in session
	virtual bool				fromGuiIsPluginInSession( EPluginType pluginType, VxGUID& onlineId = VxGUID::nullVxGUID(), VxGUID lclSessionId = VxGUID::nullVxGUID() ) = 0;

	/// Send offer of file or session to a contact
	virtual bool				fromGuiMakePluginOffer( VxGUID& onlineId, OfferBaseInfo& offerInfo ) = 0;
	/// Contact sent session offer reply
	virtual bool				fromGuiToPluginOfferReply( VxGUID& onlineId, OfferBaseInfo& offerInfo ) = 0;

	/// Plugin control such as cancel download etc.
	virtual EXferError			fromGuiFileXferControl( EPluginType pluginType, EXferAction xferAction, FileInfo& fileInfo ) = 0;
	/// Send Text Message to contact
	virtual bool				fromGuiInstMsg(	EPluginType	oluginType, 
												VxGUID&		onlineId, 
												const char*	pMsg ) = 0; 

	virtual bool				fromGuiPushToTalk( VxGUID& onlineId, bool enableTalk ) = 0;

	virtual void				fromGuiAdminViewHost( EPluginType pluginType, bool adminIsViewing ) = 0;

	//! Send to GUI all contacts for given view type selection
	virtual void				fromGuiSendContactList( EFriendViewType eFriendView, int MaxContactsToSend ) = 0;
	//! Send new announcement to anchor and refresh contact list
	virtual void				fromGuiRefreshContactList( int MaxContactsToSend ) = 0;

	///============================================================================
	/// Scan network for user shared resources
	///============================================================================

	/// Start scan of contacts on network for given resource ( About Me picture, Shared Web Cam, etc. )
	virtual void				fromGuiStartScan( EScanType eScanType, uint8_t searchFlags, uint8_t fileTypeFlags, const char* pSearchPattern = "" ) = 0;
	/// Force scan to move to next search result now instead of waiting for timer countdown
	virtual void				fromGuiNextScan( EScanType eScanType ) = 0;
	/// Stop scanning network
	virtual void				fromGuiStopScan( EScanType eScanType ) = 0;

	//============================================================================
	// misc
	//============================================================================

	/// Get current IP Address (IPv4 or IPv6 depending on if ipv6 is enabled)
	virtual InetAddress			fromGuiGetMyIpAddress( void ) = 0;
	/// Get current IPv4 IP Address
	virtual InetAddress			fromGuiGetMyIPv4Address( void ) = 0;
	/// Get current IPv6 IP Address
	virtual InetAddress			fromGuiGetMyIPv6Address( void ) = 0;

	/// Cancel a download
	virtual void				fromGuiCancelDownload( VxGUID& fileInstance ) = 0;
	/// Cancel a upload
	virtual void				fromGuiCancelUpload( VxGUID& fileInstance ) = 0;

	/// Set game action ( used for Truth Or Dare video chat game )
	virtual bool				fromGuiTodGameActionSend( EPluginType pluginType, VxGUID& onlineId, ETodGameAction todGameAction ) = 0;

	/// Send directory listing to GUI
	virtual bool				fromGuiBrowseFiles( VxGUID& appInstId, std::string& folderName, uint8_t fileFilterMask = VXFILE_TYPE_ALLNOTEXE | VXFILE_TYPE_DIRECTORY ) = 0;

	/// Returns -1 if unknown else percent downloaded
	virtual int					fromGuiGetFileDownloadState( uint8_t* fileHashId ) = 0;

	/// Share/Unshare a file
	virtual bool				fromGuiSetFileIsShared( FileInfo& fileInfo, bool addFile ) = 0;
	/// Return true if file is shared
    virtual bool				fromGuiGetIsFileShared( FileInfo& fileInfo ) = 0;

	/// Add/Remove file from library
	virtual bool				fromGuiSetFileIsInLibrary( FileInfo& fileInfo, bool inLibrary ) = 0;
	/// Return true if file is in library
	virtual bool				fromGuiGetFileIsInLibrary( FileInfo& fileInfo ) = 0;

	/// Send to GUI file that are in library of the given file type mask
	virtual void				fromGuiGetFileLibraryList( VxGUID& appInstId, uint8_t fileTypeFilter ) = 0;

	/// Send to GUI media in folder and subfolders of the given directory and of file type mask
	virtual void				fromGuiScanFolderForMedia( VxGUID& appInstId, std::string scanDir, uint8_t fileTypeFilter, bool fromThread = false ) = 0;
	virtual void				fromGuiScanItemReceived( VxGUID& appInstId ) = 0; // ack recieve to avoid overwelming the gui
	virtual void				fromGuiScanFolderCancel( VxGUID& appInstId ) = 0;

	/// Return true if video file was created by No Limit Connect
	virtual bool				fromGuiIsNoLimitVideoFile( const char* fileName ) = 0;
	/// Return true if audio file was created by No Limit Connect
	virtual bool				fromGuiIsNoLimitAudioFile( const char* fileName ) = 0;
	
	/// Delete file.. if shred file is true then write random data to file before deleting it
	virtual int					fromGuiDeleteFile( std::string fileName, bool shredFile ) = 0;

	/// Send Text Chat session assets of contact to GUI
	virtual void				fromGuiQuerySessionHistory( GroupieId& groupieId ) = 0;
	/// Text Chat asset action such as remove
	virtual bool				fromGuiAssetAction( EAssetAction assetAction, AssetBaseInfo& assetInfo, int pos0to100000 = 0 ) = 0;
	/// Text Chat asset action queue asset to be sent next time both users are online
	virtual bool				fromGuiQueueAssetAction( EAssetAction assetAction, AssetBaseInfo& assetInfo, int pos0to100000 = 0 ) = 0;
	/// Text Chat asset action such as remove
	virtual bool				fromGuiAssetAction( EPluginType pluginType, EAssetAction assetAction, VxGUID& assetId, int pos0to100000 = 0 ) = 0;
	/// Send Text Chat asset to contact
	virtual bool				fromGuiSendAsset( AssetBaseInfo& assetInfo ) = 0;
	/// Text Chat session action such as start video chat session
	virtual bool				fromGuiMultiSessionAction( EMSessionAction mSessionAction, VxGUID& onlineId, int pos0to100000, VxGUID lclSessionId = VxGUID::nullVxGUID() ) = 0;
	
	/// Get random TCP port that is not currently in use
	virtual uint16_t			fromGuiGetRandomTcpPort( void ) = 0;

    /// Get url for this node
    virtual void                fromGuiGetNodeUrl( std::string& nodeUrl ) = 0;
    /// Get internet status
    virtual EInternetStatus     fromGuiGetInternetStatus( void ) = 0;
    /// Get network status
    virtual ENetAvailStatus     fromGuiGetNetAvailStatus( void ) = 0;

	enum ETestParam1
	{
		eTestParam1Unknown					= 0,
		eTestParam1FullNetTest1				= 1,
		eTestParam1Ping						= 2,
		eTestParam1WhatsMyIp				= 3,
		eTestParam1IsMyPortOpen				= 4,
		eTestParamSoundDelayTest			= 5,

		eMaxTestParam1
	};

	/// For testing and debug only 
	virtual bool				fromGuiTestCmd(	IFromGui::ETestParam1		eTestParam1, 
												int							testParam2 = 0, 
												const char*				testParam3 = nullptr ) = 0;

	virtual int					fromGuiGetJoinedListCount( EPluginType pluginType ) = 0;
	virtual int					fromGuiGetAnnouncedHostCount( EHostType hostType ) = 0;
	virtual void				fromGuiListAction( EListAction listAction ) = 0;
	virtual std::string			fromGuiQueryDefaultUrl( EHostType hostType, bool ignoreMyself = false ) = 0;
	virtual bool				fromGuiSetDefaultUrl( EHostType hostType, std::string& hostUrl ) = 0;
	virtual bool				fromGuiQueryIdentity( std::string& url, VxNetIdent& retNetIdent, bool requestIdentityIfUnknown ) = 0;
	virtual bool				fromGuiQueryIdentity( VxGUID onlineId, VxNetIdent& retNetIdent ) = 0;
	virtual bool				fromGuiQueryHosts( std::string& netHostUrl, EHostType hostType, std::vector<HostedInfo>& hostedInfoList, VxGUID& hostIdIfNullThenAll ) = 0;
	virtual bool				fromGuiQueryMyHostedInfo( EHostType hostType, std::vector<HostedInfo>& hostedInfoList ) = 0;
	virtual bool				fromGuiQueryHostListFromNetworkHost( VxPtopUrl& netHostUrl, EHostType hostType, VxGUID& hostIdIfNullThenAll ) = 0;
	virtual bool				fromGuiQueryGroupiesFromHosted( VxPtopUrl& hostedUrl, EHostType hostType, VxGUID& onlineIdIfNullThenAll ) = 0;

	virtual bool				fromGuiDownloadWebPage( EWebPageType webPageType, VxGUID& onlineId ) = 0;
	virtual bool				fromGuiCancelWebPage( EWebPageType webPageType, VxGUID& onlineId ) = 0;

	virtual bool				fromGuiDownloadFileList( EPluginType pluginType, VxGUID& onlineId, VxGUID& sessionId, uint8_t fileTypes = 0 ) = 0;
	virtual bool				fromGuiDownloadFileListCancel( EPluginType pluginType, VxGUID& onlineId, VxGUID& sessionId ) = 0;

	virtual EJoinState		    fromGuiQueryJoinState( EHostType hostType, VxNetIdent& netIdent ) = 0;

	virtual void				fromGuiUpdatePluginPermission( EPluginType pluginType, EFriendState pluginPermission ) = 0;

	virtual bool				fromGuiQueryFileHash( FileInfo& fileInfo ) = 0;

	virtual bool				fromGuiDeleteDatabase( EDatabaseType databaseType ) = 0;

	virtual void				fromGuiSetIsAutomatedHost( bool automatedHost ) = 0;

	virtual bool				fromGuiSendRandConnectSelected( VxGUID& onlineId, bool isSelected ) = 0;

	virtual bool                fromGuiQueryFriendRequest( std::vector<std::shared_ptr<FriendRequestInfo>>& friendRequestList, VxGUID& onlineIdIfNullThenAll ) = 0;
	virtual bool                fromGuiSendFriendRequest( VxGUID& onlineId, std::string& requestText, EFriendState myFriendshipToHim ) = 0;

};

