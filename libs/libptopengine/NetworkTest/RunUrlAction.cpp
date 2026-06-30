//============================================================================
// Copyright (C) 2020 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "RunUrlAction.h"

#include <GuiInterface/IToGui.h>

#include <P2PEngine/P2PEngine.h>
#include <NetServices/NetServiceHdr.h>
#include <NetServices/NetServiceUtils.h>

#include <Network/NetworkMgr.h>
#include <UrlMgr/UrlMgr.h>
#include <NetServices/NetServiceUtils.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxParse.h>
#include <CoreLib/VxGlobals.h>

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

namespace
{
    //============================================================================
    void * UrlActionThreadFunc( void * pvContext )
    {
        VxThread* poThread = ( VxThread* )pvContext;
        poThread->setIsThreadRunning( true );
        RunUrlAction * netTest = ( RunUrlAction * )poThread->getThreadUserParam();
        if( netTest && false == poThread->isAborted() )
        {
            netTest->threadFuncRunUrlAction();
        }

        poThread->threadAboutToExit();
        return nullptr;
    }
}

//============================================================================
UrlActionInfo::UrlActionInfo()
    : m_Engine( GetPtoPEngine() )
{
}

//============================================================================
UrlActionInfo::UrlActionInfo( P2PEngine& engine, enum EHostType hostType, VxGUID& sessionId, enum ENetCmdType testType, const char* ptopUrl, const char* myUrl, 
                              UrlActionResultInterface* cbInterface, IConnectRequestCallback* cbConnectReq, enum EConnectReason connectReason )
    : m_Engine( engine )
    , m_HostType( hostType )
    , m_ResultCbInterface( cbInterface )
    , m_ConnectReqCbInterface( cbConnectReq )
    , m_TestType( testType )
    , m_MyUrl( myUrl )
    , m_RemoteUrl( ptopUrl )
    , m_ConnectReason( connectReason )
    , m_SessionId( sessionId )
{
    if( nullptr == myUrl )
    {
        m_MyUrl.setUrl( m_Engine.getMyOnlineUrl() );
    }
}

//============================================================================
UrlActionInfo::UrlActionInfo( const UrlActionInfo& rhs )
    : m_Engine( rhs.m_Engine )
    , m_HostType( rhs.m_HostType )
    , m_ResultCbInterface( rhs.m_ResultCbInterface )
    , m_ConnectReqCbInterface( rhs.m_ConnectReqCbInterface )
    , m_TestType( rhs.m_TestType )
    , m_MyUrl( rhs.m_MyUrl )
    , m_RemoteUrl( rhs.m_RemoteUrl )
    , m_ConnectReason( rhs.m_ConnectReason )
    , m_SessionId( rhs.m_SessionId )
{
}

//============================================================================
UrlActionInfo& UrlActionInfo::operator = ( const UrlActionInfo& rhs )
{
    if( this != &rhs )   
    {
        m_ResultCbInterface     = rhs.m_ResultCbInterface;
        m_ConnectReqCbInterface = rhs.m_ConnectReqCbInterface;
        m_TestType              = rhs.m_TestType;
        m_HostType              = rhs.m_HostType;
        m_MyUrl                 = rhs.m_MyUrl;
        m_RemoteUrl             = rhs.m_RemoteUrl;
        m_ConnectReason         = rhs.m_ConnectReason;
        m_SessionId             = rhs.m_SessionId;
    }

    return *this;
}

//============================================================================
bool UrlActionInfo::operator == ( const UrlActionInfo& rhs ) const
{
    return m_ResultCbInterface == rhs.m_ResultCbInterface &&
        m_ConnectReqCbInterface == rhs.m_ConnectReqCbInterface &&
        m_TestType == rhs.m_TestType &&
        m_HostType == rhs.m_HostType &&
        m_MyUrl == rhs.m_MyUrl &&
        m_RemoteUrl == rhs.m_RemoteUrl && m_ConnectReason == rhs.m_ConnectReason;
}

//============================================================================
bool UrlActionInfo::operator != ( const UrlActionInfo& rhs ) const
{
    return !(*this == rhs);
}

//============================================================================
std::string UrlActionInfo::getTestName( void )
{
    return DescribeNetCmdType( getNetCmdType() );
}

//============================================================================
//============================================================================
RunUrlAction::RunUrlAction( P2PEngine& engine, EngineSettings& engineSettings, NetServicesMgr& netServicesMgr, NetServiceUtils& netServiceUtils )
    : m_Engine( engine )
    , m_EngineSettings( engineSettings )
    , m_NetServicesMgr( netServicesMgr )
    , m_NetServiceUtils( netServiceUtils )
{
    LogMsg( LOG_VERBOSE, "RunUrlAction::RunUrlAction" );
}

//============================================================================
void RunUrlAction::runTestShutdown( void )
{
    m_RunActionThread.abortThreadRun( true );
}

//============================================================================
void RunUrlAction::runUrlAction( VxGUID& sessionId, enum ENetCmdType netCmdType, const char* ptopUrl, const char* myUrl, UrlActionResultInterface* cbInterface, 
                                 IConnectRequestCallback* cbConnectRequest, enum EHostType hostType, enum EConnectReason connectReason )
{
    UrlActionInfo urlAction( getEngine(), hostType, sessionId, netCmdType, ptopUrl, myUrl, cbInterface, cbConnectRequest, connectReason );
    std::string actionName = urlAction.getTestName();
    if( eNetCmdQueryHostOnlineIdReq != netCmdType && !urlAction.getMyVxUrl().validateUrl( true ) )
    {
        // if have callback interface then no need to send testing message
        LogModule( eLogRunTest, LOG_DEBUG, "RunUrlAction::runUrlAction Local URL is invalid" );
        sendRunTestStatus( urlAction, actionName, eRunTestStatusTestFail, eNetCmdErrorBadParameter, "My URL is invalid" );
        doRunTestFailed( urlAction, actionName, eRunTestStatusTestBadParam, eNetCmdErrorBadParameter );
    }
    else if( !urlAction.getRemoteVxUrl().validateUrl( false ) )
    {
        LogModule( eLogRunTest, LOG_DEBUG, "RunUrlAction::runUrlAction Remote URL is invalid" );
        sendRunTestStatus( urlAction, actionName, eRunTestStatusTestFail, eNetCmdErrorBadParameter, "Test URL is invalid" );
        doRunTestFailed( urlAction, actionName, eRunTestStatusTestBadParam, eNetCmdErrorBadParameter );
    }
    else
    {
        m_ActionListMutex.lock();
        // if already exists in que then do not que again
        bool existsInQue = false;
        for( UrlActionInfo& urlActionInfo : m_UrlActionList )
        {
            if( urlActionInfo == urlAction )
            {
                existsInQue = true;
                break;
            }
        }

        if( existsInQue )
        {
            m_ActionListMutex.unlock();
            sendRunTestStatus( urlAction, actionName, eRunTestStatusTestFail, eNetCmdErrorTxFailed, "URL action already qued for run" );
            doRunTestFailed( urlAction, actionName, eRunTestStatusAlreadyQueued, eNetCmdErrorTxFailed );
        }
        else
        {
            m_UrlActionList.emplace_back( urlAction );
            m_ActionListMutex.unlock();
            startUrlActionThread();
        }
    }
}

//============================================================================
void RunUrlAction::startUrlActionThread( void )
{
    if( !isThreadRunningActions() && !m_RunActionThread.isAborted() )
    {
        m_ThreadIsRunningActions = true;
        m_RunActionThread.startThread( ( VX_THREAD_FUNCTION_T )UrlActionThreadFunc, this, "UrlActionThread" );
    }
}

//============================================================================
bool RunUrlAction::isThreadRunningActions( void )
{
    return m_ThreadIsRunningActions;
}

//============================================================================
void RunUrlAction::threadFuncRunUrlAction( void )
{
    UrlActionInfo urlAction;
    while( m_ThreadIsRunningActions && !m_RunActionThread.isAborted() )
    {
        m_ActionListMutex.lock();
        size_t listSize = m_UrlActionList.size();
        if( listSize )
        {
            urlAction = m_UrlActionList.front();
            m_UrlActionList.erase( m_UrlActionList.begin() );
            m_ActionListMutex.unlock();
            doUrlAction( urlAction );
        }
        else
        {
            m_ThreadIsRunningActions = false;
            m_ActionListMutex.unlock();
            break;
        }
    }    
}

//============================================================================
ERunTestStatus RunUrlAction::doUrlAction( UrlActionInfo& urlAction )
{
    std::string actionName = urlAction.getTestName();
    std::string nodeUrl = urlAction.getRemoteUrl();
    enum ENetCmdType netCmdType = urlAction.getNetCmdType();
    if( eNetCmdQueryHostOnlineIdReq == netCmdType && urlAction.getResultInterface() )
    {
        // if we are quering the host id a previous query for another service may have already gotten it for the given ip/port
        VxGUID hostId;
        if( m_Engine.getUrlMgr().lookupOnlineId( nodeUrl, hostId ) )
        {
            urlAction.getResultInterface()->callbackQueryIdSuccess( urlAction, hostId );
            return eRunTestStatusTestSuccess;
        }
    }

    if( urlAction.getHostType() == eHostTypeNetwork )
    {
        LogMsg( LOG_VERBOSE, "RunUrlAction::%s url %s net cmd %s", __func__,
            nodeUrl.c_str(), DescribeNetCmdType( netCmdType ) );
    }

	VxSktConnectSimple netServConn;
	std::string strHost;
	std::string strFile;
	uint16_t u16Port = 0;
	VxElapseTimer testTimer;
	double connectTime = 0;
	double sendTime= 0;
	double reponseTime= 0;

    EIpAddrType addrType = eIpAddrTypeUnknown;
    switch( netCmdType )
    {
    case eNetCmdIsMyPortOpenReq:
        // for port open test we have to connect with the correct ipv type (ipv4 or ipv6)
        addrType = m_Engine.getEngineSettings().getUseIpv6() ? eIpAddrTypeIpv6 : eIpAddrTypeIpv4;
        break;

    case eNetCmdHostPing:
    case eNetCmdQueryHostOnlineIdReq:
        break;
    default:
        LogModule( eLogRunTest, LOG_INFO, "RunUrlAction: unsupport net cmd %s", DescribeNetCmdType( netCmdType ) );
        sendRunTestStatus( urlAction, actionName, eRunTestStatusTestBadParam, eNetCmdErrorBadParameter, ": Unsupport net cmd %s", DescribeNetCmdType( netCmdType ) );
        doRunTestFailed( urlAction, actionName, eRunTestStatusTestBadParam, eNetCmdErrorBadParameter );
        return eRunTestStatusConnectFail;
    }

    if(LogEnabled(eLogRunTest))LogModule( eLogRunTest, LOG_INFO, "RunUrlAction: sec %3.3f : connecting to %s thread 0x%x",
               testTimer.elapsedSec(), nodeUrl.c_str(), VxGetCurrentThreadId() );
    std::string resolveIp;
	if( false == netServConn.connectToWebsite(	nodeUrl.c_str(), 
		                                        strHost, 
		                                        strFile, 
		                                        u16Port, 
                                                resolveIp,
                                                addrType,
		                                        NETSERVICE_CONNECT_TIMEOUT ) )
	{
        if( eNetCmdQueryHostOnlineIdReq == netCmdType && ( urlAction.getHostType() == eHostTypeNetwork || urlAction.getHostType() == eHostTypeNetwork ) )
        {
            hostQueryIdConnectFailed( urlAction );
            LogMsg( LOG_VERBOSE, "RunUrlAction::%s url %s net cmd %s", __func__,
                nodeUrl.c_str(), DescribeNetCmdType( netCmdType ) );
        }

        sendRunTestStatus( urlAction, actionName, eRunTestStatusConnectFail, eNetCmdErrorConnectFailed, "Could not connect to %s IP %s..tried %3.3f seconds Please check settings", 
                            nodeUrl.c_str(), resolveIp.c_str(), testTimer.elapsedSec() );

		doRunTestFailed( urlAction, actionName, eRunTestStatusConnectFail, eNetCmdErrorConnectFailed );

		return eRunTestStatusConnectFail;
	}

    if(LogEnabled(eLogRunTest))netServConn.dumpConnectionInfo();
	std::string strNetActionUrl;
    uint16_t myPort = 0;
    int rxCmdHeaderTimeout = PING_TEST_RX_HDR_TIMEOUT;
    int rxCmdDataTimeout = PING_TEST_RX_DATA_TIMEOUT;

    switch( netCmdType )
    {
    case eNetCmdHostPing:
        rxCmdHeaderTimeout = PING_TEST_RX_HDR_TIMEOUT;
        rxCmdDataTimeout = PING_TEST_RX_DATA_TIMEOUT;
        m_NetServiceUtils.buildPingTestUrl( &netServConn, strNetActionUrl );
        break;

    case eNetCmdIsMyPortOpenReq:
        myPort = urlAction.getMyVxUrl().getPort();
        if( 0 == myPort )
        {
            if(LogEnabled(eLogRunTest))LogModule( eLogRunTest, LOG_INFO, "RunUrlAction: Invalid listen port %d", myPort );
            sendRunTestStatus( urlAction, actionName, eRunTestStatusTestBadParam, eNetCmdErrorBadParameter, ": Invalid listen port %d", myPort );
            netServConn.closeSkt();
            doRunTestFailed( urlAction, actionName, eRunTestStatusTestBadParam, eNetCmdErrorBadParameter );
            return eRunTestStatusTestBadParam;
        }

        rxCmdHeaderTimeout = IS_PORT_OPEN_RX_HDR_TIMEOUT;
        rxCmdDataTimeout = IS_PORT_OPEN_RX_DATA_TIMEOUT;
        m_NetServiceUtils.buildIsMyPortOpenUrl( &netServConn, strNetActionUrl, myPort );
        break;

    case eNetCmdQueryHostOnlineIdReq:
        rxCmdHeaderTimeout = QUERY_HOST_ID_RX_HDR_TIMEOUT;
        rxCmdDataTimeout = QUERY_HOST_ID_RX_DATA_TIMEOUT;
        m_NetServiceUtils.buildQueryHostIdUrl( &netServConn, strNetActionUrl );
        break;

    default:
        netServConn.closeSkt();
        doRunTestFailed( urlAction, actionName, eRunTestStatusConnectFail, eNetCmdErrorConnectFailed );
        return eRunTestStatusConnectFail;
    }

    connectTime = testTimer.elapsedSec();
    if(LogEnabled(eLogRunTest))LogModule( eLogRunTest, LOG_INFO, "RunUrlAction: action url %s", strNetActionUrl.c_str() );

	bool wasSent = m_NetServiceUtils.sendNetServiceRequest( netCmdType, &netServConn, strNetActionUrl, 4000 );
	if( !wasSent )
	{
        netServConn.closeSkt();
        LogModule( eLogRunTest, LOG_ERROR, "RunUrlAction: sendData error %d", netServConn.getLastError() );
        sendRunTestStatus( urlAction, actionName, eRunTestStatusConnectionDropped, eNetCmdErrorTxFailed,
                          "Connected to %s but connection was dropped (wrong network key ?) %s", nodeUrl.c_str(), m_Engine.getNetworkMgr().getNetworkKey().c_str() );
		return doRunTestFailed( urlAction, actionName, eRunTestStatusConnectionDropped, eNetCmdErrorTxFailed );
	}

	sendTime = testTimer.elapsedSec();
    VxSleep( 100 ); // wait a bit for the response

	char rxBuf[ 513 ];
    rxBuf[ 0 ] = 0;
	NetServiceHdr netServiceHdr;
	if( false == m_NetServiceUtils.rxNetServiceCmd( eNetCmdQueryHostOnlineIdReply,
                                                    &netServConn, 
													rxBuf, 
													sizeof( rxBuf ) - 1, 
													netServiceHdr, 
                                                    rxCmdHeaderTimeout,
                                                    rxCmdDataTimeout ) )
	{
        netServConn.closeSkt();
		sendRunTestStatus( urlAction, actionName, eRunTestStatusConnectionDropped, eNetCmdErrorResponseTimedOut,
			"%s Connected to %s but failed to respond (wrong network key ?) thread 0x%x", actionName.c_str(), nodeUrl.c_str(), VxGetCurrentThreadId() );
		return doRunTestFailed( urlAction, actionName, eRunTestStatusConnectionDropped, eNetCmdErrorResponseTimedOut );
	}

    netServConn.closeSkt();

    rxBuf[ sizeof( rxBuf ) - 1 ] = 0;
	std::string content = rxBuf;
	reponseTime = testTimer.elapsedSec();
    if(LogEnabled(eLogRunTest))LogModule( eLogRunTest, LOG_INFO, "RunUrlAction: response len %d total time %3.3fsec connect %3.3fsec send %3.3fsec response %3.3f sec thread 0x%x",
		content.length(),
		reponseTime, connectTime, sendTime - connectTime, reponseTime - sendTime, VxGetCurrentThreadId() );
	if( 0 == content.length() )
	{
        LogModule( eLogRunTest, LOG_ERROR, "RunUrlAction: no content in response" );
		sendRunTestStatus( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError(), "Invalid response content %s", content.c_str() );
		return doRunTestFailed( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
	}

	const char* contentBuf = content.c_str();
    if( '/' != contentBuf[content.length() -1] )
    {
        LogModule( eLogRunTest, LOG_ERROR, "%s no trailing / in content thread 0x%x", actionName.c_str(), VxGetCurrentThreadId() );
        sendRunTestStatus( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError(), "Invalid response content %s", content.c_str() );
        return doRunTestFailed( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
    }

    ((char *)contentBuf)[content.length() -1] = 0;

    std::string strPayload = content;
    if( content.empty() )
    {
        LogModule( eLogRunTest, LOG_ERROR, "%s No content thread 0x%x", actionName.c_str(), VxGetCurrentThreadId() );
        sendRunTestStatus( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError(), "Invalid host id %s", content.c_str() );
        return doRunTestFailed( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
    }

    if( eNetCmdHostPing == netCmdType )
    {
        if( strPayload.empty() || strPayload.length() < 5  )
        {
            LogModule( eLogRunTest, LOG_ERROR, "RunUrlAction no content" );
            sendRunTestStatus( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError(), "%s No PONG content", actionName.c_str() );
            return doRunTestFailed(  urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
        }

        sendTestLog( urlAction, actionName, "%s PONG Content (%s)", actionName.c_str(), strPayload.c_str() );
        std::string strPongSig = strPayload.substr( 0, 5 );
        if( strPongSig != "PONG-" )
        {
            sendRunTestStatus( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError(), "Content did not contain PONG- %s", content.c_str() );
            return doRunTestFailed( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
        }
        else
        {
            std::string strMyIP = strPayload.substr( 5, strPayload.length() - 5 );
            if( !VxIsIPv4Address( strMyIP.c_str(), true ) )
	        {
		        LogMsg( LOG_VERBOSE, "fetchExternalIpAddress Invalid ip v4 addr %s", strMyIP.c_str() );
		        return doRunTestFailed( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
	        }
            else
            {
                LogModule( eLogRunTest, LOG_VERBOSE, "%s PING test success .. My IP is %s", actionName.c_str(), strMyIP.c_str() );
                sendTestLog( urlAction, actionName, "Success. PONG returned My IP (%s)", strMyIP.c_str() );
                sendTestLog( urlAction, actionName, "Elapsed Seconds Connect %3.3f sec Send %3.3f sec Respond %3.3f sec", connectTime, sendTime - connectTime, reponseTime - sendTime );
                if( urlAction.getResultInterface() )
                {
                    urlAction.getResultInterface()->callbackPingSuccess( urlAction, strMyIP.c_str() );
                }

                m_Engine.getNetStatusAccum().setExternalIpAddress( strMyIP );
            }
        }
    }
    else if( eNetCmdQueryHostOnlineIdReq == netCmdType )
    {
        VxGUID hostId;
        hostId.fromVxGUIDHexString( strPayload.c_str() );
        if( !hostId.isValid() )
        {
            LogMsg( LOG_ERROR, "Query Host Online Id %s Invalid Content (%3.3f sec) thread 0x%x", content.c_str(), testTimer.elapsedSec(), VxGetCurrentThreadId() );
            sendRunTestStatus( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError(), "Invalid host id %s", content.c_str() );
            netServConn.closeSkt();
            return doRunTestFailed( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
        }

        std::string hostIdStr = hostId.toHexString();
        if(LogEnabled(eLogRunTest))LogModule( eLogRunTest, LOG_VERBOSE, "test success %s host id %s thread 0x%x", actionName.c_str(), hostIdStr.c_str(), VxGetCurrentThreadId() );
        sendTestLog( urlAction, actionName, "Action complete with Id %s Elapsed Seconds Connect %3.3fsec Send %3.3fsec Respond %3.3f sec", hostIdStr.c_str(), connectTime, sendTime - connectTime, reponseTime - sendTime );
        if( urlAction.getResultInterface() )
        {
            urlAction.getResultInterface()->callbackQueryIdSuccess( urlAction, hostId );
        }
    }
    else if( eNetCmdIsMyPortOpenReq  == netCmdType )
    {
        std::vector<std::string> contentParts;
        StdStringSplit( content, '-', contentParts );
        if( 2 != contentParts.size() )
        {
            LogModule( eLogRunTest, LOG_ERROR, "NetActionIsMyPortOpen::doAction: not enough parts to content" );
            sendRunTestStatus( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError(), "Invalid response content %s", content.c_str() );
            return doRunTestFailed( urlAction, actionName, eRunTestStatusInvalidResponse, netServiceHdr.getError() );
        }

        std::string retMyExternalIp = contentParts[1];
        std::string strPayload = contentParts[0];
        int iIsOpen = atoi( contentParts[0].c_str() );
        // m_Engine.getNetStatusAccum().setIpAddress( retMyExternalIp );

        if(LogEnabled(eLogRunTest))LogModule( eLogRunTest, LOG_VERBOSE, "NetActionIsMyPortOpen::doAction: direct connect %s my ip %s result %d thread 0x%x", strPayload.c_str(), retMyExternalIp.c_str(), iIsOpen, VxGetCurrentThreadId() );
        if( iIsOpen )
        {
            sendRunTestStatus(  urlAction, actionName, eRunTestStatusMyPortIsOpen, eNetCmdErrorNone, "My ip %s port %d is open", retMyExternalIp.c_str(), myPort );
        }
        else
        {
            sendRunTestStatus(  urlAction, actionName, eRunTestStatusMyPortIsClosed, netServiceHdr.getError(), "My ip %s port %d is NOT open (Relay will be required)", retMyExternalIp.c_str(), myPort );
        }

        if( urlAction.getResultInterface() )
        {
            urlAction.getResultInterface()->callbackConnectionTestSuccess( urlAction, iIsOpen, retMyExternalIp.c_str() );
        }

        sendTestLog(  urlAction, actionName, "Elapsed Seconds Connect %3.3f sec Send %3.3f sec Respond %3.3f sec", connectTime, sendTime - connectTime, reponseTime - sendTime );    
    }


    return doRunTestSuccess(  urlAction, actionName );
}

//============================================================================
ERunTestStatus RunUrlAction::doRunTestFailed( UrlActionInfo& urlAction, std::string& urlActionName, ERunTestStatus eTestStatus, ENetCmdError cmdErr )
{
    if( urlAction.getResultInterface() )
    {
        urlAction.getResultInterface()->callbackActionFailed( urlAction, eTestStatus, cmdErr );
    }

    sendRunTestStatus( urlAction, urlActionName, eTestStatus, cmdErr, "" );
	sendRunTestStatus( urlAction, urlActionName, eRunTestStatusTestCompleteFail, cmdErr, "" );
	return eRunTestStatusTestCompleteFail;
}

//============================================================================
ERunTestStatus RunUrlAction::doRunTestSuccess( UrlActionInfo& urlAction, std::string& urlActionName )
{
	sendRunTestStatus( urlAction, urlActionName, eRunTestStatusTestCompleteSuccess, eNetCmdErrorNone, "" );
	return eRunTestStatusTestCompleteSuccess;
}

//============================================================================
void RunUrlAction::sendRunTestStatus( UrlActionInfo& urlAction, std::string& urlActionName, enum ERunTestStatus eStatus, enum ENetCmdError cmdErr, const char* msg, ... )
{
    char as8Buf[ 1024 ];
    va_list argList;
    va_start( argList, msg );
    vsnprintf( as8Buf, sizeof( as8Buf ), msg, argList );
    as8Buf[ sizeof( as8Buf ) - 1 ] = 0;
    va_end( argList );
    std::string cmdMsg = as8Buf;
    cmdMsg += " - ";
    cmdMsg += DescribeNetCmdError( cmdErr );

    if( !urlAction.getResultInterface() )
    {
        IToGui::getIToGui().toGuiRunTestStatus( urlActionName.c_str(), eStatus, cmdMsg.c_str() );
    }
    else
    {
        urlAction.getResultInterface()->callbackActionStatus( urlAction, eStatus, cmdErr, cmdMsg.c_str() );
    }
}

//============================================================================
void RunUrlAction::sendTestLog( UrlActionInfo& urlAction, std::string& urlActionName, const char* msg, ... )
{
    if( !urlAction.getResultInterface() )
    {
        char as8Buf[1024];
        va_list argList;
        va_start( argList, msg );
        vsnprintf( as8Buf, sizeof( as8Buf ), msg, argList );
        as8Buf[sizeof( as8Buf ) - 1] = 0;
        va_end( argList );
        IToGui::getIToGui().toGuiRunTestStatus( urlActionName.c_str(), eRunTestStatusLogMsg, as8Buf );
    }
}

//============================================================================
void RunUrlAction::hostQueryIdConnectFailed( UrlActionInfo& urlInfo )
{
    EHostType hostType = urlInfo.getHostType();
    EAppErr appErr{ eAppErrUnknown };
    bool showPopup = true;
    static bool hasShownNetworkHostConnectFail = false;
    static bool hasShownConnectTestHostConnectFail = false;
    switch( hostType )
    {
    case eHostTypeConnectTest:
        appErr = eAppPopupErrConnectTestHostConnectFail;
        if(!hasShownConnectTestHostConnectFail )
        {
            hasShownConnectTestHostConnectFail = true;
        }
        else
        {
            showPopup = false;
        }
        break;
    case eHostTypeNetwork:
        appErr = eAppPopupErrNetworkHostConnectFail;
        if(!hasShownNetworkHostConnectFail )
        {
            hasShownNetworkHostConnectFail = true;
        }
        else
        {
            showPopup = false;
        }
        break;
    default:
        LogMsg( LOG_ERROR, "RunUrlAction::%s unknown host type for url %s", __func__,
            urlInfo.getRemoteVxUrl().getUrl().c_str() );
        return;
    }

    if( showPopup )
    {
        m_Engine.getToGui().toGuiAppPopupErr( appErr, urlInfo.getRemoteVxUrl().getUrl().c_str() );
    }
    else
    {
        LogMsg( LOG_ERROR, "RunUrlAction::%s connect failed to %s", __func__,
            urlInfo.getRemoteVxUrl().getUrl().c_str() );
    }
}