//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VirtStreamMgr.h"

#include <P2PEngine/P2PEngine.h>
#include <Plugins/PluginMgr.h>

#include <CoreLib/VFile.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>
#include <NetLib/VxSktBase.h>
#include <PktLib/PktsStreamCtrl.h>

//============================================================================
VirtFileMgr& GetVirtFileMgr()
{
	static VirtStreamMgr virtStreamMgr(GetPtoPEngine());
	return virtStreamMgr;
}

//============================================================================
VirtStreamMgr& GetVirtStreamMgr( void )
{
	return (VirtStreamMgr&)GetVirtFileMgr();
}

//============================================================================
VirtStreamMgr::VirtStreamMgr( P2PEngine& engine )
	: m_Engine( engine )
	, m_Plugin( *((PluginFileShareClient*)engine.getPluginMgr().getPlugin( ePluginTypeFileShareClient ) ) )
{
}

//============================================================================
bool VirtStreamMgr::fromGuiPlayStream( AssetBaseInfo& assetInfo, VxGUID lclSessionId, int pos0to100000 )
{
	// called just before the media player starts playing stream
	if( !lclSessionId.isValid() )
	{
		lclSessionId.initializeWithNewVxGUID();
	}

	lockStreamMgr();
	const bool sameActiveSession = getIsPlaying() &&
		(m_LiveStream.m_StreamSessionId == lclSessionId) &&
		(m_LiveStream.m_StreamAssetInfo.getAssetUniqueId() == assetInfo.getAssetUniqueId());
	if( sameActiveSession )
	{
		unlockStreamMgr();
		if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_VERBOSE, "%s ignoring duplicate start for session %s",
			__func__, lclSessionId.toHexString().c_str() );
		return true;
	}

	unlockStreamMgr();
	if( getIsPlaying() )
	{
		onStreamStop();
	}

	lockStreamMgr();
	m_LiveStream.m_StreamAssetInfo = assetInfo;
	m_LiveStream.m_StreamAssetInfo.setIsStream( true );
	m_LiveStream.m_StreamSessionId = lclSessionId;
	if( !m_LiveStream.setConnection( m_Engine.getConnectIdListMgr().findBestUserOnlineConnection( assetInfo.getDestUserId() ) ) )
	{
		LogModule( eLogStreams, LOG_ERROR, "%s failed to find online connection", __func__ );
		unlockStreamMgr();
		return false;
	}

	unlockStreamMgr();
	setIsPlaying( true );

	m_Plugin.wantFileXferCallback( this, true );
	if( !m_Plugin.startStream( m_LiveStream.getConnection(), m_LiveStream.m_StreamAssetInfo, m_LiveStream.m_StreamSessionId ) )
	{
		LogModule( eLogStreams, LOG_ERROR, "%s failed to start stream", __func__ );
		m_Plugin.wantFileXferCallback( this, false );
		return false;
	}

	return true;
}

//============================================================================
bool VirtStreamMgr::waitForStream( int64_t fileOffset, int64_t readLen )
{
	bool result{ false };
	if( readLen <= 0 )
	{
		return true;
	}
	//const double maxTimeoutMs = 20000;
	const double maxTimeoutMs = 60000;
	//const double maxTimeoutMs = 800000; // temp for debug
	VxElapseTimer timer;
	while( timer.elapsedMs() < maxTimeoutMs && !m_LiveStream.m_Error )
	{
		if( !getIsPlaying() )
		{
			LogMsg( LOG_WARN, "VirtStreamMgr::%s play steam stopped", __func__ );
			return false;
		}

		if( !m_LiveStream.m_VFile )
		{
			LogMsg( LOG_WARN, "VirtStreamMgr::%s file was closed", __func__ );
			return false;
		}

		lockStreamMgr();
		if( !m_LiveStream.isConnected() )
		{
			LogMsg( LOG_WARN, "VirtStreamMgr::%s user %s is no longer connected", __func__,
					m_Engine.describeUser( m_LiveStream.m_StreamAssetInfo.getDestUserId() ).c_str() );
			unlockStreamMgr();
			return false;
		}

		const int64_t streamLen = m_LiveStream.m_StreamCache.hasData( fileOffset, readLen );
		const int64_t tailLen = m_LiveStream.m_FileTail.hasData( fileOffset, readLen );
		if( readLen == streamLen || readLen == tailLen )
		{
			result = true;
			unlockStreamMgr();
			break;
		}

		// End-of-stream bytes can arrive split between stream cache and file tail.
		int64_t combinedLen = streamLen;
		if( combinedLen < readLen )
		{
			combinedLen += m_LiveStream.m_FileTail.hasData( fileOffset + combinedLen, readLen - combinedLen );
		}

		if( combinedLen < readLen )
		{
			combinedLen = tailLen;
			if( combinedLen < readLen )
			{
				combinedLen += m_LiveStream.m_StreamCache.hasData( fileOffset + combinedLen, readLen - combinedLen );
			}
		}

		if( combinedLen == readLen )
		{
			result = true;
			unlockStreamMgr();
			break;
		}

		unlockStreamMgr();
		VxSleep( 300 );
	}
	
	if( !result )
	{
		LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s timeout waiting for stream file %s at offs %" PRId64 " len %" PRId64, __func__,
				   m_LiveStream.m_StreamAssetInfo.getAssetNameAndPath().c_str(), fileOffset, readLen );
	}

	return result;
}

//============================================================================
void VirtStreamMgr::onPlaybackStopped( VxGUID& feedId )
{
	if( feedId == m_LiveStream.m_StreamSessionId )
	{
		onStreamStop();
	}
}

//============================================================================
void VirtStreamMgr::onPlaybackEnded( VxGUID& feedId )
{
	if( feedId == m_LiveStream.m_StreamSessionId )
	{
		onStreamStop();		
	}
}

//============================================================================
void VirtStreamMgr::onStreamStop( void )
{
	m_Plugin.wantFileXferCallback( this, false );
	VxGUID streamSessionId = m_LiveStream.m_StreamSessionId;
	m_Engine.onStreamStop( streamSessionId );

	setIsPlaying( false );
	lockStreamMgr();
	m_LiveStream.clear();
	unlockStreamMgr();
}

//============================================================================
bool VirtStreamMgr::sendStreamSeek( int64_t newPos )
{
	PktStreamCtrlReq pktReq;
	pktReq.setSrcOnlineId( m_Engine.getMyOnlineId() );
	pktReq.setDestOnlineId( m_LiveStream.m_StreamAssetInfo.getDestUserId() );
	pktReq.setPluginNum( ePluginTypeFileShareServer );

	lockStreamMgr();
	pktReq.setLclSessionId( m_LiveStream.m_StreamSessionId );
	pktReq.setRmtSessionId( m_LiveStream.m_ServerSessionId );
    pktReq.setAssetId( m_LiveStream.m_StreamAssetInfo.getAssetUniqueId() );
	unlockStreamMgr();

	pktReq.setStartOffset( newPos );
	pktReq.setStreamCtrl( eStreamSeek );
	if( m_LiveStream.getConnection().get() && m_LiveStream.getConnection()->isConnected() )
	{
		return m_LiveStream.getConnection()->txPacketWithDestId( &pktReq ) == 0;
	}

	return false;
}

//============================================================================
void VirtStreamMgr::onFileXferPktRxed( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	switch( pktHdr->getPktType() )
	{
	case  PKT_TYPE_FILE_CHUNK_REQ:
		onPktFileChunkReq( sktBase, pktHdr );
		break;

	case  PKT_TYPE_FILE_GET_REPLY:
		onPktFileGetReply( sktBase, pktHdr );
		break;

	case  PKT_TYPE_FILE_GET_COMPLETE_REQ:
		onPktFileGetCompleteReq( sktBase, pktHdr );
		break;

	case  PKT_TYPE_FILE_GET_COMPLETE_REPLY:
		onPktFileGetCompleteReply( sktBase, pktHdr );
		break;

	case  PKT_TYPE_FILE_SEND_COMPLETE_REQ:
		onPktFileSendCompleteReq( sktBase, pktHdr );
		break;

	case  PKT_TYPE_FILE_XFER_CANCEL:
		onPktFileXferCancel( sktBase, pktHdr );
		break;

	case  PKT_TYPE_FILE_SHARE_ERR:
		onPktFileShareErr( sktBase, pktHdr );
		break;

	case  PKT_TYPE_STREAM_CTRL_REPLY:	
		onPktStreamCtrlReply( sktBase, pktHdr );
		break;

	default:
		LogMsg( LOG_ERROR, "%s unhandled pkt %s", __func__, pktHdr->describePktHdr().c_str() );
	}
}

//============================================================================
void VirtStreamMgr::onPktFileGetReply( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	PktFileGetReply* pktReply = (PktFileGetReply*)pktHdr;
	VxGUID lclSessionId = pktReply->getLclSessionId();
	VxGUID rmtSessionId = pktReply->getRmtSessionId();
	if( rmtSessionId != m_LiveStream.m_StreamSessionId )
	{
		if(LogEnabled(eLogStreams)) LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s wrong session id %s", __func__, rmtSessionId.toHexString().c_str() );
		return;
	}

	if(LogEnabled(eLogStreams)) LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s stream session %s server session %s", __func__, 
										   rmtSessionId.toHexString().c_str(),  lclSessionId.toHexString().c_str() );
	if( lclSessionId.isValid() )
	{
		lockStreamMgr();
		m_LiveStream.m_ServerSessionId = lclSessionId;
		unlockStreamMgr();
	}
}

//============================================================================
void VirtStreamMgr::onPktFileChunkReq( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	if( !m_LiveStream.m_StreamSessionId.isValid() )
	{
		if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s null m_LiveStream.m_StreamSessionId", __func__ );
		return;
	}

	PktFileChunkReq* pktReq = (PktFileChunkReq*)pktHdr;
	VxGUID rmtSessionId = pktReq->getRmtSessionId();
	if( rmtSessionId != m_LiveStream.m_StreamSessionId )
	{
		if(LogEnabled(eLogStreams)) LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s wrong session id %s", __func__, rmtSessionId.toHexString().c_str() );
		return;
	}

	if(LogEnabled(eLogStreams)) LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s offs %" PRId64  "len %d", __func__, 
										   pktReq->getChunkOffset(), pktReq->getDataLen() );
	m_LiveStream.m_StreamCache.writeData( pktReq->getChunkOffset(), (char*)pktReq->getChunkBuffer(), pktReq->getDataLen() );
}

//============================================================================
void VirtStreamMgr::onPktFileGetCompleteReq( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s", __func__ );
	//PktFileGetCompleteReq* pktReply = (PktFileGetCompleteReq*)pktHdr;
}

//============================================================================
void VirtStreamMgr::onPktFileGetCompleteReply( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s", __func__ );
	//PktFileGetCompleteReply* pktReply = (PktFileGetCompleteReply*)pktHdr;
}

//============================================================================
void VirtStreamMgr::onPktFileSendCompleteReq( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s", __func__ );
}

//============================================================================
void VirtStreamMgr::onPktFileXferCancel( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s", __func__ );
	PktFileXferCancel* pktReq = (PktFileXferCancel*)pktHdr;
    VxGUID rmtSessionId = pktReq->getRmtSessionId();
	if( rmtSessionId != m_LiveStream.m_StreamSessionId )
	{
		return;
	}

	onStreamStop();
}

//============================================================================
void VirtStreamMgr::onPktFileShareErr( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	PktFileShareErr* pktError = (PktFileShareErr*)pktHdr;
	if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s error %d rx session id %s tx session id %s",
				   __func__, pktError->getError(), pktError->getRxInstance().toHexString().c_str(), pktError->getTxInstance().toHexString().c_str() );
}

//============================================================================
void VirtStreamMgr::onPktStreamCtrlReply( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr )
{
	if( LogEnabled( eLogStreams ) ) LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s", __func__ );
	PktStreamCtrlReply* pktReply = (PktStreamCtrlReply*)pktHdr;
	if( pktReply->getStreamCtrl() == eStreamReadTail )
	{
		lockStreamMgr();
		m_LiveStream.m_FileTail.writeData( pktReply->getStartOffset(), pktReply->getDataBuf(), pktReply->getEndOffset() - pktReply->getStartOffset() );
		unlockStreamMgr();
	}
}
