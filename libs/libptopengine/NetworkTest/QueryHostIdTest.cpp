//============================================================================
// Copyright (C) 2014 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "QueryHostIdTest.h"

#include <P2PEngine/P2PEngine.h>
#include <NetServices/NetServiceHdr.h>
#include <NetServices/NetServiceUtils.h>

#include <Network/NetworkMgr.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxParse.h>

#include <CoreLib/VxSktUtil.h>
#include <NetLib/VxPeerMgr.h>
#include <NetLib/VxSktConnectSimple.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# pragma warning(disable: 4355) //'this' : used in base member initializer list
#endif //_MSC_VER

//============================================================================
QueryHostIdTest::QueryHostIdTest( P2PEngine& engine, EngineSettings& engineSettings, NetServicesMgr& netServicesMgr, NetServiceUtils& netServiceUtils )
    : NetworkTestBase( engine, engineSettings, netServicesMgr, netServiceUtils )
{
    setTestName( "QUERY HOST ID: " );
}

//============================================================================
void QueryHostIdTest::runTestShutdown( void )
{
    m_RunTestThread.abortThreadRun( true );
}

//============================================================================
void QueryHostIdTest::fromGuiRunQueryHostIdTest( void )
{
    if ( !m_RunTestThread.isThreadRunning() )
	{
        m_TestIsRunning = true;
        LogModule( eLogRunTest, LOG_INFO, "QueryHostIdTest::fromGuiRunQueryHostIdTest thread 0x%x",VxGetCurrentThreadId() );
        sendTestLog( "Starting Query Host Id test" );
        startNetworkTest();
    }
    else
    {
        LogModule( eLogRunTest, LOG_INFO, "QueryHostIdTest already running fromGuiRunQueryHostIdTest thread 0x%x",VxGetCurrentThreadId() );
        sendTestLog( "Already Started Query Host Id test" );
    }
}

//============================================================================
void QueryHostIdTest::threadRunNetworkTest( void )
{
    std::string netHostUrl;

    m_EngineSettings.getNetworkHostUrl( netHostUrl );
    LogModule( eLogRunTest, LOG_INFO, "attempting connection test with host test url %s thread 0x%x", 
               netHostUrl.c_str(), VxGetCurrentThreadId() );
    doRunTest( netHostUrl );
}

//============================================================================
ERunTestStatus QueryHostIdTest::doRunTest( std::string& nodeUrl )
{
    std::string testName = getTestName();
	sendTestLog(	"start %s node %s \n", 
					testName.c_str(), 
					nodeUrl.c_str());

	VxSktConnectSimple netServConn;
	std::string strHost;
	std::string strFile;
	uint16_t u16Port;
	VxElapseTimer testTimer;
	double connectTime = 0;
	double sendTime= 0;
	double reponseTime= 0;
	std::string strResolveIp;

    LogModule( eLogRunTest, LOG_INFO, "QueryHostIdTest: sec %3.3f : connecting to %s thread 0x%x", 
               testTimer.elapsedSec(), nodeUrl.c_str(), VxGetCurrentThreadId() );
	if( false == netServConn.connectToWebsite(	nodeUrl.c_str(), 
		strHost, 
		strFile, 
		u16Port, 
		strResolveIp,
		eIpAddrTypeUnknown,
		NETSERVICE_CONNECT_TIMEOUT ) )
	{
        sendRunTestStatus( eRunTestStatusConnectFail, "%s Could not connect to %s IP %s..tried %3.3f seconds Please check settings", 
                        getTestName().c_str(), nodeUrl.c_str(), strResolveIp.c_str(), testTimer.elapsedSec() );

		doRunTestFailed();

		return eRunTestStatusConnectFail;
	}

	if(LogEnabled(eLogConnect))netServConn.dumpConnectionInfo();
	std::string strNetActionUrl;
	m_NetServiceUtils.buildQueryHostIdUrl( &netServConn, strNetActionUrl );
    LogModule( eLogRunTest, LOG_INFO, "QueryHostIdTest: action url %s", strNetActionUrl.c_str() );
	connectTime = testTimer.elapsedSec();
    LogModule( eLogRunTest, LOG_INFO, "QueryHostIdTest: sec %3.3f : sending %d action data", 
               connectTime, (int)strNetActionUrl.length() );

	int32_t rc = netServConn.sendData( strNetActionUrl.c_str(), (int)strNetActionUrl.length() );
	if( rc )
	{
        LogModule( eLogRunTest, LOG_ERROR, "QueryHostIdTest: sendData error %d", rc );
        sendRunTestStatus( eRunTestStatusConnectionDropped,
			"%s Connected to %s but dropped connection (wrong network key ?) %s", testName.c_str(), nodeUrl.c_str(), m_Engine.getNetworkMgr().getNetworkKey() );
		return doRunTestFailed();
	}

	sendTime = testTimer.elapsedSec();
	LogMsg( LOG_INFO, "QueryHostIdTest: sec %3.3f : waiting for is port open response", sendTime );
	char rxBuf[ 513 ];
    rxBuf[ 0 ] = 0;
	NetServiceHdr netServiceHdr;
	if( false == m_NetServiceUtils.rxNetServiceCmd( eNetCmdQueryHostOnlineIdReply,
													&netServConn, 
													rxBuf, 
													sizeof( rxBuf ) - 1, 
													netServiceHdr, 
                                                    QUERY_HOST_ID_RX_HDR_TIMEOUT,
                                                    QUERY_HOST_ID_RX_DATA_TIMEOUT ) )
	{
		sendRunTestStatus( eRunTestStatusConnectionDropped,
			"%s Connected to %s but failed to respond (wrong network key ?)", testName.c_str(), nodeUrl.c_str() );
		return doRunTestFailed();
	}

    rxBuf[ sizeof( rxBuf ) - 1 ] = 0;
	std::string content = rxBuf;
	reponseTime = testTimer.elapsedSec();
    LogModule( eLogRunTest, LOG_INFO, "QueryHostIdTest: response len %d total time %3.3f sec connect %3.3fsec send %3.3fsec response %3.3f sec",
		content.length(),
		reponseTime, connectTime, sendTime - connectTime, reponseTime - sendTime );
	if( 0 == content.length() )
	{
        LogModule( eLogRunTest, LOG_ERROR, "QueryHostIdTest: no content in response" );
		sendRunTestStatus( eRunTestStatusInvalidResponse, "%s invalid response content %s\n", testName.c_str(), content.c_str() );
		return doRunTestFailed();
	}

	const char* contentBuf = content.c_str();
	if( '/' != contentBuf[content.length() -1] )
	{
        LogModule( eLogRunTest, LOG_ERROR, "QueryHostIdTest no trailing / in content" );
		sendRunTestStatus( eRunTestStatusInvalidResponse, "%s invalid response content %s\n", testName.c_str(), content.c_str() );
		return doRunTestFailed();
	}

	((char *)contentBuf)[content.length() -1] = 0;

	std::string strPayload = content;
    if( content.empty() )
    {
        LogModule( eLogRunTest, LOG_ERROR, "QueryHostIdTest no host id in content" );
        sendRunTestStatus( eRunTestStatusInvalidResponse, "%s invalid host id %s\n", testName.c_str(), content.c_str() );
        return doRunTestFailed();
    }

    VxGUID hostId;
    hostId.fromVxGUIDHexString( strPayload.c_str() );
    if( !hostId.isValid() )
    {
        LogMsg( LOG_ERROR, "Query Host Online Id %s Invalid Content (%3.3f sec)", content.c_str(), testTimer.elapsedSec() );
        sendRunTestStatus( eRunTestStatusInvalidResponse, "%s invalid host id %s\n", testName.c_str(), content.c_str() );
        return doRunTestFailed();
    }

    std::string hostIdStr = hostId.toHexString();
    LogModule( eLogRunTest, LOG_VERBOSE, "Test success %s host id %s", testName.c_str(), hostIdStr.c_str() );
	sendTestLog( "Test %s complete with Id %s Elapsed Seconds Connect %3.3f sec Send %3.3f sec Respond %3.3f sec", testName.c_str(), hostIdStr.c_str(), connectTime, sendTime - connectTime, reponseTime - sendTime );
	return doRunTestSuccess( );
}

//============================================================================
ERunTestStatus QueryHostIdTest::doRunTestFailed()
{
	sendRunTestStatus( eRunTestStatusTestComplete,
		"\n" );
	return eRunTestStatusTestComplete;
}

//============================================================================
ERunTestStatus QueryHostIdTest::doRunTestSuccess( void )
{
	sendRunTestStatus( eRunTestStatusTestComplete,
		"\n" );
	return eRunTestStatusTestComplete;
}

