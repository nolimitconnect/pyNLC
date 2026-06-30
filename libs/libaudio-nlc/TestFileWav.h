#pragma once
//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <string>
#include <vector>   
#include <cstdint>

class TestFileWav
{
public:
    TestFileWav( const std::string& filePath );
    ~TestFileWav() = default;

    bool                        isValid( void ) const { return m_Valid; }
    const std::string&          getFilePath( void ) const { return m_FilePath; }
    
    int                         getSampleRate( void ) const { return m_SampleRate; }
    int                         getNumChannels( void ) const { return m_NumChannels; }
    int                         getNumFrames( void ) const { return m_NumFrames; }
    const int16_t*              getPcmData( void ) const { return m_PcmData.data(); }

    void                        resetPlayback( void ) { m_CurrentFrameIndex = 0; }
    bool                        getNextAudioFrame( int16_t* frameBuffer, int frameSize ); // frameSize is number of samples (not bytes)

protected:
    bool                        loadWavFile( std::string filePath, int& sampleRate, int& numChannels, int& bitsPerSample, std::vector<int16_t>& pcmData );
    
    bool                        m_Valid{ false };
    int                         m_SampleRate{ 0 };
    int                         m_NumChannels{ 0 };
    int                         m_NumFrames{ 0 };
    std::vector<int16_t>        m_PcmData;
    std::string                 m_FilePath;

    int                         m_CurrentFrameIndex{ 0 };
};