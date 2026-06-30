#pragma once
//============================================================================
// Copyright (C) 2014 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "NetServiceDefs.h"
#include "NetServiceUtils.h"

#include "NetActionIdle.h"

#include <NetLib/VxSktConnectSimple.h>
#include <CoreLib/VxThread.h>
#include <CoreLib/VxMutex.h>
#include <CoreLib/VxSemaphore.h>
#include <CoreLib/AppErr.h>
#include <CoreLib/VxElapseTimer.h>

#ifdef TARGET_OS_WINDOWS
# include <Network/Firewall.h>
#endif // TARGET_OS_WINDOWS

class EngineSettings;
class HostList;
class NetworkMgr;
class P2PEngine;
class PktAnnounce;
class VxGUID;
class VxSktBase;
class VxPktHdr;
class PktTestConnTestReq;
class PktTestConnTestReply;
class PktTestConnPingReq;
class PktTestConnPingReply;
class PktQueryHostUrlReq;
class PktQueryHostUrlReply;

typedef void ( *MY_PORT_OPEN_CALLBACK_FUNCTION )( void *, ENetCmdError, std::string& );
typedef void( *QUERY_HOST_ID_CALLBACK_FUNCTION )( void *, ENetCmdError, VxGUID& );

class NetServicesMgr
{
public:
	NetServicesMgr( P2PEngine& engine );
	virtual ~NetServicesMgr();
	NetServicesMgr() = delete; // don't allow default constructor
	NetServicesMgr( const NetServicesMgr& ) = delete; // don't allow copy constructor

	static const int			MINIMUM_CONTENT_PARTS		= 9;
	static const int			CONTENT_PART_IDX_VERSION	= 5;
	static const int			CONTENT_PART_IDX_ERROR		= 6;
	static const int			CONTENT_PART_IDX_PAYLOAD	= 7;

	P2PEngine&					getEngine( void )			{ return m_Engine; }
    EngineSettings&				getEngineSettings( void )	{ return m_EngineSettings; }
	NetServiceUtils&			getNetUtils( void )			{ return m_NetServiceUtils; }

	VxGUID&						getMyOnlineId( void );

    std::string                 getNetworkKey( void );
	uint16_t					getRxNetServicePort( void );
    std::string					getRxNetIpAddress( bool isIpv6Connection );

	void                        setIsTestConnectionActive( bool isActive );
	bool                        getIsTestConnectionActive( void );
	void                        setIsQueryUrlActive( bool isActive )			{ m_QueryUrlActive = isActive; }
    bool                        getIsQueryUrlActive( void )						{ return m_QueryUrlActive; }

	bool                        shouldHandleNetServicePacket( void );

	bool						getNetPktRxCryptoPassword( std::string& retPwd, std::shared_ptr<VxSktBase>& sktBase );

	bool						handlePktNetService( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr, bool& permissionError );

	bool						handlePktConnTestReq( std::shared_ptr<VxSktBase>& sktBase, PktTestConnTestReq* pkt, bool& permissionError);
	bool						handlePktConnTestReply( std::shared_ptr<VxSktBase>& sktBase, PktTestConnTestReply* pkt, bool& permissionError );
	bool						handlePktConnPingReq( std::shared_ptr<VxSktBase>& sktBase, PktTestConnPingReq* pkt, bool& permissionError );
	bool						handlePktConnPingReply( std::shared_ptr<VxSktBase>& sktBase, PktTestConnPingReply* pkt, bool& permissionError );
	bool						handlePktQueryHostUrlReq( std::shared_ptr<VxSktBase>& sktBase, PktQueryHostUrlReq* pkt, bool& permissionError );
	bool						handlePktQueryHostUrlReply( std::shared_ptr<VxSktBase>& sktBase, PktQueryHostUrlReply* pkt, bool& permissionError );

	void						netServicesStartup( void );
	void						netServicesShutdown( void );

	void						addNetActionToQueue( enum ENetActionType netAction );

	void						netActionResultIsMyPortOpen( ENetCmdError eCmdErr, std::string& myExternalIp );
    void                        netActionResultQueryHostId( ENetCmdError eCmdErr, VxGUID& hostId );

	void						setMyPortOpenResultCallback( MY_PORT_OPEN_CALLBACK_FUNCTION pfuncPortOpenCallbackHandler, void * userData );
    void						setQueryHostOnlineIdResultCallback( QUERY_HOST_ID_CALLBACK_FUNCTION pfuncQueryHostIdCallbackHandler, void * userData );

	int32_t						handleNetCmdPing( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr );
	int32_t						handleNetCmdPong( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr );
	int32_t						handleNetCmdIsMyPortOpenReq( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr );
	int32_t						handleNetCmdIsMyPortOpenReqContent( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr, std::string& fromClientNetCmdContent );
	int32_t						handleNetCmdIsMyPortOpenReply( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr );
    int32_t						handleNetCmdQueryHostIdReq( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr );
    int32_t						handleNetCmdQueryHostIdReply( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr );

	void						runNetActions( void );

	VxSktConnectSimple *		actionReqConnectToNetService( void );
	bool						actionReqConnectToNetService( VxSktConnectSimple& sktSimple );
	VxSktConnectSimple *		actionReqConnectToHost( void );
	bool						actionReqConnectToHost( VxSktConnectSimple& sktSimple );

	bool						doNetCmdPing( const char* ipAddr, uint16_t u16Port, std::string& retPong, bool isClientPing );

	ENetCmdError				doIsMyPortOpen( std::string& retMyExternalIp, bool isUserTest = false );
	bool						testLoobackPing( std::string localIP, uint16_t tcpListenPort );
	ENetCmdError				sendAndRecieveIsMyPortOpen( VxElapseTimer&				portTestTimer,
															VxSktConnectSimple *	sktSimple, 
															int						tcpListenPort,
															std::string&			retMyExternalIp,
															bool					sendMsgToUser,
															int						sendRecieveTimeout = IS_PORT_OPEN_RX_DATA_TIMEOUT );

	ENetCmdError                sendAndRecieveQueryHostId( VxElapseTimer&				portTestTimer,
                                                           VxSktConnectSimple *	netServConn,
                                                           VxGUID&			    retHostId,
                                                           bool					sendMsgToUser );

	bool						sendAndRecievePing( VxElapseTimer& pingTimer, VxSktConnectSimple& toClientConn, std::string& retPong, bool isClientPing, int receiveTimeout = 6000 );

	bool						fetchExternalIpAddress( VxSktConnectSimple* sktSimple, std::string& retExternIpAddr, int receiveTimeout = 6000 );

	bool                        sendNetServicePacket(   ENetCmdType         netCmdType, ///< which type of net service to send
                                                        std::shared_ptr<VxSktBase>& sktBase, 
                                                        std::string&        netCmd,
                                                        int                 txDataTimeout );

	ENetCmdError				doRenewPortForward( void );

protected:
	bool						shouldAbort( void );

	void						addNetActionCommand( NetActionBase * netActionBase );
	bool						isActionQued( ENetActionType eNetActionType );
	int32_t						sendPong( std::shared_ptr<VxSktBase>& sktBase, NetServiceHdr& netServiceHdr, bool isClientPing );

	bool						buildAndSendPktTestConnPingReply( std::shared_ptr<VxSktBase>& sktBase, bool isClientPing );
	bool						buildAndSendPktQueryUrlReply( std::shared_ptr<VxSktBase>& sktBase );

	void						sendUserLog( const char* msg, ... );

	//=== vars ===//
#ifdef TARGET_OS_WINDOWS
	Firewall					m_WindowsFirewall;
#endif // TARGET_OS_WINDOWS

	P2PEngine&					m_Engine;
	PktAnnounce&				m_PktAnn;
	EngineSettings&				m_EngineSettings;
	NetworkMgr&					m_NetworkMgr;
	NetServiceUtils				m_NetServiceUtils;

	std::vector<NetActionBase*>	m_NetActionQueue;
	NetActionIdle				m_NetActionIdle;
	NetActionBase *				m_CurNetAction{nullptr};

	VxThread					m_NetActionThread;
	VxMutex						m_NetActionMutex;
	VxSemaphore					m_NetActionSemaphore;

	VxSktConnectSimple			m_SktToNetServices;
    MY_PORT_OPEN_CALLBACK_FUNCTION	m_pfuncPortOpenCallbackHandler{nullptr};
    QUERY_HOST_ID_CALLBACK_FUNCTION	m_pfuncQueryHostIdCallbackHandler{ nullptr };

    void *						m_PortOpenCallbackUserData{nullptr};
    void *						m_QueryHostIdCallbackUserData{ nullptr };

	VxSktConnectSimple			m_SktToHost;

    bool                        m_TestConnectionActive{ false };
	bool						m_QueryUrlActive{ false };

	VxMutex						m_IsPortOpenMutex;
};



