//============================================================================
// Copyright (C) 2003 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxSktUtil.h"

#ifdef TARGET_OS_WINDOWS
# include "WS2tcpip.h"
#endif // TARGET_OS_WINDOWS

#include "ISktStatCallbackInterface.h"

#include "VxResolveHost.h"

#include "InetAddressParse.h"
#include "IsBigEndianCpu.h"
#include "VxDefs.h"
#include "VxDebug.h"
#include "VxThread.h"
#include "VxElapseTimer.h"
#include "VxTime.h"
#include "VxLinuxOnly.h"

#include <PktLib/VxCommon.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <memory.h>
#include <array>
#include <algorithm>

#ifdef TARGET_OS_WINDOWS
    // helpers for adapter info
	//#include <iphlpapi.h>
	//#pragma comment(lib, "IPHLPAPI.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
	#include <net/if.h> 
    #include <netdb.h>
	#include <ctype.h>
	#include <fcntl.h>
    #include <sys/types.h>
	#include <unistd.h>
# ifdef TARGET_OS_ANDROID
# include <sys/ioctl.h>
# endif
#endif // _MSC_VER
#ifdef _MSC_VER
# pragma warning( disable: 4996 ) //'strnicmp': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _strnicmp.
#endif // _MSC_VER

namespace
{
	std::vector<InetAddress>	g_aoAllIps;
	InetAddress					g_oSelectedLocalIp;
	InetAddress					g_oDefaultLocalIp;
	InetAddress					g_oDefaultIPv4;
	InetAddress					g_oDefaultIPv6;
	InetAddress					g_oGlobalIPv6;

	ISktStatCallbackInterface*  g_SktStatCallback{ nullptr };

    const char* VxDescribeAddrInfoError( int err )
    {
#ifdef TARGET_OS_WINDOWS
        const char* msg = gai_strerrorA( err );
#else
        const char* msg = gai_strerror( err );
#endif
        return msg ? msg : "Unknown getaddrinfo error";
    }
}

//============================================================================
void VxSetSktStatCallback( ISktStatCallbackInterface* sktStatCallback )
{
	g_SktStatCallback = sktStatCallback;
}

//============================================================================
ISktStatCallbackInterface* VxGetSktStatCallback( void )
{
	return g_SktStatCallback;
}

//============================================================================
//! initialize sockets
int32_t VxSocketsStartup( void )
{
	srand( (unsigned int)time( NULL ) );
	static bool bIsInitialized = 0;
	if( bIsInitialized )
	{
		return 0;
	}

#ifdef TARGET_OS_WINDOWS
	static struct WSAData wsa_state;
	memset(&wsa_state,0,sizeof(wsa_state));
	if (WSAStartup(0x0202,&wsa_state)!=0)
	{
		int err = WSAGetLastError();
		LogMsg( LOG_INFO, "SocketsStartup: Error %d", err );
		vx_assert( false );
		return(-1);
	}
#endif // TARGET_OS_WINDOWS

	//VxRefreshDefaultIps();
	bIsInitialized = true;
	return  0;
}

//============================================================================
bool VxIsPortValid( uint16_t port )
{
	return port > 79;
}

//============================================================================
bool VxIsIpValid( std::string& ipAddr )
{
	if( ipAddr.empty() )
	{
		return false;
	}

    static const std::string nullIpV4 = "0.0.0.0";
    static const std::string loopBackIpV4 = "127.0.0.1";
    static const std::string loopBackIp1V6 = "0000:0000:0000:0000:0000:0000:0000:0001/128";
    static const std::string loopBackIp2V6 = "::1/128";
    return ipAddr.size() >= nullIpV4.size() &&
        ipAddr != nullIpV4 &&
        ipAddr != loopBackIpV4 &&
        ipAddr != loopBackIp1V6 &&
        ipAddr != loopBackIp2V6 &&
        ( ipAddr.find('.') != std::string::npos || ipAddr.find(':') != std::string::npos );
}

//============================================================================
bool VxIsIpv6Address( std::string& ipAddr )
{
	if( !VxIsIpValid( ipAddr ) )
	{
		return false;
	}

	int colonCount = std::count(ipAddr.begin(), ipAddr.end(), ':');
	return colonCount >= 2;
}

//============================================================================
bool VxIsIpv4Address( std::string& ipAddr )
{
	return VxIsIpValid( ipAddr ) && !VxIsIpv6Address( ipAddr );
}

//============================================================================
bool VxMakePtopUrl( std::string& ipAddr, uint16_t port, std::string& retPtopUrl )
{
	if( ipAddr.empty() || !port )
	{
		LogMsg( LOG_ERROR, "VxMakePtopUrl bad param" );
		return false;
	}

	std::string ptopUrl( "ptop://" );

    EIpAddrType addrType = VxGetIpAddrType( ipAddr.c_str() );
    if( eIpAddrTypeIpv6 == addrType )
    {
        ptopUrl += "[";
        ptopUrl += ipAddr;
        ptopUrl += "]";
    }
    else if( eIpAddrTypeIpv4 == addrType )
    {
        ptopUrl += ipAddr;
    }
	else
	{
		LogMsg( LOG_ERROR, "VxMakePtopUrl invalid ip address %s", ipAddr.c_str() );
		return false;
	}

    ptopUrl += ":";
    ptopUrl += std::to_string( port );
	retPtopUrl = ptopUrl;

	return true;
}

//============================================================================
bool VxResolvePtopUrl( std::string ptopUrl, std::string& retIpAddr, uint16_t& retPort, bool preferIpv6 )
{
	if( ptopUrl.empty() )
	{
		LogMsg( LOG_ERROR, "VxResolvePtopUrl empty ptopUrl" );
		return false;
	}

	std::string hostName;
	std::string fileName;
	uint16_t port;
	bool wasSplit = VxSplitHostAndFile( ptopUrl.c_str(), hostName, fileName, port );
	if( !wasSplit || hostName.empty() )
	{
		LogMsg( LOG_ERROR, "VxResolvePtopUrl failed split url %s", ptopUrl.c_str() );
		return false;
	}

	if( VxIsIPv4Address( hostName.c_str() ) || VxIsIPv6Address( hostName.c_str() ) )
	{
		retPort = port;
		retIpAddr = hostName;
		return true;
	}

	uint16_t portForResolve = port;
	if( !portForResolve )
	{
		portForResolve = 80;
	}

	std::string resolvedIp;
	bool resolved = VxResolveUrl( hostName.c_str(), portForResolve, resolvedIp, preferIpv6 ? eIpAddrTypeIpv6 : eIpAddrTypeUnknown );
	if( !resolved && preferIpv6 )
	{
		// ipv6 was probably not available so try again
		resolved = VxResolveUrl( hostName.c_str(), portForResolve, resolvedIp, eIpAddrTypeIpv4 );
	}
	
	if( resolved )
	{
		retPort = port;
		retIpAddr = resolvedIp;
		return true;
	}
	
	LogMsg( LOG_ERROR, "VxResolvePtopUrl FAILED resolve %s", ptopUrl.c_str() );
	return false;
}

//============================================================================
void VxIpInNetOrderToString( uint32_t u32IpAddr, std::string& retIp )
{
	char as8Buf[ 32 ];
	unsigned char * pTemp = (unsigned char * )&u32IpAddr;
	sprintf( as8Buf, "%i.%i.%i.%i", pTemp[0], pTemp[1], pTemp[2], pTemp[3] );
	retIp  = as8Buf;
}

//============================================================================
void VxIpInHostOrderToString( uint32_t u32IpAddr, std::string& retIp )
{
	if( IsBigEndianCpu() )
	{
		char as8Buf[ 32 ];
		int32_t u32IpHostOrder = ntohl( u32IpAddr );
		unsigned char * pTemp = (unsigned char * )&u32IpHostOrder;
		sprintf( as8Buf, "%i.%i.%i.%i", pTemp[0], pTemp[1], pTemp[2], pTemp[3] );
		retIp  = as8Buf;
	}
	else
	{
		VxIpInNetOrderToString( u32IpAddr, retIp )	;
	}
}

//============================================================================
uint32_t VxStringToIpInNetOrder( std::string ip )
{
	return inet_addr( ip.c_str() );
}

//============================================================================
std::string	VxIpToString( struct sockaddr * addr )
{
	if( AF_INET == addr->sa_family )
	{
		return InetAddress::ipv4BinaryToString( (uint8_t*) & (((sockaddr_in*)(addr))->sin_addr) );
	}
	else if( AF_INET6 == addr->sa_family )
	{
		return InetAddress::ipv6BinaryToString( (uint8_t*) & (((sockaddr_in6*)(addr))->sin6_addr) );
	}

	vx_assert( false );
	return std::string( "UNKNOWN" );
}

//============================================================================
void VxFillHints( struct addrinfo& hints, bool bUdpSkt, bool ipv6Only )
{
	memset(&hints, 0, sizeof hints);
	//hints.ai_family = AF_UNSPEC;
	//hints.ai_family = AF_INET
	//hints.ai_family = AF_INET6
	hints.ai_family = ipv6Only ? AF_INET6 : AF_INET; /* AF_INET, AF_INET6 or AF_UNSPEC */
	hints.ai_socktype = bUdpSkt ? SOCK_DGRAM : SOCK_STREAM;
	hints.ai_protocol = bUdpSkt ? IPPROTO_UDP : IPPROTO_TCP;
	//hints.ai_flags = AI_PASSIVE; // AI_NUMERICHOST | AI_PASSIVE
}

//============================================================================
//! if new connection refresh default ip(s)
void VxRefreshDefaultIps( void )
{
	g_aoAllIps.clear();
	g_oDefaultLocalIp.setToInvalid();
	g_oDefaultIPv4.setToInvalid();
	g_oDefaultIPv6.setToInvalid();
	g_oGlobalIPv6.setToInvalid();

    g_oDefaultLocalIp.getAllAddresses(g_aoAllIps);
	if( g_aoAllIps.size() )
	{
		g_oDefaultLocalIp = g_aoAllIps[0];
		if( false == g_oSelectedLocalIp.isValid() )
		{
			g_oSelectedLocalIp = g_oDefaultLocalIp;
		}

		std::vector<InetAddress>::iterator iter;
		for( iter = g_aoAllIps.begin(); iter != g_aoAllIps.end(); ++iter )
		{
			if( (false == g_oDefaultIPv4.isValid()) && (*iter).isIPv4() )
			{
				g_oDefaultIPv4 = *iter;
			}
			if( (false == g_oDefaultIPv6.isValid()) && (*iter).isIPv6() )
			{
				g_oDefaultIPv6 = *iter;
			}			
			if( (false == g_oGlobalIPv6.isValid()) && (*iter).isIPv6GlobalAddress() )
			{
				g_oGlobalIPv6 = *iter;
			}
		}
	}
}

//============================================================================
bool VxCanConnectUsingIPv6( void )
{
	return ( g_oDefaultIPv6.isValid() || g_oGlobalIPv6.isValid() );
}

//============================================================================
//! get list of local ips.. needed for systems with multiple tcp hardware
std::vector<InetAddress>& VxGetLocalIps( void )	
{
	if( 0 == g_aoAllIps.size() )
	{
		VxRefreshDefaultIps();
	}
	return 	g_aoAllIps;
}

//============================================================================
void VxGetLocalIps(	std::vector<InetAddress>& aRetIpAddress )
{
    if( 0 == g_aoAllIps.size() )
    {
        VxRefreshDefaultIps();
    }
	aRetIpAddress.clear();
	std::vector<InetAddress>::iterator iter;
	for( iter = g_aoAllIps.begin(); iter != g_aoAllIps.end(); ++iter )
	{
		aRetIpAddress.push_back( *iter );
	}
}

//============================================================================
//! get default local ip address.. returns 0 if none
InetAddress VxGetDefaultLocalIp( void )
{
	return g_oDefaultLocalIp;
}

//============================================================================
//! get default local ip address.. returns 0 if none
InetAddress VxGetSelectedLocalIp( void )
{
	return g_oSelectedLocalIp;
}

//============================================================================
InetAddress	VxGetMyGlobalIPv6Address( void )
{
	return g_oGlobalIPv6;
}

//============================================================================
InetAddress	VxGetDefaultIPv4Address( void )
{
	return g_oDefaultIPv4;
}

//============================================================================
InetAddress	VxGetDefaultIPv6Address( void )
{
	return g_oDefaultIPv6;
}

//============================================================================
//! return true if ip is in list of local ips
bool VxLocalIpExists(  std::string& strIpAddress )
{
	std::vector<InetAddress>::iterator iter;
	for( iter = g_aoAllIps.begin(); iter != g_aoAllIps.end(); ++iter )
	{
		if( strIpAddress == (*iter).toString() )
		{
			return true;
		}
	}
	return false;
}

//============================================================================
//! return true if port is already in use on this local ip address
bool VxIsIpPortInUse(  uint16_t u16Port, const char* pLocalIp, bool useBind )
{
	bool bInUse = true;

	InetAddress addr;
	if( pLocalIp )
	{
		addr.setIp( pLocalIp );
	}
    else
    {
        // use loopback
        LogMsg( LOG_DEBUG, "VxIsIpPortInUse: null ip so default to loopback" );
        addr.setIp( "127.0.0.1" );
    }

	SOCKET listenSocket = socket(addr.isIPv4() ? AF_INET : AF_INET6, SOCK_STREAM, 0);
	if( INVALID_SOCKET == listenSocket )
	{
		//socket create error
		LogMsg( LOG_ERROR, "VxIsIpPortInUse: socket create error %s", VxDescribeSktError( VxGetLastError() ) );
		return	bInUse;
	}

    // must always bind when checking for random port or on windows will get
    // WSAEINVAL:"The socket is not already bound to an address."
    if( useBind )
    {
        if( false == VxBindSkt( listenSocket, addr, u16Port ) )
        {
            ::VxCloseSkt( listenSocket );
            LogMsg( LOG_INFO, "VxIsIpPortInUse: bind error %s", VxDescribeSktError( VxGetLastError() ) );
            return bInUse;
        }
    }

	int32_t rc;
	if( 0 != ( rc = listen( listenSocket, SOMAXCONN ) ) )
	{
		//listen error
		rc = VxGetLastError();
		LogMsg( LOG_ERROR, "VxIsIpPortInUse: listen error %s", VxDescribeSktError( rc ) );

	}
	else
	{
		bInUse = false;
	}

	::VxCloseSkt( listenSocket );
	return bInUse;
}



//============================================================================
//! split host name from website file path
bool VxSplitHostAndFile(	const char* pFullUrl,		// full url.. example http://www.mysite.com/index.html or www.mysite.com/images/me.png
							std::string& strRetHost,	// return host name.. example http://www.mysite.com/index.htm returns www.mysite.com
							std::string& strRetFileName,	// return file name.. images/me.png
							uint16_t& u16RetPort )			// return port if specified else return 80 as default				
{
	char * pTemp;
	char as8Url[2048];

	u16RetPort = 80;
	strRetHost = "";
	strRetFileName = "";

	pTemp = (char *)strchr( pFullUrl, '/' );
	if( NULL == pTemp )
	{
		// assume is just ip address
		if( '[' == pFullUrl[0] )
		{
			strcpy( as8Url, &pFullUrl[1] );
            if( 0 != ( pTemp = strchr(  as8Url, ']' ) ) )
			{
				*pTemp = 0;
				strRetHost = as8Url;
			}
		}
		else
		{
			// see if is just ip or ip:port
			std::string ipAddr( pFullUrl );
			std::string ipPort;
			int colonPosition = ipAddr.rfind( ':' );
			if( std::string::npos != colonPosition && colonPosition > 2 )
			{
				ipPort = ipAddr.substr( colonPosition + 1, ipAddr.length() - colonPosition );
				ipAddr = ipAddr.substr( 0, colonPosition );
			}

			strRetHost = ipAddr;
			if( !ipPort.empty() )
			{
				int port = std::stoi( ipPort );
				if( port > 1 )
				{
					u16RetPort =(uint16_t)port;
				}
			}
		}

		return true;
	}

	//copy to buffer without protocol specification like https:
	bool hasNetProtocol = false;
	bool isDottedIp = true;
	pTemp = (char *)strstr( pFullUrl, "://" );
	if( pTemp )
	{
		pTemp += 3;
		strcpy( as8Url, pTemp ) ;
		hasNetProtocol = true;
	}
	else
	{
		strcpy( as8Url, pFullUrl ) ;
	}

	if( 0 == strnicmp( pFullUrl, "https://", 8 ) )
	{
		u16RetPort = 443;
	}

	if( ( false == isdigit( as8Url[0] ) && ( '[' != as8Url[0] ) && ( ':' != as8Url[0] ) ) )
	{
		// plain url 
		isDottedIp = false;
	}

	if( hasNetProtocol && (false == isDottedIp) )
	{
		// see if just plain url with no file 
		if( NULL == strchr( as8Url, '/' ) )
		{
			// no file part but may have port specified
			pTemp = (char *)strchr( as8Url, ':' );
			if( pTemp )
			{
				// has port
				*pTemp = 0;
				pTemp++;
				u16RetPort = (uint16_t)atoi( pTemp );
			}
			strRetHost = as8Url;
			return true;
		}
	}

	bool bHasCustomPort = false;
	if( '[' == as8Url[0] )
	{
		// its a ipv6 address
		pTemp = (char *)strchr( as8Url,']' );
		if( pTemp )
		{
			*pTemp = 0;
		}
		strRetHost = &as8Url[ 1 ];
		strcpy( &as8Url[0], pTemp + 1 );
		if( ':' == as8Url[0] )
		{
			bHasCustomPort = true;
		}
	}
	else
	{
		int iHostLen = (int)strlen( as8Url );
		for( int i = 0; i < iHostLen; i++ )
		{
			char c = as8Url[ i ];
			if( c == ':' ||
				c == '/' ||
				c == '\\' )
			{
				if( ':' == c )
				{
					bHasCustomPort = true;
				}
				as8Url[i] = 0;
				strRetHost = as8Url;
				as8Url[i] = c;
				break;
			}
		}
	}
	if( bHasCustomPort )
	{
		//see if has  custom Port 
		pTemp = strchr( as8Url, ':' );
		if ( pTemp !=NULL)
		{
			//had custom port so set it
			u16RetPort = (uint16_t) atoi( pTemp + 1 );
		}
	}

	// get the url
	int iHostLen = (int)strlen( as8Url );
	for( int i = 0; i < iHostLen; i++ )
	{
		if( ('/' == as8Url[ i ] ) ||
			('\\' == as8Url[ i ] ) )
		{
			strRetFileName = &as8Url[ i + 1 ];
			break;
		}
	}

	return true;
}

//============================================================================
bool VxSetSktAllowReuseAddress( SOCKET skt )
{
    int reuseOpt = 1;
    if( setsockopt( skt, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseOpt, sizeof(reuseOpt)) < 0 )
	{
        LogMsg( LOG_ERROR,  "%s error %d %s", __func__, VxGetLastError(), strerror(errno) );
		return false;
	}

	return true;
}

//============================================================================
SOCKET VxConnectTo( InetAddrAndPort& lclIp, InetAddrAndPort& rmtIp, uint16_t u16Port, int iConnectTimeoutMs, int32_t * retSktErr )
{
	if( 0 != retSktErr )
	{
		*retSktErr = 0;
	}

    SOCKET			sktHandle = INVALID_SOCKET;
	std::string		strRmtIp = rmtIp.toString( false );
	if( strRmtIp.empty() )
	{
        LogMsg( LOG_ERROR,  "%s empty remote ip", __func__ );
		return false;
	}

	bool ipv6 = rmtIp.isIPv6();

	int32_t rc = 0;
    struct sockaddr_storage sktAddrStorage;
	socklen_t sktAddrLen = VxSktAddrInit( ipv6, sktAddrStorage, strRmtIp, u16Port );

	// test code only.. test if can connect using ipv6 without timeout/select
	/*
	if( ipv6 )
	{
		// 
		InetAddress inetAddr;
		inetAddr.setIp( sktAddrStorage );
		std::string ipAfterStore = inetAddr.toString();
		if( ipAfterStore != strRmtIp )
		{
			LogMsg( LOG_WARN, "%s storage %s does not match remote ip %s", __func__, ipAfterStore.c_str(), strRmtIp.c_str() );
		}

		struct sockaddr_storage sktStorage3;
		inetAddr.fillAddress( sktStorage3, u16Port );
		sockaddr_in6* sktAddr62 = (struct sockaddr_in6*)&sktStorage3;

		struct sockaddr_storage sktStorage;
		memset( &sktStorage, 0, sizeof( sktStorage ) );
		struct sockaddr* sktAddr = reinterpret_cast<sockaddr*>(&sktStorage);
		sktAddr->sa_family = AF_INET6;
		sockaddr_in6* sktAddr6 = (struct sockaddr_in6*)sktAddr;
		inet_pton( AF_INET6, strRmtIp.c_str(), &sktAddr6->sin6_addr );
		sktAddr6->sin6_port = htons( u16Port );
		int addrLen = sizeof( struct sockaddr_storage );

		struct sockaddr_storage sktStorage2;
		inetAddr.fillAddress( sktStorage2, u16Port );
		sockaddr_in6* sktAddr63 = (struct sockaddr_in6*)&sktStorage2;


		SOCKET skt = socket( ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP );
		int connectResult = connect( skt, sktAddr, addrLen );
		int32_t rc = VxGetLastError();
		if( connectResult != 0 )
		{
			LogMsg( LOG_ERROR, "%s error %d %s connecting to ip %s", __func__, rc, VxDescribeSktError( rc ), strRmtIp.c_str());
		}

		VxCloseSkt( skt );
	}
	*/

	int32_t sktErr = 0;
    sktHandle = socket( ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP );
	struct sockaddr* sktAddr = reinterpret_cast<sockaddr*>(&sktAddrStorage);
    if( INVALID_SOCKET != sktHandle )
    {		
        sktHandle = VxConnectToAddr( sktHandle, sktAddr, sktAddrLen, iConnectTimeoutMs, retSktErr ? retSktErr : &sktErr );
		if( retSktErr )
		{
			rc = *retSktErr;
		}
		else
		{
			rc = sktErr;
		}

		if( !rc )
		{
			rc = VxGetLastError();
		}
    }

    if( INVALID_SOCKET == sktHandle )
	{
        LogModule( eLogConnect, LOG_VERBOSE, "%s: socket connect error %s to %s", __func__, VxDescribeSktError( rc ), strRmtIp.c_str() );
        if( 0 != retSktErr )
        {
            if( 0 == *retSktErr )
            {
                *retSktErr = rc;
            }
        }
    }
    else
    {
		if ( g_SktStatCallback )
		{
			g_SktStatCallback->sktConnected( sktHandle );
		}

        VxGetLclAddress( sktHandle, lclIp );
    }

	if( g_SktStatCallback && INVALID_SOCKET != sktHandle )
	{
        g_SktStatCallback->sktSetRemoteAddr( sktHandle, rmtIp.toString() );
	}

    return sktHandle;
}

//============================================================================
std::string VxSktAddrToString( struct sockaddr* sktAddr, int sktAddrLen, bool includePort )
{
    std::string ipAndPort;
    if( sktAddr )
    {
		if( sktAddr->sa_family == AF_INET6 )
        {
			std::string ipAddr = InetAddress::ipv6BinaryToString( (uint8_t*)&((struct sockaddr_in6*)sktAddr)->sin6_addr );
			if( includePort )
			{
				ipAndPort = "[";
				ipAndPort += ipAddr;
				ipAndPort += "]:";
				ipAndPort += std::to_string( ntohs( ((struct sockaddr_in6*)sktAddr)->sin6_port ) );
			}
			else
			{
				ipAndPort = ipAddr;
			}
        }
		else if( sktAddr->sa_family == AF_INET )
        {
			struct sockaddr_in* sktAddrIn = (struct sockaddr_in*)sktAddr;
            ipAndPort = inet_ntoa( sktAddrIn->sin_addr );
			if( includePort )
			{
				ipAndPort += ":";
				ipAndPort += std::to_string( ntohs( sktAddrIn->sin_port ) );
			}
        }
		else
		{
			LogModule( eLogConnect, LOG_ERROR, "VxSktAddrToString: unknown sa_family" );
			vx_assert( false );
		}
    }

	if( ipAndPort.empty() )
	{
		ipAndPort = ""; // make sure string is empty string instead of nullptr
	}

    return ipAndPort;
}

//============================================================================
SOCKET VxConnectToAddr(SOCKET sktHandle, struct sockaddr* sktAddr, socklen_t sktAddrLen, int iConnectTimeoutMs, int32_t * retSktErr)
{
    if( INVALID_SOCKET == sktHandle )
    {
        LogModule( eLogConnect, LOG_ERROR, "VxConnectToAddr: invalid socket handle" );
        return INVALID_SOCKET;
    }

    if( !sktAddr )
    {
        LogModule( eLogConnect, LOG_ERROR, "VxConnectToAddr: skt %d null sktAddr", sktHandle );
        VxCloseSktNow( sktHandle );
        return INVALID_SOCKET;
    }

    std::string ipAndPort = VxSktAddrToString( sktAddr, sktAddrLen );

    int64_t timeStartConnect = GetGmtTimeMs();

    if( iConnectTimeoutMs > 0 )
    {
		// on windows a regular blocking connect does not timeout for 21 seconds
		// chances are that if there is no connection in 7 seconds it will probably fail with
		// timed out without establishing a connection. so use timed connect
		
        //LogMsg( LOG_SKT, "VxConnectTo: timeout %d skt handle %d connect no block ip %s port %d", iConnectTimeoutMs, sktHandle, strRmtIp.c_str(), u16Port );
        // first try connect to the ip without timeout
        // set to non blocking
        ::VxSetSktBlocking( sktHandle, false );
		 
        int iResult = connect( sktHandle, sktAddr, sktAddrLen );

        if( 0 == iResult )
        {
            // connected
            ::VxSetSktBlocking( sktHandle, true );
            LogModule( eLogConnect, LOG_VERBOSE, "%s: SUCCESS skt %d to %s in %d sec", __func__,
                       sktHandle, ipAndPort.c_str(), TimeElapsedGmtSec( timeStartConnect ) );
            return sktHandle;
        }

        int32_t rc = VxGetLastError();

        if( VxIsFatalSktError( rc ) )
        {
            // some other error other than need more time
            LogModule( eLogConnect, LOG_VERBOSE,  "%s: skt %d connect error %s in %d seconds to %s", __func__,
                       sktHandle, VxDescribeSktError( rc ), TimeElapsedGmtSec( timeStartConnect ), ipAndPort.c_str() );

            VxCloseSktNow( sktHandle );
            return INVALID_SOCKET;
        }

        // needed more time so do connect with timeout
        ::VxSetSktBlocking( sktHandle, true );
        struct timeval tv;
        fd_set sktSet;

		LogModule( eLogConnect, LOG_VERBOSE, "%s: skt %d Attempt connect with timeout %d to %s", __func__, sktHandle, iConnectTimeoutMs, ipAndPort.c_str() );

        do
        {
            tv.tv_sec = iConnectTimeoutMs / 1000;
            tv.tv_usec = (iConnectTimeoutMs % 1000) * 1000;
            FD_ZERO( &sktSet );
            FD_SET( sktHandle, &sktSet );
            iResult = select( sktHandle+1,  // __nfds
                              NULL,         // read ready
                              &sktSet,      // write ready
                              NULL,         // exceptions
                              &tv );        // time spec

            if( iResult < 0 && TimeElapsedGmtMs( timeStartConnect ) < iConnectTimeoutMs)
            {
                LogMsg( LOG_ERROR, "%s: ERROR socket %d select time out is %d but %" PRId64 " ms elapsed to %s", __func__,
                        sktHandle, iConnectTimeoutMs, TimeElapsedGmtMs( timeStartConnect ), ipAndPort.c_str() );
            }

            if ( iResult > 0 )
            {
				// Socket selected for write
				// Check if the socket is writable
				int valopt = 0;
				socklen_t len = sizeof(int);
				if (getsockopt(sktHandle, SOL_SOCKET, SO_ERROR, (char*) & valopt, &len) < 0 ) 
				{
					rc = VxGetLastError();
					if( retSktErr )
					{
						*retSktErr = rc;
					}

					LogModule( eLogConnect, LOG_VERBOSE, "%s: skt %d ERROR %d in getsockopt to %s in %d sec", __func__,
                        sktHandle, rc, ipAndPort.c_str(), TimeElapsedGmtMs( timeStartConnect ) );
					VxCloseSktNow( sktHandle );
					sktHandle = INVALID_SOCKET;
					break;
				}
				else 
				{
					if( valopt )
					{
						LogModule( eLogConnect, LOG_VERBOSE, "%s: skt %d valopt %d timeout %d ms to %s in %d sec", __func__,
								   sktHandle, valopt, iConnectTimeoutMs, ipAndPort.c_str(), TimeElapsedGmtMs( timeStartConnect ) );
						if( retSktErr )
						{
							#ifdef TARGET_OS_WINDOWS
								* retSktErr = WSAETIMEDOUT;
							#else
								* retSktErr = ETIMEDOUT;
							#endif // TARGET_OS_WINDOWS
						}
	
						VxCloseSktNow( sktHandle );
						sktHandle = INVALID_SOCKET;
						break;
					}
					else
					{
						// Connection successful
						LogModule( eLogConnect, LOG_VERBOSE, "%s: Connect Success skt %d to %s in %d sec", __func__,
								   sktHandle, ipAndPort.c_str(), TimeElapsedGmtSec( timeStartConnect ) );
						break;
					}
				} 
            }
			
			if( 0 == iResult && !FD_ISSET( sktHandle, &sktSet ) )
			{
				// this means still in progress
				if( GetGmtTimeMs() - timeStartConnect > iConnectTimeoutMs )
				{
					LogModule( eLogConnect, LOG_VERBOSE, "%s: skt %d connect exceeded max timed out %d seconds.. canceling connect to %s",
							   __func__, sktHandle, TimeElapsedGmtSec( timeStartConnect ), ipAndPort.c_str() );
					if( retSktErr )
					{
						#ifdef TARGET_OS_WINDOWS
							*retSktErr = WSAETIMEDOUT;
						#else
							*retSktErr = ETIMEDOUT;
						#endif // TARGET_OS_WINDOWS
					}

					VxCloseSktNow( sktHandle );
					sktHandle = INVALID_SOCKET;
					break;
				}
				else
				{
					// try again
                    VxSleep( 100 );
					continue;
				}
			}
            else if (iResult < 0 && errno == EINTR)
            {
                /* We were interrupted by a signal.  Just indicate a
                   timeout even though we are early. */
                if( nullptr != retSktErr )
                {
                    *retSktErr = errno;
                }

                LogModule( eLogConnect, LOG_VERBOSE, "VxConnectToAddr: skt %d timeout %d select error %d to %s in %d sec",
                           sktHandle, iConnectTimeoutMs, errno, ipAndPort.c_str(), TimeElapsedGmtSec( timeStartConnect ) );
                VxCloseSktNow( sktHandle );
                sktHandle = INVALID_SOCKET;
                break;
            }
            else if( ( iResult < 0 )
               && ( errno != 4 )
               && ( errno != 10004 ) )
            {
                LogModule( eLogConnect, LOG_VERBOSE, "VxConnectToAddr: skt %d timeout %d select error %d to %s in %d sec",
                           sktHandle, iConnectTimeoutMs, errno, ipAndPort.c_str(), TimeElapsedGmtSec( timeStartConnect ) );

                if( nullptr != retSktErr )
                {
                    *retSktErr = errno;
                }

                VxCloseSktNow( sktHandle );
                sktHandle = INVALID_SOCKET;
                break;
            }
            else if( iResult > 0 )
            {
                // Socket selected for write
                socklen_t iSktLen = sizeof(int);
                int valopt = 0;
                if( getsockopt( sktHandle, SOL_SOCKET, SO_ERROR, (char*)(&valopt), &iSktLen ) < 0 )
                {
                    rc = VxGetLastError();
                    LogModule( eLogConnect, LOG_VERBOSE, "VxConnectToAddr: skt %d Error %d in getsockopt() to %s in %d sec",
                               sktHandle, rc, ipAndPort.c_str(), TimeElapsedGmtSec( timeStartConnect ) );
                    VxCloseSktNow( sktHandle );
                    sktHandle = INVALID_SOCKET;
                    break;
                }
                else
                {
                    //LogMsg( LOG_INFO, "VxConnectTo: skt %d Selecting for write value returned %d", m_SktNumber, valopt );
                    // Check the value returned...
                    if( valopt )
                    {
                        LogModule( eLogConnect, LOG_VERBOSE, "VxConnectToAddr: skt handle %d timeout %d getsockopt valopt %d result %d in delayed connection to %s in %d sec",
                                   sktHandle, iConnectTimeoutMs, valopt, iResult, ipAndPort.c_str(), TimeElapsedGmtSec( timeStartConnect ) );

                        if( nullptr != retSktErr )
                        {
                            *retSktErr = valopt;
                        }

                        VxCloseSktNow( sktHandle );
                        sktHandle = INVALID_SOCKET;
                        break;
                    }
                    else
                    {
                        // connected
                        LogModule( eLogConnect, LOG_VERBOSE, "VxConnectTo: SUCCESS skt %d connected in delayed connection to %s in %d sec",
                                   sktHandle, ipAndPort.c_str(), TimeElapsedGmtSec( timeStartConnect ) );

                        break;
                    }
                }
            }
            else
            {
                LogModule( eLogConnect, LOG_VERBOSE, "VxConnectTo: skt %d connect timed out %d ... canceling at %d seconds to %s",
                           sktHandle, iConnectTimeoutMs, TimeElapsedGmtSec( timeStartConnect ), ipAndPort.c_str() );
                if( nullptr != retSktErr )
                {
					#ifdef TARGET_OS_WINDOWS
						*retSktErr = WSAETIMEDOUT;
					#else
						*retSktErr = ETIMEDOUT;
					#endif // TARGET_OS_WINDOWS
                }

                VxCloseSktNow( sktHandle );
                sktHandle = INVALID_SOCKET;
                break;
            }
        } while (1);
    }
    else
    {
        // connect with no timeout
        LogModule( eLogConnect, LOG_VERBOSE, "VxConnectToAddr: NO TIMEOUT skt %d connect no block to %s", sktHandle, ipAndPort.c_str() );

        ::VxSetSktBlocking( sktHandle, true );
        int iResult = connect( sktHandle, sktAddr, sktAddrLen );
        if( 0 != iResult )
        {
            // connect error
            int32_t rc = VxGetLastError();
            LogModule( eLogConnect, LOG_VERBOSE, "VxConnectToAddr: skt %d connect error %s in %d seconds to %s",
                            sktHandle,
                            VxDescribeSktError( rc ), TimeElapsedGmtSec( timeStartConnect ), ipAndPort.c_str() );
            if( nullptr != retSktErr )
            {
                *retSktErr = rc;
            }

            VxCloseSktNow( sktHandle );
            sktHandle = INVALID_SOCKET;
        }
    }

	if( g_SktStatCallback && INVALID_SOCKET != sktHandle )
	{
		g_SktStatCallback->sktConnected2( sktHandle, VxSktAddrToString( sktAddr, sktAddrLen, false ) );
	}

    return sktHandle;
}

//============================================================================
SOCKET VxConnectToIPv6( const char* ipv6Str, uint16_t u16Port, int iConnectTimeoutMs, int32_t * retSktErr )
{
	fd_set			oSktSet; 
	socklen_t		iSktLen;
	struct timeval	tv;
	SOCKET			sktHandle;
	int             iResult = 0;
	std::string		strRmtIp = ipv6Str;

	int32_t rc = 0;

	struct addrinfo * poAddrInfo;
	struct addrinfo * poResultAddr;
	struct addrinfo oHints;
	VxFillHints( oHints, false, true );

	char as8Port[16];
	sprintf( as8Port, "%d", u16Port);

	//LogMsg( LOG_INFO, "VxConnectToIPv6 port %d ms %d", u16Port, iConnectTimeoutMs ); 
	int error = getaddrinfo( strRmtIp.c_str(), as8Port, &oHints, &poAddrInfo );
	if (error != 0) 
	{
		if( 0 != retSktErr )
		{
			LogModule( eLogSkt, LOG_INFO, "VxConnectToIPv6 port %d ms %d getaddrinfo FAILED with error %d (%s)",
                       u16Port, iConnectTimeoutMs, error, VxDescribeAddrInfoError( error ) );
			*retSktErr = error;
		}

		return INVALID_SOCKET;
	}

	bool bConnectSuccess = false;
	// Try all returned addresses until one works
	for( poResultAddr = poAddrInfo; poResultAddr != NULL; poResultAddr = poResultAddr->ai_next ) 
	{
		//LogMsg( LOG_SKT, "VxConnectTo: create socket for ip %s port %d", strRmtIp.c_str(), u16Port );
		sktHandle = socket( poResultAddr->ai_family, poResultAddr->ai_socktype, poResultAddr->ai_protocol );
		if( INVALID_SOCKET == sktHandle )
		{
			// create socket error
			rc = VxGetLastError();
			LogModule( eLogSkt, LOG_INFO, "VxConnectTo: socket create error %s", VxDescribeSktError( rc ) );
			return sktHandle;
		}

        VxSetSktAllowReuseAddress( sktHandle );
		if( iConnectTimeoutMs )
		{
			//LogMsg( LOG_SKT, "VxConnectTo: timeout %d skt handle %d connect no block ip %s port %d", iConnectTimeoutMs, sktHandle, strRmtIp.c_str(), u16Port );
			// connect to the ip without timeout
			// set to non blocking
			::VxSetSktBlocking( sktHandle, false );
			//LogMsg( LOG_INFO, "VxConnectTo: skt %d attempt no block connect", m_SktNumber );
			iResult = connect( sktHandle, poResultAddr->ai_addr, poResultAddr->ai_addrlen );
			if( 0 == iResult )
			{
				// connected
				::VxSetSktBlocking( sktHandle, true );
				LogModule( eLogSkt, LOG_VERBOSE, "VxConnectTo: SUCCESS skt handle %d connect no block ip %s port %d", sktHandle, strRmtIp.c_str(), u16Port );

				bConnectSuccess = true;
				goto connected;
			}

			rc = VxGetLastError();
#ifdef TARGET_OS_WINDOWS
			if (rc != WSAEWOULDBLOCK)
#else
			if( rc != EINPROGRESS )
#endif // TARGET_OS_WINDOWS
			{
				// some other error other than need more time
				LogModule( eLogSkt, LOG_VERBOSE, "VxConnectTo: skt connect to %s %d error %s", strRmtIp.c_str(), u16Port, VxDescribeSktError( rc ) );

				VxCloseSktNow( sktHandle );
				continue;
			}

			// needed more time so do connect with timeout
			::VxSetSktBlocking( sktHandle, true );
			do
			{ 
				LogModule( eLogSkt, LOG_VERBOSE, "VxConnectTo: skt handle %d Attempt connect with timeout %d", sktHandle, iConnectTimeoutMs );

				tv.tv_sec = iConnectTimeoutMs / 1000; 
				tv.tv_usec = (iConnectTimeoutMs % 1000) * 1000; 
				FD_ZERO(&oSktSet); 
				FD_SET(sktHandle, &oSktSet); 
				iResult = select(sktHandle+1, NULL, &oSktSet, NULL, &tv); 
				//LogMsg( LOG_INFO, "VxConnectTo: skt %d Attempt connect with timeout result %d", m_SktNumber, iResult );
				if (iResult < 0 && 
					(errno != 4) &&
					(errno != 10004)  )

				{ 
					// error
                    if( LogEnabled( eLogSkt ) )
					    LogMsg( LOG_INFO, "VxConnectTo: skt handle %d timeout %d select error %d", sktHandle, iConnectTimeoutMs, errno );

					if( 0 != retSktErr )
					{
						*retSktErr = errno;
					}

					VxCloseSktNow( sktHandle );
					break;
				} 
				else if( iResult > 0 )
				{ 
					//LogMsg( LOG_INFO, "VxConnectTo: skt %d Selecting for write", m_SktNumber );
					// Socket selected for write 
					iSktLen = sizeof(int); 
					socklen_t valopt = 0;
					if( getsockopt(sktHandle, SOL_SOCKET, SO_ERROR, (char*)(&valopt), &iSktLen) < 0 )
					{ 
                        if( LogEnabled( eLogSkt ) )
						    LogMsg( LOG_INFO, "VxConnectTo: skt handle %d Error %d in getsockopt() to ip %s", sktHandle, VxGetLastError(), strRmtIp.c_str() );

						VxCloseSktNow( sktHandle );
						break;
					} 
					else
					{
						//LogMsg( LOG_INFO, "VxConnectTo: skt %d Selecting for write value returned %d", m_SktNumber, valopt );
						// Check the value returned... 
						if( valopt ) 
						{ 
                            if( LogEnabled( eLogSkt ) )
							    LogMsg( LOG_DEBUG, "VxConnectTo: skt handle %d Error %d in delayed connection to ip %s", sktHandle, valopt, strRmtIp.c_str() );

							VxCloseSktNow( sktHandle );
							break;
						}
						else
						{
							// connected
                            if( LogEnabled( eLogSkt ) )
							    LogMsg( LOG_INFO, "VxConnectTo: SUCCESS skt handle %d connected in delayed connection to ip %s", sktHandle, strRmtIp.c_str() );

							::VxSetSktBlocking( sktHandle, true );
							bConnectSuccess = true;
							break;
						} 
					}
				} 
				else 
				{ 
                    if( LogEnabled( eLogSkt ) )
					    LogMsg( LOG_DEBUG, "VxConnectTo: skt handle %d connect to %s port %d timed out.. canceling", sktHandle, strRmtIp.c_str(), u16Port );

					VxCloseSktNow( sktHandle );
					break; 
				} 
			} while (1); 
			if( bConnectSuccess )
			{
				break;
			}
		}
		else
		{
            if( LogEnabled( eLogSkt ) )
			    LogMsg( LOG_DEBUG, "VxConnectTo: NO TIMEOUT skt handle %d connect no block ip %s port %d", sktHandle, strRmtIp.c_str(), u16Port );

			// connect to the ip without timeout
			iResult = connect( sktHandle, poResultAddr->ai_addr, poResultAddr->ai_addrlen );
			if( 0 != iResult )
			{
				// connect error
				rc = VxGetLastError();
                if( LogEnabled( eLogSkt ) )
				    LogMsg( LOG_DEBUG, "VxConnectTo: skt handle %d connect to %s error %s",
					        sktHandle,
					        strRmtIp.c_str(),
					        VxDescribeSktError( rc ) );

				VxCloseSktNow( sktHandle );
				return INVALID_SOCKET;
			}
			else
			{
				bConnectSuccess = true;
				break;
			}
		}
	}
connected:
	if( bConnectSuccess )
	{
		return sktHandle;
	}
	else
	{
		return INVALID_SOCKET;
	}
}

//============================================================================
SOCKET VxConnectTo(		InetAddrAndPort&	lclIp,
						InetAddrAndPort&	rmtIp,
						const char*			pIpAddr,				// remote ip
						uint16_t			u16Port,				// port to connect to
						EIpAddrType			addrType,
						int					iTimeoutMilliSeconds,
						int32_t *				retSktError )	// milli seconds before connect attempt times out
{
	if( VxIsIPv4Address( pIpAddr ) || VxIsIPv6Address( pIpAddr ) )
	{
		rmtIp.setIp( pIpAddr );
        rmtIp.setPort( u16Port );
	}
    else if( VxResolveHostToIp( pIpAddr, u16Port, rmtIp, addrType ) )
    {
        rmtIp.setPort( u16Port );
    }
    else
	{
        LogModule( eLogConnect, LOG_INFO, "%s: FAILED to resolve %s", __func__, pIpAddr );
		return INVALID_SOCKET;
	}

	VxElapseTimer connectToTimer;
	SOCKET sktHandle = VxConnectTo( lclIp, rmtIp, u16Port, iTimeoutMilliSeconds, retSktError );
	if( INVALID_SOCKET == sktHandle )
	{
        LogModule( eLogConnect, LOG_DEBUG, "%s: FAIL connect %3.3f sec lcl ip %s to %s:%d timeout %d error %d thread 0x%x", __func__,
	               connectToTimer.elapsedSec(), lclIp.toString().c_str(), rmtIp.toString().c_str(), u16Port, iTimeoutMilliSeconds, retSktError ? *retSktError : -1, VxGetCurrentThreadId() );
	}
	else
	{
        LogModule( eLogConnect, LOG_DEBUG, "%s: SUCCESS connect %3.3f sec lcl ip %s to %s:%d thread 0x%x", __func__,
			       connectToTimer.elapsedSec(), lclIp.toString().c_str(), rmtIp.toString().c_str(), u16Port, VxGetCurrentThreadId());
	}

	return sktHandle;
}

//============================================================================
// connects to website and returns socket.. if fails returns INVALID_SOCKET
SOCKET VxConnectToWebsite(	InetAddrAndPort&	lclIp,			// ip of adapter to use
							InetAddrAndPort&	rmtIp,			// return ip and port url resolves to
							const char*			pWebsiteUrl,
							std::string&		strHost,		// return host name.. example http://www.mysite.com/index.htm returns www.mysite.com
							std::string&		strFile,		// return file name.. images/me.png
							uint16_t&			u16Port,		// return port
						    EIpAddrType			addrType,
							int					iConnectTimeoutMs,
						    int32_t*				retErrorCode )
{
	// split host name from file path
	bool bSplitOk = VxSplitHostAndFile(	pWebsiteUrl,	// full url.. example http://www.mysite.com/index.html or www.mysite.com/images/me.png
										strHost,		// return host name.. example http://www.mysite.com/index.htm returns www.mysite.com
										strFile,		// return file name.. images/me.png
										u16Port );		// return port if specified else return 80 as default
	if( false == bSplitOk )
	{
		LogMsg( LOG_ERROR, "VxConnectToWebsite: Invalid URL %s", pWebsiteUrl );
		return INVALID_SOCKET;
	}

	const char* hostname = strHost.c_str();
	//LogMsg( LOG_VERBOSE, "VxConnectToWebsite: Attempting to connect to %s port %d...", hostname, u16Port );

	SOCKET hSocket = VxConnectTo(	lclIp,					// ip of adapter to use
									rmtIp,					// return ip and port url resolves to
									hostname,				// remote ip or url
									u16Port,				// port to connect to
								    addrType,
									iConnectTimeoutMs,
									retErrorCode );	// timeout attempt to connect
	if( INVALID_SOCKET == hSocket )
	{
		//LogMsg( LOG_ERROR, "VxConnectToWebsite: failed to connect to %s %s", hostname, VxDescribeSktError( VxGetLastError() ) );
		return INVALID_SOCKET;
	}

	//LogMsg( LOG_VERBOSE, "VxConnectToWebsite: Connected to %s:%d...", hostname, u16Port );
	return hSocket;
}

//============================================================================
bool VxBindSkt( SOCKET oSocket, InetAddress & oLclAddr, uint16_t u16Port )
{
	struct sockaddr_storage 		oSktAddr;		// listen socket address
	oLclAddr.fillAddress( oSktAddr, u16Port );
	return VxBindSkt( oSocket, &oSktAddr );
}

//============================================================================
bool VxTestConnectionOnSpecificLclAddress( InetAddress& oLclAddr )
{
	bool bConnectionOk = false;
	std::string strLclAddr = oLclAddr.toString();

	InetAddress oRmtAddr;
	if( VxResolveUrl( "nolimitconnect.org", 80, oRmtAddr, VxGetIpAddrType( strLclAddr.c_str() ) ) )
	{
		// attempt connect
		// Open a socket with the correct address family for this address.
		SOCKET oVxSktConnect = socket( oRmtAddr.isIPv4() ? PF_INET : PF_INET6, SOCK_STREAM, 0 );
		if ( oVxSktConnect == INVALID_SOCKET )
		{
			LogMsg( LOG_INFO, "TestConnectionOnSpecificLclAddress: failed to create socket" );
		}
		else
		{
#if USE_BIND_LOCAL_IP
            struct sockaddr_storage oLclSktStorage;
            oLclAddr.fillAddress( oLclSktStorage, 0 );
			if ( false == VxBindSkt( oVxSktConnect, &oLclSktStorage ) )
			{
				LogMsg( LOG_INFO, "TestConnectionOnSpecificLclAddress: failed to bind skt with ip %s", strLclAddr.c_str() );
			}
			else
			{
#endif // USE_BIND_LOCAL_IP
				struct sockaddr_storage oRmtSktAddr;
				int iRmtSktAddrLen = oRmtAddr.fillAddress( oRmtSktAddr, 80 );

				int result = connect( oVxSktConnect, ( struct sockaddr * )&oRmtSktAddr, iRmtSktAddrLen );
				if ( 0 == result )
				{
					bConnectionOk = true;
				}
				else
				{
					result = VxGetLastError();
					LogMsg( LOG_ERROR, "TestConnectionOnSpecificLclAddress: connect error %s", VxDescribeSktError( result ) );
				}
#if USE_BIND_LOCAL_IP
			}
#endif // USE_BIND_LOCAL_IP
			::VxCloseSkt( oVxSktConnect );
		}
	}
	return bConnectionOk;
}

//============================================================================
bool VxResolveUrl( const char* pUrl, uint16_t u16Port, InetAddress& retInetAddr, EIpAddrType addrType )
{
	std::string resolvedIp;
	if( VxResolveUrl( pUrl, u16Port, resolvedIp, addrType ) )
	{
		retInetAddr.setIp( resolvedIp.c_str() );
		return true;
	}
	else
	{
		retInetAddr.setToInvalid();
		return false;
	}
}

//============================================================================
bool VxResolveUrl( std::string& urlIn, uint16_t& retPort, std::string& retIpAddr, EIpAddrType addrType )
{
	std::string strHost;
	std::string strFile;
	uint16_t tcpPort{ 0 };

	bool result = VxSplitHostAndFile( urlIn.c_str(), strHost, strFile, tcpPort );
	retPort = tcpPort;
    return result && VxResolveUrl( strHost.c_str(), tcpPort, retIpAddr, addrType );
}

//============================================================================
bool VxResolveUrl( const char* pUrl, uint16_t u16Port, std::string& resolvedIp, EIpAddrType addrType  ) // assumes pUrl is just host name
{
	if( !pUrl || strlen( pUrl ) < 5 )
	{
		return false;
	}

	if( VxIsIPv4Address( pUrl ) || VxIsIPv6Address( pUrl ) )
	{
		resolvedIp = pUrl;
		return true;
	}

	bool bResult = false;
	
	// get addresses
	InetAddress inetAddr;
	struct addrinfo Hints;
	struct addrinfo* AI;
	struct addrinfo* AddrInfo;

	memset( &Hints, 0, sizeof( Hints ) );
	switch( addrType )
	{
	case eIpAddrTypeIpv6:
		Hints.ai_family = PF_INET6;
		break;
	case eIpAddrTypeIpv4:
		Hints.ai_family = PF_INET;
		break;
	case eIpAddrTypeUnknown:
	default:
		Hints.ai_family = PF_UNSPEC;
	}

	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_protocol = IPPROTO_TCP;

	char as8Buf[32];
	sprintf( as8Buf, "%d", u16Port );

	int RetVal = getaddrinfo( pUrl, as8Buf, &Hints, &AddrInfo );
	if( RetVal != 0 )
	{
       LogMsg( LOG_ERROR, "VxResolveUrl: Failed to resolve host %s port %d error %d reason %s ",
               pUrl, u16Port, RetVal, VxDescribeAddrInfoError( RetVal ) );
	}
	else
	{
		for( AI = AddrInfo; AI != NULL; AI = AI->ai_next )
		{
			if( ( AI->ai_family != PF_INET ) && ( AI->ai_family != PF_INET6 ) )
			{
				continue;
			}

			struct sockaddr_storage* poSktAddr = ( struct sockaddr_storage* )AI->ai_addr;
			inetAddr.setIp( *poSktAddr );
			if( inetAddr.isIPv6() )
			{
				continue;
			}

			resolvedIp = inetAddr.toString();
			bResult = true;
			break;
		}

		freeaddrinfo( AddrInfo ); // free the linked list
	}

	return bResult;
}

//============================================================================
bool VxIsIPv6Address( const char*addr )
{
	if ( NULL == addr )
	{
		return false;
	}
	std::string addrStr = addr;
	if ( addrStr.find( ":" ) != std::string::npos )
	{
		return true;
	}

	return false;
}


//============================================================================
bool VxIsIPv4Address( const char* addr, bool checkNoLocal )
{
	if ( NULL == addr )
	{
		return false;
	}

	int strLen = strlen( addr );
	if ( strLen < 7 )
	{
		return false;
	}

	int dotCount{ 0 };
	for ( int i = 0; i < strLen; i++ )
	{
		if ( !isdigit( addr[ i ] ) && ( '.' != addr[ i ] ) )
		{
			return false;
		}

		if( '.' == addr[i] )
		{
			dotCount++;
		}
	}

	if( dotCount != 3 )
	{
		return false;
	}

	if( checkNoLocal )
	{
		std::string ip( addr );
		if( ip == "0.0.0.0" || ip == "127.0.0.1" )
		{
			return false;
		}
	}

	return true;
}

//============================================================================
int VxGetIPv6ScopeID( const char* addr )
{
	if ( false == VxIsIPv6Address( addr ) )
	{
		return 0;
	}
	std::string addrStr = addr;
	int pos = (int)addrStr.find( "%" );
	if ( pos == (int)std::string::npos )
	{
		return 0;
	}
	std::string scopeStr = addrStr.substr( pos + 1, addrStr.length() );
	return atoi( scopeStr.c_str() );
}

//============================================================================	
int32_t VxGetRmtAddress( SOCKET sktHandle, InetAddrAndPort& oRetAddr, bool isSimpleSkt )
{
	// Get the IP address of the the remote side of connection
	int32_t rc = 0;
	struct sockaddr sktAddr;
	socklen_t sktAddrLen = sizeof( sktAddr );
	memset( &sktAddr, 0, sizeof( sktAddr ) );

	if( getpeername( sktHandle, ( struct sockaddr* )&sktAddr, &sktAddrLen ) )
	{
		// error occurred
		oRetAddr.setToInvalid();
		rc = VxGetLastError();

        LogModule( eLogSkt,  LOG_DEBUG, "VxGetRmtAddress: skt handle %d error %d %s", sktHandle, rc, VxDescribeSktError( rc ) );
	}
	else
	{
		oRetAddr.setIpAndPort( sktAddr );
		// dont attempt to set skt rmt address if is simple socket.. else will log error when cannot find the skt handle
		if( g_SktStatCallback && !isSimpleSkt )
		{
			g_SktStatCallback->sktSetRemoteAddr( sktHandle, oRetAddr.toString() );
		}
	}

	return rc;
}

//============================================================================
int32_t VxGetLclAddress( SOCKET sktHandle, InetAddrAndPort& retAddr )
{
	// Get the IP address of the the local side of connection
	int32_t rc = 0;
	struct sockaddr_storage sktStorage;
	socklen_t sktAddrLen = sizeof( struct sockaddr_storage );
	memset( &sktStorage, 0, sizeof( sockaddr_storage ) );

	struct sockaddr* sktAddr = reinterpret_cast<sockaddr*>(&sktStorage);

    if( getsockname( sktHandle, sktAddr, &sktAddrLen ) )
	{
		// error occurred
		retAddr.setToInvalid();
		rc = VxGetLastError();
        LogMsg( LOG_DEBUG, "VxGetRmtAddress: skt handle %d error %d %s", sktHandle, rc, VxDescribeSktError( rc ) );
	}
	else
	{
		retAddr.setIpAndPort( *sktAddr );
	}

	return rc;
}

//============================================================================
std::string	VxGetLclIpAddress( SOCKET sktHandle, uint16_t* retPort )
{
	std::string ipAddr;

	struct sockaddr_storage sktStorage;
	socklen_t sktAddrLen = sizeof( struct sockaddr_storage );
	memset( &sktStorage, 0, sizeof( sockaddr_storage ) );

	struct sockaddr* sktAddr = reinterpret_cast<sockaddr*>(&sktStorage);

	if( 0 == getsockname( sktHandle, sktAddr, &sktAddrLen ) )
	{
		InetAddress inetAddr;
		uint16_t port = inetAddr.setIp( *sktAddr );
		if( port )
		{
			ipAddr = inetAddr.toString();
			if( retPort )
			{
				*retPort = port;
			}
		}
	}
	else
	{
		int32_t rc = VxGetLastError();
		LogMsg( LOG_ERROR, "%s error %d %s", __func__, rc, VxDescribeSktError( rc ) );
	}

	return ipAddr;
}

//============================================================================
std::string VxGetRmtHostName( SOCKET& skt )
{
	std::string ipAddr;

	// get peer address/port
	struct sockaddr_storage sktStorage;
	socklen_t sktAddrLen = sizeof( struct sockaddr_storage );
	memset( &sktStorage, 0, sizeof( sockaddr_storage ) );

	struct sockaddr* sktAddr = reinterpret_cast<sockaddr*>(&sktStorage);

	if ( 0 == getpeername( skt, sktAddr, &sktAddrLen ) )
	{
		InetAddress inetAddr;
		uint16_t port = inetAddr.setIp( *sktAddr );
		if( port )
		{
			ipAddr = inetAddr.toString();
		}
	}
	else
	{
		int32_t rc = VxGetLastError();
		LogMsg( LOG_ERROR, "%s error %d %s", __func__, rc, VxDescribeSktError( rc ) );
	}

#ifdef IP_OPTIONS
	if( !ipAddr.empty() )
	{
		// If IP options are supported, make sure there are none 
		// options can be used to pretend you are from a ip address you are not from
		// close any connection with options
		unsigned char		options[ 200 ];
		unsigned char *		ucp;
		char text[ 1024 ], *cp;
		socklen_t			optionLen;
		int					ipproto;
		struct protoent *	ip;

		if ( NULL != ( ip = getprotobyname( "ip" ) ) )
			ipproto = ip->p_proto;
		else
			ipproto = IPPROTO_IP;
		optionLen = sizeof( options );
		if ( ( 0 >= getsockopt( skt, ipproto, IP_OPTIONS, (char *)options, &optionLen ) )
			 && ( 0 != optionLen ) )
		{
			cp = text;
			if ( optionLen > 256 )
				optionLen = 256;
			//NOTE: buffer must be at least 3 times as big as options
			for ( ucp = options; optionLen > 0; ucp++, optionLen--, cp += 3 )
				sprintf( cp, " %2.2x", *ucp );
			VxReportHack( eHackerLevelSuspicious, eHackerReasonHostIpOptions, skt, ipAddr.c_str(), "Connection from %s with IP options:%.800s",
						  ipAddr.c_str(), text );
			VxCloseSktNow( skt );
		}
	}
#endif // IP_OPTIONS


	return ipAddr;
}

//============================================================================
std::string VxGetRemoteIpAddress( SOCKET skt )
{
	std::string ipAddr;

	// get peer address/port
	struct sockaddr_storage sktStorage;
	socklen_t sktAddrLen = sizeof( struct sockaddr_storage );
	memset( &sktStorage, 0, sizeof( sockaddr_storage ) );

	struct sockaddr* sktAddr = reinterpret_cast<sockaddr*>(&sktStorage);

	if ( 0 == getpeername( skt, sktAddr, &sktAddrLen ) )
	{
		InetAddress inetAddr;
		uint16_t port = inetAddr.setIp( *sktAddr );
		if( port )
		{
			ipAddr = inetAddr.toString();
		}
	}

	if( !ipAddr.empty() && g_SktStatCallback )
	{
		g_SktStatCallback->sktSetRemoteAddr( skt, ipAddr );
	}

	return ipAddr;
}

//============================================================================
//! receive data.. if timeout is set then will keep trying till buffer is full or error or timeout expires
int32_t VxReceiveSktData( SOCKET&			sktHandle,
						char *			pRetBuf,				// buffer to receive data into
						int				iBufLenIn,				// length of buffer
						int *			iRetBytesReceived,		// number of bytes actually received
						int				iTimeoutMilliSeconds )	// milliseconds before receive attempt times out ( 0 = dont wait )
{
	int32_t m_rcLastError = 0; // clear out any previous error
	*iRetBytesReceived = 0;

	int	iRecvResult;
	vx_assert( pRetBuf );
	vx_assert( iBufLenIn > 0 );
	vx_assert( iRetBytesReceived );
	// use non blocking
	::VxSetSktBlocking( sktHandle, false );

	int bufLenRemaining = iBufLenIn;
	int dataLenRxed = 0;

    int64_t timeStart = GetApplicationAliveMs();
	while( !iTimeoutMilliSeconds || (iTimeoutMilliSeconds > GetApplicationAliveMs() - timeStart) )
	{
		iRecvResult = recv( sktHandle,			// socket
							pRetBuf,			// buffer to read into
							bufLenRemaining,	// length of buffer space
							0 );				// flags

		if( INVALID_SOCKET == sktHandle )
		{
			m_rcLastError = VxGetLastError();
			LogMsg( LOG_INFO, "%s :recv error %d %s", __func__, m_rcLastError, VxDescribeSktError( m_rcLastError ) );
			return m_rcLastError;
		}

		if( 0 > iRecvResult )
		{
			m_rcLastError = VxGetLastError();
			if( VxIsFatalSktError( m_rcLastError ) )
			{
				return m_rcLastError;
			}

			// see if have timeout value
			if( iTimeoutMilliSeconds )
			{
				if( iTimeoutMilliSeconds < GetApplicationAliveMs() - timeStart )
				{
					// timeout
					break;
				}
				else
				{
					VxSleep( 50 );
					continue;
				}
			}
			else
			{
				// timeout with 0 timeout
				break;
			}
		}
		else if( 0 == iRecvResult )
		{
			// may not of had time to get some data
			// see if have timeout value
			if( iTimeoutMilliSeconds )
			{
				if( iTimeoutMilliSeconds < GetApplicationAliveMs() - timeStart )
				{
					// timeout
					break;
				}
				else
				{
					VxSleep( 50 );
					continue;
				}
			}
			else
			{
				// timeout with 0 timeout
				break;
			}
		}
		else
		{
			// got some data
			// advance to end of received data for next receive
			bufLenRemaining -= iRecvResult;
			dataLenRxed += iRecvResult;
			pRetBuf += iRecvResult;

			if( bufLenRemaining < 0 )
			{
				// should not happen
				m_rcLastError = VxGetLastError();
				if( !m_rcLastError )
				{
					m_rcLastError = -1;
				}

				LogMsg( LOG_INFO, "%s : invalid return value %d resulted in buf overflow %d rc = %d %s",
						__func__, iRecvResult, bufLenRemaining, m_rcLastError, VxDescribeSktError( m_rcLastError ) );
				return m_rcLastError;
			}
			else if( bufLenRemaining )
			{
				if( iTimeoutMilliSeconds )
				{
					if( iTimeoutMilliSeconds < GetApplicationAliveMs() - timeStart )
					{
						// timeout
						break;
					}
					else
					{
						VxSleep( 50 );
						continue;
					}
				}
				else
				{
					// timeout with 0 timeout
					break;
				}
			}
			else
			{
				// no more room in buffer
				break;
			}
		}
	}

	// fill results and return 0
	*iRetBytesReceived = dataLenRxed;

	int64_t timeEnd = GetApplicationAliveMs();
    LogModule( eLogSktData, LOG_VERBOSE, "VxReceiveSktData rxed len %d in %" PRId64 " ms with timeout %d ms thread 0x%x", dataLenRxed, timeEnd - timeStart, iTimeoutMilliSeconds, VxGetCurrentThreadId() );
	return 0;
}

//============================================================================
bool VxBindSkt( SOCKET oSocket, struct sockaddr_storage * poAddr )
{
	if ( SOCKET_ERROR == bind( oSocket, ( struct sockaddr * )poAddr, VxGetSktAddressLength( poAddr ) ) )
	{
		LogMsg( LOG_ERROR, "ERROR Bind Socket Failed rc = %d", VxGetLastError() );
		return false;
	}
	return true;
}

//============================================================================
bool VxMakeBroadcastIp( std::string localIp, std::string& retBroadcastIp )
{
	retBroadcastIp = "";
	if( VxIsIpValid( localIp ) )
	{
		if( std::string::npos == localIp.find_first_of( ':' ) )
		{
			std::string::size_type lastQuad = localIp.find_last_of( '.' );
			if( std::string::npos != lastQuad )
			{
				retBroadcastIp = localIp;
				retBroadcastIp.replace( retBroadcastIp.begin() + lastQuad, retBroadcastIp.end(), ".255" );
				return true;
			}
			else
			{
				LogMsg( LOG_ERROR, "VxMakeBroadcastIp last quad not found" );
			}
		}
		else
		{
			LogMsg( LOG_ERROR, "VxMakeBroadcastIp only works for iPv4" );
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "VxMakeBroadcastIp empty localIp" );
	}

	return false;
}

//============================================================================
// C functions
//============================================================================
NLC_BEGIN_CDECLARES

//============================================================================
//! set socket to blocking or not
int32_t VxSetSktBlocking( SOCKET sktHandle, bool bBlock )
{

#if defined(TARGET_OS_WINDOWS)

    u_long nonBlock = bBlock ? 0 : 1;
    int result = ioctlsocket( sktHandle, FIONBIO, &nonBlock );

#elif defined(TARGET_OS_LINUX)

    // this works in linux but not android
    int sktFlags = fcntl( sktHandle, F_GETFL, 0 );
    if( bBlock )
    {
		sktFlags &= ~O_NONBLOCK;
    }
    else
    {
		sktFlags |= O_NONBLOCK;
    }

    int result = fcntl( sktHandle, F_SETFL, sktFlags );

#elif defined(TARGET_OS_ANDROID)

    // this works in android
    int nonBlock = bBlock ? 0 : 1;
    int result = ioctl( sktHandle, FIONBIO, &nonBlock );

#endif

    if( result )
    {
        LogMsg( LOG_ERROR, "VxSktBase::setSktBlocking skt %d ioctlsocket error %d %s", sktHandle, result, VxDescribeSktError( result ) );
    }

    return result;
}

//============================================================================
void VxCloseSktNow( SOCKET& oSocket )
{
	if( INVALID_SOCKET != oSocket )
	{
        linger oLinger;
        oLinger.l_linger = 0;
        oLinger.l_onoff = 1;
        setsockopt(	oSocket,
                (int) IPPROTO_TCP,   //level
                (int) SO_LINGER,
                (const char*)&oLinger,
                (int)sizeof( linger ) );

		VxCloseSkt( oSocket );
	}
}

//============================================================================
void VxCloseSkt( SOCKET& oSocket )
{
	if( INVALID_SOCKET != oSocket )
	{
		if( LogEnabled( eLogConnect ) )LogModule( eLogConnect, LOG_VERBOSE, "--%s closing skt handle %d", __func__, oSocket );
		#if defined( TARGET_OS_WINDOWS )
				shutdown( oSocket, 0 );
				closesocket( oSocket );
		#elif defined( TARGET_OS_LINUX ) || defined( TARGET_OS_ANDROID )
				shutdown( oSocket, SHUT_RDWR );
				close( oSocket );
		#else
				close( oSocket );
		#endif // TARGET_OS_WINDOWS
		if( g_SktStatCallback )
		{
			g_SktStatCallback->sktClosed( oSocket );
		}

		oSocket = INVALID_SOCKET;
	}
}


//============================================================================
socklen_t VxGetSktAddressLength( struct sockaddr_storage * poAddr )
{
    socklen_t oAddrLen = 0;
    switch( poAddr->ss_family )
    {
    case AF_INET:
        oAddrLen = sizeof(struct sockaddr_in);
        break;

    case AF_INET6:
        oAddrLen = sizeof(struct sockaddr_in6);
        break;

    default:
        LogMsg( LOG_FATAL, "GetSktAddressLength: Invalid Address Family %d", poAddr->ss_family);
        vx_assert(false);
    }

    return oAddrLen;
}

//============================================================================
void VxSetSktAddressPort( struct sockaddr_storage * poAddr, uint16_t u16Port )
{
    switch( poAddr->ss_family )
    {
    case AF_INET:
        ((struct sockaddr_in *)poAddr)->sin_port = htons(u16Port);
        break;

    case AF_INET6:
        ((struct sockaddr_in6 *)poAddr)->sin6_port = htons(u16Port);
        break;

    default:
        LogMsg( LOG_FATAL, "SetSktAddressPort: Invalid Address Family %d", poAddr->ss_family);
        vx_assert(false);
    }
}


//============================================================================
int32_t VxSendSktData(	SOCKET			sktHandle,
                        const char*     pData,					// data to send
						int				iDataLen,				// length of data
						int				iTimeoutSeconds )		// seconds before send attempt times out
{
	vx_assert( iDataLen > 0 );
	int iSendResult		= 0;
	int32_t m_rcLastError		= 0;
	if( iTimeoutSeconds )
	{
		do
		{
			if( iTimeoutSeconds )
			{
				fd_set	oFdSet;
				FD_ZERO( &oFdSet );
				FD_SET( sktHandle, &oFdSet );

				// set timeout val
				struct timeval		oTimeVal;
				oTimeVal.tv_sec		= iTimeoutSeconds;
				oTimeVal.tv_usec	= 0;

				int iSelectResult = select( (int)sktHandle + 1, NULL, &oFdSet, NULL, &oTimeVal );
				if( 0 >= iSelectResult )
				{
					//select timed out or select error occurred
					m_rcLastError = VxGetLastError();
					if( 0 == m_rcLastError )
					{
						m_rcLastError = iSelectResult;
					}
					//LogMsg( LOG_INFO, "VxSktCode:sendData: Send to %s Select Error %d", getRemoteIp(), iSelectResult );
					VxCloseSktNow(sktHandle);
					return m_rcLastError;
				}
			}
			// send the data
#ifdef TARGET_OS_WINDOWS
			iSendResult =  send( sktHandle, pData, iDataLen, 0);
#else // LINUX
			// fix so broken pipe does not cause app to exit ( code 141 )
			iSendResult  =  send( sktHandle, pData, iDataLen, MSG_NOSIGNAL );
#endif // TARGET_OS_WINDOWS
			if( 0 >= iSendResult )
			{
				// send error
				m_rcLastError = VxGetLastError();
				if( 0 == m_rcLastError )
				{
					m_rcLastError = iSendResult;
				}
				VxCloseSktNow(sktHandle);
				return m_rcLastError;
			}
			pData += iSendResult;
			iDataLen -= iSendResult;
			if( 0 >= iDataLen )
			{
				// all sent
				break;
			}
			// not all was sent
			VxSleep( 10 ); 
			//if( false == this->isConnected() )
			//{
			//	LogMsg( LOG_INFO, "VxSktCode::sendData: attempted send on disconnected socket" );
			//	return -1;
			//}
		}while( iDataLen > 0 );
	}
	else
	{
		// send the data
#ifdef TARGET_OS_WINDOWS
		iSendResult =  send( sktHandle, pData, iDataLen, 0);
#else // LINUX
		// fix so broken pipe does not cause app to exit ( code 141 )
		iSendResult  =  send( sktHandle, pData, iDataLen, MSG_NOSIGNAL );
#endif // TARGET_OS_WINDOWS
		if( 0 >= iSendResult )
		{
			// send error
			m_rcLastError = VxGetLastError();
			if( 0 == m_rcLastError )
			{
				m_rcLastError = iSendResult;
			}

			VxCloseSktNow(sktHandle);
			return m_rcLastError;
		}
	}

	return 0;
}


//============================================================================
uint16_t VxGetRmtPort( SOCKET skt )
{
	struct sockaddr from;
	socklen_t addrLen;

	addrLen = sizeof(from);
	memset(&from, 0, sizeof(from));
	if( 0 > getpeername( skt, (struct sockaddr *)&from, &addrLen) )
	{
		LogMsg( LOG_ERROR, "getpeername failed: %.100s", strerror(errno) );
		return 0;
	}

	if( AF_INET == from.sa_family )
	{
		return ((sockaddr_in *)(&from))->sin_port;
	}
	else if( AF_INET6 == from.sa_family )
	{
		return ((sockaddr_in6 *)(&from))->sin6_port;
	}

	return 0;
}

//============================================================================
//=== socket errors ===//
//============================================================================
//============================================================================
const char* VxDescribeSktError( int iErr )
{
	static char as8Buf[128];

    if( iErr < 0 )
    {
        return "Connection Dropped (negative error value)";
    }

	if( iErr == 0 )
	{
		return "Socket Error 0 Unknown";
	}

#ifdef TARGET_OS_WINDOWS
	switch ( iErr )
	{
	case WSANOTINITIALISED:	return "A successful AfxSocketInit must occur before using this API.";
	case WSAEADDRINUSE:		return "The specified address is already in use.";
	case WSAEINPROGRESS:	return "A blocking Windows Sockets call is in progress.";
	case WSAEADDRNOTAVAIL:	return "The specified address is not available from the local machine.";
	case WSAEAFNOSUPPORT:	return "Addresses in the specified family cannot be used with this socket.";
		//	case WSAEDESTADDREQ:	return "A destination address is required.";
	case WSAEFAULT:			return "The nSockAddrLen argument is incorrect.";
	case WSAEINVAL:			return "The socket is not already bound to an address.";
	case WSAEMFILE:			return "No more file descriptors are available.";
	case WSAENETDOWN:		return "A socket operation encountered a dead network.";
	case WSAENETUNREACH:	return "The network cannot be reached from this host at this time.";
	case WSAENETRESET:		return "The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.";
	case WSAECONNABORTED:	return "An established connection was aborted by the software in your host machine.";
	case WSAECONNRESET:		return "An existing connection was forcibly closed by the remote host.";
	case WSAENOBUFS:		return "An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.";
	case WSAEISCONN:		return "A connect request was made on an already connected socket.";
	case WSAENOTCONN:		return "A request to send or receive data was disallowed because the socket is not connected.";
	case WSAENOTSOCK:		return "The descriptor is not a socket.";
	case WSAEWOULDBLOCK:	return "The socket is marked as nonblocking and the connection cannot be completed immediately.";
	case WSAEOPNOTSUPP:		return "MSG_OOB was specified, but the socket is not of type SOCK_STREAM.";
	case WSAESHUTDOWN:		return "The socket has been shut down; it is not possible to call Receive on a socket after ShutDown has been invoked with nHow set to 0 or 2.";
	case WSAEMSGSIZE:		return "The datagram was too large to fit into the specified buffer and was truncated.";
	case WSAETOOMANYREFS:	return "Too many references to some kernel object.";
	case WSAETIMEDOUT:		return "Attempt to connect timed out without establishing a connection.";
	case WSAECONNREFUSED:	return "The attempt to connect was rejected.";
	case WSAELOOP:			return "Cannot translate name.";
	case WSAENAMETOOLONG:	return "Name component or name was too long.";
	case WSAEHOSTDOWN:		return "A socket operation failed because the destination host was down.";
	case WSAEHOSTUNREACH:	return "A socket operation was attempted to an unreachable host.";
	case WSAENOTEMPTY:		return "Cannot remove a directory that is not empty.";
	case WSAEPROCLIM:		return "A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.";
	case WSAEUSERS:			return "Ran out of quota.";
	case WSAEDQUOT:			return "Ran out of disk quota.";
	case WSAESTALE:			return "File handle reference is no longer available.";
	case WSAEREMOTE:		return "Item is not available locally.";
	case WSASYSNOTREADY:	return "WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.";
	case WSAVERNOTSUPPORTED:return "The Windows Sockets version requested is not supported.";
		
	case EIM_ALIVE_TIMEDOUT: return "Connection I Am Alive Request Timed Out";
	default:				return "Windows Unknown socket error";
	}
#else
	LogMsg( LOG_ERROR, "VxDescribeSktError Linux %d", iErr );
	switch ( iErr )
	{
	case 1: // EPERM
		return "Operation not permitted.";
	case 2: // ENOENT           2      
		return "No such file or directory.";
	case 3: // ESRCH            3      
		return "No such process.";
	case 4: // EINTR            4      
		return "Interrupted system call.";
	case 5: // EIO              5     
		return "I/O error.";
	case 6: // ENXIO            6      
		return "No such device or address.";
	case 7: // E2BIG            7      
		return "Arg list too long.";
	case 8: // ENOEXEC          8      
		return "Exec format error.";
	case 9: // EBADF            9      
		return "Bad file number.";
	case 10: // ECHILD          10     
		return "No child processes.";
	case 11: // EAGAIN          11     
		return "Try again.";
	case 12: // ENOMEM          12     
		return "Out of memory.";

	case 13: // SOCACCES
		return "Permission denied";
	case 14: // SOCFAULT
		return "Bad address";
	case 15: // ENOTBLK
		return "Block device required";
	case 16: // EBUSY
		return "Device is busy";
	case 17: // EEXIST
		return "File exists";
	case 18: // EXDEV
		return "Cross device link";
	case 19: // ENODEV
		return "No such device";
	case 20: // ENOTDIR
		return "Not a directory";
	case 21: // EISDIR
		return "Is a directory";
	case 22: // SOCINVAL
		return "Invalid argument";
	case 23: // SOCINVAL
		return "File Table overflow";
	case 32: // EPIPE
		return "Broken Pipe";
	case 35: // SOCWOULDBLOCK
		return "Operation would block";
	case 36: //SOCINPROGRESS
		return "Operation now in progress";
	case 37: // SOCALREADY
		return "Operation already in progress";
	case 38: // SOCNOTSOCK
		return "Socket operation on non-socket";
	case 39: // SOCDESTADDRREQ
		return "Destination address required";
	case 40: // SOCMSGSIZE
		return "Message too long";
	case 42: // SOCNOPROTOOPT
		return "Protocol not available";
	case 43: // SOCPROTONOSUPPORT
		return "Protocol not supported";
	case 44: // SOCSOCKTNOSUPPORT
		return "Socket type not supported";
	case 45: // SOCOPNOTSUP
		return "Operation not supported on socket";
	case 47: // SOCAFNOSUPPORT
		return "Address family not support by protocol family";
	case 48: // SOCADDRINUSE
		return "Address already in use";
	case 49: // SOCADDRNOTAVAIL
		return "Can't assign requested address";
		/*
		#define EDEADLK 35
		#define ENAMETOOLONG 36
		#define ENOLCK 37
		#define ENOSYS 38
		#define ENOTEMPTY 39
		#define ELOOP 40
		#define EWOULDBLOCK EAGAIN
		#define ENOMSG 42
		#define EIDRM 43
		#define ECHRNG 44
		#define EL2NSYNC 45
		#define EL3HLT 46
		#define EL3RST 47
		#define ELNRNG 48
		#define EUNATCH 49
		#define ENOCSI 50
		#define EL2HLT 51
		#define EBADE 52
		#define EBADR 53
		#define EXFULL 54
		#define ENOANO 55
		#define EBADRQC 56
		#define EBADSLT 57
		#define EDEADLOCK EDEADLK
		#define EBFONT 59
		#define ENOSTR 60
		#define ENODATA 61
		#define ETIME 62
		#define ENOSR 63
		#define ENONET 64
		#define ENOPKG 65
		#define EREMOTE 66
		#define ENOLINK 67
		#define EADV 68
		#define ESRMNT 69
		#define ECOMM 70
		#define EPROTO 71
		#define EMULTIHOP 72
		#define EDOTDOT 73
		#define EBADMSG 74
		#define EOVERFLOW 75
		#define ENOTUNIQ 76
		#define EBADFD 77
		#define EREMCHG 78
		#define ELIBACC 79
		#define ELIBBAD 80
		#define ELIBSCN 81
		#define ELIBMAX 82
		#define ELIBEXEC 83
		#define EILSEQ 84
		#define ERESTART 85
		#define ESTRPIPE 86
		#define EUSERS 87
		#define ENOTSOCK 88
		#define EDESTADDRREQ 89
		#define EMSGSIZE 90
		#define EPROTOTYPE 91
		#define ENOPROTOOPT 92
		#define EPROTONOSUPPORT 93
		*/

	case 55: // SOCNOBUFS
		return "No buffer space available";
	case 56: //"SOCISCONN"
		return "Socket is already connected";
	case 57: // SOCNOTCONN
		return "Socket is not connected";
	case 60: // SOCTIMEDOUT
		return "Connection timed out";
	case 61: // SOCCONNREFUSED
		return "Connection refused";
	case 88:
		return "Operation on non socket";
    case ENOPROTOOPT: // 92
         return "ENOPROTOOPT"; // The option_name parameter is unrecognized, or the level parameter is not SOL_SOCKET.
	case ESOCKTNOSUPPORT:
		return "ESOCKTNOSUPPORT";
	case EOPNOTSUPP:
		return "EOPNOTSUPP";
	case EPFNOSUPPORT:
		return "EPFNOSUPPORT";
	case EAFNOSUPPORT:
		return "EAFNOSUPPORT";
	case EADDRINUSE:
		return "EADDRINUSE";
	case EADDRNOTAVAIL:
		return "EADDRNOTAVAIL";
	case ENETDOWN:
		return "ENETDOWN";
	case ENETUNREACH:
		return "ENETUNREACH";
	case ECONNABORTED:
		return "ECONNABORTED";
	case ECONNRESET: // 104  
		return "ECONNRESET";
	case ENOBUFS: //  105  
		return "ENOBUFS";
	case EISCONN:
		return "EISCONN";
	case ENOTCONN:
		return "ENOTCONN";
	case ESHUTDOWN:
		return "ESHUTDOWN";
	case ETOOMANYREFS:
		return "ETOOMANYREFS";
	case ETIMEDOUT: //110
		return "ETIMEDOUT";
	case ECONNREFUSED: //111
		return "ECONNREFUSED";
	case EHOSTDOWN: //112
		return "EHOSTDOWN";
	case EHOSTUNREACH: //113
		return "EHOSTUNREACH";
	case EALREADY: //114
		return "EALREADY";
	case EINPROGRESS: //115
		return "EINPROGRESS";
	case ESTALE: //116
		return "ESTALE";
	case EUCLEAN: //117
		return "EUCLEAN";
	case ENOTNAM: //118
		return "ENOTNAM";
	case ENAVAIL: //119
		return "ENAVAIL";
	case EISNAM: //129
		return "EISNAM";
	case EREMOTEIO: //121
		return "EREMOTEIO";
	case EDQUOT: //122
		return "EDQUOT";

	case 252: //ESOCINACT
		return "Socket creation is disabled in the system";
	case 254: // ESYSTEMERROR
		return "Socket system error";
	case EIM_ALIVE_TIMEDOUT:
		return "Connection I Am Alive Request Timed Out";

	default:
		sprintf( as8Buf, "Unknown Linux Socket Error %d", iErr );
		return as8Buf;
	}
#endif // TARGET_OS_WINDOWS
}

bool VxIsFatalSktError( int iErr )
{
	if( !iErr )
	{
		return false;
	}

#ifdef TARGET_OS_WINDOWS
	switch( iErr )
	{
	case WSAEINPROGRESS:	return false;
	case WSAEWOULDBLOCK:	return false;
	default:
		break;
	}
#endif // TARGET_OS_WINDOWS

	switch( iErr )
	{
    case EINTR:		// EINTR          4  ( an interrupted system call )
		return false;
	case EAGAIN:	// EAGAIN         11  (busy)   
		return false;
    case EINPROGRESS: // EINPROGRESS  115 ( Operation now in progress )   (Used by android)
        return false;

	default:
		break;
	}

	// NOTE: if WSAECONNRESET (10054) or ECONNRESET (104) then was forcable closed.. probably because we are in the hacker list

	return true;
}

NLC_END_CDECLARES


//============================================================================
socklen_t VxSktAddrInit( bool ipv6, struct sockaddr_storage& sockAddrStorage, uint16_t sktPort )
{
	memset( &sockAddrStorage, 0, sizeof( sockAddrStorage ) );
	if( ipv6 )
	{
		if( sktPort )
		{
			// port specified set family and port
			struct sockaddr_in6* addrIn = (struct sockaddr_in6*) & sockAddrStorage;
			addrIn->sin6_family = AF_INET6;
			addrIn->sin6_port = htons(sktPort);
		}

		return sizeof( struct sockaddr_in6 );
	}
	else
	{
		if( sktPort )
		{
			// port specified set family and port
			struct sockaddr_in* addrIn = (struct sockaddr_in*) & sockAddrStorage;
			addrIn->sin_family = AF_INET;
			addrIn->sin_port = htons(sktPort);
		}

		return sizeof( struct sockaddr_in );
	}
}

//============================================================================
socklen_t VxSktAddrInit( bool ipv6, struct sockaddr_storage& sockAddrStorage, std::string ipAddr, uint16_t sktPort )
{
	if( sktPort < 9 )
	{
		LogMsg( LOG_ERROR, "%s Invalid Port %d", __func__, sktPort );
	}

	memset( &sockAddrStorage, 0, sizeof( sockAddrStorage ) );
	if( ipv6 )
	{
		struct sockaddr_in6* addrIn = (struct sockaddr_in6*) & sockAddrStorage;
		addrIn->sin6_family = AF_INET6;
		addrIn->sin6_port = htons(sktPort);

		if( !ipAddr.empty() )
		{
			inet_pton( AF_INET6, ipAddr.c_str(), &addrIn->sin6_addr );
		}
		else
		{
			addrIn->sin6_addr = in6addr_any;
		}

		return sizeof( struct sockaddr_in6 );
	}
	else
	{
		struct sockaddr_in* addrIn = (struct sockaddr_in*) & sockAddrStorage;
		addrIn->sin_family = AF_INET;
		addrIn->sin_port = htons(sktPort);
		if( !ipAddr.empty() )
		{
			addrIn->sin_addr.s_addr = inet_addr( ipAddr.c_str() );
		}
		else
		{
			addrIn->sin_addr.s_addr = INADDR_ANY;
		}

		return sizeof( struct sockaddr_in );
	}
}


//============================================================================
bool VxSktAddrGetParams( bool ipv6, struct sockaddr_storage& sockAddrStorage, std::string& retIp, uint16_t& retPort )
{
	retIp.clear();
	retPort = 0;
	std::array<char, INET6_MAX_STR_LEN> ipAddr{};
	if( ipv6 )
	{
		struct sockaddr_in6* sktInAddr = (struct sockaddr_in6*) &sockAddrStorage;
		if( sktInAddr->sin6_family != AF_INET6 )
		{
			LogMsg( LOG_ERROR, "VxSktAddrGetParams: Expected  AF_INET6 %d Got %d", AF_INET6, sktInAddr->sin6_family );
			return false;
		}

		retIp = InetAddress::ipv6BinaryToString( (uint8_t*) &sktInAddr->sin6_addr );
		retPort = ntohs( sktInAddr->sin6_port );
	}
	else
	{
		struct sockaddr_in* sktInAddr = (struct sockaddr_in*) &sockAddrStorage;
		if( sktInAddr->sin_family != AF_INET )
		{
			LogMsg( LOG_ERROR, "VxSktAddrGetParams: Expected  AF_INET %d Got %d", AF_INET6, sktInAddr->sin_family );
			return false;
		}

		retIp = InetAddress::ipv4BinaryToString( (uint8_t*) &sktInAddr->sin_addr );
		retPort = ntohs( sktInAddr->sin_port );
	}
	
	return true;
}

//============================================================================
EIpAddrType VxGetIpAddrType( const char* ipAddr )
{
    EIpAddrType addrType{ eIpAddrTypeUnknown };
    const char* pszTextLocal = ipAddr;
    uint16_t pnPort;
	bool pbIsIPv6;
    unsigned char abyAddr[16];
    if( ParseIPv4OrIPv6( pszTextLocal, abyAddr, pnPort, pbIsIPv6 ) )
    {
        addrType = pbIsIPv6 ? eIpAddrTypeIpv6 : eIpAddrTypeIpv4;
    }

    return addrType;
}

//============================================================================
bool VxGetDefaultLocalIp( bool ipv6, std::string& retLocalIp )
{
#if defined(TARGET_OS_WINDOWS)
	static bool WsaInited = false;
	if( !WsaInited )
	{
		WSADATA wsaData;

		if (WSAStartup(MAKEWORD(1,1), &wsaData))
			return 0;
		WsaInited = true;
	}
#endif // defined(TARGET_OS_WINDOWS)

	retLocalIp.clear();
	// use udp so there is no actual internet connection traffic
	SOCKET skt = socket( ipv6 ? PF_INET6 : PF_INET, SOCK_DGRAM, 0 );
	if( INVALID_SOCKET == skt )
	{
		int32_t rc = VxGetLastError();
		LogMsg( LOG_ERROR, "%s could not create udp socket %s error %s", __func__, ipv6 ? "ipv6" :  "ipv4", VxDescribeSktError( rc ) );
		return false;
	}

	struct sockaddr_storage sktStorage;
	memset( &sktStorage, 0, sizeof( sockaddr_storage ) );
	// google.com address although does not matter because no internet traffic will result
	// "2001:4860:4860::8888" // ping6 hangs on rasberry pi
	// "2a00:1450:4016:80a::200e"
	std::string testIpAddr = ipv6 ? "2a00:1450:4016:80a::200e" : "142.250.191.206";
	int addrLen = VxSktAddrInit( ipv6, sktStorage, testIpAddr, 9 ); // 9 is debugging port
	sockaddr* sktAddr = reinterpret_cast<sockaddr*>( &sktStorage );
	if( !sktAddr )
	{
		LogMsg( LOG_ERROR, "%s cast to sockaddr failed %s", __func__, ipv6 ? "ipv6" :  "ipv4" );
		VxCloseSkt( skt );
		return false;
	}

	if (connect(skt, sktAddr, addrLen) == -1) 
	{
		LogMsg( LOG_ERROR, "%s could not connect %s error %s", __func__, ipv6 ? "ipv6" :  "ipv4", VxDescribeSktError( VxGetLastError() ) );
		VxCloseSkt( skt );
		return false;
    }

	InetAddrAndPort inetAddr;
	int32_t rc = VxGetLclAddress( skt, inetAddr );
	VxCloseSkt( skt );
	if( rc ) 
	{
		LogMsg( LOG_ERROR, "%s get local ip %s error %s", __func__, ipv6 ? "ipv6" :  "ipv4", VxDescribeSktError( rc ) );	
		return false;
    }

	retLocalIp = inetAddr.toString( false );
	return !retLocalIp.empty();
}
