//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AudioMgr.h"

#include "AudioDefs.h"
#include "AudioUtils.h"
#include "TestFileWavMgr.h"

#include <GuiInterface/IToGui.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxTime.h>
#include <CoreLib/VxElapseTimer.h>

#include <algorithm>

//============================================================================
AudioMgr::AudioMgr()
    : MiniAudioDevices()
    , AudioMixerMgr()
    , m_TestFile("")
    , m_AudioInIo( *this )
    , m_AudioOutIo( *this )
    , m_TestFileWavMgr(TestFileWavMgr::getInstance())
{
    m_AudioTestTimer.setCallback( [this]() { this->callbackAudioTestTimer(); } );
    m_AudioOutDisableTimer.setCallback( [this]() { this->callbackAudioOutDisablePoll(); } );
}

//============================================================================
AudioMgr::~AudioMgr()
{
    audioIoSystemShutdown();
}

//============================================================================
void AudioMgr::audioIoSystemStartup()
{
    if( !m_AudioIoInitialized )
    {
        int startTime = GetApplicationAliveMs();
        LogMsg( LOG_DEBUG, "%s begin at %d", __func__, startTime );

        startupMiniAudio();
        setAgcEnabled( false );

        if( getIsSpeakerAvailable() )
        {
            int deviceIndex = 0;
            getSoundOutDeviceIndex( deviceIndex );

            m_AudioOutIo.initAudioOut( m_AudioOutFormat, deviceIndex );
            m_ToneGenerator.setAudioFormat( m_AudioOutFormat );
        }

        if( getIsMicrophoneAvailable() )
        {
            int deviceIndex = 0;
            getSoundInDeviceIndex( deviceIndex );

            m_AudioInIo.initAudioIn( m_AudioInFormat, deviceIndex );
        }

        updateHardwareTotalLatencyMs();
        LogMsg( LOG_DEBUG, "%s calculated hardware total latency is %d ms", __func__, m_EchoHardwareTotalLatencyMs );

        m_AudioIoInitialized = true;
    }
}

//============================================================================
void AudioMgr::audioIoSystemShutdown()
{
    // Cancel timers first so no deferred callbacks race shutdown.
    m_AudioTestTimer.stop();
    cancelDeferredAudioOutDisable();
    m_PlayerNlcSpeakerDisablePending = false;

    // Stop input immediately and join worker if needed.
    enableAudioIn( false );
    if( m_AudioInWorkerThread.joinable() )
    {
        stopAudioInWorker();
    }

    // Force immediate output stop for process shutdown; do not defer for drain.
    setIsSpeakerWanted( false );
    if( m_EnableAudioOut )
    {
        m_EnableAudioOut = false;
        m_AudioOutIo.stopAudioOutHardware();
        stopAudioOutWorker();
    }
    else if( m_AudioOutWorkerThread.joinable() )
    {
        stopAudioOutWorker();
    }

    if( m_AudioIoInitialized )
    {
        m_AudioIoInitialized = false;
        m_AudioInIo.audioInShutdown();
        m_AudioOutIo.audioOutShutdown();
        shutdownMiniAudio();
    }
}

//============================================================================
void AudioMgr::enableAudioIn( bool enable )
{
    if( enable )
    {
        // If output stop is waiting for drain, keep output alive when input is re-enabled.
        cancelDeferredAudioOutDisable();
    }

    if( m_EnableAudioIn != enable )
    {
        m_EnableAudioIn = enable;
        setIsMicrophoneWanted( enable );

        if( enable )
        {
            startAudioInWorker();
            m_AudioInIo.startAudioInHardware();
        }
        else
        {
            m_AudioInIo.stopAudioInHardware();
            stopAudioInWorker();
            m_AudioInPeakAmplitude = 0;
        }
    }
}

//============================================================================
void AudioMgr::requestDeferredAudioOutDisable( void )
{
    m_AudioOutDisablePending = true;

    m_AudioOutDisableTimer.start(10);
}

//============================================================================
void AudioMgr::cancelDeferredAudioOutDisable( void )
{
    if( !m_AudioOutDisablePending )
    {
        return;
    }

    m_AudioOutDisablePending = false;

    m_AudioOutDisableTimer.stop();
}

//============================================================================
int AudioMgr::getAudioOutPipelineQueuedSampleCnt( void )
{
    int queuedSamples = 0;
    int playerCacheSamples = 0;

    {
        std::lock_guard<std::mutex> lk( m_SpeakerOutBufferMutex );
        queuedSamples += static_cast<int>( m_SpeakerOutBuffer.size() );
    }

    lockPlayerCache();
    playerCacheSamples += m_PlayerCacheBuf.getSampleCnt();
    playerCacheSamples += m_PlayerCacheQueue.size() * AUDIO_SAMPLES_PER_FRAME;
    unlockPlayerCache();

    int maxModuleSamplesInMixer = 0;
    lockModuleMixerBuffer();
    for( auto& [module, mixerBuf] : m_AppModuleToSpeakerMap )
    {
        if( !isModuleOutputWanted( module ) )
        {
            continue;
        }

        int mixerBufSamples = mixerBuf.getSampleCnt();
        if( module == eMediaModulePlayerNlc )
        {
            mixerBufSamples += playerCacheSamples;
        }

        if( mixerBufSamples > maxModuleSamplesInMixer )
        {
            maxModuleSamplesInMixer = mixerBufSamples;
        }
    }
    unlockModuleMixerBuffer();

    return queuedSamples + maxModuleSamplesInMixer;
}

//============================================================================
void AudioMgr::completeAudioOutDisable( void )
{
    m_AudioOutDisablePending = false;

    m_AudioOutDisableTimer.stop();

    if( !m_EnableAudioOut )
    {
        return;
    }

    m_EnableAudioOut = false;
    m_AudioOutIo.stopAudioOutHardware();
    stopAudioOutWorker();
}

//============================================================================
void AudioMgr::callbackAudioOutDisablePoll( void )
{
    if( m_PlayerNlcSpeakerDisablePending )
    {
        const int queuedSamples = getAudioOutPipelineQueuedSampleCnt();
        if( queuedSamples <= 0 )
        {
            m_PlayerNlcSpeakerDisablePending = false;
            toGuiWantSpeakerOutput( eMediaModulePlayerNlc, false );
        }
    }

    if( !m_AudioOutDisablePending )
    {
        m_AudioOutDisableTimer.stop();
        return;
    }

    const int queuedSamples = getAudioOutPipelineQueuedSampleCnt();

    if( queuedSamples <= 0 )
    {
        completeAudioOutDisable();
    }
}

//============================================================================
void AudioMgr::enableAudioOut( bool enable )
{
    if( enable )
    {
        cancelDeferredAudioOutDisable();
        setIsSpeakerWanted( true );

        if( m_EnableAudioOut )
        {
            return;
        }

        m_EnableAudioOut = true;
        startAudioOutWorker();
        m_AudioOutIo.startAudioOutHardware();
        return;
    }

    // Disable requests are deferred until queued audio drains to speaker hardware.
    setIsSpeakerWanted( false );
    if( !m_EnableAudioOut )
    {
        return;
    }

    requestDeferredAudioOutDisable();
}

//============================================================================
void AudioMgr::sendToSpeakerOutput( int16_t* pcmData, int sampleCnt )
{    
    if( !pcmData || sampleCnt <= 0 )
    {
        return;
    }

    std::lock_guard<std::mutex> lk(m_SpeakerOutBufferMutex);
    m_SpeakerOutBuffer.insert( m_SpeakerOutBuffer.end(), pcmData, pcmData + sampleCnt );

    const uint64_t queueDepth = static_cast<uint64_t>( m_SpeakerOutBuffer.size() );
    uint64_t currentHighWater = m_SpeakerQueueHighWatermark.load();
    while( queueDepth > currentHighWater &&
            !m_SpeakerQueueHighWatermark.compare_exchange_weak( currentHighWater, queueDepth ) )
    {
    }

    // remove old samples if buffer gets too large to prevent unbounded growth
    if( m_SpeakerOutBuffer.size() > MAX_SPEAKER_OUT_BUFFER_SAMPLES )
    {      
        m_SpeakerOverflowCount++;
        m_SpeakerOutBuffer.erase( m_SpeakerOutBuffer.begin(), 
            m_SpeakerOutBuffer.begin() + static_cast<std::vector<int16_t>::difference_type>(m_SpeakerOutBuffer.size() - MAX_SPEAKER_OUT_BUFFER_SAMPLES) );
    }
}

//============================================================================
void AudioMgr::playTestFile( TestFileWav& testFile )
{
    if( getIsPlayingTestFile() )
    {
        LogMsg( LOG_WARNING, "%s Already playing a test file. Ignoring new request.", __func__ );
        return;
    }
    
    m_TestFile = testFile;
    m_TestFile.resetPlayback();

    setIsMicrophoneWanted( false );
    setIsPlayingTestFile( true );
}

//============================================================================
int AudioMgr::updateHardwareTotalLatencyMs( void ) 
{
    m_SpeakerHardwareLatencyMs = m_AudioOutIo.getHardwareDelayMs();
    m_EchoHardwareTotalLatencyMs = m_AudioInIo.getHardwareDelayMs() + m_SpeakerHardwareLatencyMs;
    return m_EchoHardwareTotalLatencyMs;
}

//============================================================================
void AudioMgr::setEchoDelayParam( int delayMs )
{ 
    m_EchoDelayMs = delayMs; 
    m_Aec.setEchoDelay(delayMs);
    LogMsg( LOG_DEBUG, "%s set echo delay to %d ms", __func__, delayMs );
}

//============================================================================
int AudioMgr::getWantMicrophoneCount( void ) 
{ 
    m_WantMicMutex.lock();
    int count = static_cast<int>( m_WantMicList.size() );   
    m_WantMicMutex.unlock();
    return count; 
}

//============================================================================
int AudioMgr::getWantSpeakerCount( void ) 
{
    m_WantSpeakerMutex.lock();
    int count = static_cast<int>( m_WantSpeakerList.size() );
    m_WantSpeakerMutex.unlock();
    return count;
}

//============================================================================
void AudioMgr::writeMixerAudioToSpeakerHardware( int16_t* pcmData, int sampleCount )
{
    sendToSpeakerOutput( pcmData, sampleCount );
}

//============================================================================
int AudioMgr::getSpeakerHardwareBufferedSampleCnt( void )
{
    return static_cast<int>( m_SpeakerOutBuffer.size() + getSpeakerHardwareLatencyMs() * m_AudioOutFormat.sampleRate() / 1000 );
}

//============================================================================
int AudioMgr::getSpeakerHardwareFreeSpaceSampleCnt( void )
{
    int availPcmSamplesCache = 0;

    {
        std::lock_guard<std::mutex> lk( m_SpeakerOutBufferMutex );
        availPcmSamplesCache += MAX_SPEAKER_OUT_BUFFER_SAMPLES - static_cast<int>( m_SpeakerOutBuffer.size() );
    }

    return availPcmSamplesCache;
}

//============================================================================
void AudioMgr::callbackAudioOut60msSpaceAvail( int freeSpaceLenBytes )
{
    AudioMixerMgr::callbackAudioOut60msSpaceAvail( freeSpaceLenBytes );
    std::lock_guard<std::mutex> lk( m_AudioOutSpaceAvailableClientListMutex );
    for( auto& client : m_AudioOutSpaceAvailableClientList )
    {
        client->callbackAudioOutSpaceAvail( freeSpaceLenBytes );
    }
}

//============================================================================
bool AudioMgr::isModuleOutputWanted( EMediaModule mediaModule )
{
    m_WantSpeakerMutex.lock();
    bool wanted = std::find( m_WantSpeakerList.begin(), m_WantSpeakerList.end(), mediaModule ) != m_WantSpeakerList.end();
    m_WantSpeakerMutex.unlock();
    return wanted;
}

//============================================================================
void AudioMgr::playTestFile( std::string testFile )
{
    for( auto& testFileWav : m_TestFileWavMgr.getTestFileWavList() ) {
        if( testFileWav.getFilePath() == testFile ) {
            playTestFile( const_cast<TestFileWav&>(testFileWav) );
            return;
        }
    } 

    TestFileWav resWav( testFile );
    if( !resWav.isValid() ) {
        return;
    }

    playTestFile( resWav );
}

//============================================================================
bool AudioMgr::setAudioInDevice( std::string deviceDescription )
{
    int deviceIndex = MiniAudioDevices::findAudioInDeviceIndexByDescription( deviceDescription );
    bool result = (deviceIndex != -1);
    if( result ) {
        if(deviceIndex == m_SndInDeviceIndex) {
            LogModule( eLogAudioIo, LOG_INFO, "Audio In Device unchanged, index: %d %s", deviceIndex, deviceDescription.c_str() );
            return true;
        }

        LogModule( eLogAudioIo, LOG_INFO, "Audio In Device changed, index: %d %s", deviceIndex, deviceDescription.c_str() );
        m_AudioInPeakAmplitude = 0;
        result = m_AudioInIo.soundInDeviceChanged( deviceIndex );
        if( result ) {
            m_SndInDeviceIndex = deviceIndex;
            updateHardwareTotalLatencyMs();
        }
    }

    return result;
}

//============================================================================
bool AudioMgr::setAudioOutDevice( std::string deviceDescription )
{
    int deviceIndex = MiniAudioDevices::findAudioOutDeviceIndexByDescription( deviceDescription );
    bool result = (deviceIndex != -1);
    if( result ) {
        if(deviceIndex == m_SndOutDeviceIndex) {
            LogModule( eLogAudioIo, LOG_INFO, "Audio Out Device unchanged, index: %d %s", deviceIndex, deviceDescription.c_str() );
            return true;
        }

        LogModule( eLogAudioIo, LOG_INFO, "Audio Out Device changed, index: %d %s", deviceIndex, deviceDescription.c_str() );
        result = m_AudioOutIo.soundOutDeviceChanged( deviceIndex );
        if( result ) {
            m_SndOutDeviceIndex = deviceIndex;
            updateHardwareTotalLatencyMs();
        }
    }

    return result;
}

//============================================================================
void AudioMgr::wantAudioDelayTestCallbacks( AudioDelayTestCallback* client, bool enable )
{
    if( enable ) {
        m_AudioDelayTestCallback = client;
    } else {
        if( m_AudioDelayTestCallback == client ) {
            m_AudioDelayTestCallback = nullptr;
        }
    }
}