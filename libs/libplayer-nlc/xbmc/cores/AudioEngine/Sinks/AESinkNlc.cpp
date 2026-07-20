/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "config_components_kodi.h"
#ifdef HAVE_NLC_GUI

#include "AESinkNlc.h"

#include <stdint.h>
#include <limits.h>
#include <unistd.h>

#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"

#include "utils/log.h"
#include "threads/SingleLock.h"
#include <sstream>

#include <sys/ioctl.h>

#include <GuiInterface/IAudioInterface.h>
#include <GuiInterface/IToGui.h>

#include <CoreLib/VxDebug.h>

#define OSS_FRAMES 256

//static enum AEChannel QtChannelMap[ 9 ] = { AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_NULL };
//static enum AEChannel QtChannelMap[ 9 ] = { AE_CH_FL, AE_CH_FR, AE_CH_NULL, AE_CH_NULL, AE_CH_NULL, AE_CH_NULL, AE_CH_NULL, AE_CH_NULL, AE_CH_NULL };

//static const unsigned int PassthroughSampleRates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };


//============================================================================
CAESinkNlc::CAESinkNlc()
{
}

//============================================================================
CAESinkNlc::~CAESinkNlc()
{
    Deinitialize();
}

//============================================================================
void CAESinkNlc::Register()
{
    AE::AESinkRegEntry entry;
    entry.sinkName = "AESinkNlc";
    entry.createFunc = CAESinkNlc::Create;
    entry.enumerateFunc = CAESinkNlc::EnumerateDevicesEx;
    AE::CAESinkFactory::RegisterSink( entry );
}

//============================================================================
std::unique_ptr<IAESink> CAESinkNlc::Create( std::string &device, AEAudioFormat& desiredFormat )
{
    auto sink = std::make_unique<CAESinkNlc>();
    if (sink->Initialize(desiredFormat, device))
        return sink;

    return {};
}

//============================================================================
std::string CAESinkNlc::GetDeviceUse( const AEAudioFormat& format, const std::string &device )
{
    return device;
}

//============================================================================
bool CAESinkNlc::Initialize( AEAudioFormat& desiredFormat, std::string &device )
{
	desiredFormat.m_channelLayout.setTo2ChannelLayout();
	desiredFormat.m_sampleRate = AUDIO_SAMPLE_RATE_KODI;
	desiredFormat.m_dataFormat = AE_FMT_FLOAT; // float so do not have to worry about cpu endian 
	// m_frameSize is misnamed.. is really number of bytes per sample of all channels ie 2 channels * 16 bit samples = 4 or if float format then 2 channels * 4 bytes per sample = 8
	desiredFormat.m_frameSize = AUDIO_CHANNELS_KODI * sizeof( float );
	// frames is misnamed.. is really number of samples per frame 
	desiredFormat.m_frames = AUDIO_SAMPLE_RATE_KODI * AUDIO_SEC_KODI;  // 60 ms of samples per frame

    desiredFormat.m_streamInfo.m_channels = 0;
    desiredFormat.m_streamInfo.m_sampleRate = 0;


	m_DesiredFormat = desiredFormat;
	m_AvailableFormat = desiredFormat;

//#if OUTPUT_PCM_SAMPLES
//    m_AvailableFormat.m_dataFormat = AE_FMT_S16LE;
//    // m_frameSize is misnamed.. is really number of bytes per sample of all channels ie 2 channels * 16 bit samples = 4 or if float format then 2 channels * 4 bytes per sample = 8
//    m_AvailableFormat.m_frameSize = 4;
//    m_AvailableFormat.m_frames = 15360 / m_AvailableFormat.m_frameSize;  // 80 ms samples of 2 channel 4 bytes per sample * 48000 Hz
//#else
//    m_AvailableFormat.m_dataFormat = AE_FMT_FLOAT;
    // m_frameSize is misnamed.. is really number of bytes per sample of all channels ie 2 channels * 16 bit samples = 4 or if float format then 2 channels * 4 bytes per sample = 8
//    m_AvailableFormat.m_frameSize = 2 * sizeof( float );
    // frames is misnamed.. is really number of samples per frame 
 //   m_AvailableFormat.m_frames = 48000 * 0.040;  // 40 ms samples
//#endif // OUTPUT_PCM_SAMPLES

    //desiredFormat = m_AvailableFormat;
    device = GetName();
    m_device = device;

    CLog::Log( LOGDEBUG, " Nlc Audio Device   : %s", device.c_str() );
    CLog::Log( LOGDEBUG, " Nlc Sample Rate    : %d", desiredFormat.m_sampleRate );
    CLog::Log( LOGDEBUG, " Nlc Sample Format  : %s", CAEUtil::DataFormatToStr( desiredFormat.m_dataFormat ) );
    CLog::Log( LOGDEBUG, " Nlc Channel Count  : %d", desiredFormat.m_channelLayout.Count() );
    CLog::Log( LOGDEBUG, " Nlc Frames         : %d", desiredFormat.m_frames );
    CLog::Log( LOGDEBUG, " Nlc Frame Size     : %d", desiredFormat.m_frameSize );

    return true;
}

//============================================================================
void CAESinkNlc::Deinitialize()
{
    Stop();
}

//============================================================================
inline CAEChannelInfo CAESinkNlc::GetChannelLayout( const AEAudioFormat& format )
{
    // int count = 2;
    CAEChannelInfo info;
	info.setTo2ChannelLayout();
 //   for( unsigned int i = 0; i < count; ++i )
 //   {
 //       info += QtChannelMap[ i ];
 //   }

    return info;
}

//============================================================================
void CAESinkNlc::Stop()
{
}

//============================================================================
void CAESinkNlc::GetDelay( AEDelayStatus& status )
{
    // return current cached data duration in seconds 
    status.SetDelay( IAudioRequests::getIAudioRequests().toGuiGetAudioDelaySeconds(eMediaModulePlayerNlc));
}

//============================================================================
double CAESinkNlc::GetCacheTotal( )
{
    // return total possible cached data duration in seconds 
    return  IAudioRequests::getIAudioRequests().toGuiGetAudioCacheMaxSeconds( eMediaModulePlayerNlc );
}

extern bool movieStarted;

//============================================================================
unsigned int CAESinkNlc::AddPackets( uint8_t **data, unsigned int frames, unsigned int offset )
{
    int totalBytes = frames * m_AvailableFormat.m_frameSize;
    // if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_VERBOSE, "CAESinkNlc::AddPackets %d bytes", totalBytes );

    unsigned char* audioBuffer = (unsigned char*)data[ 0 ] + offset * m_AvailableFormat.m_frameSize;

    //static bool firstSample = true;
    //if( firstSample && ( frames > 16 ) )
    //{
    //    float * inSampleData = ( float * )buffer;
    //    testDataHasVolume( inSampleData, frames );
    //    if( g_BeginDebug )
    //    {
    //        firstSample = false;
    //        LogMsg( LOG_DEBUG, "CAESinkNlc::AddPackets sample dump \n" );
    //        DumpFloat( LOG_DEBUG, inSampleData, 16 );
    //    }
    //}

	int retryCnt = 0;
    while( ( IAudioRequests::getIAudioRequests().toGuiGetAudioCacheFreeSpaceBytes( eMediaModulePlayerNlc ) < totalBytes ) && ( retryCnt < 50 ) )
    {
        int requiredSpace = totalBytes - IAudioRequests::getIAudioRequests().toGuiGetAudioCacheFreeSpaceBytes( eMediaModulePlayerNlc );
        if( requiredSpace > 0 )
        {
            VxSleep( 10 );
			retryCnt++;
        }
		else
		{
			break;
		}
    }

//#ifdef DEBUG_KODI_AUDIO
	if( retryCnt >= 50 )
	{
        LogModule( eLogPlayerNlc, LOG_DEBUG, "CAESinkNlc::AddPackets timeout snd buf needs %d free space but has %d", totalBytes, IAudioRequests::getIAudioRequests().toGuiGetAudioCacheFreeSpaceBytes( eMediaModulePlayerNlc )  );
	}
//#endif // DEBUG_KODI_AUDIO

    //float* pBuffer = ( float* )data[ 0 ] + offset*m_AvailableFormat.m_frameSize;
    //static int frameCnt = 0;
    //if( pBuffer && frames > 16 )
    //{
    //    bool hasValue = false;
    //    for( int i = 0; i < 16; i++ )
    //    {
    //        hasValue = pBuffer[ i ] != 0.0f;
    //        if( hasValue )
    //        {
    //            break;
    //        }
    //    }

    //    if( hasValue )
    //    {
    //        frameCnt++;
    //        if( m_AvailableFormat.m_dataFormat == AE_FMT_U8 )
    //        {
    //            DumpInt8( LOG_DEBUG, ( int8_t * )data[ 0 ], 16, frameCnt, "sink add u8" );
    //        }
    //        else if( m_AvailableFormat.m_dataFormat == AE_FMT_S16LE || m_AvailableFormat.m_dataFormat == AE_FMT_S16NE )
    //        {
    //            DumpInt16( LOG_DEBUG, ( int16_t * )data[ 0 ], 16 * 2, frameCnt, "sink add s16" );
    //        }
    //        else if( m_AvailableFormat.m_dataFormat == AE_FMT_FLOATP || m_AvailableFormat.m_dataFormat == AE_FMT_FLOAT )
    //        {
    //            DumpFloat( LOG_DEBUG, ( float * )data[ 0 ], 16 * 4, frameCnt, "sink add float" );
    //        }
    //    }
    //}

#if 0
    static int pktCnt = 0;
    if( movieStarted )
    {
        static int lineCnt = 0;
        float* audioData = (float*)audioBuffer;
        int sampleCnt = totalBytes / sizeof( float );
        while( sampleCnt >= 16 )
        {
            if( pktCnt > 21 )
            //if( 0 )
            {
                LogMsg( LOG_DEBUG, "pkt %d line %d - %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f", pktCnt, lineCnt,
                    audioData[ 0 ], audioData[ 1 ], audioData[ 2 ], audioData[ 3 ], audioData[ 4 ], audioData[ 5 ],
                    audioData[ 6 ], audioData[ 7 ], audioData[ 8 ], audioData[ 9 ], audioData[ 10 ], audioData[ 11 ], audioData[ 12 ], audioData[ 13 ],
                    audioData[ 14 ], audioData[ 15 ] );
            }

            lineCnt++;
            sampleCnt -= 16;
            audioData += 16;
        }

        pktCnt++;
        if( pktCnt > 22 )
        {
//            LogMsg( LOG_DEBUG, "pkt %d", pktCnt );
        }
    }
#endif

    // if(LogEnabled(eLogAudioIo)) LogModule( eLogAudioIo, LOG_VERBOSE, "CAESinkNlc toGuiPlayerNlcAudio %d bytes", totalBytes );

    int wrote = IAudioRequests::getIAudioRequests().toGuiPlayerNlcAudio( eMediaModulePlayerNlc, (float *)audioBuffer, totalBytes );
    if( wrote < 0 )
    {
        CLog::Log( LOGERROR, "CAESinkNlc::AddPackets - Failed to write" );
        return INT_MAX;
    }
    else if( wrote != totalBytes )
    {
        CLog::Log( LOGERROR, "CAESinkNlc::AddPackets - write not complete %d", wrote );
        return INT_MAX;
    }


    return wrote / m_AvailableFormat.m_frameSize; // number of frames used
}

//============================================================================
void CAESinkNlc::Drain()
{
}

//============================================================================
void CAESinkNlc::EnumerateDevicesEx( AEDeviceInfoList &list, bool force )
{
    // BRJ
    CAEDeviceInfo deviceInfo;
    deviceInfo.m_deviceName           = std::string( "AESinkNlc" );
    deviceInfo.m_displayName          = std::string( "AESinkNlc" );
	deviceInfo.m_displayNameExtra = std::string( "" );

//#if OUTPUT_PCM_SAMPLES
//    info.m_dataFormats.push_back( AE_FMT_S16LE );
//#else
	deviceInfo.m_dataFormats.push_back( AE_FMT_FLOAT );
//#endif // OUTPUT_PCM_SAMPLES

	deviceInfo.m_deviceType           = AE_DEVTYPE_PCM;
	deviceInfo.m_wantsIECPassthrough  = false;
	deviceInfo.m_channels.setTo2ChannelLayout(); // = AE_CH_LAYOUT_2_0;

	deviceInfo.m_sampleRates.push_back( 48000 );
	//deviceInfo.m_dataFormats.push_back( AEDataFormat( AE_FMT_FLOAT ) );
    list.push_back( deviceInfo );
}

#endif // HAVE_NLC_GUI
