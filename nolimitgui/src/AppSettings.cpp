//============================================================================
// Copyright (C) 2014 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppSettings.h"

#include <CoreLib/AppVersion.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxParse.h>

#include <QString>

namespace
{
	#define APP_SETTINGS_DBVERSION		1
	#define SELECTED_THEME_TYPE			"SELECTED_THEME_TYPE"
}

//============================================================================
AppSettings::AppSettings()
: VxSettings( "AppSettingsDb" )
{
}

//============================================================================
const char* AppSettings::getAppShortName( void )
{
	return APP_NAME;
}

//============================================================================
int32_t AppSettings::appSettingStartup( std::string dbSettingsFile )
{
	int32_t rc = dbStartup( APP_SETTINGS_DBVERSION, dbSettingsFile );
	if( 0 == rc )
	{
		m_AppSettingsInitialized = true;
	}

	return rc;
}

//============================================================================
void AppSettings::appSettingShutdown( void )
{
	dbShutdown();
}

//============================================================================
void AppSettings::setIsMessengerFullScreen( bool isFullScreen )
{
	setIniValue( "MessangerFullScreen", isFullScreen );
}

//============================================================================
bool AppSettings::getIsMessengerFullScreen( void )
{
	bool fullScreen;
	getIniValue( "MessangerFullScreen", fullScreen, false );
	return fullScreen;
}

//============================================================================
void AppSettings::setLastSelectedTheme( EThemeType selectedTheme )
{
	uint32_t themeType = (uint32_t)selectedTheme;
	setIniValue( SELECTED_THEME_TYPE, themeType );
}

//============================================================================
EThemeType AppSettings::getLastSelectedTheme( void )
{
	uint32_t themeType = 1;
	getIniValue( SELECTED_THEME_TYPE, themeType, (uint32_t)eThemeTypeDark );
	return (EThemeType)themeType;
}

//============================================================================
void AppSettings::setSelectedLanguage( ELanguageType selectedLanguage )
{
	uint32_t langType = (uint32_t)selectedLanguage;
	setIniValue( "SelectedLanguage", langType );
}

//============================================================================
ELanguageType AppSettings::getSelectedLanguage( void )
{
	uint32_t langType = (uint32_t)eLangEnglish;
	getIniValue( "SelectedLanguage", langType, (uint32_t)eLangEnglish );
	return (ELanguageType)langType;
}

//============================================================================
void AppSettings::setRandomConnectOfferType( EOfferType offerType )
{
	uint32_t offerTypeValue = (uint32_t)offerType;
	setIniValue( "RandomConnectOfferType", offerTypeValue );
}

//============================================================================
EOfferType AppSettings::getRandomConnectOfferType( void )
{
	uint32_t offerTypeValue = (uint32_t)eOfferTypeVideoChat;
	getIniValue( "RandomConnectOfferType", offerTypeValue, (uint32_t)eOfferTypeVideoChat );

	switch( (EOfferType)offerTypeValue )
	{
	case eOfferTypeTruthOrDare:
	case eOfferTypeVideoChat:
	case eOfferTypeVoicePhone:
		return (EOfferType)offerTypeValue;

	default:
		return eOfferTypeVideoChat;
	}
}

//============================================================================
void AppSettings::setLastBrowseDir( EFileFilterType eFileFilterType, std::string& browseDir )
{
	if( 0 != browseDir.length() )
	{
		QString keyStr = QString("BrowseDir%1").arg( (int)eFileFilterType );
		setIniValue( keyStr.toUtf8().constData(), browseDir );
	}
}

//============================================================================
void AppSettings::getLastBrowseDir( EFileFilterType eFileFilterType, std::string& browseDir )
{
	QString keyStr = QString("BrowseDir%1").arg( (int)eFileFilterType );
	getIniValue( keyStr.toUtf8().constData(), browseDir, "" );
}

//============================================================================
void AppSettings::setLastBrowseFilter( EFileFilterType fileFilterType )
{
	int32_t filterType = (int32_t)fileFilterType;
	setIniValue( "LastBrowseFilter", filterType );
}

//============================================================================
EFileFilterType AppSettings::getLastBrowseFilter( void )
{
	int32_t filterType = 0;
	getIniValue( "LastBrowseFilter", filterType, (int)eFileFilterAll );
	return (EFileFilterType)filterType;	
}

//============================================================================
void AppSettings::setLastFileOfferFilter( EFileFilterType fileFilterType )
{
	int32_t filterType = (int32_t)fileFilterType;
	setIniValue( "LastFileOfferFilter", filterType );
}

//============================================================================
EFileFilterType AppSettings::getLastFileOfferFilter( void )
{
	int32_t filterType = 0;
	getIniValue( "LastFileOfferFilter", filterType, (int)eFileFilterAll );
	return (EFileFilterType)filterType;	
}

//============================================================================
void AppSettings::setLastFileShareViewFilter( EFileFilterType fileFilterType )
{
	int32_t filterType = (int32_t)fileFilterType;
	setIniValue( "LastShareViewFilter", filterType );
}

//============================================================================
EFileFilterType AppSettings::getLastFileShareViewFilter( void )
{
	int32_t filterType = 0;
	getIniValue( "LastShareViewFilter", filterType, (int)eFileFilterAll );
	return (EFileFilterType)filterType;	
}

//============================================================================
void AppSettings::setLastLibraryFilter( EFileFilterType fileFilterType )
{
	int32_t filterType = (int32_t)fileFilterType;
	setIniValue( "LastLibraryFilter", filterType );
}

//============================================================================
EFileFilterType AppSettings::getLastLibraryFilter( void )
{
	int32_t filterType = 0;
	getIniValue( "LastLibraryFilter", filterType, (int)eFileFilterAll );
	return (EFileFilterType)filterType;	
}

//============================================================================
void AppSettings::setLastLibraryAudioDir( std::string& browseDir )
{
	setIniValue( "LibAudioDir", browseDir );
}

//============================================================================
void AppSettings::getLastLibraryAudioDir( std::string& browseDir )
{
	getIniValue( "LibAudioDir", browseDir, "" );
}

//============================================================================
void AppSettings::setLastLibraryImageDir( std::string& browseDir )
{
	setIniValue( "LibImageDir", browseDir );
}

//============================================================================
void AppSettings::getLastLibraryImageDir( std::string& browseDir )
{
	getIniValue( "LibImageDir", browseDir, "" );
}

//============================================================================
void AppSettings::setLastLibraryVideoDir( std::string& browseDir )
{
	setIniValue( "LibVideoDir", browseDir );
}

//============================================================================
void AppSettings::getLastLibraryVideoDir( std::string& browseDir )
{
	getIniValue( "LibVideoDir", browseDir, "" );
}

//============================================================================
void AppSettings::setLastGalleryDir( std::string& galleryDir )
{
	setIniValue( "GalleryDir", galleryDir );
}

//============================================================================
void AppSettings::getLastGalleryDir( std::string& galleryDir )
{
	getIniValue( "GalleryDir", galleryDir, "" );
}

//============================================================================
void AppSettings::setLastVideoFileDir( std::string& fileDir )
{
	setIniValue( "VideoFileDir", fileDir );
}

//============================================================================
void AppSettings::getLastVideoFileDir( std::string& fileDir )
{
	getIniValue( "VideoFileDir", fileDir, "" );
}

//============================================================================
void AppSettings::setLastAudioFileDir( std::string& fileDir )
{
	setIniValue( "AudioFileDir", fileDir );
}

//============================================================================
void AppSettings::getLastAudioFileDir( std::string& fileDir )
{
	getIniValue( "AudioFileDir", fileDir, "" );
}

//============================================================================
void AppSettings::setLastAddFileDir( std::string addFileDir )
{
	setIniValue( "AddFileDir", addFileDir );
}

//============================================================================
void AppSettings::getLastAddFileDir( std::string& addFileDir )
{
	getIniValue( "AddFileDir", addFileDir, "" );
}

//============================================================================
void AppSettings::setLastFolderScanDir( std::string addFileDir )
{
	setIniValue( "FolderScanDir", addFileDir );
}

//============================================================================
void AppSettings::getLastFolderScanDir( std::string& addFileDir )
{
	getIniValue( "FolderScanDir", addFileDir, "" );
}

//============================================================================
void AppSettings::setLastBrowseShareDir( std::string& browseDir )
{
	setIniValue( "ShareBrowseDir", browseDir );
}

//============================================================================
void AppSettings::getLastBrowseShareDir( std::string& browseDir )
{
	getIniValue( "ShareBrowseDir", browseDir, "" );
}

//============================================================================
void AppSettings::getLastHostSearchText( ESearchType searchType, std::string& searchText )
{
    getIniValue( getAppendedType("LastHostSearchText", searchType).c_str(), searchText, "" );
}

//============================================================================
void AppSettings::setLastHostSearchText( ESearchType searchType, std::string& searchText )
{
    setIniValue( getAppendedType("LastHostSearchText", searchType).c_str(), searchText );
}

//============================================================================
void AppSettings::setLastHostSearchAgeType( ESearchType searchType, EAgeType ageType )
{
    uint32_t val = (uint32_t)ageType;
    setIniValue( getAppendedType("LastHostSearchAge", searchType).c_str(), val );
}

//============================================================================
void AppSettings::getLastHostSearchAgeType( ESearchType searchType, EAgeType& ageType )
{
    uint32_t val = 0;
    getIniValue( getAppendedType("LastHostSearchAge", searchType).c_str(), val, 0 );
    ageType = (EAgeType)val;
}

//============================================================================
void AppSettings::setLastHostSearchGender( ESearchType searchType, EGenderType genderType )
{
    uint32_t val = (uint32_t)genderType;
    setIniValue( getAppendedType("LastHostSearchGender", searchType).c_str(), val );
}

//============================================================================
void AppSettings::getLastHostSearchGender( ESearchType searchType, EGenderType& genderType )
{
    uint32_t val = 0;
    getIniValue( getAppendedType("LastHostSearchGender", searchType).c_str(), val, 0 );
    genderType = (EGenderType)val;
}

//============================================================================
void AppSettings::setLastHostSearchLanguage( ESearchType searchType, ELanguageType languageType )
{
    uint32_t val = (uint32_t)languageType;
    setIniValue( getAppendedType("LastHostSearchLanguage", searchType).c_str(), val );
}

//============================================================================
void AppSettings::getLastHostSearchLanguage( ESearchType searchType, ELanguageType& languageType )
{
    uint32_t val = 0;
    getIniValue( getAppendedType("LastHostSearchLanguage", searchType).c_str(), val, 0 );
    languageType = (ELanguageType)val;
}

//============================================================================
void AppSettings::setLastHostSearchContentRating( ESearchType searchType, EContentRating contentRating )
{
    uint32_t val = (uint32_t)contentRating;
    setIniValue( getAppendedType("LastHostSearchRating", searchType).c_str(), val );
}

//============================================================================
void AppSettings::getLastHostSearchContentRating( ESearchType searchType, EContentRating& contentRating )
{
    uint32_t val = 0;
    getIniValue( getAppendedType("LastHostSearchRating", searchType).c_str(), val, 0 );
    contentRating = (EContentRating)val;
}

//============================================================================
void AppSettings::setCamRotation( std::string camId, uint32_t camRotation )
{
	std::string camKey;
	StdStringFormat( camKey, "CamRotation%s", camId.c_str() );
	setIniValue( camKey.c_str(), camRotation );
}

//============================================================================
uint32_t AppSettings::getCamRotation( std::string camId )
{
	uint32_t camRotation = 0;
	std::string camKey;
	StdStringFormat( camKey, "CamRotation%s", camId.c_str() );
	getIniValue( camKey.c_str(), camRotation, 0 );
	return camRotation;
}

//============================================================================
void AppSettings::setCamEnable( bool camEnableIn )
{
	uint32_t camEnable = camEnableIn ? 1 : 0;
	setIniValue( "CamEnable", camEnable );
}

//============================================================================
bool AppSettings::getCamEnable( void )
{
	uint32_t camEnable = 0;
	getIniValue( "CamEnable", camEnable, 1 );
	return camEnable ? true : false;
}

//============================================================================
void AppSettings::setCamSourceId( std::string camId )
{
	setIniValue( "CamSourceId", camId );
}

//============================================================================
std::string AppSettings::getCamSourceId( void )
{
	std::string camSourceId;
	getIniValue( "CamSourceId", camSourceId, "" );
	return camSourceId;
}

//============================================================================
void AppSettings::setCamShowPreview( bool showPreview )
{
	uint32_t showPreviewVal = showPreview ? 1 : 0;
	setIniValue( "CamShowPreview", showPreviewVal );
}

//============================================================================
bool AppSettings::getCamShowPreview( void )
{
	uint32_t showPreview = 0;
	getIniValue( "CamShowPreview", showPreview, 0 );
	return showPreview ? true : false;
}

//============================================================================
void AppSettings::setVidFeedRotation( uint32_t feedRotation )
{
	setIniValue( "VidFeedRotation", feedRotation );
}

//============================================================================
uint32_t AppSettings::getVidFeedRotation( void )
{
	uint32_t feedRotation = 0;
	getIniValue( "VidFeedRotation", feedRotation, 0 );
	return feedRotation;
}

//============================================================================
void AppSettings::setLastAppletLaunched( ELaunchFrame launchPage, EApplet applet )
{
    uint32_t appletVal = (EApplet)applet;
	const char* settingName = launchPage == eLaunchFrameHome ? "LastAppletLaunched1" : "LastAppletLaunched2";
    setIniValue( settingName, appletVal );
}

//============================================================================
EApplet AppSettings::getLastAppletLaunched( ELaunchFrame launchPage )
{
    uint32_t appletVal = 0;
	const char* settingName = launchPage == eLaunchFrameHome ? "LastAppletLaunched1" : "LastAppletLaunched2";
    getIniValue( settingName, appletVal, 0 );
    return (EApplet)appletVal;
}

//============================================================================
void AppSettings::setVerboseLog( bool verbose )
{
    uint32_t appletVal = verbose;
    setIniValue( "VerboseLog", appletVal );
}

//============================================================================
bool AppSettings::getVerboseLog( void )
{
    uint32_t appletVal = 0;
    getIniValue( "VerboseLog", appletVal, 0 );
    return (bool)appletVal;
}

//============================================================================
void AppSettings::setLogLevels( uint32_t logLevelFlags )
{
#if defined(DEBUG)
    setIniValue( "LogLevelsD", logLevelFlags );
#else
    setIniValue( "LogLevels", logLevelFlags );
#endif // defined(DEBUG)
}

//============================================================================
uint32_t AppSettings::getLogLevels( void )
{
    uint32_t logLevelFlags = 0;
#if defined(DEBUG)
    logLevelFlags = LOG_PRIORITY_MASK;	// LOG_VERBOSE
    getIniValue( "LogLevelsD", logLevelFlags, LOG_PRIORITY_MASK );
#else
    logLevelFlags = LOG_FATAL | LOG_SEVERE | LOG_ASSERT;
    getIniValue( "LogLevels", logLevelFlags, logLevelFlags );
#endif // defined(DEBUG)
    return logLevelFlags;
}

//============================================================================
void AppSettings::setLogModules( uint64_t logModuleFlags )
{
#if defined(DEBUG)
    setIniValue( "LogModulesD", logModuleFlags );
#else
    setIniValue( "LogModules", logModuleFlags );
#endif // defined(DEBUG)
}

//============================================================================
uint64_t AppSettings::getLogModules( void )
{
    uint64_t logModuleFlags = 0;
#if defined(DEBUG)
	uint64_t logModuleFlagsDefault = 0;
    getIniValue( "LogModulesD", logModuleFlags, logModuleFlagsDefault );
#else
    getIniValue( "LogModules", logModuleFlags, logModuleFlags );
#endif // defined(DEBUG)
    return logModuleFlags;
}

//============================================================================
bool AppSettings::getMutePhoneRing( void )
{
	bool bMute;
	getIniValue( "MutePhoneRing", bMute, false );

	return bMute;
}

//============================================================================
void AppSettings::setMutePhoneRing( bool bMutePhoneRing )
{
	setIniValue( "MutePhoneRing", bMutePhoneRing );
}

//============================================================================
bool AppSettings::getMuteNotifySound( void )
{
	bool bMute;
	getIniValue( "MuteNotifySound", bMute, false );

	return bMute;
}

//============================================================================
void AppSettings::setMuteNotifySound( bool bNotifySound )
{
	uint32_t u32Value = bNotifySound;
	setIniValue( "MuteNotifySound", u32Value );
}

//============================================================================
void AppSettings::setIsConfirmDeleteDisabled( bool confirmIsDisabled )
{
	uint32_t u32Value = confirmIsDisabled;
	setIniValue( "ConfirmDeleteDisable", u32Value );
}

//============================================================================
bool AppSettings::getIsConfirmDeleteDisabled( void )
{
	bool confirmIsDisabled;
	getIniValue( "ConfirmDeleteDisable", confirmIsDisabled, false );
	return confirmIsDisabled;
}

//============================================================================
std::string AppSettings::getAppendedType( const char* key, ESearchType searchType )
{
    std::string result;
    StdStringFormat( result, "%s%d", key, (int)searchType );
    return result;
}

//============================================================================
void AppSettings::setLastUsedTestUrl( std::string& testUrl )
{
    setIniValue( "LastTestUrl", testUrl );
}

//============================================================================
void AppSettings::getLastUsedTestUrl( std::string& testUrl )
{
    getIniValue( "LastTestUrl", testUrl, "" );
}

//============================================================================
void AppSettings::setFavoriteHostGroupUrl( std::string& hostUrl )
{
	setIniValue( "FavGroupHostUrl", hostUrl );
}

//============================================================================
void AppSettings::getFavoriteHostGroupUrl( std::string& hostUrl )
{
	getIniValue( "FavGroupHostUrl", hostUrl, "" );
}

//============================================================================
void AppSettings::setSoundInDevice( std::string deviceDescription )
{
	setIniValue( "SoundInDevice", deviceDescription );
}

//============================================================================
std::string AppSettings::getSoundInDevice( void )
{
	std::string deviceDescription;
	getIniValue( "SoundInDevice", deviceDescription, "" );
	return deviceDescription;
}

//============================================================================
void AppSettings::setSoundOutDevice( std::string deviceDescription )
{
	setIniValue( "SoundOutDevice", deviceDescription );
}

//============================================================================
std::string AppSettings::getSoundOutDevice( void )
{
	std::string deviceDescription;
	getIniValue( "SoundOutDevice", deviceDescription, "" );
	return deviceDescription;
}

//============================================================================
void AppSettings::setRunOnStartupCamServer( bool runOnStartup )
{
	setIniValue( "RunOnStartupCamServer", runOnStartup );
}

//============================================================================
bool AppSettings::getRunOnStartupCamServer( void )
{
	bool runOnStartup = false;
	getIniValue( "RunOnStartupCamServer", runOnStartup, false );
	return runOnStartup;
}

//============================================================================
void AppSettings::setRunOnStartupFileShareServer( bool runOnStartup )
{
	setIniValue( "RunOnStartupFileShareServer", runOnStartup );
}

//============================================================================
bool AppSettings::getRunOnStartupFileShareServer( void )
{
	bool runOnStartup = false;
	getIniValue( "RunOnStartupFileShareServer", runOnStartup, false );
	return runOnStartup;
}

//============================================================================
void AppSettings::setEchoDelayParam( int delayMs )
{
	uint32_t delayVal = (uint32_t)delayMs;
	setIniValue( "EchoDelayParam", delayVal );
}

//============================================================================
int AppSettings::getEchoDelayParam( void )
{
	uint32_t defaultParamVal = 140;
#if defined(TARGET_OS_ANDROID)
	defaultParamVal = 280;
#elif defined(TARGET_OS_WINDOWS) 
	defaultParamVal = 140;
#elif defined(TARGET_OS_LINUX) 
	defaultParamVal = 140;
#endif // defined(TARGET_OS_ANDROID)

	uint32_t paramVal;
	getIniValue( "EchoDelayParam", paramVal, defaultParamVal );
	return (int)paramVal;
}

//============================================================================
void AppSettings::setUseMilitaryTime( bool enable )
{
	setIniValue( "UseMilitaryTime", enable );
}

//============================================================================
bool AppSettings::getUseMilitaryTime( void )
{
	bool useMilitaryTime = false;
	getIniValue( "UseMilitaryTime", useMilitaryTime, false );
	return useMilitaryTime;
}

//============================================================================
void AppSettings::setMaxMessageHistory( int32_t maxHistory )
{
	setIniValue( "MaxMessageHistory", maxHistory );
}

//============================================================================
int32_t AppSettings::getMaxMessageHistory( void )
{
	int32_t maxMsgHistory = 0;
	getIniValue( "MaxMessageHistory", maxMsgHistory, 200 );
	return maxMsgHistory;
}

//============================================================================
void AppSettings::setWantMicrophone( bool enable )
{
	setIniValue( "WantMic", enable );
}

//============================================================================
bool AppSettings::getWantMicrophone( void )
{
	bool wantMick = false;
	getIniValue( "WantMic", wantMick, false );
	return wantMick;
}

//============================================================================
void AppSettings::setIsMicrophoneMuted( bool enable )
{
	setIniValue( "MuteMic", enable );
}

//============================================================================
bool AppSettings::getIsMicrophoneMuted( void )
{
	bool muteMic = false;
	getIniValue( "MuteMic", muteMic, false );
	return muteMic;
}

//============================================================================
void AppSettings::setWantSpeaker( bool enable )
{
	setIniValue( "WantSpeaker", enable );
}

//============================================================================
bool AppSettings::getWantSpeaker( void )
{
	bool wantSpeaker = false;
	getIniValue( "WantSpeaker", wantSpeaker, false );
	return wantSpeaker;
}

//============================================================================
void AppSettings::setIsSpeakerMuted( bool enable )
{
	setIniValue( "MuteSpeaker", enable );
}

//============================================================================
bool AppSettings::getIsSpeakerMuted( void )
{
	bool muteSpeaker = false;
	getIniValue( "MuteSpeaker", muteSpeaker, false );
	return muteSpeaker;
}

//============================================================================
void AppSettings::setNoAecLoopback( bool enable )
{
	setIniValue( "NoAecLoopback", enable );	
}

//============================================================================
bool AppSettings::getNoAecLoopback( void )
{
	bool enableAudioLoopback = false;
	getIniValue( "NoAecLoopback", enableAudioLoopback, false );
	return enableAudioLoopback;
}

//============================================================================
void AppSettings::setWithAecLoopback( bool enable )
{
	setIniValue( "WithAecLoopback", enable );	
}

//============================================================================
bool AppSettings::getWithAecLoopback( void )
{
	bool enableAudioLoopback = false;
	getIniValue( "WithAecLoopback", enableAudioLoopback, false );
	return enableAudioLoopback;
}

//============================================================================
void AppSettings::setLastUserConnectionsUserViewType( int comboIdx )
{
	uint32_t comboVal = (uint32_t)comboIdx;
	setIniValue( "UserConnectionsUserViewType", comboVal );
}

//============================================================================
int AppSettings::getLastUserConnectionsUserViewType( void )
{
	uint32_t comboVal = 0;
	getIniValue( "UserConnectionsUserViewType", comboVal, 1 );
	return comboVal;
}

//============================================================================
void AppSettings::setAppletEyeUsersVisible( EApplet applet, bool visible )
{
    uint32_t val = (uint32_t)visible;
    std::string key = std::string( "AppletEyeUsers" ) + std::to_string( (uint32_t)applet );
    setIniValue( key.c_str(), val );
}

//============================================================================
bool AppSettings::getAppletEyeUsersVisible( EApplet applet, bool defaultVal )
{
    uint32_t val = (uint32_t)defaultVal;
    std::string key = std::string( "AppletEyeUsers" ) + std::to_string( (uint32_t)applet );
    getIniValue( key.c_str(), val, (uint32_t)defaultVal );
    return (bool)val;
}

//============================================================================
void AppSettings::setAppletEyeSessionVisible( EApplet applet, bool visible )
{
    uint32_t val = (uint32_t)visible;
    std::string key = std::string( "AppletEyeSession" ) + std::to_string( (uint32_t)applet );
    setIniValue( key.c_str(), val );
}

//============================================================================
bool AppSettings::getAppletEyeSessionVisible( EApplet applet, bool defaultVal )
{
    uint32_t val = (uint32_t)defaultVal;
    std::string key = std::string( "AppletEyeSession" ) + std::to_string( (uint32_t)applet );
    getIniValue( key.c_str(), val, (uint32_t)defaultVal );
    return (bool)val;
}

//============================================================================
void AppSettings::setLastHostJoined( std::string lastJoinedHost )
{
	setIniValue( "LastHostJoined", lastJoinedHost );
}

//============================================================================
std::string AppSettings::getLastHostJoined( void )
{
	std::string lastHost;
	getIniValue( "LastHostJoined", lastHost, "" );
	return lastHost;
}

//============================================================================
void AppSettings::setLastHostJoined( EHostType hostType, std::string lastJoinedHost )
{
	bool valid{ false };
	std::string settingName{ "LastJoined" };
	switch( hostType )
	{
	case eHostTypeChatRoom:
		settingName += "ChatRoom";
		valid = true;
		break;
	case eHostTypeGroup:
		settingName += "Group";
		valid = true;
		break;
	case eHostTypeRandomConnect:
		settingName += "RandomConnect";
		valid = true;
		break;
	default:
		break;
	}

	if( valid )
	{
		setIniValue( settingName.c_str(), lastJoinedHost);
        setLastHostJoined( lastJoinedHost );
	}
	else
	{
		LogMsg( LOG_ERROR, "AppSettings::%s invalid host type", __func__ );
	}
}

//============================================================================
std::string AppSettings::getLastHostJoined( EHostType hostType )
{
	std::string lastHost;
	bool valid{ false };
	std::string settingName{ "LastJoined" };
	switch( hostType )
	{
	case eHostTypeChatRoom:
		settingName += "ChatRoom";
		valid = true;
		break;
	case eHostTypeGroup:
		settingName += "Group";
		valid = true;
		break;
	case eHostTypeRandomConnect:
		settingName += "RandomConnect";
		valid = true;
		break;
	default:
		break;
	}

	if( valid )
	{
		getIniValue( settingName.c_str(), lastHost, "" );
	}
	else
	{
		LogMsg( LOG_ERROR, "AppSettings::%s invalid host type", __func__ );
	}

	return lastHost;
}

//============================================================================
void AppSettings::setIsAutomatedHost( bool enable )
{
	setIniValue( "UnattendedHost", enable );
}

//============================================================================
bool AppSettings::getIsAutomatedHost( void )
{
	bool unattendedHost = false;
	getIniValue( "UnattendedHost", unattendedHost, false );
	return unattendedHost;
}

//============================================================================
void AppSettings::setAllowJoinMultipleHosts( bool enable )
{
	setIniValue( "AllowJoinMultipleHosts", enable );
}

//============================================================================
bool AppSettings::getAllowJoinMultipleHosts( void )
{
	bool allowJoinMultipleHosts = false;
	getIniValue( "AllowJoinMultipleHosts", allowJoinMultipleHosts, false );
	return allowJoinMultipleHosts;
}

//============================================================================
void AppSettings::setDisableAllSoundEffects( bool disable )
{
	m_DisableSoundEffectsValue = disable;
	m_DisableSoundEffectsCached = true;
	setIniValue( "DisableSoundEffect", disable );
}

//============================================================================
bool AppSettings::getDisableAllSoundEffects( void )
{
	if( m_DisableSoundEffectsCached )
	{
		return m_DisableSoundEffectsValue;
	}

	bool disableSoundEffects = false;
	getIniValue( "DisableSoundEffect", disableSoundEffects, false );
	m_DisableSoundEffectsValue = disableSoundEffects;
	m_DisableSoundEffectsCached = true;

	return disableSoundEffects;
}

//============================================================================
void AppSettings::setDisableSndTrash( bool disable )
{
	setIniValue( "DisableSndTrash", disable );
}

//============================================================================
bool AppSettings::getDisableSndTrash( void )
{
	bool disableSound = false;
	getIniValue( "DisableSndTrash", disableSound, false );
	return disableSound;
}

//============================================================================
void AppSettings::setDisableSndKeyClick( bool disable )
{
	setIniValue( "DisableSndKeyClick", disable );
}

//============================================================================
bool AppSettings::getDisableSndKeyClick( void )
{
	bool disableSound = false;
	getIniValue( "DisableSndKeyClick", disableSound, false );
	return disableSound;
}

//============================================================================
void AppSettings::setDisableSndNotify( bool disable )
{
	setIniValue( "DisableSndNotify", disable );
}

//============================================================================
bool AppSettings::getDisableSndNotify( void )
{
	bool disableSound = false;
	getIniValue( "DisableSndNotify", disableSound, false );
	return disableSound;
}

//============================================================================
void AppSettings::setDisableSndMsgRx( bool disable )
{
	setIniValue( "DisableSndMsgRx", disable );
}

//============================================================================
bool AppSettings::getDisableSndMsgRx( void )
{
	bool disableSound = false;
	getIniValue( "DisableSndMsgRx", disableSound, false );
	return disableSound;
}

//============================================================================
void AppSettings::setUseSystemMediaPlayer( bool useSystemPlayer )
{
	setIniValue( "UseSystemPlayer", useSystemPlayer );
}

//============================================================================
bool AppSettings::getUseSystemMediaPlayer( void )
{
	bool useSystemPlayer = false;
	getIniValue( "UseSystemPlayer", useSystemPlayer, false );

	return useSystemPlayer;
}

//============================================================================
void AppSettings::setLastPlayedMovie( std::string& movieFile )
{
    setIniValue( "LastPlayedMovie", movieFile );
}

//============================================================================
void AppSettings::getLastPlayedMovie( std::string& movieFile )
{
    getIniValue( "LastPlayedMovie", movieFile, "" );
}

//============================================================================
void AppSettings::setShowInWaveForm( bool show )
{
	setIniValue( "ShowInWaveForm", show );
}

//============================================================================
bool AppSettings::getShowInWaveForm( void )
{
	bool showInWaveForm = false;
	getIniValue( "ShowInWaveForm", showInWaveForm, true );
	return showInWaveForm;
}

//============================================================================
void AppSettings::setShowOutWaveForm(  bool show )
{
	setIniValue( "ShowOutWaveForm", show );
}

//============================================================================
bool AppSettings::getShowOutWaveForm( void )
{
	bool showOutWaveForm = false;
	getIniValue( "ShowOutWaveForm", showOutWaveForm, true );
	return showOutWaveForm;
}

//============================================================================
void AppSettings::setShowSoundOutSettings(  bool show )
{
	setIniValue( "ShowSoundOutSettings", show );
}

//============================================================================
bool AppSettings::getShowSoundOutSettings( void )
{
	bool showSoundOut = false;
	getIniValue( "ShowSoundOutSettings", showSoundOut, false );
	return showSoundOut;
}

//============================================================================
void AppSettings::setShowSoundInSettings( bool show )
{
	setIniValue( "ShowSoundInSettings", show );
}

//============================================================================
bool AppSettings::getShowSoundInSettings( void )
{
	bool showSoundIn = false;
	getIniValue( "ShowSoundInSettings", showSoundIn, false );
	return showSoundIn;
}

//============================================================================
void AppSettings::setShowSoundLog( bool show )
{
	setIniValue( "ShowSoundLog", show );
}

//============================================================================
bool AppSettings::getShowSoundLog( void )
{
	bool showSoundLog = false;
	getIniValue( "ShowSoundLog", showSoundLog, false );
	return showSoundLog;
}

//============================================================================
void AppSettings::setAgcEnabled( bool enabled )
{
	setIniValue( "AgcEnabled", enabled );
}

//============================================================================
bool AppSettings::getAgcEnabled( void )
{
	bool agcEnabled = false;
	getIniValue( "AgcEnabled", agcEnabled, false );
	return agcEnabled;
}

//============================================================================
void AppSettings::setNoiseSuppressionEnabled( bool enabled )
{
	setIniValue( "NoiseSuppressionEnabled", enabled );
}

//============================================================================
bool AppSettings::getNoiseSuppressionEnabled( void )
{
	bool noiseSuppressionEnabled = true;
	getIniValue( "NoiseSuppressionEnabled", noiseSuppressionEnabled, true );
	return noiseSuppressionEnabled;
}

//============================================================================
void AppSettings::setShowVoicePhoneInWaveForm( bool show )
{
	setIniValue( "ShowVoicePhoneInWaveForm", show );
}

//============================================================================
bool AppSettings::getShowVoicePhoneInWaveForm( void )
{
	bool showVoicePhoneInWaveForm = false;
	getIniValue( "ShowVoicePhoneInWaveForm", showVoicePhoneInWaveForm, false );
	return showVoicePhoneInWaveForm;
}

//============================================================================
void AppSettings::setShowVoicePhoneOutWaveForm( bool show )
{
	setIniValue( "ShowVoicePhoneOutWaveForm", show );
}

//============================================================================
bool AppSettings::getShowVoicePhoneOutWaveForm( void )
{
	bool showVoicePhoneOutWaveForm = false;
	getIniValue( "ShowVoicePhoneOutWaveForm", showVoicePhoneOutWaveForm, false );
	return showVoicePhoneOutWaveForm;
}