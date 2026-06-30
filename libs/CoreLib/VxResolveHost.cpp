//============================================================================
// Copyright (C) 2003 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#include "VxResolveHost.h"
#include "VxSktUtil.h"

#include <CoreLib/VxParse.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxElapseTimer.h>

#include <stdio.h>
#include <memory.h>

#ifdef TARGET_OS_WINDOWS
#include "Winsock2.h"
#include <Ws2tcpip.h>  
#else
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

//============================================================================
bool VxResolveHostToIp(	const char*		pUrl,			// web name to resolve
						std::string &	strRetIp,		// return dotted ip string 
						uint16_t&		u16RetPort, 	// return port
						EIpAddrType		addrType )
{
	std::string strHost;
	std::string strFileName;

    strRetIp.clear();
    u16RetPort = 0;

	if( VxSplitHostAndFile( pUrl,
							strHost,
							strFileName,
							u16RetPort ) )
	{
		if( (':' == strHost.c_str()[0]) ||
			( isdigit( strHost.c_str()[0]) ) )
		{
			// already a ip address
			strRetIp = strHost;
			return true;
		}

		InetAddrAndPort oAddr;
		if( VxResolveHostToIp(	strHost.c_str(),			//web host name to resolve
			u16RetPort,
			oAddr,
			addrType ) )
		{
			strRetIp = oAddr.toString();
			return true;
		}
	}
	return false;
}

//============================================================================
bool VxResolveHostToIp(	const char*		pHostOnly,			// web host name to resolve
						uint16_t		u16Port,
						InetAddress&	retIpAddr,
						EIpAddrType		addrType )
{
	if( VxIsAppShuttingDown() )
	{
		return false;
	}

    if( ( 0 == pHostOnly ) || ( 0 == u16Port ) )
    {
        LogMsg( LOG_DEBUG, "%s bad parameter", __func__ );
        return false;
    }

	//resolve host into ip address
	struct addrinfo *res, *aip;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_ADDRCONFIG;

	switch( addrType )
	{
	case eIpAddrTypeIpv6:
		hints.ai_family = AF_INET6;
		break;
	case eIpAddrTypeIpv4:
		hints.ai_family = AF_INET;
		break;
	default:
		hints.ai_family = AF_UNSPEC;
	}

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char as8Port[ 16 ];
	sprintf( as8Port, "%d", u16Port );

	int error = getaddrinfo(pHostOnly, as8Port, &hints, &res);
	if (error != 0) 
	{
        LogMsg( LOG_ERROR, "%s: error %s for host %s port %s",
                    __func__, gai_strerror(error), pHostOnly, as8Port );
		return false;
	}

	bool bFoundAddr = false;;

	// Try all returned addresses until one works 
	for( aip = res; aip != NULL; aip = aip->ai_next ) 
	{
		if( VxIsAppShuttingDown() )
		{
			break;
		}

		InetAddrAndPort rmtIp;	

		rmtIp.setIp( *((struct sockaddr_storage *)aip->ai_addr) );
		std::string strRmtIp = rmtIp.toString( false );

		if( rmtIp.isValid() 
			&& ( false == rmtIp.isLoopBack() ) )
		{
			bFoundAddr = true;
			retIpAddr.setIp( strRmtIp.c_str() );
			break;
		}
		else
		{
			continue;
		}
	}

	freeaddrinfo(res);
	return bFoundAddr;
}

//============================================================================
bool VxResolveHostToIps(	const char* pHostOnly,			// web host name to resolve
							uint16_t u16Port,
							std::vector<InetAddress>& retAddrList,
							EIpAddrType	addrType )
{
	if( VxIsAppShuttingDown() )
	{
		return false;
	}

	//resolve host into ip address
	struct addrinfo *res, *aip;
	struct addrinfo hints;
	SOCKET sock = -1;
	int error;

	retAddrList.clear();

	/* Get host address. Any type of address will do. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;

	char as8Port[ 10 ];
	sprintf( as8Port, "%d", u16Port );

	//LogMsg( LOG_INFO, "VxResolveHostToIps %s\n", pHostOnly ); 
	error = getaddrinfo( pHostOnly, as8Port, &hints, &res );
	if (error != 0) 
	{
        #ifdef DEBUG_NETLIB
		LogMsg( LOG_ERROR,
			"getaddrinfo: %s for host %s service %s",
			gai_strerror(error), pHostOnly, as8Port);
        #endif // DEBUG_NETLIB
		return false;
	}
	/* Try all returned addresses until one works */
	for (aip = res; aip != NULL; aip = aip->ai_next) 
	{
		if( addrType == eIpAddrTypeIpv4 && aip->ai_family != AF_INET )
		{
			LogMsg( LOG_WARN, "%s excluding non ipv4", __func__ );
			continue;
		}
		else if( addrType == eIpAddrTypeIpv6 && aip->ai_family != AF_INET6 )
		{
			LogMsg( LOG_WARN, "%s excluding non ipv6", __func__ );
			continue;
		}

		/*
		* Open socket. The address type depends on what
		* getaddrinfo() gave us.
		*/
		sock = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
		if (sock == -1) 
		{
            #ifdef DEBUG_NETLIB
                LogMsg( LOG_ERROR, "VxResolveHostToIps: could not create socket" );
            #endif // DEBUG_NETLIB
			continue;
		}

		/* Connect to the host. */
		if( -1 == connect(sock, aip->ai_addr, (int)aip->ai_addrlen) ) 
		{
			VxCloseSkt( sock );
			LogMsg( LOG_WARN, "%s failed to connect", __func__ );
			continue;
		}

		// found a ip
		InetAddress oAddr;
		oAddr.setIp( *((struct sockaddr_storage *)aip->ai_addr) );
		retAddrList.push_back( oAddr );
	}

	freeaddrinfo(res);
	return !retAddrList.empty();
}
