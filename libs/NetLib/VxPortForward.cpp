//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxPortForward.h"

#include <libminiupnpc/upnpc.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxGlobals.h>

#include <array>

namespace // anonymouse
{
	const int MAX_UPNP_CMD_ARGS = 10;

	class UpnpCmdLine
	{
	public:
		UpnpCmdLine()
		{
		}

		void setupUpnpCmdLine( bool ipv6 )
		{
			std::string strExePathAndFileName;
			if( 0 == VxFileUtil::getExecuteFullPathAndName( strExePathAndFileName ) && !strExePathAndFileName.empty() )
			{
				LogModule( eLogPortForward, LOG_VERBOSE, "UpnpCmdLine app exe: %s", strExePathAndFileName.c_str() );
				addArg( strExePathAndFileName.c_str() );
			}
			else
			{
				LogModule( eLogPortForward, LOG_ERROR, "UpnpCmdLine failed to retrieve app exe" );
				addArg( "FakeExe" );
			}

			if( ipv6 )
			{
				addArg( "-6" );
			}
		}

		void addArg( const std::string argStr )
		{
			cmdParams.emplace_back( argStr );
			argc++;
		}

		bool runCmd( void )
		{
			char** vargs = new char* [argc];
			for( int i = 0; i < argc; i++ )
			{
				vargs[i] =  (char *)cmdParams[i].c_str();
			}

			int rc = runUpnp( argc, vargs );

            delete[] vargs;

			return 0 == rc;
		}


		int argc{ 0 };

		std::vector<std::string> cmdParams;
	};

} // namespace

bool VxPortForward::m_ForwardEnable = false;
bool VxPortForward::m_IsIpv6 = false;
bool VxPortForward::m_UseIpv6 = false;
std::string VxPortForward::m_IpAddr("");
uint16_t VxPortForward::m_Port = 0;
std::mutex VxPortForward::runCmdMutex;
std::thread VxPortForward::runUpnpThread;
bool VxPortForward::m_IsShutdown = false;

namespace
{
	// Ensure background UPnP worker is joined before static thread destruction.
	struct PortForwardShutdownFinalizer
	{
		~PortForwardShutdownFinalizer()
		{
			VxPortForward::shutdownPortForward();
		}
	} g_PortForwardShutdownFinalizer;
}

//============================================================================
void VxPortForward::shutdownPortForward( void )
{
	std::unique_lock<std::mutex> lock( runCmdMutex );
	if( !m_IsShutdown )
	{
		m_IsShutdown = true;
		if( runUpnpThread.joinable() )
		{
			std::thread localThread = std::move( runUpnpThread );
			lock.unlock();
			localThread.join();
		}
	}
}

//============================================================================
void VxPortForward::waitForThreadToFinish( void )
{
    std::unique_lock<std::mutex> lock( runCmdMutex );
	if( runUpnpThread.joinable() )
	{
		std::thread localThread = std::move( runUpnpThread );
		lock.unlock();
		localThread.join();
	}
}

//============================================================================
void VxPortForward::killThread( void )
{
    waitForThreadToFinish();
}

//============================================================================
void VxPortForward::setEnablePortForward( bool enable )
{
	LogModule( eLogPortForward, LOG_VERBOSE, "VxPortForward::setEnablePortForward %d ", enable );
	m_ForwardEnable = enable;
}

//============================================================================
bool VxPortForward::getEnablePortForward( void )
{
	return m_ForwardEnable;
}

//============================================================================
void VxPortForward::setUseIpv6( bool enable )
{
	LogModule( eLogPortForward, LOG_VERBOSE, "VxPortForward::setUseIpv6 %d ", enable );
	m_UseIpv6 = enable;
}

//============================================================================
bool VxPortForward::getUseIpv6( void )
{
	return m_UseIpv6;
}

//============================================================================
bool VxPortForward::addPortForward( bool ipv6, std::string ipAddr, uint16_t port, bool runInThread )
{
	if( m_IsShutdown || VxIsAppShuttingDown() )
	{
		return false;
	}

	if( !m_ForwardEnable )
	{
		LogModule( eLogPortForward, LOG_DEBUG, "VxPortForward::addPortForward not enabled " );
		return false;
	}

	if( port < 80 )
	{
		LogModule( eLogPortForward, LOG_DEBUG, "VxPortForward::addPortForward invalid port %d", (int)port );
		return false;
	}

    std::unique_lock<std::mutex> lock( runCmdMutex );
	if( runUpnpThread.joinable() )
	{
		std::thread localThread = std::move( runUpnpThread );
		lock.unlock();
		localThread.join();
		lock.lock();
	}

	m_IsIpv6 = ipv6;
	m_IpAddr = ipAddr;
	m_Port = port;
	if( runInThread )
	{
		runUpnpThread = std::thread( &doAddPortForward );
		return true;
	}

    lock.unlock();
	return doAddPortForward();
}

//============================================================================
bool VxPortForward::doAddPortForward( void )
{
	if( m_IsShutdown || VxIsAppShuttingDown() )
	{
		return false;
	}

	LogModule( eLogPortForward, LOG_DEBUG, "VxPortForward::%s removing old port mapping for port %d ", __func__, m_Port );
	doRemovePortForward();

	LogModule( eLogPortForward, LOG_DEBUG, "VxPortForward::%s adding new port mapping for port %d ", __func__, m_Port );
	UpnpCmdLine upnpCmdLine;
	upnpCmdLine.setupUpnpCmdLine( m_IsIpv6 );
	
	upnpCmdLine.addArg( "-a" );

	upnpCmdLine.addArg( m_IpAddr.c_str() );
	
	std::string port = std::to_string( m_Port );
	upnpCmdLine.addArg( port.c_str() );
	upnpCmdLine.addArg( port.c_str() );

	upnpCmdLine.addArg( "tcp" );

	return upnpCmdLine.runCmd();
}

//============================================================================
bool VxPortForward::removePortForward( bool ipv6, uint16_t port, bool runInThread )
{
	if( !m_ForwardEnable )
	{
		LogModule( eLogPortForward, LOG_DEBUG, "VxPortForward::removePortForward not enabled " );
		return false;
	}

    std::unique_lock<std::mutex> lock( runCmdMutex );
	if( runUpnpThread.joinable() )
	{
		std::thread localThread = std::move( runUpnpThread );
		lock.unlock();
		localThread.join();
		lock.lock();
	}
	m_IsIpv6 = ipv6;
	m_Port = port;
	if( runInThread )
	{
		runUpnpThread = std::thread( &doRemovePortForward );
		return true;
	}

    lock.unlock();
	return doRemovePortForward();
}

//============================================================================
bool VxPortForward::doRemovePortForward( void )
{
	UpnpCmdLine upnpCmdLine;
	upnpCmdLine.setupUpnpCmdLine( m_IsIpv6 );

	upnpCmdLine.addArg( "-d" );

	std::string portStr = std::to_string( m_Port );
	upnpCmdLine.addArg( portStr.c_str() );

	upnpCmdLine.addArg( "tcp" );

	return upnpCmdLine.runCmd();

}

//============================================================================
bool VxPortForward::listPortForward( std::string ipAddr, bool ipv6, bool runInThread )
{
	if( !m_ForwardEnable )
	{
		LogModule( eLogPortForward, LOG_DEBUG, "VxPortForward::listPortForward not enabled " );
	}

    std::unique_lock<std::mutex> lock( runCmdMutex );
	if( runUpnpThread.joinable() )
	{
		std::thread localThread = std::move( runUpnpThread );
		lock.unlock();
		localThread.join();
		lock.lock();
	}
	m_IsIpv6 = ipv6;
	m_IpAddr = ipAddr;
	if( runInThread )
	{
		runUpnpThread = std::thread( &doListPortForward );
		return true;
	}

    lock.unlock();
	return doListPortForward();
}

//============================================================================
bool VxPortForward::doListPortForward( void )
{
	UpnpCmdLine upnpCmdLine;
	upnpCmdLine.setupUpnpCmdLine( m_IsIpv6 );

	upnpCmdLine.addArg( "-l" );
	upnpCmdLine.addArg( m_IpAddr.c_str() );
	upnpCmdLine.addArg( "TCP");

	return upnpCmdLine.runCmd();
}


