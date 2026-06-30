
cmake_minimum_required (VERSION 3.22.1)

project(audio-nlc, LANGUAGES C CXX)

set(audio_nlc_HEADERS)
list(APPEND audio_nlc_HEADERS

    AudioMgr.h
    AudioMixerMgr.h

    TestFileWav.h
    TestFileWavMgr.h

    miniaudio/AudioFrameAecBuffer.h
    miniaudio/AudioMixerBuf.h
    miniaudio/AudioMixerInterface.h
    miniaudio/AudioSampleBuf.h
    miniaudio/AudioTestToneGenerator.h
    miniaudio/AudioUtils.h

    miniaudio/MiniAudioDevices.h
    miniaudio/MiniAudioIn.h
    miniaudio/MiniAudioInDevice.h
    miniaudio/MiniAudioOut.h
    miniaudio/MiniAudioOutDevice.h
)

set(audio_nlc_SOURCES)
list(APPEND audio_nlc_SOURCES 

    AudioMgr.cpp
    AudioMgrDelayTest.cpp
    AudioMgrIn.cpp
    AudioMgrOut.cpp
    AudioMgrRequests.cpp
    AudioMixerMgr.cpp

    TestFileWav.cpp
    
    miniaudio/AudioMixerBuf.cpp
    miniaudio/AudioSampleBuf.cpp
    miniaudio/AudioTestToneGenerator.cpp
    miniaudio/AudioUtils.cpp

    miniaudio/MiniAudioDevices.cpp
    miniaudio/MiniAudioIn.cpp
    miniaudio/MiniAudioInDevice.cpp
    miniaudio/MiniAudioOut.cpp
    miniaudio/MiniAudioOutDevice.cpp
    miniaudio/miniaudio-link.cpp
)

add_library(audio-nlc STATIC 
    ${audio_nlc_SOURCES}
    ${audio_nlc_HEADERS}
)

target_compile_definitions(audio-nlc
  PRIVATE

)

target_include_directories(audio-nlc
  PRIVATE
    miniaudio
    ../
    ../libptopengine
    ../ffmpeg
    ../ffmpeg/opus
)


