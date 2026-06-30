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

class P2PEngine;
class MediaProcessor;
class IToGui;
class AssetInfo;
class OpusCodec;
class OpusFileEncoder;

class SndWriter : public MediaCallbackInterface
{
public:
	SndWriter( P2PEngine& engine, MediaProcessor& mediaProcessor );
	virtual ~SndWriter();

	void						setIsRecording( bool isRecording )				{ m_IsRecording = isRecording; }
	bool						getIsRecording( void )							{ return m_IsRecording; }
	void						setIsRecordingPaused( bool isPaused );
	bool						getIsRecordingPaused( void )					{ return m_IsRecordingPaused; }


	bool						fromGuiSndRecord( ESndRecordState eRecState, VxGUID& feedId, const char* fileName  );
	bool						fromGuiAssetAction( AssetBaseInfo& assetInfo, EAssetAction assetAction, int pos0to100000 );
	virtual void				callbackOpusEncoded( uint8_t* encodedAudio, uint16_t opusLenBytes ) override;

	bool						startSndWrite( const char* fileName, bool beginInPausedState );
	void						stopSndWrite( void );

protected:
	bool						setSndParameters(	uint32_t imageWidth, 
													uint32_t imageHeight, 
													uint32_t microSecBetweenFrames, 
													uint32_t frameCnt, 
													uint32_t totalJpgDataLen );
	uint64_t					calculateRiffSize( uint32_t frameCnt, uint32_t totalJpgDataLen );
	bool						writeRiffHeader( void );
	bool						writeVideoHeader( void );
	void						closeSndFile( void );


	//=== vars ===//
	P2PEngine&					m_Engine; 
	MediaProcessor&				m_MediaProcessor;

	EPluginType					m_EPluginType;
	bool						m_IsRecording;
	bool						m_IsRecordingPaused;
	std::string					m_FileName;
	FILE *						m_FileHandle;
	int							m_MicroSecBetweenFrames;
	VxElapseTimer						m_RecordElapseTimer;
	double						m_TotalElapsedMs;
	bool						m_IsFirstFrameAfterResumeRecording;

	std::vector<uint32_t>		m_FrameOffsetList;
	VxMutex						m_RecMutex;
	OpusCodec&					m_OpusCodec;
	OpusFileEncoder&			m_OpusFileEncoder;
	VxGUID						m_MediaSessionId;
};
