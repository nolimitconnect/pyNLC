//============================================================================
// Copyright (C) 2014 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "NetServiceUtils.h"

#include <P2PEngine/P2PEngine.h>
#include <Network/NetworkMgr.h>
#include <NetServices/NetServiceHdr.h>
#include <NetServices/NetServicesMgr.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxParse.h>
#include <NetLib/VxSktConnectSimple.h>
#include <NetLib/VxSktBase.h>
#include <PktLib/PktAnnounce.h>
#include <PktLib/PktsTestConnection.h>
#include <PktLib/PktsQueryHostUrl.h>

#include <array>
#include <memory>
#include <stdlib.h>
#include <string.h>

namespace
{
	const int		MAX_URL_VERSION_LEN_DIGITS			= 3;
	const int		MAX_CONTENT_LEN_DIGITS				= 13;
	const int		MAX_CMD_VERSION_LEN_DIGITS			= 3;
	const int		MAX_ERROR_LEN_DIGITS				= 8;
	const int		MAX_NET_CMD_LEN_CHARS				= 19;
	const int		MAX_NET_SERVICE_URL_LEN				= 16384;
}

//============================================================================
NetServiceUtils::NetServiceUtils( P2PEngine& engine )
: m_Engine( engine )
, m_NetworkMgr( engine.getNetworkMgr() )
, m_PktAnn( engine.getMyPktAnnounce() )
{
}

//============================================================================
std::string NetServiceUtils::getNetworkKey( void )
{
    return m_NetworkMgr.getNetworkKey();
}

//============================================================================
bool NetServiceUtils::verifyAllDataArrivedOfNetServiceUrl( std::shared_ptr<VxSktBase>& sktBase )
{
	//ptop://GET /Crypto Key/total length of data/ 
	// 12 + 32 + 1 + MAX_CONTENT_LEN_DIGITS + 1

	int iSktDataLen = sktBase->getSktBufDataLen();
	if( iSktDataLen < NET_SERVICE_HDR_LEN )
	{
		LogMsg( LOG_VERBOSE, "verifyAllDataArrivedOfNetServiceUrl: only %d of len %d recieved. waiting for more", iSktDataLen, NET_SERVICE_HDR_LEN );
		sktBase->sktBufAmountRead( 0 );
		return false;
	}

	char *	pSktBuf = (char *)sktBase->getSktReadBuf();

	int contentLen = getTotalLengthFromNetServiceUrl( pSktBuf, iSktDataLen );
	if( 0 >= contentLen )
	{
		sktBase->sktBufAmountRead( 0 );
		LogMsg( LOG_ERROR, "verifyAllDataArrivedOfNetServiceUrl: not valid" );
		VxReportHack( eHackerLevelSuspicious, eHackerReasonNetSrvUrlInvalid, sktBase, "parseHttpNetServiceUrl: not http prefix" );
		sktBase->closeSkt( eSktCloseNetSrvUrlInvalid );
		return false;
	}

	sktBase->sktBufAmountRead( 0 );
	return true; 
}

//============================================================================
// returns content length or -1 if invalid url
int  NetServiceUtils::getTotalLengthFromNetServiceUrl( char * dataBuf, int dataLen )
{
	//                     			32
	//ptop://GET /url version/Crypto Key/total length of data/Net service command/VxGUID/cmd version/error code/content"
	//               15                46 
	//ptop://GET /001/                 /
	if( dataLen < NET_SERVICE_HDR_LEN )
	{
		return -1;
	}

	if( 0 != strncmp( dataBuf, "ptop://GET /", 12 ) )
	{
		LogMsg( LOG_ERROR, "getTotalLengthFromNetServiceUrl: invalid prefix");
		return -1;
	}

	if( '/' != dataBuf[15] )
	{
		LogMsg( LOG_ERROR, "getTotalLengthFromNetServiceUrl: invalid / location1");
		return -1;
	}

	if( '/' != dataBuf[48] )
	{
		LogMsg( LOG_ERROR, "getTotalLengthFromNetServiceUrl: invalid / location2");
		return -1;
	}

	if( '/' != dataBuf[ 48 + MAX_CONTENT_LEN_DIGITS + 1 ] )
	{
		LogMsg( LOG_ERROR, "getTotalLengthFromNetServiceUrl: invalid / location3");
		return -1;
	}

	int contentLen = atoi( &dataBuf[ 48 + 1 ] );
	if( ( NET_SERVICE_HDR_LEN > contentLen   )
		|| ( MAX_NET_SERVICE_URL_LEN < contentLen ) )
	{
		LogMsg( LOG_ERROR, "getTotalLengthFromNetServiceUrl: invalid content length %d", contentLen );
		return -1;
	}

	return contentLen;
}


//============================================================================
void  NetServiceUtils::buildNetCmd( std::string& retResult, ENetCmdType netCmd, std::string& netServChallengeHash, std::string& strContent, ENetCmdError errCode, int version )
{
	std::string strNetCmd = netCmdEnumToString( netCmd );
	//ptop://GET /  1/ = len 16 + 6 /'s
	//ptop://GET /url version/Crypto Key/total length of data/Net service command/cmd version/error code/content/"
	// total header = 16 + 6 + 32 + 13 + 19 + 3 + 8   = 96

	int totalLen = (int)(16 + 6
				+ netServChallengeHash.length()
				+ MAX_CONTENT_LEN_DIGITS 
				+ MAX_NET_CMD_LEN_CHARS
				+ MAX_CMD_VERSION_LEN_DIGITS
				+ MAX_ERROR_LEN_DIGITS
				+ strContent.length());

	if( strContent.length() )
	{
		StdStringFormat( retResult, "ptop://GET /  1/%s/%13d/%s/%3d/%8d/%s/", 
			netServChallengeHash.c_str(), 
			totalLen, 
			strNetCmd.c_str(), 
			version, 
			errCode,
			strContent.c_str() );
	}
	else
	{
		StdStringFormat( retResult, "ptop://GET /  1/%s/%13d/%s/%3d/%8d//", 
			netServChallengeHash.c_str(), 
			totalLen, 
			strNetCmd.c_str(), 
			version, 
			errCode );
	}

    LogModule( eLogNetService, LOG_INFO, "buildNetCmd %s", retResult.c_str() );
}

//============================================================================
int  NetServiceUtils::buildNetCmdHeader( std::string& retResult, ENetCmdType netCmd, std::string& netServChallengeHash, int contentLength, ENetCmdError errCode, int version )
{
	std::string strNetCmd = netCmdEnumToString( netCmd );
	//ptop://GET /  1/ = len 16
	// + 5 /s  = 22 for header and /'s

	int totalLen = (int)(16 + 5
		+ netServChallengeHash.length()
		+ MAX_CONTENT_LEN_DIGITS 
		+ MAX_NET_CMD_LEN_CHARS
		+ MAX_CMD_VERSION_LEN_DIGITS
		+ MAX_ERROR_LEN_DIGITS
		+ contentLength);

	StdStringFormat( retResult, "ptop://GET /  1/%s/%13d/%s/%3d/%8d/", 
		netServChallengeHash.c_str(), 
		totalLen, 
		strNetCmd.c_str(), 
		version, 
		errCode );
	return totalLen;
}

//============================================================================
bool NetServiceUtils::buildIsMyPortOpenUrl( VxSktConnectSimple * netServConn, std::string& strHttpUrl, uint16_t u16Port )
{
    std::string strContent;
    StdStringFormat( strContent, "%d", u16Port );

    return buildNetCmd( netServConn, strHttpUrl, eNetCmdIsMyPortOpenReq, strContent );
}

//============================================================================
bool NetServiceUtils::buildQueryHostIdUrl( VxSktConnectSimple * netServConn, std::string& strNetCmdHttpUrl )
{
    std::string strContent = "QUERYHOSTID";

    return buildNetCmd( netServConn, strNetCmdHttpUrl, eNetCmdQueryHostOnlineIdReq, strContent );
}

//============================================================================
bool NetServiceUtils::buildPingTestUrl( VxSktConnectSimple * netServConn, std::string& strNetCmdHttpUrl )
{
    std::string strContent = "PING";

    return buildNetCmd( netServConn, strNetCmdHttpUrl, eNetCmdHostPing, strContent );
}

//============================================================================
bool NetServiceUtils::buildNetCmd( VxSktConnectSimple * netServConn, std::string& retResult, ENetCmdType netCmd, std::string& strContent, ENetCmdError errCode, int version )
{
    if( netServConn && netServConn->isConnected() )
    {
        uint16_t cryptoKeyPort = netServConn->getCryptoKeyPort();
        return buildNetCmd( cryptoKeyPort, retResult, netCmd, strContent, errCode, version );
    }

    return false;
}

//============================================================================
bool NetServiceUtils::buildNetCmd( uint16_t cryptoKeyPort, std::string& retResult, ENetCmdType netCmd, std::string& strContent, ENetCmdError errCode, int version )
{
    if( cryptoKeyPort )
    {
        std::string netServChallengeHash;
        generateNetServiceChallengeHash( netServChallengeHash, cryptoKeyPort, m_NetworkMgr.getNetworkKey() );
        buildNetCmd( retResult, netCmd, netServChallengeHash, strContent, errCode, version );
        return !retResult.empty();
    }

    return false;
}

//============================================================================
EPluginType NetServiceUtils::parseHttpNetServiceUrl( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr )
{
	netServiceHdr.m_NetCmdType = eNetCmdUnknown;

	int iSktDataLen = sktBase->getSktBufDataLen();
	char *	pSktBuf = (char *)sktBase->getSktReadBuf();
	pSktBuf[ iSktDataLen ] = 0;
	EPluginType pluginType = parseHttpNetServiceHdr( pSktBuf, iSktDataLen, netServiceHdr );
	if( ePluginTypeInvalid == pluginType )
	{
		VxReportHack( eHackerLevelSuspicious, eHackerReasonNetSrvUrlInvalid, sktBase, "Invalid Netservice URL" );
		sktBase->sktBufAmountRead( 0 );
		sktBase->closeSkt( eSktCloseNetSrvUrlInvalid );
		return ePluginTypeInvalid;
	}

	sktBase->sktBufAmountRead( 0 );

	return pluginType;
}

//============================================================================
bool NetServiceUtils::isValidHexString( const char* hexString, int dataLen )
{
	for( int i = 0; i < dataLen; i++ )
	{
		char ch = hexString[ i ];
		if( false == ( (('0' <= ch) && ('9' >= ch)) 
						|| (('A' <= ch) && ('F' >= ch))
						|| (('a' <= ch) && ('f' >= ch)) ) )
		{
			return false;
		}
	}

	return true;
}

//============================================================================
EPluginType NetServiceUtils::parseHttpNetService( char* dataBuf, int dataLen, NetServiceHdr& netServiceHdr, std::string& netCmdContent )
{
	netCmdContent.clear();
	EPluginType pluginType = parseHttpNetServiceHdr( dataBuf, dataLen, netServiceHdr );
	if( ePluginTypeInvalid != pluginType )
	{
		int contentLen = dataLen - NET_SERVICE_HDR_LEN;
		if( contentLen > 1 )
		{
			char* contentBuf = new char[contentLen + 1];
			strncpy( contentBuf, &dataBuf[NET_SERVICE_HDR_LEN], contentLen );
			contentBuf[contentLen] = 0;
			netCmdContent = contentBuf;
			delete[] contentBuf;
		}
	}

	return pluginType;
}

//============================================================================
EPluginType NetServiceUtils::parseHttpNetServiceHdr( char * dataBuf, int dataLen, NetServiceHdr& netServiceHdr )
{
	if( dataLen < NET_SERVICE_HDR_LEN )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceHdr: data len < NET_SERVICE_HDR_LEN" );
		return ePluginTypeInvalid;
	}

	if( 0 != strncmp( dataBuf, "ptop://GET /", 12 ) )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceHdr: not http prefix" );
		return ePluginTypeInvalid;
	}

	dataBuf += 12;
	int dataUsed = 12;

	std::string strValue;
	int partLen = getNextPartOfUrl( dataBuf, strValue );
	dataBuf += partLen + 1;
	dataUsed += partLen + 1;
	if( ( MAX_URL_VERSION_LEN_DIGITS != partLen )
		|| ( dataUsed >= dataLen ) )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: Invalid URL Version" );
		return ePluginTypeInvalid;
	}

	std::string strKey;
	partLen = getNextPartOfUrl( dataBuf, strKey );
	dataBuf += partLen + 1;
	dataUsed += partLen + 1;
	if( ( 0 == partLen )
		|| ( dataUsed >= dataLen ) )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: no data past crypto Key" );
		return ePluginTypeInvalid;
	}
	
	if( 32 != partLen )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: crypto Key wrong length %d", partLen );
		return ePluginTypeInvalid;
	}

	// parse crypto key
	const char* pKeyBegin = strKey.c_str();
	if( false == isValidHexString( pKeyBegin, 32 ) )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: crypto Key contains invalid chars %s", strKey.c_str() );
		return ePluginTypeInvalid;
	}

	netServiceHdr.m_ChallengeHash = strKey;

	partLen = getNextPartOfUrl( dataBuf, strValue );
	dataBuf += partLen + 1;
	dataUsed += partLen + 1;
	if( ( MAX_CONTENT_LEN_DIGITS != partLen )
		|| ( dataUsed >= dataLen ) )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: no data past total data length" );
		return ePluginTypeInvalid;
	}

	netServiceHdr.m_TotalDataLen = atoi( strValue.c_str() );
	if( netServiceHdr.m_TotalDataLen < NET_SERVICE_HDR_LEN )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: invalid total data length %d", netServiceHdr.m_TotalDataLen );
		return ePluginTypeInvalid;
	}
	
	netServiceHdr.m_ContentDataLen = netServiceHdr.m_TotalDataLen - NET_SERVICE_HDR_LEN;

	// which net service command
	partLen = getNextPartOfUrl( dataBuf, strValue );
	dataBuf += partLen + 1;
	dataUsed += partLen + 1;
	if( ( 0 == partLen )
		|| ( dataUsed >= dataLen ) )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: no data past net command" );
		return ePluginTypeInvalid;
	}

	netServiceHdr.m_NetCmdType = netCmdStringToEnum( strValue.c_str() );
	if( eNetCmdUnknown == netServiceHdr.m_NetCmdType )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: not known NET COMMAND" );
		return ePluginTypeInvalid;
	}

	partLen = getNextPartOfUrl( dataBuf, strValue );
	dataBuf += partLen + 1;
	dataUsed += partLen + 1;
	if( ( MAX_CMD_VERSION_LEN_DIGITS != partLen )
		|| ( dataUsed >= dataLen ) )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: no data past Net command" );
		return ePluginTypeInvalid;
	}
	
	netServiceHdr.m_CmdVersion = atoi( strValue.c_str() );

	partLen = getNextPartOfUrl( dataBuf, strValue );
	dataBuf += partLen + 1;
	dataUsed += partLen + 1;
	if( MAX_ERROR_LEN_DIGITS != partLen )
	{
		LogMsg( LOG_ERROR, "parseHttpNetServiceUrl: invalid error digit len" );
		return ePluginTypeInvalid;
	}

	netServiceHdr.m_CmdError = (ENetCmdError)atoi( strValue.c_str() );

	netServiceHdr.m_SktDataUsed = dataUsed;

	EPluginType pluginType = ePluginTypeNetServices;
	if( eNetCmdQueryHostOnlineIdReq == netServiceHdr.m_NetCmdType )
    {
        pluginType = ePluginTypeHostNetwork;
    }
    else if( eNetCmdIsMyPortOpenReq == netServiceHdr.m_NetCmdType )
    {
        pluginType = ePluginTypeHostConnectTest;
    }

	if(  netServiceHdr.m_TotalDataLen < MIN_NET_CMD_LEN ||  netServiceHdr.m_TotalDataLen > MAX_NET_CMD_LEN )
	{
		LogMsg( LOG_ERROR, "NetActionAnnounce::parseHttpNetServiceUrl: invalid net cmd len %d type %s",  netServiceHdr.m_TotalDataLen, DescribeNetCmdType( netServiceHdr.m_NetCmdType ) );
		return ePluginTypeInvalid;
	}

    LogModule( eLogNetService, LOG_VERBOSE, "parseHttpNetServiceUrl: cmd %s plugin %s %s", netCmdEnumToString( netServiceHdr.m_NetCmdType ), 
		DescribePluginType( pluginType ), DescribeNetCmdError( netServiceHdr.m_CmdError ) );

	return pluginType;
}

//============================================================================
int NetServiceUtils::getNextPartOfUrl( char * buf, std::string& strValue )
{
	strValue = "";
	int len = 0;
	char * pTemp = strchr( buf, '/' );
	if( pTemp )
	{
		char cSave = *pTemp;
		*pTemp = 0;
		strValue = buf;
		*pTemp = cSave;
		len = (int)strValue.length();
	}

	return len;
}

//============================================================================
bool  NetServiceUtils::getNetServiceUrlContent( std::string& netServiceUrl, std::string& retFromClientContent )
{
	retFromClientContent = "";
	if( NET_SERVICE_HDR_LEN >= netServiceUrl.length() )
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::getNetServiceUrlContent: invalid netService Length" );
		return false;
	}

	const char* buf1 = netServiceUrl.c_str();
	retFromClientContent = &buf1[ NET_SERVICE_HDR_LEN ];
	if( 0 == retFromClientContent.length() )
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::getNetServiceUrlContent: invalid content Length" );
		return false;
	}

	const char* content = retFromClientContent.c_str();
	if( '/' != content[ retFromClientContent.length() - 1 ] )
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::getNetServiceUrlContent: no trailing /" );
		retFromClientContent = "";
		return false;
	}

	((char *)content)[ retFromClientContent.length() - 1 ] = 0;
	return true;
}

//============================================================================
ENetCmdType  NetServiceUtils::netCmdStringToEnum( const char* netCmd )
{
	if( 0 == strcmp( NET_CMD_HOST_PING, netCmd ) )
	{
		return eNetCmdHostPing;
	}
	else if( 0 == strcmp( NET_CMD_HOST_PONG, netCmd ) )
	{
		return eNetCmdHostPong;
	}
	else if( 0 == strcmp( NET_CMD_CLIENT_PING, netCmd ) )
	{
		return eNetCmdClientPing;
	}
	else if( 0 == strcmp( NET_CMD_CLIENT_PONG, netCmd ) )
	{
		return eNetCmdClientPong;
	}
	else if( 0 == strcmp( NET_CMD_PORT_TEST_REQ, netCmd ) )
	{
		return eNetCmdIsMyPortOpenReq;
	}
	else if( 0 == strcmp( NET_CMD_PORT_TEST_REPLY, netCmd ) )
	{
		return eNetCmdIsMyPortOpenReply;
	}
    else if( 0 == strcmp( NET_CMD_HOST_ID_REQ, netCmd ) )
    {
        return eNetCmdQueryHostOnlineIdReq;
    }
    else if( 0 == strcmp( NET_CMD_HOST_ID_REPLY, netCmd ) )
    {
        return eNetCmdQueryHostOnlineIdReply;
    }
	else
	{
		return eNetCmdUnknown;
	}
}

//============================================================================
const char*  NetServiceUtils::netCmdEnumToString( ENetCmdType	eNetCmdType )
{
	switch( eNetCmdType )
	{
	case eNetCmdHostPing:
		return NET_CMD_HOST_PING;
	case eNetCmdHostPong:
		return NET_CMD_HOST_PONG;
	case eNetCmdClientPing:
		return NET_CMD_CLIENT_PING;
	case eNetCmdClientPong:
		return NET_CMD_CLIENT_PONG;
	case eNetCmdIsMyPortOpenReq:
		return NET_CMD_PORT_TEST_REQ;
	case eNetCmdIsMyPortOpenReply:
		return NET_CMD_PORT_TEST_REPLY;
    case eNetCmdQueryHostOnlineIdReq:
        return NET_CMD_HOST_ID_REQ;
    case eNetCmdQueryHostOnlineIdReply:
        return NET_CMD_HOST_ID_REPLY;

	case eNetCmdUnknown:
	default:
		return NET_CMD_UNKNOWN;
	}
}

//============================================================================
int  NetServiceUtils::getIndexOfCrLfCrLf( std::shared_ptr<VxSktBase>& sktBase )
{
	int indexOfCrLfCrLf = -1;
	int iDataLen = sktBase->getSktBufDataLen();
	if( iDataLen >= 4 )
	{
		char *	pSktBuf = (char *)sktBase->getSktReadBuf();
		pSktBuf[ iDataLen ] = 0;

		char * pEndPtr = strstr( pSktBuf, "\r\n\r\n" );
		if( pEndPtr )
		{
			indexOfCrLfCrLf = (int)( pEndPtr - pSktBuf );
		}

		sktBase->sktBufAmountRead( 0 );
	}

	return indexOfCrLfCrLf;
}

//============================================================================
bool NetServiceUtils::isRequestTypeNetCmd( ENetCmdType eNetCmdType )
{
    bool isRequest{false};
    switch( eNetCmdType )
    {
    case eNetCmdIsMyPortOpenReq:
    case eNetCmdIsMyPortOpenReply:
    case eNetCmdQueryHostOnlineIdReq:
        isRequest = true;

    default:
        break;
    }

    return isRequest;
}

//============================================================================
bool NetServiceUtils::buildCmd( std::string& retCmd, std::shared_ptr<VxSktBase>& sktBase, ENetCmdType netCmd, std::string& cmdContent, ENetCmdError errCode, int version )
{
	retCmd.clear();
	std::string netServChallengeHash;
    generateNetServiceChallengeHash( netServChallengeHash, sktBase, m_NetworkMgr.getNetworkKey() );

	buildNetCmd( retCmd, netCmd, netServChallengeHash, cmdContent, errCode, version );
	return !retCmd.empty() && sktBase->isConnected();
}

//============================================================================
bool NetServiceUtils::buildCmd( std::string& retCmd, VxSktConnectSimple* sktBase, ENetCmdType netCmd, std::string& cmdContent, ENetCmdError errCode, int version )
{
	retCmd.clear();
	std::string netServChallengeHash;
    generateNetServiceChallengeHash( netServChallengeHash, sktBase, m_NetworkMgr.getNetworkKey() );
	buildNetCmd( retCmd, netCmd, netServChallengeHash, cmdContent, errCode, version );
	return !retCmd.empty() && sktBase->isConnected();
}

//============================================================================
int32_t NetServiceUtils::buildAndSendCmd( std::shared_ptr<VxSktBase>& sktBase, ENetCmdType netCmd, std::string& cmdContent, ENetCmdError errCode, int version )
{
	int32_t rc = -1;
	std::string retResult;
	if( buildCmd( retResult, sktBase, netCmd, cmdContent, errCode, version ) && isAllAscii( retResult ) )
	{
		if( GetPtoPEngine().getNetServicesMgr().sendNetServicePacket( netCmd, sktBase, retResult, errCode ) )
		{
			rc = 0;
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::buildAndSendCmd build failed for VxSktBase" );
	}

	return rc;
}

//============================================================================
int32_t NetServiceUtils::buildAndSendCmd( VxSktConnectSimple * sktBase, ENetCmdType netCmd, std::string& cmdContent, ENetCmdError errCode, int version )
{
	int32_t rc = -1;
    std::string retResult;
	if( buildCmd( retResult, sktBase, netCmd, cmdContent, errCode, version ) && isAllAscii( retResult ) )
	{
		if( sendNetServiceRequest( netCmd, sktBase, retResult, errCode ) )
		{
			rc = 0;
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::buildAndSendCmd build failed for simple socket" );
	}

	return rc;
}

//============================================================================
void NetServiceUtils::generateNetServiceChallengeHash(	std::string&				strKey,	
                                                        std::shared_ptr<VxSktBase>&	skt,
                                                        std::string					netKey)
{
	uint16_t clientPort;
	if( skt->isAcceptSocket() )
	{
		clientPort = skt->m_RmtIp.getPort();
	}
	else
	{
		clientPort = skt->m_LclIp.getPort();
	}

    generateNetServiceChallengeHash( strKey, clientPort, netKey );
}

//============================================================================
void NetServiceUtils::generateNetServiceChallengeHash(	std::string&			strKey,	
                                                        VxSktConnectSimple *	skt,
                                                        std::string             netKey)
{
	uint16_t clientPort = skt->m_LclIp.getPort();
    generateNetServiceChallengeHash( strKey, clientPort, netKey );
}

//============================================================================
void NetServiceUtils::generateNetServiceChallengeHash( std::string&	strKeyHash,	uint16_t clientPort, std::string netKey )
{
	std::string strPwd;
    StdStringFormat( strPwd, "xs%dAfj%sd%d75!Bjsaf",
                    clientPort,
                    netKey.c_str(),
                    clientPort );

    VxKey key;
	key.setKeyFromPassword( strPwd.c_str(), (int)strPwd.size() );
	key.exportToAsciiString( strKeyHash );
    // LogModule( eLogNetService, LOG_INFO, "generateNetServiceChallengeHash: clientPort = %d, network %s hash %s", 
    //           clientPort, netKey.c_str(), strPwd.c_str() );
}

//============================================================================
void NetServiceUtils::generateNetServiceCryptoKey( VxKey& key, uint16_t clientPort, std::string netKey )
{
	std::string strPwd;
	StdStringFormat( strPwd, "xz&gdf%d%s!?d%d759sdc", 
		             clientPort,
                     netKey.c_str(),
		             clientPort );
	key.setKeyFromPassword( strPwd.c_str(), (int)strPwd.size() );
}

//============================================================================
bool NetServiceUtils::generateNetPktCryptoPassword( std::string& retPwd, std::string netKey, uint16_t port, std::string ip )
{
	retPwd.clear();
	if( netKey.empty() || !port || ip.empty() )
	{
		LogMsg( LOG_ERROR, " NetServicesMgr::%s Invalid param", __func__ );
		return false;
	}

	std::string strPwd;
    StdStringFormat( strPwd, "yz%sAYAj%dk89%sBM67uzm",
						ip.c_str(),
						port,
						netKey.c_str() );

    VxKey key;
	key.setKeyFromPassword( strPwd.c_str(), (int)strPwd.size() );
	key.exportToAsciiString( retPwd );
	return true;
}

//============================================================================
bool NetServiceUtils::decryptNetServiceContent( char * content, int contentDataLen, uint16_t clientPort )
{
	if( ( 0 == contentDataLen )
		|| ( 0x0f & contentDataLen ) )
	{
		LogMsg( LOG_ERROR, "NetActionAnnounce::decryptNetServiceContent: invalid length %d", contentDataLen );
		return false;
	}

	VxKey key;
    generateNetServiceCryptoKey( key, clientPort, m_NetworkMgr.getNetworkKey() );
	VxSymDecrypt( &key, content, contentDataLen );

	return true;
}

//============================================================================
bool NetServiceUtils::sendNetServiceRequest( ENetCmdType netCmdRequestType, ///< which type of net service to request
											 VxSktConnectSimple* netServConn,
											 std::string& netCmd,
											 int txDataTimeout )
{
	if( netCmd.length() < MIN_NET_CMD_LEN || netCmd.length() > MAX_NET_CMD_LEN )
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::%s: invalid net cmd len %d type %s cmd %s", 
				__func__, netCmd.length(), DescribeNetCmdType( netCmdRequestType ), netCmd.c_str() );
		return false;
	}

	std::unique_ptr<VxPktHdr> pktPtr;
	std::string cryptoPwd;

	generateNetPktCryptoPassword( cryptoPwd, getNetworkKey(), netServConn->getRemotePort(), "0.0.0.0" );

	if( netCmd.empty() )
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::%s: empty crypto key. Is network name not set?", __func__ );
		return false;
	}

	if( eNetCmdHostPing == netCmdRequestType ||  eNetCmdClientPing == netCmdRequestType )
	{
		PktTestConnPingReq* pkt = new PktTestConnPingReq();
		pkt->setNetCmd( netCmd );
		pktPtr.reset( pkt );
	}
	else if( eNetCmdIsMyPortOpenReq == netCmdRequestType )
	{
		PktTestConnTestReq* pkt = new PktTestConnTestReq();
		pkt->setNetCmd( netCmd );
		pktPtr.reset( pkt );
	}
	else if( eNetCmdQueryHostOnlineIdReq == netCmdRequestType )
	{
		PktQueryHostUrlReq* pkt = new PktQueryHostUrlReq();
		pkt->setNetCmd( netCmd );
		pktPtr.reset( pkt );
	}
	else
	{
		LogMsg( LOG_ERROR, "NetActionAnnounce::sendNetServiceRequest: unknown net cmd request %d", netCmdRequestType );
		return false;
	}

	bool wasSent{ false };
	int pktLen = pktPtr->getPktLength();
	if( pktLen && !cryptoPwd.empty() )
	{
		uint8_t* pktData = (uint8_t*)pktPtr.get();
		std::unique_ptr<VxCrypto> txCrypto = std::make_unique<VxCrypto>();
		txCrypto->setPassword( cryptoPwd.c_str(), cryptoPwd.length() );
		if(0 == txCrypto->encrypt( pktData, pktLen ) )
		{
			LogModule( eLogNetService, LOG_DEBUG, "NetServicesMgr::%s: cmd %s %s pwd %s ",
					   __func__, DescribeNetCmdType( netCmdRequestType ), netCmd.c_str(), cryptoPwd.c_str() );
			int32_t rc = netServConn->sendData( (char *)pktData, pktLen, txDataTimeout );
			wasSent = 0 == rc;
			if( !wasSent )
			{
				LogMsg( LOG_ERROR, "NetServicesMgr::%s: simple sendData failed timout %d error %d %s", __func__, txDataTimeout, rc, VxDescribeSktError( rc ) );
			}
		}
		else
		{
			LogMsg( LOG_ERROR, "NetServicesMgr::%s: simple could not encrypt len %d", __func__, pktLen );
			return false;
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "NetServicesMgr::sendNetServicePacket: simple invalid pkt len %d or cryptoPwdEmpty", pktLen );
		return false;
	}

	return wasSent;
}

//============================================================================
bool NetServiceUtils::rxNetServiceCmd( ENetCmdType expectedRxNetCmd, ///< which type of net service response to recieve
									   VxSktConnectSimple* netServConn, 
									   char* rxBuf, int rxBufLen, 
									   NetServiceHdr& netServiceHdr, 
									   int rxHdrTimeout, 
									   int rxDataTimeout )
{
	vx_assert( rxBufLen > NET_SERVICE_HDR_LEN );

	if( eNetCmdHostPong != expectedRxNetCmd && eNetCmdClientPong != expectedRxNetCmd &&
		eNetCmdIsMyPortOpenReply != expectedRxNetCmd && eNetCmdQueryHostOnlineIdReply  != expectedRxNetCmd ) 
	{
		LogMsg( LOG_ERROR, "NetServiceUtils::%s: unknown net cmd request %d", __func__, expectedRxNetCmd );
		return false;
	}

	if( !netServConn->isConnected() )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d connection to %s:%d closed abruptly",
				netServConn->getSktHandle(), netServConn->getRemoteIpAddress().c_str(), netServConn->getRemotePort() );
		return false;
	}

	std::unique_ptr<VxCrypto> rxCrypto = std::make_unique<VxCrypto>();
	std::string cryptoPwd;

	if( eNetCmdClientPong == expectedRxNetCmd )
	{
		generateNetPktCryptoPassword( cryptoPwd, getNetworkKey(), m_Engine.getMyPktAnnounce().getOnlinePort(), "0.0.0.0" );
	}
	else
	{
		generateNetPktCryptoPassword( cryptoPwd, getNetworkKey(), netServConn->getRemotePort(), netServConn->getRemoteIpAddress() );
	}

	rxCrypto->setPassword( cryptoPwd.c_str(), cryptoPwd.length() );

	// first get enough to get the total length of pkt then get remaining data
	int pktHdrLen = ROUND_TO_16BYTE_BOUNDRY( sizeof( VxPktHdr ) );
	std::array<char, MAX_PKT_LEN> rxPktBuf;

	int iRxed{ 0 };

	VxElapseTimer rxCmdTimer;
	netServConn->recieveData( rxPktBuf.data(),			// data buffer to read into
							  pktHdrLen,				// length of data	
							  &iRxed,					// number of bytes actually received
							  rxHdrTimeout );			// timeout attempt to receive

	if( !netServConn->isConnected() )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd skt %d connection to %s:%d closed abruptly",
				netServConn->getSktHandle(), netServConn->getRemoteIpAddress().c_str(), netServConn->getRemotePort() );
		if( iRxed == 0 )
		{
			LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd you are probably marked as Hacker by %s",
				netServConn->getRemoteIpAddress().c_str() );
		}

		return false;
	}

	if( iRxed != pktHdrLen )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd skt %d hdr timeout %3.3f sec rxed data len %d",
				netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), iRxed );
		if( iRxed == 0 && rxCmdTimer.elapsedSec() >= rxHdrTimeout )
		{
			LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd you are probably marked as Hacker by %s",
				netServConn->getRemoteIpAddress().c_str() );
		}

		return false;
	}

	// decrypt the pkt header
	if( 0 != rxCrypto->decrypt( (uint8_t*)rxPktBuf.data(), iRxed ) )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd skt %d hdr timeout %3.3f failed to decrypt pkt hdr len %d",
				netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), iRxed );
		return false;
	}

	VxPktHdr* pktHdr = (VxPktHdr*)rxPktBuf.data();
	int pktLen = pktHdr->getPktLength();
	int remainingLen = pktLen - iRxed;
	int pktType = pktHdr->getPktType();

	if( pktLen & 0x0f || pktLen < pktHdrLen || pktLen > MAX_PKT_LEN || remainingLen < 0 || remainingLen & 0x0f )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd skt %d timeout %3.3f invalid pkt len %d", 
			netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), pktLen );
		return false;
	}

	bool isExpectedPktType{ false };
	switch( expectedRxNetCmd )
	{
	case eNetCmdHostPong:

	case eNetCmdClientPong:
		isExpectedPktType = PKT_TYPE_TEST_CONN_PING_REPLY == pktType;
		break;
	case eNetCmdIsMyPortOpenReply:
		isExpectedPktType = PKT_TYPE_TEST_CONN_TEST_REPLY == pktType;
		break;
	case eNetCmdQueryHostOnlineIdReply:
		isExpectedPktType = PKT_TYPE_QUERY_HOST_URL_REPLY == pktType;
		break;
	default:
		break;
	}

	if( !isExpectedPktType )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d timeout %3.3f unexpected pkt type %d for net cmd %d", 
			netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), pktType, expectedRxNetCmd );
		return false;
	}
	
	if( remainingLen )
	{
		// get the remaing pkt data
		int rxedLen2{ 0 };
		netServConn->recieveData( &rxPktBuf.data()[iRxed],		// data buffer to read into
								  remainingLen,			// length of data	
								  &rxedLen2,				// number of bytes actually received
								  rxDataTimeout );			// timeout attempt to receive
		if( rxedLen2 != remainingLen )
		{
			LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d timeout %3.3f failed to get remaining pkt len %d", 
			netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), remainingLen );
			return false;
		}

		// decrypt remaining
		if( 0 != rxCrypto->decrypt( (uint8_t*)(&rxPktBuf.data()[iRxed]), remainingLen ) )
		{
			LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d hdr timeout %3.3f failed to decrypt pkt remaining len %d",
					netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), remainingLen );
			return false;
		}
	}

	std::string strCmd;
	switch( expectedRxNetCmd )
	{
	case eNetCmdHostPong:

	case eNetCmdClientPong:
	{
		PktTestConnPingReply* pkt = (PktTestConnPingReply *)pktHdr;
		pkt->getNetCmd( strCmd );
		break;
	}
	case eNetCmdIsMyPortOpenReply:
	{
		PktTestConnTestReply* pkt = (PktTestConnTestReply*)pktHdr;
		pkt->getNetCmd( strCmd );
		break;
	}
	case eNetCmdQueryHostOnlineIdReply:
	{
		PktQueryHostUrlReply* pkt = (PktQueryHostUrlReply *)pktHdr;
		pkt->getNetCmd( strCmd );
		break;
	}
	default:
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d hdr timeout %3.3f invalid expected cmd type %d",
		netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), expectedRxNetCmd );
		return false;
	}

	if( strCmd.length() < NET_SERVICE_HDR_LEN )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d hdr timeout %3.3f failed to retrieve net cmd",
					netServConn->getSktHandle(), rxCmdTimer.elapsedSec() );
		return false;
	}

	std::vector<char> netCmd(strCmd.begin(), strCmd.end() );

	if( ePluginTypeNetServices != parseHttpNetServiceHdr( netCmd.data(), NET_SERVICE_HDR_LEN, netServiceHdr) )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d hdr parse error", netServConn->getSktHandle() );
		return false;
	}

    if( netServiceHdr.m_TotalDataLen <= NET_SERVICE_HDR_LEN )
    {
        LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d too small netServiceHdr.m_TotalDataLen %d", netServConn->getSktHandle(), netServiceHdr.m_TotalDataLen );
        return false;
    }

	if( netServiceHdr.m_TotalDataLen > rxBufLen )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd:skt %d too large netServiceHdr.m_TotalDataLen %d", netServConn->getSktHandle(), netServiceHdr.m_TotalDataLen );
		return false;
	}

	int contentLen = netServiceHdr.m_TotalDataLen - NET_SERVICE_HDR_LEN;
	if( contentLen <= 0 || contentLen >= rxBufLen )
	{
		LogMsg( LOG_ERROR, "### ERROR NetServiceUtils::rxNetServiceCmd: skt %d timeout %3.3f sec recieving content len %d", 
			netServConn->getSktHandle(), rxCmdTimer.elapsedSec(), contentLen );
		return false;
	}

	netServiceHdr.m_ContentDataLen = contentLen;
	memcpy( rxBuf, &netCmd[NET_SERVICE_HDR_LEN], contentLen );
	rxBuf[ contentLen ] = 0;
	LogModule( eLogNetService, LOG_DEBUG, "### SUCCESS NetServiceUtils::rxNetServiceCmd: skt %d success recieving netcmd %s content %s len %d in %3.3f sec from IP %s", 
			   netServConn->getSktHandle(), DescribeNetCmdType( expectedRxNetCmd ), rxBuf, contentLen, rxCmdTimer.elapsedSec(), netServConn->getRemoteIpAddress().c_str() );
	return true;
}

//============================================================================
bool NetServiceUtils::isAllAscii( std::string& netCmd )
{
	if( netCmd.empty() )
	{
		LogModule( eLogNetService, LOG_DEBUG, "### SUCCESS NetServiceUtils::isAllAscii: empty string" );
		return false;
	}


	// check if all ascii
	bool isAscii = true;
	for( auto& character : netCmd )
	{
		if( ( character < 2 ) || ( character > 127 ) )
		{
			LogModule( eLogNetService, LOG_DEBUG, "### SUCCESS NetServiceUtils::isAllAscii: not and ascii string" );
			isAscii = false;
			break;
		}
	}

	return isAscii;
}
