#pragma once
//============================================================================
// Copyright (C) 2014 Brett R. Jones
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

#include "AppDefs.h"
#include "AudioDefs.h"

#include <CoreLib/VxFileTypeMasks.h>
#include <CoreLib/VxSettings.h>

class AppSettings : public VxSettings
{
public:
	AppSettings();
	virtual ~AppSettings() = default;

	const char*					getAppShortName( void );

	int32_t						appSettingStartup( std::string dbSettingsFile );
	void						appSettingShutdown( void );

	bool						getIsAppSettingInitialized( void )			{ return m_AppSettingsInitialized; }

	void						setIsMessengerFullScreen( bool isFullScreen );
	bool						getIsMessengerFullScreen( void );

	void						setLastSelectedTheme( EThemeType selectedTheme );
	EThemeType					getLastSelectedTheme( void );

	void						setSelectedLanguage( ELanguageType selectedLanguage );
	ELanguageType				getSelectedLanguage( void );

	void					    setRandomConnectOfferType( EOfferType offerType );
	EOfferType				    getRandomConnectOfferType( void );

	void						setMutePhoneRing( bool bMutePhoneRing );
	bool						getMutePhoneRing( void );

	void						setMuteNotifySound( bool bMuteNotifySound );
	bool						getMuteNotifySound( void );

	void						setIsConfirmDeleteDisabled( bool confirmIsDisabled );
	bool						getIsConfirmDeleteDisabled( void );

	void						setLastAddFileDir( std::string addFileDir );
	void						getLastAddFileDir( std::string& addFileDir );

	void						setLastFolderScanDir( std::string addFileDir );
	void						getLastFolderScanDir( std::string& addFileDir );

	void						setLastBrowseShareDir( std::string& browseDir );
	void						getLastBrowseShareDir( std::string& browseDir );

	void						setLastBrowseDir( EFileFilterType fileFilterType, std::string& browseDir );
	void						getLastBrowseDir( EFileFilterType fileFilterType, std::string& browseDir );

	void						setLastBrowseFilter( EFileFilterType fileFilterType );
	EFileFilterType				getLastBrowseFilter( void );

	void						setLastFileOfferFilter( EFileFilterType fileFilterType );
	EFileFilterType				getLastFileOfferFilter( void );

	void						setLastFileShareViewFilter( EFileFilterType fileFilterType );
	EFileFilterType				getLastFileShareViewFilter( void );

	void						setLastLibraryFilter( EFileFilterType fileFilterType );
	EFileFilterType				getLastLibraryFilter( void );
	void						setLastLibraryAudioDir( std::string& browseDir );
	void						getLastLibraryAudioDir( std::string& browseDir );
	void						setLastLibraryImageDir( std::string& browseDir );
	void						getLastLibraryImageDir( std::string& browseDir );
	void						setLastLibraryVideoDir( std::string& browseDir );
	void						getLastLibraryVideoDir( std::string& browseDir );

	void						setLastGalleryDir( std::string& galleryDir );
	void						getLastGalleryDir( std::string& galleryDir );
	void						setLastVideoFileDir( std::string& fileDir );
	void						getLastVideoFileDir( std::string& fileDir );
	void						setLastAudioFileDir( std::string& fileDir );
	void						getLastAudioFileDir( std::string& fileDir );

    void						setLastHostSearchText( ESearchType searchType, std::string& searchText );
    void						getLastHostSearchText( ESearchType searchType, std::string& searchText );
    void						setLastHostSearchAgeType( ESearchType searchType, EAgeType ageType );
    void						getLastHostSearchAgeType( ESearchType searchType, EAgeType& ageType );
    void						setLastHostSearchGender( ESearchType searchType, EGenderType genderType );
    void						getLastHostSearchGender( ESearchType searchType, EGenderType& genderType );
    void						setLastHostSearchLanguage( ESearchType searchType, ELanguageType languageType );
    void						getLastHostSearchLanguage( ESearchType searchType, ELanguageType& languageType );
    void						setLastHostSearchContentRating( ESearchType searchType, EContentRating contentRating );
    void						getLastHostSearchContentRating( ESearchType searchType, EContentRating& contentRating );

	void						setCamEnable( bool camEnable );
	bool						getCamEnable( void );

	void						setCamSourceId( std::string camId );
	std::string					getCamSourceId( void );

	void						setCamShowPreview( bool showPreview );
	bool						getCamShowPreview( void );

	void						setCamRotation( std::string camId, uint32_t camRotation );
	uint32_t					getCamRotation( std::string camId );

	void						setVidFeedRotation( uint32_t feedRotation );
	uint32_t					getVidFeedRotation( void );

    void						setLastAppletLaunched( ELaunchFrame launchPage, EApplet applet );
    EApplet						getLastAppletLaunched( ELaunchFrame launchPage );

    void						setLastUsedTestUrl( std::string& testUrl );
    void						getLastUsedTestUrl( std::string& testUrl );

    void                        setVerboseLog( bool verbose );
    bool                        getVerboseLog( void );

    void                        setLogLevels( uint32_t logLevelFlags );
    uint32_t                    getLogLevels( void );

    void                        setLogModules( uint64_t logModuleFlags );
    uint64_t                    getLogModules( void );

	void						setFavoriteHostGroupUrl( std::string& hostUrl );
	void						getFavoriteHostGroupUrl( std::string& hostUrl );

	void						setRunOnStartupCamServer( bool runOnStartup );
	bool						getRunOnStartupCamServer( void );

	void						setRunOnStartupFileShareServer( bool runOnStartup );
	bool						getRunOnStartupFileShareServer( void );

	void						setUseMilitaryTime( bool enable );
	bool						getUseMilitaryTime( void );

	void						setMaxMessageHistory( int32_t maxHistory );
	int32_t						getMaxMessageHistory( void );

	void						setLastUserConnectionsUserViewType( int comboIdx );
	int							getLastUserConnectionsUserViewType( void );

	void						setAppletEyeUsersVisible( EApplet applet, bool visible );
	bool						getAppletEyeUsersVisible( EApplet applet, bool defaultVal = true );

	void						setAppletEyeSessionVisible( EApplet applet, bool visible );
	bool						getAppletEyeSessionVisible( EApplet applet, bool defaultVal = true );

	void						setLastHostJoined( std::string lastJoinedHost );
	std::string					getLastHostJoined( void );

	void						setLastHostJoined( EHostType hostType, std::string lastJoinedHost );
	std::string					getLastHostJoined( EHostType hostType );

	void						setIsAutomatedHost( bool enable );
	bool						getIsAutomatedHost( void );

	void						setAllowJoinMultipleHosts( bool enable );
	bool						getAllowJoinMultipleHosts( void );

	void						setUseSystemMediaPlayer( bool useSystemPlayer );
	bool						getUseSystemMediaPlayer( void );

	void						setLastPlayedMovie( std::string& movieFile );
	void						getLastPlayedMovie( std::string& movieFile );

	void						setDisableAllSoundEffects( bool disable );
	bool						getDisableAllSoundEffects( void );

	void						setDisableSndTrash( bool disable );
	bool						getDisableSndTrash( void );

	void						setDisableSndKeyClick( bool disable );
	bool						getDisableSndKeyClick( void );

	void						setDisableSndNotify( bool disable );
	bool						getDisableSndNotify( void );

	void						setDisableSndMsgRx( bool disable );
	bool						getDisableSndMsgRx( void );

	void						setSoundInDevice( std::string deviceDescription );
	std::string					getSoundInDevice( void );

	void						setSoundOutDevice( std::string deviceDescription );
	std::string					getSoundOutDevice( void );

	void						setWantMicrophone( bool enable );
	bool						getWantMicrophone( void );

	void						setIsMicrophoneMuted( bool enable );
	bool						getIsMicrophoneMuted( void );

	void						setWantSpeaker( bool enable );
	bool						getWantSpeaker( void );

	void						setIsSpeakerMuted( bool enable );
	bool						getIsSpeakerMuted( void );

	void						setNoAecLoopback( bool enable );
	bool						getNoAecLoopback( void );

	void						setWithAecLoopback( bool enable );
	bool						getWithAecLoopback( void );

	void						setEnableNetworkLoopback( bool enable );
	bool						getEnableNetworkLoopback( void );

	void						setWantAudioIn( bool want );
	bool						getWantAudioIn( void );

	void						setWantAudioOut( bool want );
	bool						getWantAudioOut( void );

	void						setAgcEnabled( bool enabled );
	bool						getAgcEnabled( void );

	void						setNoiseSuppressionEnabled( bool enabled );
	bool						getNoiseSuppressionEnabled( void );

    void						setEchoDelayParam( int delayMs );
    int							getEchoDelayParam( void );

	void						setShowInWaveForm( bool show );
	bool						getShowInWaveForm( void );

	void						setShowOutWaveForm( bool show );
	bool						getShowOutWaveForm( void );

	void						setShowSoundInSettings( bool show );
	bool						getShowSoundInSettings( void );

	void						setShowSoundOutSettings( bool show );
	bool						getShowSoundOutSettings( void );
	
	void						setShowSoundLog( bool show );
	bool						getShowSoundLog( void );

	void						setShowVoicePhoneInWaveForm( bool show );
	bool						getShowVoicePhoneInWaveForm( void );

	void						setShowVoicePhoneOutWaveForm( bool show );
	bool						getShowVoicePhoneOutWaveForm( void );

protected:
    std::string                 getAppendedType( const char* key, ESearchType searchType );

	bool						m_AppSettingsInitialized{ false };

	bool						m_DisableSoundEffectsCached{ false };
	bool						m_DisableSoundEffectsValue{ false };

};

