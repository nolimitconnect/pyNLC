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

#include <CoreLib/VxXferDefs.h>

#define ENABLE_COMPONENT_NEARBY         0 // enable/disable nearby users discovery 
#define ENABLE_IPV6                     0 // if not zero enable ipv6 listen and usage 
#define ENABLE_STREAMING                1 // if not zero enable streaming of shared media files 

#define AUDIO_SAMPLE_RATE       16000
#define AUDIO_CHUNK_SIZE        1280

#define MIN_SEARCH_TEXT_LEN             3
#define MAX_OFFER_HISTORY_LIST_SIZE     100

// NOTE: if you add or remove a enum then must update corresponding DescribeXXX( enumXXX )

enum EMediaModule
{
    eMediaModuleInvalid = 0,
    eMediaModuleAll,
    eMediaModuleCamClient,
    eMediaModuleCamServer,
    eMediaModuleCamSettings,
    eMediaModuleChatRoomClient,
    eMediaModuleChatRoomHost,
    eMediaModuleGroupClient,
    eMediaModuleGroupHost,
    eMediaModuleMediaPlayer,
    eMediaModuleMediaReader,
    eMediaModuleMediaWriter,
    eMediaModuleMessenger,
    eMediaModuleMicrophone,
    eMediaModulePtoP,
    eMediaModulePeerUserClient,
    eMediaModulePeerUsertHost,
    eMediaModulePersonalNotes,
    eMediaModulePhotoPlayer,
    eMediaModulePlayerNlc,
    eMediaModulePushToTalk,
    eMediaModuleRandomConnectClient,
    eMediaModuleRandomConnectHost,
    eMediaModuleSnapshot,

    eMediaModuleTruthOrDare,
    eMediaModuleVideoPhone,
    eMediaModuleVoicePhone,

    eMediaModuleSoundFx,
    eMediaModuleSoundSettings,
    eMediaModuleSoundDelayTest,
    eMediaModuleSoundLoopbackTest,

    eMediaModuleAudioPlayWidget,
    eMediaModuleVideoPlayWidget,

    eMaxMediaModule // must be last
};

enum EAgeType
{
    eAgeTypeUnspecified = 0,
    eAgeTypeUnder21,
    eAgeType21OrOlder,

    eMaxAgeType
};

enum EAudioTestState
{
	eAudioTestStateNone,
	eAudioTestStateInit,
	eAudioTestStateRun,
	eAudioTestStateResult,
	eAudioTestStateDone,

	eMaxAudioTestState
};

enum ECacheType
{
    eCacheTypeNone = 0,
    eCacheTypeThumbnail,

    eMaxCacheType
};

enum ECallState
{
    eCallStateInvalid = 0,
    eCallStateWaiting,
    eCallStateCalling,
    eCallStateInCall,
    eCallStateHangedUp,

    eMaxCallState
};

enum ECommErr
{
    eCommErrNone = 0,
    eCommErrInvalidPkt,
    eCommErrUserOffline,
    eCommErrSearchTextToShort, 
    eCommErrSearchTextToLong,
    eCommErrSearchNoMatch, 
    eCommErrInvalidHostType, 
    eCommErrPluginNotEnabled,
    eCommErrPluginPermission,
    eCommErrNotFound,
    eCommErrInvalidParam,

    eMaxCommErr
};

enum EConnectReason
{
    eConnectReasonUnknown = 0,

    eConnectReasonGroupAnnounce,
    eConnectReasonGroupJoin,
    eConnectReasonGroupLeave,
    eConnectReasonGroupUnJoin,
    eConnectReasonGroupSearch,
    eConnectReasonGroupUserConnect,

    eConnectReasonChatRoomAnnounce,
    eConnectReasonChatRoomJoin,
    eConnectReasonChatRoomLeave,
    eConnectReasonChatRoomUnJoin,
    eConnectReasonChatRoomSearch,
    eConnectReasonChatRoomUserConnect,

    eConnectReasonRandomConnectAnnounce,
    eConnectReasonRandomConnectJoin,
    eConnectReasonRandomConnectLeave,
    eConnectReasonRandomConnectUnJoin,
    eConnectReasonRandomConnectSearch,
    eConnectReasonRandomConnectUserConnect,

    eConnectReasonAnnouncePing,
    eConnectReasonStayConnected,
    eConnectReasonPlugin,
    eConnectReasonOtherSearch,

    eConnectReasonNearbyLan,
    eConnectReasonSameExternalIp,
    eConnectReasonReverseConnectRequested,
    eConnectReasonPktTcpPunch,
    eConnectReasonRelayService,

    eConnectReasonRequestIdentity,
    eConnectReasonNetworkHostListSearch,

    eConnectReasonGroupHostedUserListSearch,
    eConnectReasonChatRoomHostedUserListSearch,
    eConnectReasonRandomConnectHostedUserListSearch,

    eConnectReasonGroupGroupieUserListSearch,
    eConnectReasonChatRoomGroupieUserListSearch,
    eConnectReasonRandomConnectGroupieUserListSearch,

    eConnectReasonUserRelayedConnect,
    eConnectReasonUserDirectConnect,
    eConnectReasonNetworkHost,
    eConnectReasonConnectTest,

    eConnectReasonPeerInvite,

    eMaxConnectReason
};

enum EConnectStatus
{
    eConnectStatusUnknown = 0,

    eConnectStatusReady,
    eConnectStatusBadParam,
    eConnectStatusBadAddress,
    eConnectStatusPermissionDenied,
    eConnectStatusConnecting,
    eConnectStatusConnectFailed,
    eConnectStatusSendPktAnnFailed,
    eConnectStatusHandshaking,
    eConnectStatusHandshakeTimeout,
    eConnectStatusConnectSuccess,
    eConnectStatusDropped,
    eConnectStatusRxAnnTimeout,

    eMaxConnectStatus
};

enum EConnectType
{
    eConnectTypeAny = 0,

    eConnectTypeHost,
    eConnectTypeClient,
    eConnectTypeDirect,

    eMaxConnectType
};

enum EContentRating
{
    eContentRatingUnspecified = 0,
    eContentRatingFamily,
    eContentRatingAdult,
    eContentRatingXXX,
    eContentRatingDarkWeb,
    eContentRatingPersonal,

    eMaxContentRating
};

enum EContentCatagory
{
    eContentCatagoryUnspecified = 0,
    eContentCatagoryVideo,
    eContentCatagoryAudio,
    eContentCatagoryImage,
    eContentCatagoryText,
    eContentCatagoryPersonal,
    eContentCatagoryOther,

    eMaxContentCatagory
};

enum EDatabaseType
{
    eDatabaseTypeNone = 0,
    eDatabaseTypeAssets,
    eDatabaseTypeBlobAssets,
    eDatabaseTypeConnectMgr,
    eDatabaseTypeEngineParams,
    eDatabaseTypeEngineSettings,
    eDatabaseTypeHostServerJoin,
    eDatabaseTypeOffers,
    eDatabaseTypeThumbs,
    eDatabaseTypeUserJoin,
    eDatabaseTypeAllUsers,

    eMaxDatabaseType
};

enum EFriendRequestState
{
    eFriendRequestUnknown	= 0,	
    eFriendRequestRxed,
    eFriendRequestAccepted,
    eFriendRequestRejected,	
    eFriendRequestOffline,
    eFriendRequestBadParam,
    eFriendRequestSending,
    eFriendRequestSendSuccess,
    eFriendRequestSendFail,

    eMaxFriendRequestState
};

//! \public Enumeration of friendship type. NOTE: also used as plugin permission type
enum EFriendState
{
    eFriendStateIgnore		= 0,	//!< Unknown or disabled or ignore
    eFriendStateAnonymous	= 1,	//!< Anonymous contact
    eFriendStateGuest		= 2,	//!< Guest contact
    eFriendStateFriend		= 3,	//!< Friend contact
    eFriendStateAdmin		= 4		//!< Administrator contact
};

//! \public Enumeration of which contacts to show in contact list
enum EFriendViewType
{
    eFriendViewEverybody		= 0,
    eFriendViewAdministrators   = 1,
    eFriendViewFriendsAndGuests = 2,
    eFriendViewAnonymous		= 3,
    eFriendViewIgnored			= 4,
    eFriendViewRefresh			= 5,

    eMaxFriendViewType // must be last
};

enum EFirewallTestType
{
    eFirewallTestUrlConnectionTest = 0,
    eFirewallTestAssumeNoFirewall = 1,

    eMaxFirewallTestType
};

enum EGenderType
{
    eGenderTypeUnspecified = 0,
    eGenderTypeMale,
    eGenderTypeFemale,

    eMaxGenderType
};

enum EGroupieViewType
{
    eGroupieViewTypeNone,
    eGroupieViewTypeFriendsOnly,
    eGroupieViewTypeGroupOnly,
    eGroupieViewTypeGroupAndFriends,
    eGroupieViewTypeEverybody,
    eGroupieViewTypeIgnored,
    eGroupieViewTypeNearby,
    eGroupieViewTypeOnline,
    eGroupieViewTypeDirectConnect,

    eMaxGroupieViewType,
};

enum EHackerLevel
{
    eHackerLevelUnknown,
    eHackerLevelSuspicious,
    eHackerLevelMedium,
    eHackerLevelSevere,

    eMaxHackerLevel
};

enum EHackerReason
{
    eHackerReasonUnknown,
    eHackerReasonPeerName,
    eHackerReasonHostByName,
    eHackerReasonNoHostIpAddr,
    eHackerReasonHostIpOptions,
    eHackerReasonNetCmdLength,
    eHackerReasonNetCmdListInvalid,
    eHackerReasonNetSrvUrlInvalid,
    eHackerReasonNetSrvPluginInvalid,
    eHackerReasonNetSrvQueryIdPermission,
    eHackerReasonHttpAttack,
    eHackerReasonPktOnlineIdMeFromMyIp,
    eHackerReasonPktOnlineIdMeFromAnotherIp,
    eHackerReasonPktAnnNotFirstPacket,
    eHackerReasonPktHdrInvalid,
    eHackerReasonInvalidPkt,
    eHackerReasonAccessDenied,
    eHackerReasonLurkerDidNotSendPktAnn,
    eHackerReasonFriendRequestFromIgnoredUser,

    eMaxHackerReason
};

enum EHostAnnounceStatus
{
    eHostAnnounceUnknown = 0,
    eHostAnnounceInvalidUrl = 1,
    eHostAnnounceQueryIdInProgress = 2,
    eHostAnnounceQueryIdSuccess = 3,
    eHostAnnounceQueryIdFailed = 4,
    eHostAnnounceConnecting = 5,
    eHostAnnounceHandshaking = 6,
    eHostAnnounceHandshakeTimeout = 7,
    eHostAnnounceConnectSuccess = 8,
    eHostAnnounceConnectFailed = 9,
    eHostAnnounceSendingJoinRequest = 10,
    eHostAnnounceSendJoinRequestFailed = 11,
    eHostAnnounceSuccess = 12,
    eHostAnnounceFailRequiresOpenPort = 13,
    eHostAnnounceFailPermission = 14,
    eHostAnnounceFailConnectDropped = 15,
    eHostAnnounceInvalidParam = 16,
    eHostAnnouncePluginDisabled = 17,
    eHostAnnounceDone = 18,

    eMaxHostAnnounceStatus
};

enum EHostJoinStatus
{
    eHostJoinUnknown = 0,
    eHostJoinInvalidUrl = 1,
    eHostJoinQueryIdInProgress = 2,
    eHostJoinQueryIdSuccess = 3,
    eHostJoinQueryIdFailed = 4,
    eHostJoinConnecting = 5,
    eHostJoinHandshaking = 6,
    eHostJoinHandshakeTimeout = 7,
    eHostJoinConnectSuccess = 8,
    eHostJoinConnectFailed = 9,

    eHostJoinFailPermission = 10,
    eHostJoinFailConnectDropped = 11,
    eHostJoinInvalidParam = 12,
    eHostJoinPluginDisabled = 13,
    eHostJoinDone = 14,

    eHostJoinSendingJoinRequest = 15,
    eHostJoinSendJoinRequestFailed = 16,
    eHostJoinSuccess = 17,
    eHostJoinFail = 18,

    eHostJoinSendingLeaveRequest = 19,
    eHostJoinSendLeaveRequestFailed = 20,
    eHostLeaveSuccess = 21,
    eHostLeaveFail = 22,

    eHostJoinSendingUnJoinRequest = 23,
    eHostJoinSendUnJoinRequestFailed = 24,
    eHostJoinUnJoinSuccess = 25,
    eHostJoinUnJoinFail = 26,

    eMaxHostJoinStatus
};

enum EHostSearchStatus
{
    eHostSearchUnknown = 0,
    eHostSearchInvalidUrl = 1,
    eHostSearchQueryIdInProgress = 2,
    eHostSearchQueryIdSuccess = 3,
    eHostSearchQueryIdFailed = 4,
    eHostSearchConnecting = 5,
    eHostSearchHandshaking = 6,
    eHostSearchHandshakeTimeout = 7,
    eHostSearchConnectSuccess = 8,
    eHostSearchConnectFailed = 9,
    eHostSearchSendingSearchRequest = 10,
    eHostSearchSendSearchRequestFailed = 11,
    eHostSearchSuccess = 12,
    eHostSearchFail = 13,
    eHostSearchFailPermission = 14,
    eHostSearchFailConnectDropped = 15,
    eHostSearchInvalidParam = 16,
    eHostSearchPluginDisabled = 17,
    eHostSearchNoMatches = 18,
    eHostSearchCompleted = 19,
    eHostSearchDone = 20,

    eMaxHostSearchStatus
};

//! \public Host connection test state
enum EHostTestStatus
{
    eHostTestStatusUnknown = 0,
    eHostTestStatusLogMsg = 1,

    eHostTestStatusHostOk = 2,
    eHostTestStatusHostConnectFail = 3,
    eHostTestStatusHostConnectionDropped = 4,
    eHostTestStatusHostTestComplete = 5,

    eHostTestStatusNetServiceOk = 6,
    eHostTestStatusNetServiceConnectFail = 7,
    eHostTestStatusNetServiceConnectionDropped = 8,
    eHostTestStatusNetServiceTestComplete = 9,

    eMaxHostTestStatusType
};

enum EHostType
{
    eHostTypeUnknown = 0,

    eHostTypeConnectTest = 1,   
    eHostTypeNetwork = 2,    

    eHostTypePeerUser = 3,  // user<->user 
    // these hosts provide relay services and are announced to network host
    eHostTypeGroup = 4,
    eHostTypeChatRoom = 5,
    eHostTypeRandomConnect = 6,

    eMaxHostType
};

enum EInfoType
{
    eInfoTypeInvalid,
    eInfoTypePlugin,
    eInfoTypePermission,
    eInfoTypeNetworkKey,
    eInfoTypeNetworkHost,
    eInfoTypeConnectTestUrl,
    eInfoTypeConnectTestSettings,
    eInfoTypeRandomConnectUrl,
    eInfoTypeDefaultGroupHostUrl,
    eInfoTypeDefaultChatRoomHostUrl,
    eInfoTypeNetworkSettingsInvite,

    eInfoTypeFriendsList,
    eInfoTypeIgnoredList,
    eInfoTypeOfflineList,

    eInfoTypeGroupStatus,
    eInfoTypeHostChatRoom,
    eInfoTypeHostGroup,
    eInfoTypeHostNetwork,
    eInfoTypeHostRandomConnect,

    eInfoTypeMaxMessageHistory,
    
    eInfoTypeIpv6,

    eInfoTypeWhatIsAInvite,

    eMaxInfoType // must be last
};

enum EInternetStatus
{
    eInternetNoInternet = 0,
    eInternetInternetAvailable,
    eInternetTestHostUnavailable,
    eInternetTestHostAvailable,
    eInternetAssumeDirectConnect,
    eInternetCanDirectConnect,
    eInternetRequiresRelay,

    eMaxInternetStatus
};

// Can Direct Connect test state
enum EIsPortOpenStatus
{
    eIsPortOpenStatusUnknown						= 0,
    eIsPortOpenStatusLogMsg							= 1,

    eIsPortOpenStatusOpen							= 2,
    eIsPortOpenStatusClosed							= 3,
    eIsPortOpenStatusConnectFail					= 4,
    eIsPortOpenStatusConnectionDropped				= 5,
    eIsPortOpenStatusInvalidResponse				= 6,
    eIsPortOpenStatusTestComplete					= 7,

    eMaxIsPortOpenStatusType
};

enum EJoinState
{
    eJoinStateNone     = 0,
    eJoinStateSending,
    eJoinStateSendFail,
    eJoinStateSendAcked,
    eJoinStateJoinRequested,
    eJoinStateJoinWasGranted, // is member but not currently connected
    eJoinStateJoinIsGranted,  // is member and currently connected
    eJoinStateJoinDenied,
    eJoinStateJoinLeaveHost,

    eMaxJoinState	
};

enum ELanguageType
{
    eLangUnspecified = 0,
    eLangEnglish,       //  (en) English
    eLangBulgarian,	    //  (bg) Bulgarian
    eLangChinese,	    //  (ch) Chinese
    eLangCroatian,      //  (hr) Croatian
    eLangCzech,         //  (cs) Czech
    eLangDanish,        //  (da) Danish
    eLangDutch,         //  (nl) Dutch
    eLangEstonian,      //  (et) Estonian
    eLangFinnish,       //  (fi) Finnish
    eLangFrench,        //	(fr) French
    eLangGerman,        //	(de) German
    eLangGreek,         //  (el) Greek
    eLangHindi,         //  (hi) Hindi
    eLangHungarian,     //	(hu) Hungarian
    eLangItalian,       //	(it) Italian
    eLangJapanese,      //	(jp) Japanese
    eLangLatvian,       //	(lv) Latvian
    eLangLithuanian,    //	(lt) Lithuanian
    eLangPolish,        //	(pl) Polish
    eLangPortuguese,    //	(pt) Portuguese
    eLangRomanian,      //	(ro) Romanian
    eLangRussian,       //	(ru) Russian
    eLangSerbian,       //  (sr) Serbian
    eLangSlovak,        //	(sl) Slovak
    eLangSpanish,       //	(es) Spanish
    eLangSwedish,       //	(sv) Swedish
    eLangThai,          //  (th) Thiawanese
    eLangTurkish,       //  (tr) Turkish
    eLangUkrainian,     //	(uk) Ukrainian
    eLangArabic,        //  (ar) Arabic
    eLangKorean,        //  (ko) Korean
    eLangIndonesian,    //  (id) Indonesian

    eMaxLanguageType
};

enum EListAction
{
    eListActionAnnounced,
    eListActionJoinedHost,
    eListActionJoinedClient,

    eMaxListAction
};


//! \public In Text Chat Session Actions
enum EMSessionAction
{
	eMSessionActionNone = 0,
	eMSessionActionChatSessionReq,
	eMSessionActionChatSessionAccept,
	eMSessionActionChatSessionReject,
	eMSessionActionOffer,
	eMSessionActionAccept,
	eMSessionActionReject,
	eMSessionActionHangup,

	eMaxMSessionAction
};

//! \public Session Types
enum EMSessionType
{
	eMSessionTypePhone 						= 0,
	eMSessionTypeVidChat 					= 1,
	eMSessionTypeTruthOrDare 				= 2,

	eMaxMSessionType
};

enum EMediaError
{
    eMediaErrorNone = 0,
    eMediaErrorCodecNotFound,
    eMediaErrorFileNotFound,
    eMediaErrorPlayerAssetNotFound,
    eMediaErrorPlayerInit,

    eMaxMediaError
};

enum EMediaPlayerAction
{
    eMediaPlayerActionNone = 0,
    eMediaPlayerActionClose,
    eMediaPlayerActionOpen,
    eMediaPlayerActionPlayPause,
    eMediaPlayerActionPlayPosition,
    eMediaPlayerActionPlayStart,
    eMediaPlayerActionPlayStop,
    eMediaPlayerActionPlayEnd,
    eMediaPlayerActionSelectFile,

    eMaxMediaPlayerAction
};

enum EMembershipState
{
    eMembershipStateNone = 0,
    eMembershipStateCanBeRequested,
    eMembershipStateJoined,
    eMembershipStateJoinDenied,

    eMaxMembershipState
};

enum EModuleState
{
    eModuleStateUnknown = 0,
    eModuleStateInitialized,
    eModuleStateDeinitialized,
    eModuleStateInitError,

    eMaxModuleState // must be last
};

enum ENetAvailStatus
{
    eNetAvailNoInternet = 0,
    eNetAvailHostAvail,
    eNetAvailP2PAvail,
    eNetAvailOnlineButNoRelay,
    eNetAvailFullOnlineWithRelay,
    eNetAvailFullOnlineDirectConnect,
    eNetAvailRelayGroupHost,
    eNetAvailDirectGroupHost,

    eMaxNetAvailStatus
};

enum ENetCmdType
{
    eNetCmdUnknown						= 0,
    eNetCmdHostPing						= 1,		
    eNetCmdHostPong						= 2,		
    eNetCmdClientPing					= 3,		
    eNetCmdClientPong					= 4,
    eNetCmdIsMyPortOpenReq				= 5,		
    eNetCmdIsMyPortOpenReply			= 6,			
    eNetCmdQueryHostOnlineIdReq         = 7,
    eNetCmdQueryHostOnlineIdReply       = 8,

    eMaxNetCmdType
};

enum ENetCmdError
{
    eNetCmdErrorUnknown                 = 0,
    eNetCmdErrorNone                    = 1,
    eNetCmdErrorServiceDisabled         = 2,
    eNetCmdErrorPermissionLevel         = 3,  
    eNetCmdErrorFailedResolveIpAddr     = 4,
    eNetCmdErrorInvalidContent          = 5,
    eNetCmdErrorPortIsClosed            = 6,
    eNetCmdErrorConnectFailed           = 7,
    eNetCmdErrorTxFailed                = 8,
    eNetCmdErrorRxFailed                = 9,
    eNetCmdErrorBadParameter            = 10,
    eNetCmdErrorResponseTimedOut        = 11,

    eMaxNetCmdError
};

enum ENetActionType
{
    eNetActionUnknown = 0,
    eNetActionIdle,
    eNetActionIsPortOpen,
    eNetActionResolveConnectTestUrl,
    eNetActionResolveDefaultUserHosts,
    eNetActionResolveNetworkHostUrl,
    eNetActionWaitForInternet,
    eNetActionRenewPortForward,

    eMaxNetAction
};

//! \public Network protocol layer
enum ENetLayerType
{
    eNetLayerTypeUndefined = 0,		    //< not specified/initialized
    eNetLayerTypeInternet,		        //< can communicate with internet
    eNetLayerTypePtoP,		            //< tcp and extern ip found
    eNetLayerTypeNetHost,		        //< network host available
    eNetLayerTypeNetGroup,		        //< network host available

    eMaxNetLayerType		            //< max must be last
};

enum ENetLayerState
{
    eNetLayerStateUndefined = 0,		//< not specified/initialized
    eNetLayerStateWrongType,		    //< invalid or not relevant net layer type
    eNetLayerStateTesting,		        //< testing for available 
    eNetLayerStateFailed,		        //< failed or some issue
    eNetLayerStateAvailable,		    //< available and ready for use

    eMaxNetLayerState		            //< max must be last
};

//! Enumeration of Network State Machine states/actions
enum ENetworkStateType
{
    eNetworkStateTypeUnknown				= 0,
    eNetworkStateTypeLost					= 1,
    eNetworkStateTypeAvail					= 2,
    eNetworkStateTypeTestConnection			= 3,
    eNetworkStateTypeOnlineDirect			= 4,
    eNetworkStateTypeWaitForRelay           = 5,
    eNetworkStateTypeOnlineThroughRelay		= 6,
    eNetworkStateTypeNoInternetConnection	= 7,
    eNetworkStateTypeFailedResolveHostNetwork	= 8,
    eNetworkStateTypeFailedResolveHostGroupList = 9,
    eNetworkStateTypeFailedResolveHostGroup     = 10,
    eNetworkStateTypeIpChange                   = 12,

    eMaxNetworkStateType
};

//! Enumeration of Notify Icon state (dot in upper right of icon)
enum ENotifyType
{
    eNotifyNone         = 0, // hide the online dot
    eNotifyOffline      = 1, // online dot indicates offline (usually grey color)
    eNotifyOnline       = 2, // online dot indicates online (usually greeen color)
    eNotifyRelayed      = 3, // online dot indicates through relay or needs attention but currenly offline (usually burnt orange color)
    eNotifyAttention    = 4, // online dot indicates online and needs user attention (usually red color) 

    eMaxNotifyType // must be last
};

//! Enumeration of session offer response
enum EOfferResponse
{
    eOfferResponseNotSet		= 0,	//!< Unknown or not set offer response from contact
    eOfferResponseAccept		= 1,	//!< Contact accepted offer
    eOfferResponseReject		= 2,	//!< Contact rejected offer
    eOfferResponseBusy			= 3,	//!< Contact cannot accept session because already in session
    eOfferResponseCancelSession	= 4,	//!< Contact exited session
    eOfferResponseEndSession	= 5,	//!< Session end because of any reason
    eOfferResponseUserOffline	= 6,	//!< Session end because contact is not online

    eMaxOfferResponseType // must be last
};

//! public Enumeration of offer state
enum EOfferState
{
    eOfferStateNone                 = 0,	// no offer state
    eOfferStateSending              = 1,
    eOfferStateSent                 = 2,
    eOfferStateSendFailed           = 3,
    eOfferStateRxedByUser           = 4,    // recieved by destination user
    eOfferStateBusy                 = 5,    // destination user is already in session
    eOfferStateAccepted,
    eOfferStateRejected,
    eOfferStateCanceled,
    eOfferStateUserOffline,
    eOfferStateInSession,           // in session or in xfer progress
    eOfferStateSessionComplete,     // session or xfer completed
    eOfferStateSessionFailed,       // session or xfer failed

    eOfferStateNoResponse,          // offer timed out without response
    eOfferStateMissedCall,          // offer rxed but timed out before response

    eMaxOfferState
};

//! public Enumeration of offer
enum EOfferType
{
    eOfferTypeUnknown		        = 0,	//!< Unknown or no offer
    eOfferTypeJoinGroup		        = 1,
    eOfferTypeJoinChatRoom		    = 2,
    eOfferTypeJoinRandomConnect		= 3,
    eOfferTypeFriendship		    = 4, 
    eOfferTypeMessenger		        = 5,    // instant message session 
    eOfferTypeTruthOrDare           = 6,	// Video Chat Truth Or Dare game  
    eOfferTypeVideoChat            = 7,	// Video Chat with motion detect and stream recording
    eOfferTypeVoicePhone            = 8,	// VOIP audio only phone call
    eOfferTypePersonFile		    = 9,    // person to person file xfer
    eOfferTypeRandomConnect         = 10,   // offer to connect to random user

    eMaxOfferType
};

enum EOfferLocation
{
    eOfferLocUnknown				= 0x00,
    eOfferLocLibrary				= 0x01,
    eOfferLocShared					= 0x02,
    eOfferLocPersonFile			    = 0x04,
    eOfferLocRandomConnect           = 0x08,
};

enum EOfferAction
{
    eOfferActionUnknown				= 0,
    eOfferActionDeleteFile			= 1,
    eOfferActionShreadFile			= 2,
    eOfferActionAddToOfferMgr		= 3,
    eOfferActionRemoveFromOfferMgr	= 4,	
    eOfferActionUpdateOffer			= 5,
    eOfferActionAddOfferAndSend		= 6,
    eOfferActionOfferResend			= 7,

    eOfferActionAddToShare			= 8,
    eOfferActionRemoveFromShare		= 9,
    eOfferActionAddToLibrary		= 10,
    eOfferActionRemoveFromLibrary	= 11,
    eOfferActionAddToHistory		= 12,
    eOfferActionRemoveFromHistory	= 13,

    eOfferActionRecordBegin			= 14,
    eOfferActionRecordPause			= 15,
    eOfferActionRecordResume		= 16,
    eOfferActionRecordProgress		= 17,
    eOfferActionRecordEnd			= 18,
    eOfferActionRecordCancel		= 19,

    eOfferActionPlayBegin			= 20,
    eOfferActionPlayOneFrame		= 21,
    eOfferActionPlayPause			= 22,
    eOfferActionPlayResume			= 23,
    eOfferActionPlayProgress		= 24,
    eOfferActionPlayEnd				= 25,
    eOfferActionPlayCancel			= 26,

    eOfferActionTxBegin				= 27,
    eOfferActionTxProgress			= 28,
    eOfferActionTxSuccess			= 29,
    eOfferActionTxError				= 30,
    eOfferActionTxCancel			= 31,
    eOfferActionTxPermission		= 32,

    eOfferActionRxBegin				= 33,
    eOfferActionRxProgress			= 34,
    eOfferActionRxSuccess			= 35,
    eOfferActionRxError				= 36,
    eOfferActionRxCancel			= 37,
    eOfferActionRxPermission		= 38,

    eOfferActionRxNotifyNewMsg		= 39,
    eOfferActionRxViewingMsg		= 40,

    eMaxOfferAction
};

enum EOfferMgrType
{
    eOfferMgrNotSet = 0,
    eOfferMgrClient,        // reciever of offer.. even if peer to peer plugin
    eOfferMgrHost,          // sender of offer.. even if peer to peer plugin

    eMaxOfferMgrType
};

enum EOfferSendState
{
    eOfferSendStateNone     = 0,
    eOfferSendStateTxProgress,
    eOfferSendStateRxProgress,
    eOfferSendStateTxSuccess,
    eOfferSendStateTxFail,
    eOfferSendStateRxSuccess,
    eOfferSendStateRxFail,
    eOfferSendStateTxPermissionErr,
    eOfferSendStateRxPermissionErr,

    eMaxOfferSendState	
};

enum EOfferViewType
{
    eOfferViewTypeNone,
    eOfferViewTypeActive,
    eOfferViewTypeHistory,

    eMaxOfferViewType
};

enum EOnlineState
{
    eOnlineStateUnknown         = 0,
    eOnlineStateConnecting      = 1,
    eOnlineStateConnectFailed   = 2,
    eOnlineStateOnline          = 3,
    eOnlineStateOffline         = 4,

    eMaxOnlineState	
};

//! \public Enumeration of plugin accessibility
enum EPluginAccess
{
    ePluginAccessNotSet			= 0,		//!< Plugin access not initialized
	ePluginAccessOk				= 1,		//!< Plugin access allowed
	ePluginAccessLocked			= 2,		//!< Insufficient Friend permission level
	ePluginAccessDisabled		= 3,		//!< Plugin disabled or no files shared or no web cam broadcast
	ePluginAccessIgnored		= 4,		//!< Access denied because you are being ignored
	ePluginAccessInactive		= 5,		//!< Enabled and have access but no shared files or web cam
	ePluginAccessBusy			= 6,		//!< Sufficient permission but cannot accept any more sessions
	ePluginAccessRequiresDirectConnect = 7,	//!< Plugin access requires contact have direct connection
	ePluginAccessRequiresOnline		= 8,	//!< Plugin access requires contact to be online

    eMaxPluginAccess
};

// Enumeration of plugin message
enum EPluginMsgType
{
    ePluginMsgNone = 0,		
    ePluginMsgConnecting,
    ePluginMsgConnectFailed,
    ePluginMsgRetrieveInfo,
    ePluginMsgRetrieveInfoComplete,
    ePluginMsgRetrieveInfoFailed,
    ePluginMsgDownloading,
    ePluginMsgDownloadFailed,
    ePluginMsgDownloadComplete,
    ePluginMsgCanceled,
    ePluginMsgPermissionError,
    ePluginMsgLowDiskSpace,
    ePluginMsgInvalidParam,

    eMaxPluginMsgType
};

//! \public Enumerated plugins
enum EPluginType 
{
    // NOTE: *** update DescribePluginType and GuiParams::describePluginType if you change this
    //! NOTE: don't handle packets for ePluginTypeInvalid
    ePluginTypeInvalid			    = 0,	//!< unknown or disabled

    ePluginTypeHostConnectTest      = 1,	//!< Connection Test Service
    ePluginTypeHostNetwork          = 2,	//!< master network hosting

    ePluginTypeHostChatRoom         = 3,	//!< chat room hosting
    ePluginTypeHostGroup            = 4,    //!< group hosting
    ePluginTypeHostRandomConnect    = 5,	//!< Random connect to another person hosting

    ePluginTypeHostPeerUser         = 6,	//!< mainly for avatar image

    ePluginTypeAboutMePageServer    = 7,	//!< about me web page plugin  
    ePluginTypeMessenger            = 8,	//!< Text, voice and video message texting with voice phone, video chat and truth or dare game available in session
    ePluginTypePushToTalk           = 9,	//!< VOIP audio push to talk   
    ePluginTypePersonFileXfer       = 10,	//!< Offer/accept send a file person to person
    ePluginTypeCamServer            = 11,	//!< Web cam broadcast plugin
    ePluginTypeFileShareServer      = 12,	//!< Shared files server
    ePluginTypeStoryboardServer     = 13,	//!< User editable story board web page server
    ePluginTypeTruthOrDare          = 14,	//!< Video Chat Truth Or Dare game  
    ePluginTypeVideoChat           = 15,	//!< Video Chat with motion detect and stream recording
    ePluginTypeVoicePhone           = 16,	//!< VOIP audio only phone call

    ePluginTypeFriendRequest        = 17,	//!< Who can send a join host or friend request

    // NOTE: plugin types 17 - 47 not implemented .. reserved for future use
    eMaxPermissionPluginType	    = 18, 
    // plugins 0-47 are part of PktAnnounce
    // plugins after 47 do not go out in announcement pkt
    eMaxAnnouncedPluginType         = 48, // this marks end of announced permissions

    ePluginTypeClientConnectTest    = 49,	//!< Connection Test Client
    ePluginTypeClientNetwork        = 50,	//!< network client

    ePluginTypeClientChatRoom       = 51,	//!< chat room user client plugin
    ePluginTypeClientGroup          = 52,	//!< group client   
    ePluginTypeClientRandomConnect  = 53,	//!< Random connect to another person client
    ePluginTypeClientPeerUser       = 54,	//!< mainly for avatar image
    
    ePluginTypeAboutMePageClient    = 55,	//!< about me web page plugin client
    ePluginTypeCamClient            = 56,   //!< cam server plugin client 
    ePluginTypeFileShareClient      = 57,   //!< shared files client
    ePluginTypeStoryboardClient     = 58,	//!< storyboard web page plugin client
    eMaxNetUsePluginType            = 59,

    // 0 to 255 are possible to send over network so internal plugins should start at 256
    ePluginTypeMaxNetRange          = 255,
    ePluginTypeInternalStart        = 256,

    ePluginTypeThumbnail            = 257,  // not used except in asset database for pluginType
    ePluginTypeNetServices,         // internal handle network service requests
    ePluginTypeLibraryServer,       // library
    ePluginTypePersonalRecorder,    // personal recorder
   
    ePluginTypeMJPEGReader, 
    ePluginTypeMJPEGWriter, 
    ePluginTypeSndReader,
    ePluginTypeSndWriter,

    eMaxPluginType	
};

enum EPushToTalkStatus
{
    ePushToTalkStatusInvalid,
    ePushToTalkStatusNotActive,
    ePushToTalkStatusTxEnabled,
    ePushToTalkStatusRxEnabled,
    ePushToTalkStatusDuplexEnabled,

    ePushToTalStatusNoConnection,

    eMaxStatusPushToTalk
};

enum ERelayErr
{
    eRelayErrNone = 0,
    eRelayErrSrcNotJoined,
    eRelayErrDestNotJoined,
    eRelayErrUserNotOnline,
    eRelayErrInvalidPktAnn,

    eMaxRelayErr
};

//! \public Enumeration of relay status
enum ERelayStatus
{
    eRelayStatusUnknown			= -1,	//!< Unknown
    eRelayStatusOk				= 0,	//!< Relay enabled and access accepted 
    eRelayStatusPermissionErr	= 1,	//!< Insufficient permission
    eRelayStatusBusy			= 2,	//!< Relay cannot accept any more session

    eMaxRelayStatus
};

//! \public Connect by shaking phone ( or press simulate phone shake ) status
enum ERandomConnectStatus
{
    eRandomConnectStatusUnknown						= 0,
    eRandomConnectStatusContactHostFail				= 1,
    eRandomConnectStatusFoundFriend					= 2,
    eRandomConnectStatusEmptyList					= 3,
    eRandomConnectStatusSendRequestFail				= 4,
    eRandomConnectStatusInvalidResponse				= 5,
    eRandomConnectStatusDecryptError				= 6,
    eRandomConnectStatusSearchComplete				= 7,

    eMaxRandomConnectStatusType
};

//! \public run a test like query host id state
enum ERunTestStatus
{
    eRunTestStatusUnknown = 0,
    eRunTestStatusLogMsg = 1,

    eRunTestStatusTestSuccess = 2,
    eRunTestStatusTestFail = 3,
    eRunTestStatusTestBadParam = 4,
    eRunTestStatusAlreadyQueued = 5,
    eRunTestStatusConnectFail = 6,
    eRunTestStatusConnectionDropped = 7,
    eRunTestStatusInvalidResponse = 8,
    eRunTestStatusMyPortIsOpen = 9,
    eRunTestStatusMyPortIsClosed = 10,
    eRunTestStatusTestComplete = 11,
    eRunTestStatusTestCompleteFail = 12,
    eRunTestStatusTestCompleteSuccess = 13,

    eMaxRunTestStatusType
};

//! \public Scan/search type enumeration
enum EScanType
{
    eScanTypeNone,				    //!< Unknown or not set search type
    eScanTypeChatRoomJoinSearch,	//!< Search for Chat Room to Join
    eScanTypeGroupJoinSearch,	    //!< Search for Group to Join
    eScanTypeRandomConnect,		    //!< Get contacts who have done phone shake connect in last 20 seconds
    eScanTypePeopleSearch,		    //!< Search for contact of given name 
    eScanTypeMoodMsgSearch,		    //!< Search for contacts with given text in mood message
    eScanTypeProfilePic,		    //!< Search for contacts with not default About Me Web Page picture
    eScanTypeCamServer,			    //!< Search for contacts with shared web cam
    eScanTypeFileSearch,		    //!< Search for contacts with shared files containing given search text
    eScanTypeStoryBoard,		    //!< Search for contacts with modified Story Board Web Page

    eMaxScanType
};

enum ESessionState
{
    eSessionStateUnknown = 0,
    eSessionStateWaitingResponse = 1,
    eSessionStateRejected = 2,
    eSessionStateInSession = 3,
    eSessionStateSessionEnded = 4,
    eSessionContactOffline = 5,
    eSessionUserExitedSession = 6,

    eMaxSessionState
};

enum ESearchType
{
    eSearchNone,
    eSearchChatRoomHost,	        //!< Search for Chat Room to Join
    eSearchGroupHost,	            //!< Search for Group to Join
    eSearchRandomConnectHost,		//!< Search for Random Connect Server to Join

    eMaxSearchType
};

enum ESha1GenResult
{
    eSha1GenResultNoError = 0,
    eSha1GenResultDuplicateRequest,
    eSha1GenResultInvalidParam,
    eSha1GenResultGenerateSha1Failed,

    eMaxSha1GenResult
};

enum ESktType
{
    eSktTypeNone				= 0,
    eSktTypeTcpConnect			= 1,
    eSktTypeTcpAccept			= 2,
    eSktTypeUdp					= 3,
    eSktTypeUdpBroadcast		= 4,
    eSktTypeLoopback    		= 5,
    eSktTypeSimple              = 6,
    eSktTypeListen              = 7,

    eMaxSktType			// always last
};

enum ESktCloseReason
{
    eSktCloseReasonUnknown				= 0,
    eSktCloseNotNeeded                  = 1,
    eSktCloseHackLevelUnknown           = 2,
    eSktCloseHackLevelSuspicious        = 3,
    eSktCloseHackLevelMedium            = 4,
    eSktCloseHackLevelSevere            = 5,
    eSktCloseAcceptFailed               = 6,
    eSktCloseConnectionLost             = 7,
    eSktCloseConnectTimeout             = 8,
    eSktCloseSktDestroy                 = 9,
    eSktCloseAll                        = 10,
    eSktCloseDisconnectAfterSend        = 11,
    eSktCloseSendComplete               = 12,
    eSktCloseSktWithError               = 13,
    eSktCloseCryptoNullData             = 14,
    eSktCloseCryptoInvalidLength        = 15,
    eSktCloseCryptoInvalidKey,
    eSktClosePktLengthInvalid,
    eSktCloseUdpCreate,
    eSktCloseUpnpStart,
    eSktCloseUpnpDone,
    eSktCloseMulticastListenDone,
    eSktCloseWebBrowser,
    eSktCloseImAliveTimeout,
    eSktClosePingTimeout,
    eSktCloseSktHandleInvalid,
    eSktCloseNetCmdLengthInvalid,
    eSktCloseNetCmdListInvalid,
    eSktCloseNetSrvUrlInvalid,
    eSktCloseNetSrvPluginInvalid,
    eSktCloseNetSrvQueryIdPermission,
    eSktCloseNetSrvQueryIdSent,
    eSktClosePtopUrlInvalid,
    eSktCloseHttpPluginInvalid,
    eSktCloseHttpHandleError,
    eSktCloseHttpNotAcceptSkt,
    eSktCloseHttpCloseNoError,
    eSktCloseHttpCloseWithError,
    eSktCloseHttpNoWebSrv,
    eSktCloseHttpSktDestroy,
    eSktCloseConnectReasonsEmpty,
    eSktCloseFindBigInfoFail,
    eSktCloseFindConnectedInfoFail,
    eSktClosePktAnnSendFail,
    eSktCloseToRelayPktAnnSendFail,
    eSktCloseThroughRelayPktAnnSendFail,
    eSktClosePktOnlineIdMeFromMyIp,
    eSktClosePktOnlineIdMeFromAnotherIp,
    eSktClosePktAnnNotFirstPacket,
    eSktClosePktAnnInvalid,
    eSktClosePktHdrInvalid,
    eSktClosePktAnnUpdateFailed,
    eSktClosePktPingReqSendFail,
    eSktCloseP2PNotReadyForAcceptSkt,
    eSktCloseUserIgnored,
    eSktCloseRelaySessionNotFound,
    eSktCloseNetSrvHostPingReplySent,
    eSktCloseTxFailed,
    eSktClosePktPingRequestIsFirstPkt,
    eSktCloseNetServiceHandled,
    eSktCloseBlockedUser,
    eSktCloseClosedByUser,
    
    // done with connection
    eSktCloseGroupAnnounce,
    eSktCloseGroupJoin,
    eSktCloseGroupLeave,
    eSktCloseGroupUnJoin,
    eSktCloseGroupSearch,
    eSktCloseGroupUserConnect,

    eSktCloseChatRoomAnnounce,
    eSktCloseChatRoomJoin,
    eSktCloseChatRoomLeave,
    eSktCloseChatRoomUnJoin,
    eSktCloseChatRoomSearch,
    eSktCloseChatRoomUserConnect,

    eSktCloseRandomConnectAnnounce,
    eSktCloseRandomConnectJoin,
    eSktCloseRandomConnectLeave,
    eSktCloseRandomConnectUnJoin,
    eSktCloseRandomConnectSearch,
    eSktCloseRandomConnectUserConnect,

    eSktCloseAnnouncePing,
    eSktCloseOtherSearch,
    eSktCloseNetworkHostListSearch,

    eSktCloseGroupHostedUserListSearch,
    eSktCloseChatRoomHostedUserListSearch,
    eSktCloseRandomConnectHostedUserListSearch,

    eSktCloseGroupGroupieUserListSearch,
    eSktCloseChatRoomGroupieUserListSearch,
    eSktCloseRandomConnectGroupieUserListSearch,

    eSktCloseUserRelayedConnect,
    eSktCloseUserDirectConnect,
    eSktCloseNetworkHost,
    eSktCloseConnectTest,

    eSktClosePeerInvite,

    eSktCloseNetServiceTimeout,

    eSktCloseWrongPktAnnSize,
    eSktCloseExternalIpNotDeterminedYet,
    eSktCloseNoRxEncryptionKey,

    eSktCloseHostLeave,

    eSktCloseInviteOnTempConnection,

    eMaxSktCloseReason			// always last
};

enum ESubCatagory
{
    eSubCatagoryUnspecified = 0,

    eSubCatagoryVideoOther,
    eSubCatagoryVideoPersonal,
    eSubCatagoryVideoXXX,
    eSubCatagoryVideoMovie,
    eSubCatagoryVideoMovieClip,
    eSubCatagoryVideoTvShow,
    eSubCatagoryVideo3d,
    eSubCatagoryVideoCam,

    eSubCatagoryAudioOther,
    eSubCatagoryAudioPersonal,
    eSubCatagoryAudioXXX,
    eSubCatagoryAudioMusic,
    eSubCatagoryAudioBook,
    eSubCatagoryAudioSoundClip,

    eSubCatagoryImageOther,
    eSubCatagoryImagePersonal,
    eSubCatagoryImageXXX,
    eSubCatagoryImagePictures,
    eSubCatagoryImageCovers,

    eSubCatagoryOtherPersonal,
    eSubCatagoryOtherXXX,
    eSubCatagoryOtherEBook,
    eSubCatagoryOtherComic,

    eMaxSubCatagory
};

enum ETodGameAction
{
    eTodGameActionNone,
    eTodGameActionChoiceDare,
    eTodGameActionDareAccepted,
    eTodGameActionDareRejected,
    eTodGameActionChoiceTruth,
    eTodGameActionTruthAccepted,
    eTodGameActionTruthRejected,

    eMaxTodGameAction // must be last
};

enum EUserViewType
{
    eUserViewTypeNone,
    eUserViewTypeEverybody,
    eUserViewTypeFriendsOnline,
    eUserViewTypeFriendsOffline,
    eUserViewTypeGroup,
    eUserViewTypeChatRoom,
    eUserViewTypeRandomConnect,
    eUserViewTypeIgnored,
    eUserViewTypeOnline,
    eUserViewTypeDirectConnect,
    eUserViewTypeOffline,

    eMaxUserViewType
};

enum EWebPageType
{
    eWebPageTypeNone = 0,
    eWebPageTypeAboutMe,
    eWebPageTypeStoryboard,

    eMaxWebPageType
};

enum EXferAction
{
    eXferActionNone = 0,
    eXferActionDownload,
    eXferActionUpload,
    eXferActionCancelXfer,

    eMaxXferAction
};

//============================================================================
// There is a qt translated version of these functions in GuiParams for the user interface
//============================================================================

ESktCloseReason ConnectReasonToCloseReason( enum EConnectReason connectReason );

const char* DescribeMediaModule( enum EMediaModule mediaModule );
const char* DescribeAge( enum EAgeType ageType );
const char* DescribeCommError( enum ECommErr commErr );

const char* DescribeConnectReason( enum EConnectReason connectReason );
bool         IsConnectReasonAnnounce( enum EConnectReason connectReason );
bool         IsConnectReasonJoin( enum EConnectReason connectReason );
bool         IsConnectReasonLeave( enum EConnectReason connectReason );
bool         IsConnectReasonUnJoin( enum EConnectReason connectReason );
bool         IsConnectReasonSearch( enum EConnectReason connectReason );
bool         IsConnectReasonTemporary( enum EConnectReason connectReason );

const char* DescribeConnectStatus( enum EConnectStatus connectStatus );
const char* DescribeConnectType( enum EConnectType connectType );

const char* DescribeDatabaseType( enum EDatabaseType );

const char* DescribeFriendRequest( enum EFriendRequestState friendRequest );
//! describe friend state
const char* DescribeFriendState( enum EFriendState friendState );
const char* DescribeGroupieViewType( enum EGroupieViewType groupieViewType );
const char* DescribeHackerLevel( enum EHackerLevel hackLevel );
const char* DescribeHackerReason( enum EHackerReason hackReason );
//! Host announce status as text
const char* DescribeHostAnnounceStatus( enum EHostAnnounceStatus hostStatus );
//! Host join status as text
const char* DescribeHostJoinStatus( enum EHostJoinStatus joinStatus );
//! Host search status as text
const char* DescribeHostSearchStatus( enum EHostSearchStatus searchStatus );

//! Host type as text
const char* DescribeHostType( enum EHostType hostType );

//! Internet Status as text
const char* DescribeInternetStatus( enum EInternetStatus internetStatus );
const char* DescribeJoinState( enum EJoinState joinState );
const char* DescribeModuleState( enum EModuleState moduleState );
const char* DescribeListAction( enum EListAction listAction );
//! Media error as text
const char* DescribeMediaError(enum EMediaError mediaError);
//! Media action as text
const char* DescribeMediaPlayerAction( enum EMediaPlayerAction playerAction );
//! Network action as text
const char* DescribeNetAction( enum ENetActionType netAction );
//! Network State as text
const char* DescribeNetworkState( enum ENetworkStateType networkStateType );
//! Net Available Status as text
const char* DescribeNetAvailStatus( enum ENetAvailStatus netAvailStatus );
//! Net Command type as text
const char* DescribeNetCmdType( enum ENetCmdType netCmdType );
//! Net Command Error as text
const char* DescribeNetCmdError( enum ENetCmdError netCmdError );
//! Notify enum as text
const char* DescribeNotifyType( enum ENotifyType notifyType );
//! Offer Action enum as text
const char* DescribeOfferAction( enum EOfferAction offerAction );
//! Offer Response as text
const char* DescribeOfferResponse( enum EOfferResponse offerResponse );
//! Offer Offer send state as text
const char* DescribeOfferSendState( enum EOfferSendState offerSendState );
//! Offer state as text
const char* DescribeOfferState( enum EOfferState offerState );
//! Offer type as text
const char* DescribeOfferType( enum EOfferType offerType );
//! Plugin Access as text
const char* DescribePluginAccess( enum EPluginAccess pluginAccess );
//! Describe plugin.. the first DescribePluginType is translated.. this one is not
const char* DescribePluginType( enum EPluginType pluginType );
//! Describe Direct Connect test state as text
const char* DescribePortOpenStatus( enum EIsPortOpenStatus ePortOpenStatus );
//! Describe walkie talkie push to talk status as text
const char* DescribePushToTalkStatus( enum EPushToTalkStatus pushToTalkStatus );
//! Describe connect by shaking phone ( or press simulate phone shake ) status as text
const char* DescribeRandomConnectStatus( enum ERandomConnectStatus ePortOpenStatus );
//! Describe relay error as text
const char* DescribeRelayError( enum ERelayErr relayError );
//! Describe run network test state as text
const char* DescribeRunTestStatus( enum ERunTestStatus eTestStatus );
//! Describe scan type
const char* DescribeScanType( enum EScanType scanType );
//! Describe search type
const char* DescribeSearchType( enum ESearchType searchType );
//! Describe sha1 generate result
const char* DescribeSha1GenResult( enum ESha1GenResult sha1GenerateResult );
//! Describe socket close reason
const char* DescribeSktCloseReason( enum ESktCloseReason closeReason );
//! Describe skt type
const char* DescribeSktType( enum ESktType sktType );
//! Describe skt type
const char* DescribeTodGameAction( enum ETodGameAction todGameAction );
//! Describe user view type
const char* DescribeUserViewType( enum EUserViewType sktType );

const char* DescribeWebPageType( enum EWebPageType webPageType );
const char* DescribeXferAction( enum EXferAction xferAction );


// for use in database mainly 
// If you add a plugin type be sure to update getPluginName
const char* GetPluginName( enum EPluginType pluginType );

//! convert Host Type to connect reason
EConnectReason HostTypeToConnectAnnounceReason( enum EHostType hostType );
//! convert Host Type to connect reason
EConnectReason HostTypeToConnectJoinReason( enum EHostType hostType );
//! convert Host Type to connect reason
EConnectReason HostTypeToConnectLeaveReason( enum EHostType hostType );
//! convert Host Type to connect reason
EConnectReason HostTypeToConnectUnJoinReason( enum EHostType hostType );
//! convert Host Type to connect reason
EConnectReason HostTypeToConnectSearchReason( enum EHostType hostType );

EHostType ConnectReasonToHostType( enum EConnectReason connectReason );
EHostType ConnectReasonToJoinHostType( enum EConnectReason connectReason );

//! convert Host Type to service plugin  type
EPluginType HostTypeToHostPlugin( enum EHostType hostType );
//! convert Host Type to client plugin type
EPluginType HostTypeToClientPlugin( enum EHostType hostType );
//! convert Host Type to user connect reason
EConnectReason HostTypeToGroupieConnectReason( enum EHostType hostType );
//! convert Plugin Type to host type
EHostType PluginTypeToHostType( enum EPluginType pluginType );
// if host plugin return its client plugin else return pluginType param
EPluginType HostPluginToClientPluginType( enum EPluginType pluginType );
// if client plugin return its host plugin else return pluginType param
EPluginType ClientPluginToHostPluginType( enum EPluginType pluginType );

//! return true if host can be announced to network or is a client of such a host
bool IsAnnounceHostOrClientPluginType( enum EPluginType pluginType );
//! return true if host can be announced to network or is a client of such a host
bool IsAnnounceHostOrClientHostType( enum EHostType hostType );

//! return true if is a client plugin
bool IsClientPluginType( enum EPluginType pluginType );
//! return true if view type is to show members of host
bool IsHostedMembersViewType( enum EUserViewType userViewType );
//! return true if is a host plugin
bool IsHostPluginType( enum EPluginType pluginType );
//! return true if is a host or client relationship plugin
bool IsHostOrClientPluginType( enum EPluginType pluginType );
//! return true if host can act as relay for user
bool IsHostARelayForUsers( enum EHostType hostType ); // also used to determine if is hosted type Group, ChatRoom or RandomConnect

//! return true if plugin should announce to network host
bool PluginShouldAnnounceToNetwork( enum EPluginType pluginType );
//! return true if host should announce to network host
bool HostShouldAnnounceToNetwork( enum EHostType hostType );

//! return true if plugin can act as relay for user
bool IsPluginARelayForUser( enum EPluginType pluginType );

//! return true if only one user can access at a time. example ePluginTypeVideoChat
bool IsPluginSingleSession( EPluginType pluginType );

