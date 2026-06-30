//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#ifdef TARGET_OS_WINDOWS
# include <WinSock2.h>
# include <windows.h>
#elif defined(TARGET_OS_ANDROID)
# include <android/log.h>
# include <unistd.h>
#else
# include <unistd.h>
#endif // TARGET_OS_WINDOWS

#include "VxDebug.h"
#include "VxMutex.h"
#include "VxGlobals.h"
#include "VxFileUtil.h"
#include "AppErr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <array>

#include "VxElapseTimer.h"

#define MAX_ERR_MSG_SIZE 16384

bool g_StreamActive = false;

namespace
{
#if defined(LOG_IN_RELEASE_BUILD)
uint64_t g_ModuleEnableLoggingFlags = (uint64_t)(
    eLogHackers
    // | eLogFileXfer
    // | eLogConnect
    // | eLogOffer
    // | eLogSkt
    // | eLogPkt
    // | eLogNetworkState
    // | eLogSession
    // | eLogIsPortOpenTest
    // | eLogThread
    | eLogWebCam
    // | eLogAssets
    // | eLogWindowPositions
     | eLogStartup
    // | eLogHostJoin
    // | eLogStreams
    // | eLogSktData
    //| eLogSktData
    //| eLogOffer
    //| eLogPortForward
    // | eLogNetService
    // | eLogRunTest
    // | eLogHostConnect
    // | eLogHostSearch
    //| eLogStreams
    | eLogPlayerNlc
    //| eLogFfmpeg
    //| eLogAudioIo
    //| eLogVideoIo
    );
#elif defined(CAMERATESTBUILD)
        uint64_t g_ModuleEnableLoggingFlags = (uint64_t)(
        eLogHackers
        // | eLogFileXfer
        // | eLogConnect
        // | eLogOffer
        // | eLogSkt
        // | eLogPkt
        // | eLogNetworkState
        // | eLogSession
        // | eLogIsPortOpenTest
        // | eLogThread
        // | eLogWebCam
        // | eLogAssets
        // | eLogWindowPositions
         | eLogStartup
        // | eLogHostJoin
        // | eLogStreams
        // | eLogSktData
        //| eLogSktData
        //| eLogOffer
        //| eLogPortForward
        // | eLogNetService
        // | eLogRunTest
        // | eLogHostConnect
        // | eLogHostSearch
        //| eLogStreams
        //| eLogPlayerNlc
        //| eLogFfmpeg
        | eLogAudioIo
        //| eLogVideoIo
        );

#elif defined(DEBUG) || defined(_DEBUG) || defined(FLATPAKBUILD)
    uint64_t g_ModuleEnableLoggingFlags = (uint64_t)(
        eLogHackers
        // | eLogFileXfer
        // | eLogConnect
        // | eLogOffer
        // | eLogSkt
        // | eLogPkt
        // | eLogNetworkState
        // | eLogSession
        // | eLogIsPortOpenTest
        // | eLogThread
        // | eLogWebCam
        // | eLogAssets
        // | eLogWindowPositions
         | eLogStartup
        // | eLogHostJoin
        // | eLogStreams
        // | eLogSktData
        //| eLogSktData
        //| eLogOffer
        //| eLogPortForward
        // | eLogNetService
        // | eLogRunTest
        // | eLogHostConnect
        // | eLogHostSearch
        //| eLogStreams
        //| eLogPlayerNlc
        //| eLogFfmpeg
        | eLogAudioIo
        //| eLogVideoIo
        );


#else
    uint64_t g_ModuleEnableLoggingFlags = (uint64_t)eLogHackers;
#endif // defined(DEBUG)

    VxMutex& getLogMutex( void )
    {
        static  VxMutex logMutex;
        return logMutex;
    }

    VxMutex						g_oFileLogMutex;
#if defined(LOG_IN_RELEASE_BUILD)
    uint32_t				    g_u32LogFlags = LOG_PRIORITY_MASK & ~LOG_VERBOSE;
#else
    uint32_t				    g_u32LogFlags = LOG_PRIORITY_MASK;
#endif // defined(LOG_IN_RELEASE_BUILD)
    void* g_pvUserData = nullptr;

    int							g_iLogNameLen = 0;
    char						g_as8LogFileName[VX_MAX_PATH] = { 0 };

    class VxLogMgr
    {
    public:
        const int MAX_LOG_FUNCTIONS = 16;

        VxLogMgr() = default;

        void handleLog( void* userData, uint32_t u32LogFlags, const char* logMsg )
        {
            if( VxIsAppShuttingDown() )
            {
                return;
            }

            if( m_LogCallbackList.size() )
            {
                getLogMutex().lock();
                if( m_LogCallbackList.size() && logMsg )
                {
                    for( auto callback : m_LogCallbackList )
                    {
                        callback->onLogEvent( u32LogFlags, logMsg );
                    }
                }

                getLogMutex().unlock();
            }
        }

        // add a log handler
        void addLogHandler( ILogCallbackInterface* callbackHandler )
        {
            getLogMutex().lock();
            if( callbackHandler && ((int)m_LogCallbackList.size() < MAX_LOG_FUNCTIONS) )
            {
                m_LogCallbackList.emplace_back( callbackHandler );
            }

            getLogMutex().unlock();
        }

        // remove a log handler
        void removeLogHandler( ILogCallbackInterface* callbackHandler )
        {
            getLogMutex().lock();
            if( callbackHandler && m_LogCallbackList.size() )
            {
                for( auto iter = m_LogCallbackList.begin(); iter != m_LogCallbackList.end(); ++iter )
                {
                    if( (*iter) == callbackHandler )
                    {
                        m_LogCallbackList.erase( iter );
                        break;
                    }
                }
            }

            getLogMutex().unlock();
        }

        std::vector<ILogCallbackInterface*> m_LogCallbackList;
    };

    VxLogMgr& GetVxLogMgr()
    {
        static VxLogMgr g_VxLogMgr;
        return g_VxLogMgr;
    }

    void ExtendLogHandler( void* userData, uint32_t u32LogFlags, const char* logMsg )
    {
        GetVxLogMgr().handleLog( userData, u32LogFlags, logMsg );
    }
}

VxElapseTimer g_LogTimer;
static bool g_enableLogTimer{ false };

//============================================================================
void EnableLogTimer( bool enable )
{
    g_enableLogTimer = enable;
    if( g_enableLogTimer )
    {
        g_LogTimer.startTimer();
    }
}

//============================================================================
std::string GetLogTime( void )
{
    if( g_enableLogTimer )
    {
        const int maxTimeStrLen = 128;
        double elapsed = g_LogTimer.elapsedSec();
        std::array<char, maxTimeStrLen> strData;
        snprintf( strData.data(), maxTimeStrLen, "%3.3f ", elapsed);
        return strData.data();
    }

    return "";
}

//============================================================================
// add a log handler
void VxAddLogHandler( ILogCallbackInterface* callbackHandler )
{
    GetVxLogMgr().addLogHandler( callbackHandler );
}

//============================================================================
// remove a log handler
void VxRemoveLogHandler( ILogCallbackInterface* callbackHandler )
{
    GetVxLogMgr().removeLogHandler( callbackHandler );
}

//============================================================================
/// @brief return true if should log the given module
bool LogEnabled( ELogModule logModule )
{
    return (g_ModuleEnableLoggingFlags & logModule) ? true : false;
}

//============================================================================
void VxSetLogModuleEnable( ELogModule logModule, bool enable )
{
    uint64_t modFlag = (uint64_t)logModule;
    if( enable )
    {
        g_ModuleEnableLoggingFlags |= modFlag;
    }
    else
    {
        g_ModuleEnableLoggingFlags &= ~modFlag;
    }
}

//============================================================================
/// @brief set all the log module flags
void VxSetLogModuleFlags( uint64_t logModuleFlags )
{
    g_ModuleEnableLoggingFlags = logModuleFlags;
}

//============================================================================
/// @brief return all the log module flags
uint64_t VxGetLogModuleFlags( void )
{
    return g_ModuleEnableLoggingFlags;
}

//============================================================================
NLC_BEGIN_CDECLARES
void LogAppendLineFeed( char* buf, size_t sizeOfBuf );
void                    default_log_output( void* userData, uint32_t u32MsgType, const char* pLogMsg );
void                    vx_error( uint32_t u32MsgType, const char* msg, ... );

static LOG_FUNCTION		g_pfuncLogHandler = ExtendLogHandler;
static bool             g_enableDefaultHandler = true;
NLC_END_CDECLARES

//============================================================================
void LogModule( ELogModule eLog, uint32_t u32MsgType, const char* msg, ... )
{
    if( LOG_FATAL != u32MsgType )
    {
        if( !VxGetDebugLoggingEnable() )
        {
            return;
        }

        if( 0 == (g_u32LogFlags && u32MsgType) )
        {
            return; // don't log
        }

        if( !LogEnabled( eLog ) )
        {
            return;
        }
    }

    std::array<char, MAX_ERR_MSG_SIZE> strData;
        int timeLen = 0;
    if( g_enableLogTimer )
    {
        std::string timeStamp = GetLogTime();
        timeLen = timeStamp.length();
        strncpy( strData.data(), timeStamp.c_str(), MAX_ERR_MSG_SIZE - 1 );
        strData.data()[MAX_ERR_MSG_SIZE - 1] = 0;
    }

    va_list argList;
    va_start( argList, msg );
    vsnprintf( &strData.data()[timeLen], MAX_ERR_MSG_SIZE, msg, argList );
    strData.data()[MAX_ERR_MSG_SIZE - 2] = 0;

    LogAppendLineFeed( strData.data(), MAX_ERR_MSG_SIZE - 2 );
    va_end( argList );

    VxHandleLogMsg( u32MsgType, strData.data() );
}

//============================================================================
void LogModule2( ELogModule eLog, uint32_t u32MsgType, const char* msg )
{
    if( LOG_FATAL != u32MsgType )
    {
        if( !VxGetDebugLoggingEnable() )
        {
            return;
        }

        if( 0 == (g_u32LogFlags && u32MsgType) )
        {
            return; // don't log
        }

        if( !LogEnabled( eLog ) )
        {
            return;
        }
    }

    VxHandleLogMsg( u32MsgType, msg );
}

//============================================================================
/// a version of LogModule that can be called from c code
void LogCModule( int logModuleType, uint32_t u32MsgType, const char* msg, ... )
{
    if( LOG_FATAL != u32MsgType )
    {
        if( !VxGetDebugLoggingEnable() )
        {
            return;
        }

        if( 0 == (g_u32LogFlags && u32MsgType) )
        {
            return; // don't log
        }

        if( !LogEnabled( (ELogModule)logModuleType ) )
        {
            return;
        }
    }

    std::array<char, MAX_ERR_MSG_SIZE> strData;
    int timeLen = 0;
    if( g_enableLogTimer )
    {
        std::string timeStamp = GetLogTime();
        timeLen = timeStamp.length();
        strncpy( strData.data(), timeStamp.c_str(), MAX_ERR_MSG_SIZE - 1 );
        strData.data()[MAX_ERR_MSG_SIZE - 1] = 0;
    }

    va_list argList;
    va_start( argList, msg );
    vsnprintf( &strData.data()[timeLen], MAX_ERR_MSG_SIZE, msg, argList );
    strData.data()[MAX_ERR_MSG_SIZE - 2] = 0;

    LogAppendLineFeed( strData.data(), MAX_ERR_MSG_SIZE - 2 );
    va_end( argList );

    VxHandleLogMsg( u32MsgType, strData.data() );
}

NLC_BEGIN_CDECLARES
//============================================================================

//============================================================================
void VxSetLogLevelFlags( uint32_t u32LogFlags )
{
    g_u32LogFlags = u32LogFlags;
}

//============================================================================
uint32_t VxGetLogLevelFlags( void )
{
    return g_u32LogFlags;
}

//============================================================================
// enable/disable default log handler
void VxEnableDefaultLogHandler( bool enableDefaultHandler )
{
    g_enableDefaultHandler = enableDefaultHandler;
}

//============================================================================
//============================================================================
uint64_t VxGetModuleLogFlags( void )
{
    return g_ModuleEnableLoggingFlags;
}

//============================================================================
void  VxSetModuleLogFlags( uint64_t flags )
{
    g_ModuleEnableLoggingFlags = flags;
}

//============================================================================
void log_to_file( const char* pFileName, const char* pMsg )
{
    static bool firstLog = true;
    if( false == VxIsAppShuttingDown() )
    {
        if( pMsg )
        {
            int iMsgLen = strlen( pMsg );
            g_oFileLogMutex.lock();
            if( firstLog )
            {
                unlink( pFileName );
                firstLog = false;
            }

            FILE* poFile = fopen( pFileName, "ab+" );
            if( poFile )
            {
                fwrite( pMsg, iMsgLen, 1, poFile );
                fclose( poFile );
            }

            g_oFileLogMutex.unlock();
        }
    }
}

//============================================================================
void VxSetLogToFile( const char* pFileName )
{
    int strLen = strlen( g_as8LogFileName );
    if( (0 == g_iLogNameLen) && strLen )
    {
        // first time we set the log file delete any previous so we
        // don't keep filling up the log
        VxFileUtil::deleteFile( pFileName );
    }

    g_iLogNameLen = strLen;
    strncpy( g_as8LogFileName, pFileName, sizeof( g_as8LogFileName ) - 1 );
    g_as8LogFileName[ sizeof( g_as8LogFileName ) - 1 ] = 0;
}

//============================================================================
void LogAppendLineFeed( char* buf, size_t sizeOfBuf )
{
    size_t strLen = strlen( buf );
    if( ((int)sizeOfBuf > 2) && (0 < strLen) && (strLen < ((int)sizeOfBuf - 2)) )
    {
        if( buf[strLen - 1] != '\n' )
        {
            buf[strLen] = '\n';
            buf[strLen + 1] = 0;
        }
    }
}

//============================================================================
// This function is called by vx_assert() when the assertion fails.
void  vx_error_output( uint32_t u32MsgType, char* exp, char* file, int line )
{
    vx_error( u32MsgType, "** VX ASSERTION **\r\n expression: %s\r\n file: %s\r\n line: %d\r\n", exp, file, line );
    AppErr( eAppErrBadParameter, "ASSERTION: %s\r\n file: %s\r\n line: %d\r\n", exp, file, line );
}

//============================================================================
// This function is called by vx_assert2() when the assertion fails.
void vx_error_output2( uint32_t u32MsgType, char* exp, char* msg, char* file, int line )
{
    vx_error( u32MsgType, "** VX ASSERTION ***\r\n programmer says: %s\r\nexpression: %s\r\n file: %s\r\n line: %d\r\n", msg, exp, file, line );
    AppErr( eAppErrBadParameter, "ASSERTION: %s\r\n expression: %s\r\n file: %s\r\n line: %d\r\n", msg, exp, file, line );
}

//============================================================================
// This function is called when a serious situation is encountered which
// requires abortion of the program.
void vx_error( uint32_t u32MsgType, const char* msg, ... )
{
    std::string strData;
    strData.reserve( MAX_ERR_MSG_SIZE );

    va_list argList;
    va_start( argList, msg );
    vsnprintf( (char*)strData.c_str(), MAX_ERR_MSG_SIZE, msg, argList );
    ((char*)strData.c_str())[MAX_ERR_MSG_SIZE - 2] = 0;

    LogAppendLineFeed( (char*)strData.c_str(), MAX_ERR_MSG_SIZE - 2 );
    va_end( argList );

    LogMsg( u32MsgType, (char*)strData.c_str() ); // send to log
    if( LOG_FATAL == u32MsgType )
    {
        //exit(99);
    }
};

//============================================================================
/// default log handler
void default_log_output( void* userData, uint32_t u32MsgType, const char* pLogMsg )
{
    NLC_UNUSED( userData );

#ifdef TARGET_OS_ANDROID
#define MY_LOG_TAG "AppLog:"

    u32MsgType = (u32MsgType & LOG_PRIORITY_MASK);
    switch( u32MsgType )
    {
    case LOG_INFO:
        __android_log_write( ANDROID_LOG_INFO, MY_LOG_TAG, pLogMsg );
        break;

    case LOG_FATAL:
    case LOG_ASSERT:
    case LOG_ERROR:
        __android_log_write( ANDROID_LOG_ERROR, MY_LOG_TAG, pLogMsg );
        break;

    case LOG_WARN:
        __android_log_write( ANDROID_LOG_WARN, MY_LOG_TAG, pLogMsg );
        break;

    case LOG_VERBOSE:
        __android_log_write( ANDROID_LOG_VERBOSE, MY_LOG_TAG, pLogMsg );
        break;

    default:
        __android_log_write( ANDROID_LOG_DEBUG, MY_LOG_TAG, pLogMsg );
}
#else
#ifdef TARGET_OS_WINDOWS
    OutputDebugStringA( pLogMsg );
#else
    printf( "%s", pLogMsg );
    fflush( stdout );
#endif // TARGET_OS_WINDOWS
#endif// TARGET_OS_ANDROID
}

//============================================================================
void LogMsgVarg( uint32_t u32MsgType, const char* fmt, va_list argList )
{
    if( !VxGetDebugLoggingEnable() )
    {
        return;
    }

    if( 0 == (g_u32LogFlags && u32MsgType) )
    {
        return; // don't log
    }

    std::string strData;
    strData.reserve( MAX_ERR_MSG_SIZE );
    //va_start(argList, fmt );
    vsnprintf( (char *)strData.c_str(), MAX_ERR_MSG_SIZE, fmt, argList);
    ((char *)strData.c_str())[MAX_ERR_MSG_SIZE - 2] = 0;
    LogAppendLineFeed( (char *)strData.c_str(), MAX_ERR_MSG_SIZE - 2 );
    //va_end(argList);

    VxHandleLogMsg( u32MsgType, (char *)strData.c_str() );
}

//============================================================================
void LogMsg( uint32_t u32MsgType, const char* msg, ... )
{
    if( u32MsgType != LOG_FATAL )
    {
        if( !VxGetDebugLoggingEnable() )
        {
            return;
        }

        if( 0 == (g_u32LogFlags & u32MsgType) )
        {
            return; // don't log
        }
    }

    std::string strData;
    strData.reserve( MAX_ERR_MSG_SIZE );

    va_list argList;
    va_start( argList, msg );
    vsnprintf( (char*)strData.c_str(), MAX_ERR_MSG_SIZE, msg, argList );
    ((char*)strData.c_str())[MAX_ERR_MSG_SIZE - 2] = 0;

    LogAppendLineFeed( (char*)strData.c_str(), MAX_ERR_MSG_SIZE - 2 );
    va_end( argList );

    VxHandleLogMsg( u32MsgType, (char*)strData.c_str() );
}

//============================================================================
void LogMsg2( uint32_t u32MsgType, const char* msg )
{
    if( u32MsgType != LOG_FATAL )
    {
        if( !VxGetDebugLoggingEnable() )
        {
            return;
        }

        if( 0 == (g_u32LogFlags && u32MsgType) )
        {
            return; // don't log
        }
    }

    VxHandleLogMsg( u32MsgType, msg );
}


//============================================================================
void VxHandleLogMsg( uint32_t u32MsgType, const char* logMsg )
{
    if( g_iLogNameLen )
    {
        log_to_file( g_as8LogFileName, logMsg );
    }

    if( VxIsAppShuttingDown() )
    {
#ifdef TARGET_OS_ANDROID
        __android_log_write( ANDROID_LOG_INFO, "SHUTDOWN", logMsg );
#endif // TARGET_OS_ANDROID
        return;
    }

#if ENABLE_LOG_LIST
    AddLogEntry( u32MsgType, as8Buf );
#endif // ENABLE_LOG_LIST

    if( g_enableDefaultHandler )
    {
        default_log_output( g_pvUserData, u32MsgType, logMsg );
    }

    if( g_pfuncLogHandler )
    {
        g_pfuncLogHandler( g_pvUserData, u32MsgType, (char *)logMsg );
    }

    if( u32MsgType == LOG_FATAL )
    {
        vx_assert( false );
    }
}

//============================================================================
static void DumpMsg( uint32_t u32MsgType, int instance, int sampleCnt, char* msg )
{
    if( msg && strlen( msg ) )
    {
        LogMsg( u32MsgType, "Dump %d Cnt %d %s", instance, sampleCnt, msg );
    }
}

//============================================================================
void HexDump( uint32_t u32MsgType, unsigned char* data, int dataLen, int instance, char* msg )
{
    DumpMsg( u32MsgType, instance, dataLen, msg );
    char buf[132];

    while( dataLen )
    {
        int dataThisDump = dataLen;
        if( dataThisDump > 16 )
        {
            dataThisDump = 16;
        }

        int curIdx = 0;

        for( int i = 0; i < dataThisDump; ++i )
        {
            sprintf( &buf[curIdx], "%2.2x ", data[i] );
            curIdx += 3;
        }

        if( dataThisDump < 16 )
        {
            int spaces = 16 - dataLen;
            for( int i = 0; i < spaces; ++i )
            {
                buf[curIdx] = ' ';
                curIdx++;
            }
        }

        for( int i = 0; i < dataThisDump; ++i )
        {
            if( (buf[curIdx] < 20) || (buf[curIdx] > 127) || !isalnum( buf[curIdx] ) )
            {
                buf[curIdx] = '.';
            }
            else
            {
                sprintf( &buf[curIdx], "%c", data[i] );
            }

            curIdx++;
        }

        strcpy( &buf[curIdx], "\n" );
        LogMsg( u32MsgType, buf );
        data += dataThisDump;
        dataLen -= dataThisDump;
    }
}

//============================================================================
void DumpInt8( uint32_t u32MsgType, int8_t* data, int dataLen, int instance, char* msg )
{
    int sampleCnt = dataLen / sizeof( int8_t );
    DumpMsg( u32MsgType, instance, sampleCnt, msg );

    char outBuf[4096];
    outBuf[0] = 0;
    int sampleIdx = 0;
    //LogMsg( u32MsgType, "DumpInt8 cnt %d ", sampleCnt );
    while( sampleCnt >= 16 )
    {
        for( int i = 0; i < 16; i++ )
        {
            size_t curIdx = strlen( outBuf );
            sprintf( &outBuf[curIdx], "%d ", data[sampleIdx] );
            sampleIdx++;
        }

        LogMsg( u32MsgType, outBuf );
        outBuf[0] = 0;
        sampleCnt -= 16;
    }

    if( sampleCnt )
    {
        outBuf[0] = 0;
        while( sampleCnt )
        {
            size_t curIdx = strlen( outBuf );
            sprintf( &outBuf[curIdx], "%d ", data[sampleIdx] );
            sampleIdx++;
            sampleCnt--;
        }

        LogMsg( u32MsgType, outBuf );
    }
}

//============================================================================
void DumpInt16( uint32_t u32MsgType, int16_t* data, int dataLen, int instance, char* msg )
{
    int sampleCnt = dataLen / sizeof( int16_t );
    DumpMsg( u32MsgType, instance, sampleCnt, msg );

    char outBuf[4096];
    outBuf[0] = 0;
    int sampleIdx = 0;
    //LogMsg( u32MsgType, "DumpInt16 cnt %d ", sampleCnt );
    while( sampleCnt >= 16 )
    {
        for( int i = 0; i < 16; i++ )
        {
            size_t curIdx = strlen( outBuf );
            sprintf( &outBuf[curIdx], "%d ", data[sampleIdx] );
            sampleIdx++;
        }

        LogMsg( u32MsgType, outBuf );
        outBuf[0] = 0;
        sampleCnt -= 16;
    }

    if( sampleCnt )
    {
        outBuf[0] = 0;
        while( sampleCnt )
        {
            size_t curIdx = strlen( outBuf );
            sprintf( &outBuf[curIdx], "%d ", data[sampleIdx] );
            sampleIdx++;
            sampleCnt--;
        }

        LogMsg( u32MsgType, outBuf );
    }
}

//============================================================================
void DumpFloat( uint32_t u32MsgType, float* data, int dataLen, int instance, char* msg )
{
    int sampleCnt = dataLen / sizeof( float );
    DumpMsg( u32MsgType, instance, sampleCnt, msg );

    char outBuf[4096];
    outBuf[0] = 0;
    int sampleIdx = 0;
    //LogMsg( u32MsgType, "DumpFloat cnt %d ", sampleCnt );
    while( sampleCnt >= 16 )
    {
        for( int i = 0; i < 16; i++ )
        {
            size_t curIdx = strlen( outBuf );
            sprintf( &outBuf[curIdx], "%3.3f ", data[sampleIdx] );
            sampleIdx++;
        }

        LogMsg( u32MsgType, outBuf );
        outBuf[0] = 0;
        sampleCnt -= 16;
    }

    if( sampleCnt )
    {
        outBuf[0] = 0;
        while( sampleCnt )
        {
            size_t curIdx = strlen( outBuf );
            sprintf( &outBuf[curIdx], "%3.3f ", data[sampleIdx] );
            sampleIdx++;
            sampleCnt--;
        }

        LogMsg( u32MsgType, outBuf );
    }
}

NLC_END_CDECLARES
