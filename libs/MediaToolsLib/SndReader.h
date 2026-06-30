#pragma once
//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "SndDefs.h"

#include <GuiInterface/IFromGui.h>
#include <PktLib/VxCommon.h>

#include <CoreLib/VxElapseTimer.h>
#include <CoreLib/VxMutex.h>
#include <CoreLib/MediaCallbackInterface.h>

#include <string>
#include <stdio.h>
#include <memory.h>

enum ESndPlayState
{
	eSndPlayStateDisabled,
	eSndPlayStateStopPlaying,
	eSndPlayStateStartPlaying,
	eSndPlayStateStartPlayInSeekPos,
	eSndPlayStatePausePlaying,
	eSndPlayStateResumePlaying,
	eSndPlayStateCancelPlaying,
	eSndPlayStateError,

	eMaxSndPlayState
};

class P2PEngine;
class MediaProcessor;
class IToGui;
class AssetInfo;
class OpusCodec;
class OpusFileDecoder;
class VFile;

class SndReader : public MediaCallbackInterface
{
public:
	SndReader( P2PEngine& engine, MediaProcessor& mediaProcessor );
	virtual ~SndReader();

	void						setIsPlaying( bool isPlaying )				{ m_IsPlaying = isPlaying; }
	bool						getIsPlaying( void )						{ return m_IsPlaying; }
	void						setIsPlayingPaused( bool isPaused );
	bool						getIsPlayingPaused( void )					{ return m_IsPlayingPaused; }


	bool						fromGuiIsNoLimitAudioFile( const char* fileName );	
	bool						fromGuiSndPlay( ESndPlayState ePlayState, VxGUID& assetId, int pos0to100000  );
	bool						fromGuiAssetAction( AssetBaseInfo& assetInfo, EAssetAction assetAction, int pos0to100000  );

	bool						startSndRead( const char* fileName, VxGUID& assetId, int pos0to100000 );
	void						stopSndRead( void );

protected:
	void						closeSndFile( void );

	//=== vars ===//
	P2PEngine&					m_Engine; 
	MediaProcessor&				m_MediaProcessor;	

	EPluginType					m_EPluginType{ ePluginTypeSndReader };
	bool						m_IsPlaying{ false };
	bool						m_IsPlayingPaused{ false };
	std::string					m_FileName;
	VxGUID						m_AssetId;

	VFile*						m_FileHandle{ nullptr };
	VxElapseTimer						m_PlayElapseTimer;
	double						m_TotalElapsedMs{ 0 };
	bool						m_IsFirstFrameAfterResumePlaying{ false };

	VxMutex						m_RecMutex;
	OpusCodec&					m_OpusCodec;
	OpusFileDecoder&			m_OpusFileDecoder;
};
