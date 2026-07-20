//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "MediaProcessor.h"

#include "AudioUtil.h"
#include "MediaClient.h"

#include <P2PEngine/P2PEngine.h>
#include <GuiInterface/IToGui.h>

#include <Plugins/PluginBase.h>
#include <Plugins/PluginMgr.h>

#if defined(USE_LIBJPEG_TURBO)
#include <libjpeg-turbo/VxJpgLib.h>
#else
#include <libjpg/VxJpgLib.h>
#endif // defined(USE_LIBJPEG_TURBO)

#include <VxVideoLib/VxVideoLib.h>
#include <VxVideoLib/VxRescaleRgb.h>

#include <MediaToolsLib/MediaTools.h>
#include <opus/OpusCodec.h>

#include <PktLib/PktsVideoFeed.h>
#include <PktLib/PktVoiceReq.h>
#include <PktLib/PktVoiceReply.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxElapseTimer.h>
#include <CoreLib/StdMinMaxForWindows.h>

#include <stdlib.h>

#include <algorithm> 

// requried due to some wierd microsoft std::min / max issue
using namespace std;

//#define DEBUG_AUDIO_PROCESSOR_LOCK 1

namespace
{
	const int VIDEO_DATA_BYTE_CNT					    = (320*240*3);
	const int VIDEO_MAX_MOTION_VALUE				    = 100000;
	const int MAX_PIC_PKT_DATA_PAYLOAD					= MAX_PKT_LEN - ( sizeof(PktVideoFeedPic) + 16 );

	//============================================================================
    static void * AudioInProcessThreadFunc( void * pvContext )
	{
		VxThread* poThread = (VxThread*)pvContext;
		poThread->setIsThreadRunning( true );
		MediaProcessor * processor = (MediaProcessor *)poThread->getThreadUserParam();
        if( processor )
        {
            processor->processAudioInThreaded();
        }

		poThread->threadAboutToExit();
        return nullptr;
	}
};

//============================================================================
MediaProcessor::MediaProcessor( P2PEngine& engine )
: m_Engine( engine )
, m_MediaTools( * ( new MediaTools( engine, *this ) ) )
, m_OpusCodec( AUDIO_DEVICE_SAMPLE_RATE, AUDIO_CHANNELS )
{
	memset( m_QuietAudioBuf, 0, sizeof( m_QuietAudioBuf ) );
	memset( m_MixerBuf, 0, sizeof( m_MixerBuf ) );
	
	m_PktVideoFeedPic = PktVideoFeedPic::allocateNewWithMaximumSizeAJpgCanBe();
	m_PktVideoFeedPic->setWidth( 320 );
	m_PktVideoFeedPic->setHeight( 240 );
	m_PktVideoFeedPic->setBitsPerPixel( 24 );
	m_PktVideoFeedPic->setPicType( 1 );

	int maxJpgSize = VIDEO_DATA_BYTE_CNT + 1024; // JPG header should only be less that 32 bytes but just in case because is variable and compression may be poor
	int maxChunkPayloadNeeded = maxJpgSize - MAX_PIC_PKT_DATA_PAYLOAD;
	int maxPicChunkPayload = MAX_PIC_CHUNK_LEN;
	int maxPicChuncsRequired = maxChunkPayloadNeeded / maxPicChunkPayload + ((maxChunkPayloadNeeded % maxPicChunkPayload) ? 1 : 0);
	for( int i = 0; i < maxPicChuncsRequired; i++ )
	{
		m_VidChunkList.emplace_back( new PktVideoFeedPicChunk() );
	}

	m_ProcessAudioInThread.startThread( (VX_THREAD_FUNCTION_T)AudioInProcessThreadFunc, this, "AudioInProcessor" );
}

//============================================================================
MediaProcessor::~MediaProcessor()
{
	delete &m_MediaTools;
	delete m_PktVideoFeedPic;
	std::vector<PktVideoFeedPicChunk *>::iterator iter;
	for( iter = m_VidChunkList.begin(); iter != m_VidChunkList.end(); ++iter )
	{
		delete *iter;
	}
}

//============================================================================
void MediaProcessor::shutdownMediaProcessor( void )
{
	m_ProcessAudioInThread.abortThreadRun( true );
	m_AudioInSemaphore.signal();
}

//============================================================================
void MediaProcessor::playAudio( int16_t * pcmData, int dataLenInBytes )
{
	if( AUDIO_BUF_SIZE == dataLenInBytes )
	{
		#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "MediaProcessor::playAudio m_MixerBufferMutex.lock()" );
		#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		m_MixerBufferMutex.lock();
		if( m_MixerBufUsed )
		{
			// data already exists in buffer.. do mixing
			AudioUtil::mixPcmAudio( pcmData, m_MixerBuf, AUDIO_SAMPLES_PER_FRAME );
		}
		else
		{
			memcpy( m_MixerBuf, pcmData, dataLenInBytes );
			m_MixerBufUsed = true;
		}
		
		#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "MediaProcessor::playAudio m_MixerBufferMutex.unlock()" );
		#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		m_MixerBufferMutex.unlock();
	}
	else
	{
		LogMsg( LOG_ERROR, "MediaProcessor::playAudio wrong length %d", dataLenInBytes );
	}
}

//============================================================================
void MediaProcessor::processAudioInThreaded( void )
{
	while( false == m_ProcessAudioInThread.isAborted() )
	{
		m_AudioInSemaphore.wait();
		if( m_ProcessAudioInThread.isAborted() )
		{
			LogMsg( LOG_INFO, "MediaProcessor::processAudioIn aborting1" );
			break;
		}

		while( m_ProcessAudioQue.size() )
		{
			m_AudioQueInMutex.lock();
			AudioPcmData audioPcmData = std::move( m_ProcessAudioQue[0] );
			m_ProcessAudioQue.erase( m_ProcessAudioQue.begin() );
			m_AudioQueInMutex.unlock();

			processRawAudioIn( audioPcmData );

			if( m_ProcessAudioInThread.isAborted() )
			{
				LogMsg( LOG_INFO, "MediaProcessor::processAudioIn aborting2" );
				break;
			}
		}

		if( m_ProcessAudioInThread.isAborted() )
		{
			LogMsg( LOG_INFO, "MediaProcessor::processAudioIn aborting3" );
			break;
		}
	}
		
	LogMsg( LOG_INFO, "MediaProcessor::processAudioIn leaving function" );
}

//============================================================================
void MediaProcessor::processRawAudioIn( const AudioPcmData& audioPcmData )
{
	int16_t* pcmData			= const_cast<int16_t*>( audioPcmData.getPcmData() );
	uint16_t pcmDataLen			= static_cast<uint16_t>( audioPcmData.getSampleCnt() * AUDIO_BYTES_PER_SAMPLE );
	uint64_t frameTag			= audioPcmData.getFrameTag();
	EMediaModule sourceModule	= audioPcmData.getSourceModule();
	// PCM data len = 60ms of sound in bytes
	// it seams that microphone volume is a bit low.. especially on android so increase volume before processing
    // TODO microphone boost

	if( m_AudioPcmList.size() )
	{
		#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "processRawAudioIn m_AudioPcmList AudioProcessorLock" );
		#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		AudioProcessorLock mgrMutexLock( this );
		doAudioClientRemovals( m_AudioClientRemoveList );
		#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "processRawAudioIn m_AudioPcmList begin" );
		#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		for( auto& client : m_AudioPcmList )
		{
			client.m_Callback->callbackPcm( m_Engine.getMyOnlineId(), pcmData, pcmDataLen );
		}

		#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "processRawAudioIn m_AudioPcmList done" );
		#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	}

	if( m_AudioOpusList.size() || m_AudioPktsList.size() )
	{
		int encodedLenBytes = m_OpusCodec.encode( pcmData, pcmDataLen / AUDIO_BYTES_PER_SAMPLE, m_PktVoiceReq.getCompressedData(), m_PktVoiceReq.getMaxCompressedDataBufLen() );
		if( encodedLenBytes <= 0 )
		{
			LogMsg( LOG_ERROR, "MediaProcessor::processRawAudioIn opus encode failed %d tag=%llu module=%d", encodedLenBytes, static_cast<unsigned long long>( frameTag ), static_cast<int>( sourceModule ) );
			return;
		}

		uint16_t opusLenBytes = static_cast<uint16_t>( encodedLenBytes );

		if( m_AudioOpusList.size() )
		{
			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "processRawAudioIn m_AudioOpusList AudioProcessorLock" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
			AudioProcessorLock mgrMutexLock( this );
			doAudioClientRemovals( m_AudioClientRemoveList );
			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
						LogMsg( LOG_INFO, "processRawAudioIn m_AudioOpusList start" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK

			for( auto& client : m_AudioOpusList )
			{
				client.m_Callback->callbackOpusEncoded( m_PktVoiceReq.getCompressedData(), opusLenBytes );
			}

			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
				LogMsg( LOG_INFO, "processRawAudioIn m_AudioOpusList done" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		}

		if( m_AudioPktsList.size() )
		{
			m_PktVoiceReq.setCompressedDataLen( opusLenBytes );
			m_PktVoiceReq.setTimeMs( GetGmtTimeMs() );
			m_PktVoiceReq.calcPktLen();

			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
				LogMsg( LOG_INFO, "processRawAudioIn m_AudioPktsList AudioProcessorLock callbackOpusPkt" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
			AudioProcessorLock mgrMutexLock( this );
			doAudioClientRemovals( m_AudioClientRemoveList );
			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
				LogMsg( LOG_INFO, "processRawAudioIn m_AudioPktsList start" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
			for( auto& client : m_AudioPktsList )
			{
				client.m_Callback->callbackOpusPkt( &m_PktVoiceReq );
			}

			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
				LogMsg( LOG_INFO, "processRawAudioIn m_AudioPktsList done" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		}
	}
}

//============================================================================
void MediaProcessor::increasePcmSampleVolume( int16_t * pcmData, uint16_t pcmDataLen, float volumePercent0To100 )
{
	int pcmSamples = pcmDataLen >> 1;
	float volMultiplier = 1.0f +  ( volumePercent0To100 / 100.0f );
	int sndVal;
	for( int i = 0; i < pcmSamples; i++ )
	{
		sndVal = (int)(( float )pcmData[i] * volMultiplier);
		if( sndVal > S16_MAXVAL )
		{
			pcmData[i] = S16_MAXVAL;
		}
		else if( sndVal < S16_MINVAL )
		{
			pcmData[i] = S16_MINVAL;
		}
		else
		{
			pcmData[i] = (int16_t)sndVal;
		}
	}
}

//============================================================================
void MediaProcessor::processFriendAudioFeed( VxGUID& onlineId, int16_t * pcmData, uint16_t pcmDataLen, bool dontLock )
{
	if( m_AudioPcmList.size() )
	{
		if( !dontLock )
		{
			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
				LogMsg( LOG_INFO, "processFriendAudioFeed m_AudioMutex.lock()" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
			m_AudioMutex.lock();
			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
				LogMsg( LOG_INFO, "processFriendAudioFeed begin" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		}

		for( auto& client : m_AudioPcmList )
		{
			client.m_Callback->callbackPcm( onlineId, pcmData, pcmDataLen );
		}

		if( !dontLock )
		{
			#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
				LogMsg( LOG_INFO, "processFriendAudioFeed m_AudioMutex.unlock()" );
			#endif // DEBUG_AUDIO_PROCESSOR_LOCK
			m_AudioMutex.unlock();
		}
	}
}

//============================================================================
void MediaProcessor::processCamCaptureJpgVideo( std::shared_ptr<CamJpgVideo>& jpgVideo )
{
	if( false == m_VidCaptureEnabled )
	{
		// nobody wants it 
		//LogMsg( LOG_WARN, "MediaProcessor::%s not enabled %d", __func__, GetApplicationAliveMs() );
		return;
	}

	sendCamPackets( jpgVideo );

    sendJpgVideo( m_Engine.getMyOnlineId(), jpgVideo );
}

//============================================================================
void MediaProcessor::sendCamPackets( std::shared_ptr<CamJpgVideo>& jpgVideo )
{
	if( !m_VideoPktsList.size() )
	{
		return; // no one to send to;
	}

	uint8_t* jpgData = jpgVideo->m_VidData.get();
	int32_t s32JpgDataLen = jpgVideo->m_VidDataLen;

	int32_t picPktDataLen		= s32JpgDataLen > MAX_PIC_PKT_DATA_PAYLOAD ? MAX_PIC_PKT_DATA_PAYLOAD : s32JpgDataLen;
	int32_t dataOverflow		= s32JpgDataLen > MAX_PIC_PKT_DATA_PAYLOAD ? s32JpgDataLen - MAX_PIC_PKT_DATA_PAYLOAD : 0;
	int32_t chunkPktsRequired	= dataOverflow / MAX_PIC_CHUNK_LEN + ((dataOverflow % MAX_PIC_CHUNK_LEN)?1:0);

	#ifdef DEBUG_PROCESSOR_LOCK
	LogMsg( LOG_INFO, "m_VideoPktsList VideoProcessorLock" );
	#endif // DEBUG_PROCESSOR_LOCK
	VideoProcessorLock mgrMutexLock( this );
	doVideoClientRemovals( m_VideoClientRemoveList );

	m_PktVideoFeedPic->setThisDataLen( picPktDataLen );
	m_PktVideoFeedPic->setTotalDataLen( s32JpgDataLen );
	m_PktVideoFeedPic->setTimeStampMs( GetGmtTimeMs() );
    m_PktVideoFeedPic->setMotionDetect( jpgVideo->m_Motion );
	m_PktVideoFeedPic->setTotalPktsInSeq( 1 + chunkPktsRequired );
	m_PktVideoFeedPic->setPktSeqNum( 1 );

	memcpy( m_PktVideoFeedPic->getDataPayload(), jpgData, picPktDataLen );
	m_PktVideoFeedPic->calcPktLen();

	for( auto client : m_VideoPktsList )
	{
		client.m_Callback->callbackVideoPktPic( m_Engine.getMyOnlineId(), m_PktVideoFeedPic, 1 + chunkPktsRequired, 1  );
	}

	if( chunkPktsRequired )
	{
		int curDataIdx = MAX_PIC_PKT_DATA_PAYLOAD;
		int dataLeftToSend = dataOverflow;
		for( int i = 0; i < chunkPktsRequired; i++ )
		{
			PktVideoFeedPicChunk * pktChunk = m_VidChunkList[i];
            int32_t dataThisChunk = dataLeftToSend > (int)MAX_PIC_CHUNK_LEN ? (int)MAX_PIC_CHUNK_LEN : dataLeftToSend;
			memcpy( pktChunk->getDataPayload(), &(jpgData[curDataIdx]), dataThisChunk );

			pktChunk->setThisDataLen( dataThisChunk );
			pktChunk->setPktSeqNum( 2 + i );
			pktChunk->setTotalPktsInSeq( chunkPktsRequired + 1 );
			pktChunk->calcPktLen();
			for( auto client : m_VideoPktsList )
			{
				client.m_Callback->callbackVideoPktPicChunk( m_Engine.getMyOnlineId(), pktChunk, chunkPktsRequired + 1, 2 + i );
			}

			curDataIdx += dataThisChunk;
			dataLeftToSend -= dataThisChunk;
		}
	}
}

//============================================================================
void MediaProcessor::sendJpgVideo( VxGUID& onlineId, std::shared_ptr<CamJpgVideo>& jpgVideo )
{
	if( m_GuiPlayerCallback )
	{
		m_GuiPlayerCallback->callbackVideoJpg( onlineId, jpgVideo );
	}

	if( !m_VideoJpgList.size() )
	{
		return;
	}

	#ifdef DEBUG_PROCESSOR_LOCK
	LogMsg( LOG_INFO, "processFriendVideoFeed VideoProcessorLock start" );
	#endif // DEBUG_PROCESSOR_LOCK
	VideoProcessorLock mgrMutexLock( this );
	#ifdef DEBUG_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "processFriendVideoFeed VideoProcessorLock done" );
	#endif // DEBUG_PROCESSOR_LOCK
	doVideoClientRemovals( m_VideoClientRemoveList );

	for( auto& client : m_VideoJpgList )
	{
		if( !client.m_OnlineId.isValid() || client.m_OnlineId == onlineId )
		{
            client.m_Callback->callbackVideoJpg( onlineId, jpgVideo );
		}
	}

	#ifdef DEBUG_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "processFriendVideoFeed VideoProcessorLock callbacks done" );
	#endif // DEBUG_PROCESSOR_LOCK

}

//============================================================================
void MediaProcessor::processFriendVideoFeed(	VxGUID&			onlineId, 
												uint8_t *		pu8Jpg, 
												uint32_t		jpgDataLen,
												int				motion0To100000 )
{
	if( !m_VideoJpgList.size() )
	{
			return;
	}

	std::shared_ptr<uint8_t> vidData( new uint8_t[jpgDataLen] );
	memcpy( vidData.get(), pu8Jpg, jpgDataLen );
	std::shared_ptr<CamJpgVideo> jpgVideo( new CamJpgVideo( vidData, jpgDataLen, motion0To100000, 0, eMediaModulePtoP ) );
	sendJpgVideo( onlineId, jpgVideo );
}

//============================================================================
bool MediaProcessor::isAudioMediaType( EMediaInputType mediaType )
{
	switch( mediaType )
	{
	case eMediaInputAudioPkts:
	case eMediaInputAudioOpus:
	case eMediaInputAudioPcm:
	case eMediaInputMixer:
		return true;
	default:
		return false;
	}
}

//============================================================================
void MediaProcessor::wantMediaInput(	VxGUID&						onlineId,
										EMediaInputType				mediaType, 
										MediaCallbackInterface *	callback, 
										EMediaModule				mediaModule,
										VxGUID&						sessionId,
										bool						wantInput )
{
    if( eMediaModuleInvalid == mediaModule )
	{
		LogMsg( LOG_ERROR, "MediaProcessor::%s cannot subscribe with invalid media type", __func__ );
		vx_assert( false );
		return;
	}

	if( isAudioMediaType( mediaType ) )
	{
		if( eMediaInputMixer == mediaType )
		{
			wantMixerMediaInput( onlineId, mediaType, callback, mediaModule, sessionId, wantInput );
		}
		else
		{
			wantAudioMediaInput( onlineId, mediaType, callback, mediaModule, sessionId, wantInput );
		}
	}
	else
	{
		wantVideoMediaInput( onlineId, mediaType, callback, mediaModule, sessionId, wantInput );
	}
}

//============================================================================
bool MediaProcessor::clientExistsInList(	std::vector<MediaClient>&		clientList, 
											VxGUID&							onlineId,
											EMediaModule					mediaModule,
											EMediaInputType					mediaType, 
											VxGUID&							sessionId,
											MediaCallbackInterface *		callback )
{
	for( auto& client : clientList )
	{
		if( client.m_OnlineId == onlineId &&
			client.m_Callback == callback &&
			client.m_MediaModule == mediaModule &&
			client.m_MediaInputType == mediaType &&
			client.m_SessionId == sessionId )
		{
			return true;
		}
	}

	return false;
}

//============================================================================
bool MediaProcessor::removeClientFromListist(   std::vector<MediaClient>& clientList,
											    VxGUID&					onlineId,
												EMediaModule			mediaModule,
												EMediaInputType			mediaType,
											    VxGUID&					sessionId,
												MediaCallbackInterface* callback )
{
	std::vector<MediaClient>::iterator iter;
	for( iter = clientList.begin(); iter != clientList.end(); ++iter )
	{
		MediaClient& client = *iter;
		if( client.m_OnlineId == onlineId &&
			client.m_Callback == callback &&
			client.m_MediaModule == mediaModule &&
			client.m_MediaInputType == mediaType && 
			client.m_SessionId == sessionId )
		{
			clientList.erase( iter );
			return true;
		}
	}

	return false;
}


//============================================================================
bool MediaProcessor::clientToRemoveExistsInList(	std::vector<ClientToRemove>&	clientRemoveList, 
													VxGUID&							onlineId,
													EMediaModule					mediaModule,
													EMediaInputType					mediaType, 
													VxGUID&							sessionId,
													MediaCallbackInterface *		callback )
{
	for( auto& client : clientRemoveList )
	{
		if( client.m_OnlineId == onlineId &&
			client.m_Callback == callback &&
			client.m_MediaModule == mediaModule &&
			client.m_MediaType == mediaType &&
			client.m_SessionId == sessionId )
		{
			return true;
		}
	}

	return false;
}

//============================================================================
bool MediaProcessor::clientToRemoveRemoveFromList(	std::vector<ClientToRemove>&	clientRemoveList, 
												    VxGUID&							onlineId,
													EMediaModule					mediaModule,
													EMediaInputType					mediaType, 
												    VxGUID&							sessionId,
													MediaCallbackInterface *		callback )
{
	bool removed = false;
	std::vector<ClientToRemove>::iterator iter;
	for( iter = clientRemoveList.begin(); iter != clientRemoveList.end(); ++iter )
	{
		ClientToRemove& client = *iter;
		if( client.m_OnlineId == onlineId &&
			client.m_Callback == callback &&
			client.m_MediaModule == mediaModule &&
			client.m_MediaType == mediaType &&
			client.m_SessionId == sessionId )
		{
			clientRemoveList.erase( iter );
			removed = true;
			break;
		}
	}

	return removed;
}

//============================================================================
void MediaProcessor::wantMixerMediaInput(	VxGUID&						onlineId,
											EMediaInputType				mediaType, 
											MediaCallbackInterface *	callback, 
											EMediaModule				mediaModule,
											VxGUID&						sessionId,
											bool						wantInput )
{
    if( false == wantInput )
	{
		// user wants to be removed but is probably being called from a callback function.
		// if we remove now it might crash because iterator may expect more items in array
		m_MixerRemoveMutex.lock();
		if( !clientToRemoveExistsInList( m_MixerClientRemoveList, onlineId, mediaModule, mediaType, sessionId, callback ) )
		{
			m_MixerClientRemoveList.emplace_back( ClientToRemove( onlineId, mediaType, callback, mediaModule, sessionId ) );
		}

		m_MixerRemoveMutex.unlock();

		bool stopSpeakerOutput = (m_SpeakerOutputEnabled) && (1 == m_MixerList.size());
		// set capture stated before unlocking mutex
		if( stopSpeakerOutput )
		{
			m_SpeakerOutputEnabled = false;
            if( mediaModule != eMediaModuleSoundFx && ( LogEnabled(eLogVoice) || LogEnabled(eLogAudioIo) ) )
            {
                LogMsg( LOG_VERBOSE, "MediaProcessor::wantMixerMediaInput stopping speaker output module %s", DescribeMediaModule( mediaModule ) );
            }
		}

		return;
	}
	else
	{
		// if been commanded to remove and now adding remove the previous command
		m_MixerRemoveMutex.lock();
		bool wasRemoved = clientToRemoveRemoveFromList( m_MixerClientRemoveList, onlineId, mediaModule, mediaType, sessionId, callback );
		m_MixerRemoveMutex.unlock();
		if( wasRemoved )
		{
			m_MixerClientsMutex.lock();
			removeClientFromListist( m_MixerList, onlineId, mediaModule, mediaType, sessionId, callback );
			m_MixerClientsMutex.unlock();
		}
	}

	#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "wantAudioMediaInput m_MixerClientsMutex.lock start" );
	#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_MixerClientsMutex.lock();
	#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "wantAudioMediaInput m_MixerClientsMutex.lock done" );
	#endif // DEBUG_AUDIO_PROCESSOR_LOCK

	if( clientExistsInList( m_MixerList, onlineId, mediaModule, mediaType, sessionId, callback ) )
	{
		LogMsg( LOG_INFO, "WARNING. Ignoring New Mixer Media Client because already in list" );
		#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "m_MixerClientsMutex.unlock" );
		#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		m_MixerClientsMutex.unlock();
		return;
	}

	// not found add new client
	MediaClient newClient( onlineId, mediaModule, mediaType, callback, sessionId );
	m_MixerList.emplace_back( newClient );

	bool startSpeakerOutput = ( false == m_SpeakerOutputEnabled ) && ( 1 == m_MixerList.size() );
	// set capture stated before unlocking mutex
	if( startSpeakerOutput )
	{
		m_SpeakerOutputEnabled = true;
	}

	#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "m_MixerClientsMutex.unlock" );
	#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_MixerClientsMutex.unlock();

	IAudioRequests::getIAudioRequests().toGuiWantSpeakerOutput( mediaModule, wantInput );

	if( startSpeakerOutput )
	{
        if( mediaModule != eMediaModuleSoundFx && ( LogEnabled(eLogVoice) || LogEnabled(eLogAudioIo) ) )
        {
            LogMsg( LOG_VERBOSE, "MediaProcessor::wantMixerMediaInput starting speaker output module %s", DescribeMediaModule( mediaModule ) );
        }
	}
}

//============================================================================
void MediaProcessor::doMixerClientRemovals( std::vector<ClientToRemove>& clientRemoveList )
{
	if( !clientRemoveList.empty() )
	{
		m_MixerRemoveMutex.lock();
		for( size_t i = 0; i < clientRemoveList.size(); i++ )
		{
			VxGUID&						onlineId = clientRemoveList[i].m_OnlineId;
			EMediaInputType				mediaType = clientRemoveList[i].m_MediaType;
			MediaCallbackInterface*		callback = clientRemoveList[i].m_Callback;
			EMediaModule				mediaModule = clientRemoveList[i].m_MediaModule;
			VxGUID						sessionId = clientRemoveList[i].m_SessionId;

			std::vector<MediaClient>* clientList = getClientList( mediaType );
			if( !clientList )
			{
				LogMsg( LOG_ERROR, "MediaProcessor::doClientRemovals UNKNOWN TYPE" );
				vx_assert( false );
				continue;
			}

			for( auto iter = clientList->begin(); iter != clientList->end(); ++iter )
			{
				MediaClient& client = *iter;
				if( client.m_OnlineId == onlineId &&
					client.m_Callback == callback &&
					client.m_MediaModule == mediaModule &&
					client.m_MediaInputType == mediaType &&
					client.m_SessionId == sessionId )
				{
					clientList->erase( iter );
					break;
				}
			}

			bool stopSpeakerOutput = m_SpeakerOutputEnabled && (0 == m_MixerList.size());
			// set capture stated before unlocking mutex
			if( stopSpeakerOutput )
			{
				m_SpeakerOutputEnabled = false;
				LogMsg( LOG_VERBOSE, "MediaProcessor::doMixerClientRemovals stopping speaker output module %s", DescribeMediaModule( mediaModule ) );
			}

            int mediaReqCnt = countMediaModuleRequests( mediaType, mediaModule );
            if( !mediaReqCnt )
            {
                IAudioRequests::getIAudioRequests().toGuiWantSpeakerOutput( mediaModule, false );
            }
		}

		clientRemoveList.clear();
		m_MixerRemoveMutex.unlock();
	}
}

//============================================================================
void MediaProcessor::wantAudioMediaInput(	VxGUID&						onlineId,
											EMediaInputType				mediaType, 
											MediaCallbackInterface *	callback, 
											EMediaModule				mediaModule,
											VxGUID&						sessionId,
											bool						wantInput )
{
	if( false == wantInput )
	{
		// user wants to be removed but is probably being called from a callback function.
		// if we remove not will crash because iterator may expect more items in array or there may be a dead lock
		m_AudioRemoveMutex.lock();
		if( m_MicCaptureEnabled && !clientToRemoveExistsInList( m_AudioClientRemoveList, onlineId, mediaModule, mediaType, sessionId, callback ) )
		{
			m_AudioClientRemoveList.emplace_back( ClientToRemove( onlineId, mediaType, callback, mediaModule, sessionId ) );
		}

		m_AudioRemoveMutex.unlock();
		return;
	}
	else
	{
		// if been commanded to remove and now adding remove the previous command
		m_AudioRemoveMutex.lock();
		clientToRemoveRemoveFromList( m_AudioClientRemoveList, onlineId, mediaModule, mediaType, sessionId, callback );
		m_AudioRemoveMutex.unlock();
	}

	#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "wantAudioMediaInput m_AudioMutex.lock start" );
	#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_AudioMutex.lock();
	#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "wantAudioMediaInput m_AudioMutex.lock done" );
	#endif // DEBUG_AUDIO_PROCESSOR_LOCK

    std::vector<MediaClient>* clientList = getClientList( mediaType );
    if( !clientList )
	{
		LogMsg( LOG_ERROR, "wantAudioMediaInput unknown mediaType type %d", mediaType );
	#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "m_AudioMutex.unlock" );
	#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		m_AudioMutex.unlock();
        vx_assert( false );
		return;
	}

	if( clientExistsInList( *clientList, onlineId, mediaModule, mediaType, sessionId, callback ) )
	{
		LogMsg( LOG_INFO, "WARNING. Ignoring New Audio Media Client because already in list" );
		#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "m_AudioMutex.unlock" );
		#endif // DEBUG_AUDIO_PROCESSOR_LOCK
		m_AudioMutex.unlock();
		return;
	}

	// add new client
	MediaClient newClient( onlineId, mediaModule, mediaType, callback, sessionId );
	clientList->emplace_back( newClient );

	bool startMicInput = ( false == m_MicCaptureEnabled ) && ( 1 == ( m_AudioPcmList.size() + m_AudioOpusList.size() + m_AudioPktsList.size() ) );
	// set capture stated before unlocking mutex
	if( startMicInput )
	{
		m_MicCaptureEnabled = true;
	}

    int mediaReqCnt = countMediaModuleRequests( mediaType, mediaModule );
    if( 1 == mediaReqCnt )
    {
        IAudioRequests::getIAudioRequests().toGuiWantMicrophoneRecording( mediaModule, true );
    }

#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
	LogMsg( LOG_INFO, "m_AudioMutex.unlock" );
#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_AudioMutex.unlock();

	if( startMicInput )
	{
		LogMsg( LOG_INFO, "starting microphone input" );
	}
}

//============================================================================
void MediaProcessor::doAudioClientRemovals( std::vector<ClientToRemove>& clientRemoveList )
{
	if( !clientRemoveList.empty() )
	{
		m_AudioRemoveMutex.lock();
		for( size_t i = 0; i < clientRemoveList.size(); i++ )
		{
			VxGUID&						onlineId = clientRemoveList[i].m_OnlineId;
			EMediaInputType				mediaType = clientRemoveList[i].m_MediaType;
			MediaCallbackInterface*		callback = clientRemoveList[i].m_Callback;
			EMediaModule				mediaModule = clientRemoveList[i].m_MediaModule;
			VxGUID						sessionId = clientRemoveList[i].m_SessionId;

			std::vector<MediaClient>* clientList = getClientList( mediaType );
			if( !clientList )
			{
				LogMsg( LOG_ERROR, "MediaProcessor::%s UNKNOWN TYPE", __func__ );
				vx_assert( false );
				continue;
			}

			for( auto iter = clientList->begin(); iter != clientList->end(); ++iter )
			{
				MediaClient& client = *iter;
				if( client.m_OnlineId == onlineId &&
					client.m_Callback == callback &&
					client.m_MediaModule == mediaModule &&
					client.m_MediaInputType == mediaType &&
					client.m_SessionId == sessionId )
				{
					clientList->erase( iter );
					break;
				}
			}

			bool stopMicInput = m_MicCaptureEnabled && (0 == (m_AudioPcmList.size() + m_AudioOpusList.size() + m_AudioPktsList.size()));
            // set audiocapture stopped before unlocking mutex
			if( stopMicInput )
			{
				m_MicCaptureEnabled = false;
			}

            int mediaReqCnt = countMediaModuleRequests( mediaType, mediaModule );
            if( !mediaReqCnt )
            {
                IAudioRequests::getIAudioRequests().toGuiWantMicrophoneRecording( mediaModule, false );
            }
		}

		clientRemoveList.clear();
		m_AudioRemoveMutex.unlock();
	}
}

//============================================================================
void MediaProcessor::wantVideoMediaInput(	VxGUID&						onlineId,
										    EMediaInputType				mediaType, 
											MediaCallbackInterface *	callback, 
											EMediaModule				mediaModule,
											VxGUID&						sessionId,
											bool						wantInput )
{
	LogModule( eLogWebCam, LOG_DEBUG, "%s %s wantInput %d", __func__, DescribeMediaModule( mediaModule ), wantInput );
	if( mediaModule == eMediaModuleMediaPlayer && wantInput )
	{
		// gui media player wants all jpg streams and is never removed and we do not want it to trigger video capture
		m_GuiPlayerCallback = callback;
		return;
	}

	if( false == wantInput )
	{
		// user wants to be removed but is probably being called from a callback function.
		// if we remove now it will crash because iterator may expect more items in array
		m_VideoRemoveMutex.lock();
		if( m_VidCaptureEnabled && !clientToRemoveExistsInList( m_VideoClientRemoveList, onlineId, mediaModule, mediaType, sessionId, callback ) )
		{
			m_VideoClientRemoveList.emplace_back( ClientToRemove( onlineId, mediaType, callback, mediaModule, sessionId ) );
		}

		m_VideoRemoveMutex.unlock();
		return;
	}
	else
	{
		// if has been commanded to remove previously and now want input then remove from the to be removed list
		m_VideoRemoveMutex.lock();
		clientToRemoveRemoveFromList( m_VideoClientRemoveList, onlineId, mediaModule, mediaType, sessionId, callback );
		m_VideoRemoveMutex.unlock();
	}

	#ifdef DEBUG_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "wantVideoMediaInput m_VideoMutex.lock start" );
	#endif // DEBUG_PROCESSOR_LOCK
		m_VideoMutex.lock();
	#ifdef DEBUG_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "wantVideoMediaInput m_VideoMutex.lock done" );
	#endif // DEBUG_PROCESSOR_LOCK

    std::vector<MediaClient> * clientList = getClientList( mediaType );
    if( !clientList )
    {
		LogMsg( LOG_ERROR, "wantVideoMediaInput unknown mediaType type %d", mediaType );
		#ifdef DEBUG_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "m_VideoMutex.unlock" );
		#endif // DEBUG_PROCESSOR_LOCK
		m_VideoMutex.unlock();
        vx_assert( false );
		return;
	}

	if( clientExistsInList( *clientList, onlineId, mediaModule, mediaType, sessionId, callback ) )
	{
		LogMsg( LOG_INFO, "WARNING. Ignoring New Video Media Client because already in list" );
		#ifdef DEBUG_PROCESSOR_LOCK
			LogMsg( LOG_INFO, "m_VideoMutex.unlock" );
		#endif // DEBUG_PROCESSOR_LOCK
		m_VideoMutex.unlock();
		return;
	}

	MediaClient newClient( onlineId, mediaModule, mediaType, callback, sessionId );
	clientList->emplace_back( newClient );

	int idsInVidPktListCnt = getMyIdInVidPktListCount(); // this also updates m_VidPktListContainsMyId
	#ifdef DEBUG_PROCESSOR_LOCK
		LogMsg( LOG_INFO, "m_VideoMutex.unlock" );
	#endif // DEBUG_PROCESSOR_LOCK
	bool startVidCapture = ( false == m_VidCaptureEnabled ) && ( 1 == ( m_VideoJpgList.size() + idsInVidPktListCnt ) );
	m_VideoMutex.unlock();
	if( startVidCapture )
	{
		#ifdef TEST_JPG_SPEED
			testJpgSpeed();
		#endif // TEST_JPG_SPEED
		m_VidCaptureEnabled = true;
		LogModule( eLogWebCam, LOG_INFO, "starting video capture" );
		IToGui::getIToGui().toGuiWantCamCapture( mediaModule, true );
	}
}

//============================================================================
void MediaProcessor::doVideoClientRemovals( std::vector<ClientToRemove>& clientRemoveList )
{
	if( !clientRemoveList.empty() )
	{
		m_VideoRemoveMutex.lock();
		for( size_t i = 0; i < clientRemoveList.size(); i++ )
		{
			VxGUID&						onlineId = clientRemoveList[i].m_OnlineId;
			EMediaInputType				mediaType = clientRemoveList[i].m_MediaType;
			MediaCallbackInterface*		callback = clientRemoveList[i].m_Callback;
			EMediaModule				mediaModule = clientRemoveList[i].m_MediaModule;
			VxGUID						sessionId = clientRemoveList[i].m_SessionId;

			std::vector<MediaClient>* clientList = getClientList( mediaType );
			if( !clientList )
			{
				LogMsg( LOG_ERROR, "MediaProcessor::doClientRemovals UNKNOWN TYPE" );
				vx_assert( false );
				continue;
			}

			std::vector<MediaClient>::iterator iter;
			for( iter = clientList->begin(); iter != clientList->end(); ++iter )
			{
				MediaClient& client = *iter;
				if( client.m_OnlineId == onlineId &&
					client.m_Callback == callback &&
					client.m_MediaInputType == mediaType &&
					client.m_SessionId == sessionId )
				{
					clientList->erase( iter );
					break;
				}
			}

			int idsInVidPktListCnt2 = getMyIdInVidPktListCount(); // this also updates m_VidPktListContainsMyId
			bool stopVidCapture = m_VidCaptureEnabled && (0 == ( m_VideoJpgList.size() + idsInVidPktListCnt2 ) );
			if( stopVidCapture )
			{
				m_VidCaptureEnabled = false;
				LogMsg( LOG_INFO, "stopping video capture" );
				IToGui::getIToGui().toGuiWantCamCapture( mediaModule, false );
			}
		}

		clientRemoveList.clear();
		m_VideoRemoveMutex.unlock();
	}
}

//============================================================================
void MediaProcessor::fromGuiEchoCanceledSamplesThreaded( const AudioPcmData& audioPcmData )
{
	const int16_t* pcmData = audioPcmData.getPcmData();
	const int sampleCnt = audioPcmData.getSampleCnt();
	const EMediaModule sourceModule = audioPcmData.getSourceModule();
	const uint64_t frameTag = audioPcmData.getFrameTag();

	vx_assert( sampleCnt == AUDIO_SAMPLES_PER_FRAME );
	if( false == m_MicCaptureEnabled || !pcmData || sampleCnt < 100 )
	{
		// invalid params or microphone not in capture mode
		m_AudioInSemaphore.signal();
		return;
	}

	if( m_ProcessAudioQue.size() < 5 )
	{
		AudioPcmData inAudioPcmData( pcmData, sampleCnt, sourceModule, frameTag );

		m_AudioQueInMutex.lock();
		m_ProcessAudioQue.emplace_back( std::move( inAudioPcmData ) );
		m_AudioQueInMutex.unlock();
	}
	else
	{
		LogModule( eLogStreams, LOG_WARN,
			"MediaProcessor::%s queue full drop tag=%llu module=%d sampleCnt=%d qSize=%zu",
			__func__,
			static_cast<unsigned long long>( frameTag ),
			static_cast<int>( sourceModule ),
			sampleCnt,
			m_ProcessAudioQue.size() );
	}

	m_AudioInSemaphore.signal();
}

//============================================================================
void MediaProcessor::fromGuiAudioOutSpaceAvaiThreaded( int freeSpaceLenBytes )
{
	//#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
	//	LogMsg( LOG_INFO, "fromGuiAudioOutSpaceAvail m_MixerClientsMutex.lock start" );
	//#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_MixerClientsMutex.lock();
	//#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
	//	LogMsg( LOG_INFO, "fromGuiAudioOutSpaceAvail m_MixerClientsMutex.lock done" );
	//#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	doMixerClientRemovals( m_MixerClientRemoveList );
	bool hasMixerClients = m_MixerList.size() > 0;
	for( auto& client : m_MixerList )
	{
		//LogMsg( LOG_INFO, "fromGuiAudioOutSpaceAvail callbackAudioOutSpaceAvail %d", iClientIdx );
		client.m_Callback->callbackAudioOutSpaceAvail( AUDIO_BUF_SIZE );
	}

	//#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
	//	LogMsg( LOG_INFO, "fromGuiAudioOutSpaceAvail m_MixerClientsMutex.unlock" );
	//#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_MixerClientsMutex.unlock();
	//#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
	//	LogMsg( LOG_INFO, "fromGuiAudioOutSpaceAvail m_MixerBufferMutex.lock()" );
	//#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_MixerBufferMutex.lock();
	if( hasMixerClients || m_MixerBufUsed )
	{
		if( !m_MixerBufUsed || m_MuteSpeaker )
		{
			IAudioRequests::getIAudioRequests().toGuiModuleAudioFrame( eMediaModulePtoP, (int16_t*)m_QuietAudioBuf, AUDIO_BUF_SIZE );
		}
		else
		{
			IAudioRequests::getIAudioRequests().toGuiModuleAudioFrame( eMediaModulePtoP, (int16_t*)m_MixerBuf, AUDIO_BUF_SIZE );
		}
	}

	m_MixerBufUsed = false;

	//#ifdef DEBUG_AUDIO_PROCESSOR_LOCK
	//	LogMsg( LOG_INFO, "fromGuiAudioOutSpaceAvail m_MixerBufferMutex.unlock()" );
	//#endif // DEBUG_AUDIO_PROCESSOR_LOCK
	m_MixerBufferMutex.unlock();
}

//============================================================================
EMediaInputCategory MediaProcessor::getMediaInputCategory( EMediaInputType mediaInputType )
{
	EMediaInputCategory mediaCategory{ eMediaInputCategoryUnknown };
	switch( mediaInputType )
	{
	case eMediaInputMixer:
		mediaCategory = eMediaInputCategoryMixer;
		break;
	case eMediaInputAudioPkts:
		mediaCategory = eMediaInputCategoryAudio;
		break;
	case eMediaInputAudioPcm:
		mediaCategory = eMediaInputCategoryAudio;
		break;
	case eMediaInputAudioOpus:
		mediaCategory = eMediaInputCategoryAudio;
		break;
	case eMediaInputVideoJpg:
		mediaCategory = eMediaInputCategoryVideo;
		break;
	case eMediaInputVideoPkts:
		mediaCategory = eMediaInputCategoryVideo;
		break;

	default:
		LogMsg( LOG_ERROR, "MediaProcessor::%s UNKNOWN TYPE", __func__ );
		break;
	}

	return mediaCategory;
}


//============================================================================
int MediaProcessor::countMediaModuleRequests( EMediaInputType mediaType, EMediaModule mediaModule )
{
	EMediaInputCategory mediaCategory = getMediaInputCategory( mediaType );
	std::vector<MediaClient>* clientList = getClientList( mediaType );

	if( clientList && mediaCategory != eMediaInputCategoryUnknown )
	{
        return countClientModuleTypeRequests( mediaModule, clientList );
	}
	else
	{
		LogMsg( LOG_ERROR, "MediaProcessor::%s Invalid Param", __func__ );
		vx_assert( false );
		return 0;
	}
}

//============================================================================
std::vector<MediaClient>* MediaProcessor::getClientList( EMediaInputType mediaInputType )
{
	switch( mediaInputType )
	{
	case eMediaInputAudioPkts:
		return &m_AudioPktsList;
	case eMediaInputVideoPkts:
		return &m_VideoPktsList;
	case eMediaInputAudioPcm:
		return &m_AudioPcmList;
	case eMediaInputVideoJpg:
		return &m_VideoJpgList;
	case eMediaInputAudioOpus:
		return &m_AudioOpusList;
	case eMediaInputMixer:
		return &m_MixerList;

	default:
		LogMsg( LOG_ERROR, "MediaProcessor::%s UNKNOWN TYPE", __func__ );
		vx_assert( false );
		return nullptr;
	}
}

//============================================================================
void MediaProcessor::lockClientList( EMediaInputCategory mediaCategory )
{
	switch( mediaCategory )
	{
	case eMediaInputCategoryMixer:
		getMixerMutex().lock();
		break;
	case eMediaInputCategoryAudio:
		getAudioMutex().lock();
		break;
	case eMediaInputCategoryVideo:
		getVideoMutex().lock();
		break;

	default:
		LogMsg( LOG_ERROR, "MediaProcessor::%s UNKNOWN TYPE", __func__ );
		vx_assert( false );
		break;
	}
}

//============================================================================
void MediaProcessor::unlockClientList( EMediaInputCategory mediaCategory )
{
	switch( mediaCategory )
	{
	case eMediaInputCategoryMixer:
		getMixerMutex().unlock();
		break;
	case eMediaInputCategoryAudio:
		getAudioMutex().unlock();
		break;
	case eMediaInputCategoryVideo:
		getVideoMutex().unlock();
		break;

	default:
		LogMsg( LOG_ERROR, "MediaProcessor::%s UNKNOWN TYPE", __func__ );
		vx_assert( false );
		break;
	}
}

//============================================================================
int MediaProcessor::countClientModuleTypeRequests( EMediaModule mediaModule, std::vector<MediaClient>* clientList )
{
	int cnt{ 0 };
	for( auto& client : *clientList )
	{
        if( client.m_MediaModule == mediaModule )
		{
			cnt++;
		}
	}

	return cnt;
}
