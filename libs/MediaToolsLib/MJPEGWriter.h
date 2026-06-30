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

#include "AviDefs.h"

#include <GuiInterface/IFromGui.h>
#include <PktLib/VxCommon.h>

#include <CoreLib/VxElapseTimer.h>
#include <CoreLib/VxMutex.h>
#include <CoreLib/MediaCallbackInterface.h>

#include <stdio.h>
#include <string>

class IToGui;
class MediaProcessor;
class P2PEngine;
class VFile;

class MJPEGWriter : public MediaCallbackInterface
{
public:
	MJPEGWriter( P2PEngine& engine, MediaProcessor& mediaProcessor );
	virtual ~MJPEGWriter() = default;

	void						setIsRecording( bool isRecording )				{ m_IsRecording = isRecording; }
	bool						getIsRecording( void )							{ return m_IsRecording; }
	void						setIsRecordingPaused( bool isPaused );
	bool						getIsRecordingPaused( void )					{ return m_IsRecordingPaused; }


	bool						fromGuiVideoRecord( EVideoRecordState eRecState, VxGUID& feedId, const char* fileName  );
	virtual void				callbackVideoJpg( VxGUID& feedId, std::shared_ptr<CamJpgVideo>& jpgVideo ) override;
	virtual void				callbackPcm( VxGUID& feedId, int16_t * pcmData, uint16_t pcmDataLen ) override;

	bool						startAviWrite( const char* fileName, uint32_t MicroSecBetweenFrames, bool beginInPausedState );
	void						stopAviWrite( bool deleteFile = false );

protected:
	bool						setAviParameters(	uint32_t imageWidth, 
													uint32_t imageHeight, 
													uint32_t microSecBetweenFrames, 
													uint32_t frameCnt, 
													uint32_t totalJpgDataLen );
	bool						writeRiffHeader( void );
	bool						writeVideoHeader( void );
	void						closeAviFile( void );


	//=== vars ===//
	P2PEngine&					m_Engine; 
	MediaProcessor&				m_MediaProcessor;
	
	VxGUID						m_FeedId;

	EPluginType					m_EPluginType;
	bool						m_IsRecording;
	bool						m_IsRecordingPaused;
	std::string					m_FileName;
	VFile*						m_FileHandle{ nullptr };
	int							m_MicroSecBetweenFrames;
	uint32_t					m_TotalJpgDataLen;
	uint32_t					m_TotalFrameCnt;
	VxElapseTimer						m_RecordElapseTimer;
	double						m_TotalElapsedMs;
	bool						m_IsFirstFrameAfterResumeRecording;

	AviRiffHeader				m_RiffHdr;
	AviVideoHdr					m_AviHdr;
	std::vector<uint32_t>		m_FrameOffsetList;
	uint32_t					m_PrevFrameJpgLen;
	VxMutex						m_RecMutex;
	int							m_ImageWidth;
	int							m_ImageHeight;
	AviJpgHdr					m_AviJpgHdr;
	std::vector<uint32_t>		m_PcmOffsetList;
	AviAudioHdr					m_AviAudioHdr;
	VxMutex						m_AviFileAccessMutex;
	VxGUID						m_MediaSessionId;
};
