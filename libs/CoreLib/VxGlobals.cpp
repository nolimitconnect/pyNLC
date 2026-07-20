//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxGlobals.h"

#include "AppVersion.h"
#include "IGlobalDb.h"
#include "VxDebug.h"
#include "VxFileShredder.h"
#include "VxFileUtil.h"
#include "VxGUID.h"
#include "VxMutex.h"

#include <time.h>
#include <string>
#include <stdio.h>
#include <algorithm>

#ifndef TARGET_OS_WINDOWS
    #include <sys/time.h>
#endif // TARGET_OS_WINDOWS

//============================================================================
// globals
//============================================================================

namespace
{
	VxMutex				g_GlobalAccessMutex;

    std::string			g_strNetworkHostName            = "nolimitconnect.net";
    uint16_t            g_NetworkHostPort               = 45124;
    std::string			g_strNetworkHostUrl             = "ptop://nolimitconnect.net:45124";


	// exe and app resouces paths
	// qt paths
	std::string			g_strAppData					= "";


	// storage paths
	std::string			g_strRootDataStorageDir         = "";
	std::string			g_strAppTempDir                 = "";
	std::string			g_strAppLogsDir                 = "";
	std::string			g_strAppNoLimitDataDir          = "";
    std::string			g_strPlayerNlcDataDir           = "";

    std::string			g_strFontsDir					= "";

	// user specific writable paths
	std::string			g_strRootUserDataDir            = "";

	std::string			g_strUserSpecificDataDir        = "";
	std::string			g_strUserXferDir                = "";

	std::string			g_strRootXferDir				= "";
    std::string			g_strAboutMePageServerDir       = "";
	std::string			g_strAboutMePageClientDir		= "";
	std::string			g_strStoryBoardPageServerDir	= "";
	std::string			g_strStoryBoardPageClientDir	= "";
    std::string			g_strSettingsDir				= "";

	std::string			g_strUploadsDir					= "";
	std::string			g_strDownloadsDir				= "";
	std::string			g_strIncompleteDir				= "";
	std::string			g_strPersonalRecordDir			= "";
    std::string			g_strAppThumbsDir               = "";
    std::string			g_strAppCamRecord               = "";

	std::string			g_strTranslationsDir			= "";

	bool				g_bIsAppShuttingDown			= false;
	bool				g_bIsNetLoopbackAllowed			= false;

	bool				g_UserMilitaryTime				= true;
	int32_t				g_MaxMessageHistory				= 200;

    #if defined(_DEBUG) || defined(DEBUG) || defined(LOG_IN_RELEASE_BUILD)
		bool			g_bIsDebugEnabled				= true;
	#else
        // default to no logging in release mode
		bool			g_bIsDebugEnabled				= false;
	#endif //	_DEBUG

	bool				g_bShowMyselfInLists			= false;
	bool				g_bFastHostAnnounce				= false;
	bool				g_bCanDeleteUserFromDb 			= false;
}

//============================================================================

// directory structure on disk

// data storage paths linux      /home/user/.local/share/nolimitconnect
//                    windows    C:\Users\user\AppData\Roaming\NoLimitConnect
//                    android ?
// /storage/nolimitconnect/temp/		temporary files path
//                  /logs/		log files path
//                  /nolimit/	ShredFilesDb.db3 and app generated files
//					/fonts/		fonts for subtitiles directory
//
// user xfer directories      
//                  Documents Directory/NoLimitConnect/userId/downloads
//																/uploads		uploading directory
//																/incomplete		not yet completed downloads
//																/me/			personal recordings
//																/contacts/		contact assets
//																/camrecord/		web cam recordings

//============================================================================
void VxSetAppDirectory( enum EAppDir appDir, std::string setDir )
{
    if( !setDir.empty() )
    {
        switch (appDir)
        {
		// qt paths
		case eAppData:
            g_strAppData = setDir;
            break;

		// storage
        case eAppDirRootDataStorage:
            g_strRootDataStorageDir = setDir;
            break;
        case eAppDirAppTempData:
            g_strAppTempDir = setDir;
            break;
        case eAppDirAppLogs:
            g_strAppLogsDir = setDir;
            break;

        case eAppDirAppNoLimitData:
            g_strAppNoLimitDataDir = setDir;
            break;

        case eAppDirPlayerNlcData:
            g_strPlayerNlcDataDir = setDir;
            break;

        case eAppDirRootUserData:
            g_strRootUserDataDir = setDir;
            break;

        case eAppDirUserSpecific:
            g_strUserSpecificDataDir = setDir;
            break;
        case eAppDirSettings:
            g_strSettingsDir = setDir;
            break;
        case eAppDirAboutMePageServer:
            g_strAboutMePageServerDir = setDir;
            break;
		case eAppDirAboutMePageClient:
			g_strAboutMePageClientDir = setDir;
			break;
		case eAppDirStoryboardPageServer:
			g_strStoryBoardPageServerDir = setDir;
			break;
		case eAppDirStoryBoardPageClient:
			g_strStoryBoardPageClientDir = setDir;
			break;
        case eAppDirRootXfer:
            g_strRootXferDir = setDir;
            break;
        case eAppDirUserXfer:
            g_strUserXferDir = setDir;
            break;
        case eAppDirDownloads:
            g_strDownloadsDir = setDir;
            break;
        case eAppDirUploads:
            g_strUploadsDir = setDir;
            break;
        case eAppDirIncomplete:
            g_strIncompleteDir = setDir;
            break;
        case eAppDirPersonalRecords:
            g_strPersonalRecordDir = setDir;
            break;
        case eAppDirThumbs:
            g_strAppThumbsDir = setDir;
            break;
        case eAppDirCamRecord:
            g_strAppCamRecord = setDir;
            break;
        case eAppDirFonts:
            g_strFontsDir = setDir;
            break;
		case eAppDirTranslations:
			g_strTranslationsDir = setDir;
			break;
        default:
            LogMsg( LOG_ERROR, "VxSetAppDirectory invalid param %d", appDir);
        }
    }
    else
    {
        LogMsg( LOG_ERROR, "VxSetAppDirectory setDir is null");
    }
}

//============================================================================
std::string& VxGetAppDirectory( EAppDir appDir )
{
	switch (appDir)
	{
	// qt paths
	case eAppData:
        return g_strAppData;

	// storage
	case eAppDirRootDataStorage:
		return g_strRootDataStorageDir;
	case eAppDirAppTempData:
		return g_strAppTempDir;
	case eAppDirAppLogs:
		return g_strAppLogsDir;

	case eAppDirAppNoLimitData:
		return g_strAppNoLimitDataDir;
    case eAppDirPlayerNlcData:
        return g_strPlayerNlcDataDir;

	case eAppDirRootUserData:
		return g_strRootUserDataDir;

	case eAppDirUserSpecific:
		return g_strUserSpecificDataDir;
	case eAppDirSettings:
		return g_strSettingsDir;
	case eAppDirAboutMePageServer:
		return g_strAboutMePageServerDir;
	case eAppDirAboutMePageClient:
		return g_strAboutMePageClientDir;
	case eAppDirStoryboardPageServer:
		return g_strStoryBoardPageServerDir;
	case eAppDirStoryBoardPageClient:
		return g_strStoryBoardPageClientDir;
	case eAppDirRootXfer:
		return g_strRootXferDir;
	case eAppDirUserXfer:
		return g_strUserXferDir;
	case eAppDirDownloads:
		return g_strDownloadsDir;
	case eAppDirUploads:
		return g_strUploadsDir;
	case eAppDirIncomplete:
		return g_strIncompleteDir;
	case eAppDirPersonalRecords:
		return g_strPersonalRecordDir;
    case eAppDirThumbs:
        return g_strAppThumbsDir;
    case eAppDirCamRecord:
        return g_strAppCamRecord;
    case eAppDirFonts:
        return g_strFontsDir;
	case eAppDirTranslations:
		return g_strTranslationsDir;
    default:
        break;
	}

    LogMsg( LOG_ERROR, "VxGetAppDirectory ERROR No directory for %d", appDir );
static std::string emptyStr = "";
	return emptyStr;
}

//============================================================================
void VxSetAppIsShuttingDown( bool bIsShuttingDown )
{
	g_bIsAppShuttingDown = bIsShuttingDown;
}

//============================================================================
bool VxIsAppShuttingDown( void )
{
	return g_bIsAppShuttingDown;
}

//============================================================================
const char* VxGetCompanyDomain( void )
{
	return APP_DOMAIN_NAME;
}

//============================================================================
const char* VxGetOrginizationName( void )
{
	return ""; // there is no organization
}

//============================================================================
const char* VxGetCompanyWebsite( void )
{
	return APP_URL;
}

//============================================================================
const char* VxGetApplicationTitle( void )
{
	return APP_TITLE;
}

//============================================================================
const char* VxGetApplicationNameNoSpaces( void )
{
	return APP_NAME;
}

//============================================================================
const char* VxGetApplicationNameNoSpacesLowerCase( void )
{
    return APP_DOMAIN_NAME;
}

//============================================================================
uint16_t VxGetAppVersionShort( void )
{
	return ((uint16_t)APP_MAJOR_VERSION << 8) + APP_MINOR_VERSION;
}

//============================================================================
uint32_t VxGetAppVersionFull( void )
{
    return APP_VERSION_BINARY;
}

//============================================================================
const char* VxGetAppVersionString( void )
{
	return APP_VERSION;
}

//============================================================================
void VxSetDebugLoggingEnable( bool enableDebug )
{
	g_bIsDebugEnabled = enableDebug;
}

//============================================================================
bool VxGetDebugLoggingEnable( void )
{
	return g_bIsDebugEnabled;
}

//============================================================================
//=== miscellaneous ===//
//============================================================================

//============================================================================
//! set true if loop back is allowed ( default is false )
void VxSetNetworkLoopbackAllowed( bool bIsLoopbackAllowed )
{
    g_bIsNetLoopbackAllowed = bIsLoopbackAllowed;
}

//============================================================================
//! return true if loop back is allowed
bool VxIsNetworkLoopbackAllowed( void )
{
    return g_bIsNetLoopbackAllowed;
}

//============================================================================
void VxSetRootDataStorageDirectory(const char* rootDataDir)
{
	g_strRootDataStorageDir = rootDataDir;
	VxFileUtil::assureTrailingDirectorySlash( g_strRootDataStorageDir );
	VxFileUtil::makeDirectory( rootDataDir );

	g_strAppTempDir = g_strRootDataStorageDir + "temp/";
	VxFileUtil::makeDirectory( g_strAppTempDir.c_str() );

	g_strAppLogsDir = g_strRootDataStorageDir + "logs/";
	VxFileUtil::makeDirectory( g_strAppLogsDir.c_str() );

	g_strAppNoLimitDataDir = g_strRootDataStorageDir + "nolimit/";
	VxFileUtil::makeDirectory(g_strAppNoLimitDataDir.c_str());

    g_strPlayerNlcDataDir = g_strRootDataStorageDir + "playernlc/";
    VxFileUtil::makeDirectory( g_strPlayerNlcDataDir.c_str() );

	g_strFontsDir = g_strRootDataStorageDir + "fonts/";
	VxFileUtil::makeDirectory( g_strFontsDir.c_str());

	g_strTranslationsDir = g_strRootDataStorageDir + "translations/";
	VxFileUtil::makeDirectory( g_strTranslationsDir.c_str() );

	GetVxFileShredder().initShredder( g_strAppNoLimitDataDir );

    IGlobalDb::getIGlobalDb().initGlobalDb( g_strAppNoLimitDataDir );
}

//============================================================================
std::string& VxGetRootDataStorageDirectory(void) { return g_strRootDataStorageDir; }
std::string& VxGetAppTempDirectory(void) { return g_strAppTempDir; }
std::string& VxGetAppLogsDirectory(void) { return g_strAppLogsDir; }
std::string& VxGetAppNoLimitDataDirectory(void) { return g_strAppNoLimitDataDir; }
std::string& VxGetAppThumbnailDirectory(void) { return g_strAppThumbsDir; }

//============================================================================
void VxSetRootUserDataDirectory( const char* rootUserDataDir )
{
    // basically /storage/NoLimitConnect/
	g_strRootUserDataDir = rootUserDataDir;
	VxFileUtil::assureTrailingDirectorySlash( g_strRootUserDataDir );
	VxFileUtil::makeDirectory( g_strRootUserDataDir.c_str() );
}

//============================================================================
std::string& VxGetRootUserDataDirectory( void ) { return g_strRootUserDataDir; }

//============================================================================
void VxSetUserSpecificDataDirectory( const char* userDataDir  )
{ 
	g_strUserSpecificDataDir = userDataDir;
	VxFileUtil::makeDirectory( userDataDir );

	g_strSettingsDir = g_strUserSpecificDataDir + "settings/";
	VxFileUtil::makeDirectory( g_strSettingsDir.c_str() );

    g_strAboutMePageServerDir = g_strUserSpecificDataDir + "aboutmepage/";
    VxFileUtil::makeDirectory( g_strAboutMePageServerDir.c_str() );

	g_strStoryBoardPageServerDir = g_strUserSpecificDataDir + "storyboardpage/";
	VxFileUtil::makeDirectory( g_strStoryBoardPageServerDir.c_str() );
}

//============================================================================
std::string& VxGetUserSpecificDataDirectory( void )			{ return g_strUserSpecificDataDir; }
std::string& VxGetSettingsDirectory( void )					{ return g_strSettingsDir; }
std::string& VxGetFontDirectory( void )						{ return g_strFontsDir; }
std::string& VxGetAboutMePageServerDirectory( void )		{ return g_strAboutMePageServerDir; }
std::string& VxGetStoryBoardPageServerDirectory( void )		{ return g_strStoryBoardPageServerDir; }
std::string& VxGetTranslationsDirectory( void )				{ return g_strTranslationsDir; }

//============================================================================
std::string VxGetAboutMePageClientDirectory( VxGUID& onlineId )
{ 
	std::string clientDir = g_strAboutMePageClientDir + onlineId.toHexString();
	VxFileUtil::makeDirectory( clientDir.c_str() );

    return clientDir;
}

//============================================================================
std::string VxGetStoryBoardPageClientDirectory( VxGUID& onlineId )
{
	std::string clientDir = g_strStoryBoardPageClientDir + onlineId.toHexString();
	VxFileUtil::makeDirectory( clientDir.c_str() );

    return clientDir;
}

//============================================================================
void VxSetRootXferDirectory( const char* rootXferDir  )
{ 
	g_strRootXferDir = rootXferDir; 
	VxFileUtil::assureTrailingDirectorySlash( g_strRootXferDir );
	VxFileUtil::makeDirectory(g_strRootXferDir.c_str());
	std::string noLimitDir = g_strRootXferDir + "nolimit/";
	VxFileUtil::makeDirectory( noLimitDir.c_str() );
	g_strAppThumbsDir = noLimitDir + "thumbs/";
	VxFileUtil::makeDirectory( g_strAppThumbsDir.c_str() );
	g_strAboutMePageClientDir = noLimitDir + "aboutme_cache/";
	VxFileUtil::makeDirectory( g_strAboutMePageClientDir.c_str() );
	g_strStoryBoardPageClientDir = noLimitDir + "storyboard_cache/";
	VxFileUtil::makeDirectory( g_strStoryBoardPageClientDir.c_str() );
}

//============================================================================
std::string& VxGetRootXferDirectory( void ) { return g_strRootXferDir; }

//============================================================================
void VxSetUserXferDirectory( std::string userXferDir  )
{
	g_strUserXferDir		= userXferDir;
	VxFileUtil::makeDirectory( g_strUserXferDir.c_str() );

	g_strDownloadsDir		= g_strUserXferDir + "downloads/";
	VxFileUtil::makeDirectory( g_strDownloadsDir.c_str() );

	g_strUploadsDir			= g_strUserXferDir + "uploads/";
	VxFileUtil::makeDirectory( g_strUploadsDir.c_str() );

	g_strIncompleteDir		= g_strUserXferDir + "incomplete/";
	VxFileUtil::makeDirectory( g_strIncompleteDir.c_str() );

	g_strPersonalRecordDir	= g_strUserXferDir + "contacts/me/";
	VxFileUtil::makeDirectory( g_strPersonalRecordDir.c_str() );

    g_strAppCamRecord       = g_strUserXferDir + "camrecord/";
    VxFileUtil::makeDirectory( g_strAppCamRecord.c_str() );

}

//============================================================================
std::string& VxGetUserXferDirectory( void  ) { return g_strUserXferDir; }
std::string& VxGetDownloadsDirectory( void ) { return g_strDownloadsDir; }
std::string& VxGetUploadsDirectory( void ) { return g_strUploadsDir; }
std::string& VxGetIncompleteDirectory( void ) { return g_strIncompleteDir; }
std::string& VxGetPersonalRecordDirectory( void ) { return g_strPersonalRecordDir; }

//============================================================================
int VxGlobalAccessLock( void )
{
	return g_GlobalAccessMutex.lock();
}

//============================================================================
int VxGlobalAccessUnlock( void )
{
	return g_GlobalAccessMutex.unlock();
}

//============================================================================
void SetUseMilitaryTime( bool useMilitaryTime )
{
	g_UserMilitaryTime = useMilitaryTime;
}

//============================================================================
bool GetUseMilitaryTime( void )
{
	return g_UserMilitaryTime;
}

//============================================================================
void VxSetMaxMessageHistory( int16_t maxHistory )
{
	g_MaxMessageHistory = maxHistory;
}

//============================================================================
int32_t VxGetMaxMessageHistory( void )
{
	return g_MaxMessageHistory;
}

//============================================================================
void VxSetShowMyselfInLists( bool showMyself )
{
	g_bShowMyselfInLists = showMyself;
}

//============================================================================
bool VxGetShowMyselfInLists( void )
{
	return g_bShowMyselfInLists;
}

//============================================================================
void VxSetFastHostAnnounce( bool fastHostAnnounce )
{
	g_bFastHostAnnounce = fastHostAnnounce;
}

//============================================================================
bool VxGetFastHostAnnounce( void )
{
	return g_bFastHostAnnounce;
}

//============================================================================
void VxSetCanDeleteUserFromDb( bool canDeleteFromDb )
{
	g_bCanDeleteUserFromDb = canDeleteFromDb;
}

//============================================================================
bool VxGetCanDeleteUserFromDb( void )
{
	return g_bCanDeleteUserFromDb;
}

void							VxSetCanDeleteUserFromDb( bool canDeleteFromDb ); // NOT recommended.. (for debug only)
bool							VxGetCanDeleteUserFromDb( void );
