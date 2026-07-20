//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================
#pragma once

#include "VirtStreamFile.h"

#include <Plugins/FileXferCallback.h>
#include <Plugins/PluginFileShareClient.h>

#include <CoreLib/VirtFileMgr.h>

#include <vector>
#include <atomic>

class P2PEngine;
class VirtProviderFile;
class VirtStreamFile;

class VirtStreamMgr : public VirtFileMgr, public FileXferCallback
{
public:
	VirtStreamMgr() = delete;
	VirtStreamMgr( P2PEngine& engine );

	bool                        directoryExists( const char* dirPath ) override;
    uint64_t                    fileExists( const char* fileName ) override;
    bool                        fileIsProviderFile( const char* fileName ) override;

	bool						getFileInfo( const char* fileNameAndPath, VxFileInfoBase& retFileInfo ) override;

	bool						seperatePathAndFile( const char* pFullPath,					// path and file name			
													 std::string& strRetPath,				// return path of file
													 std::string& strRetFile ) override;	// return file name

	VFile*						fileOpen( const char* fileName, const char* fileMode )  override;
	int							fileClose( VFile* vFile )  override;
	int							fileEof( VFile* fp )  override;
	int							fileError( VFile* fp )  override;
	int							fileFlush( VFile* fp )  override;
	size_t						fileRead( void* buf, size_t size, size_t count, VFile* fp ) override;
	size_t						fileWrite( const void* buf, size_t size, size_t count, VFile* fp ) override;
	int							fileGetC( VFile* fp ) override;
	char*						fileGetS( char* buf, int size, VFile* fp ) override;
	int							filePutC( int ch, VFile* fp ) override;
	int							filePutS( const char* s, VFile* fp ) override;
	int							fileGetPos( VFile* fp, fpos_t* pos ) override;
	int							fileSetPos( VFile* fp, const fpos_t* pos ) override;
	int							fileSeek( VFile* fp, size_t offset, int whence ) override;
	int							fileSeek64( VFile* fp, uint64_t offs ) override;

	int							listProviderFilesAndFolders( const char* srcDir, std::vector<VxFileInfoBase>& fileList, uint8_t fileFilterMask ) override;

	virtual bool				fromGuiPlayStream( AssetBaseInfo& assetInfo, VxGUID lclSessionId, int pos0to100000 );

	void						setIsPlaying( bool isPlaying )		{ m_IsPlaying = isPlaying; }
	bool						getIsPlaying( void )				{ return m_IsPlaying; }

	void						onPlaybackStopped( VxGUID& feedId );
	void						onPlaybackEnded( VxGUID& feedId );
	void						onStreamStop( void );

protected:
	void						lockProviderMgr() { m_ProviderMgrMutex.lock(); }
	void						unlockProviderMgr() { m_ProviderMgrMutex.unlock(); }

	void						lockStreamMgr() { m_StreamMgrMutex.lock(); }
	void						unlockStreamMgr() { m_StreamMgrMutex.unlock(); }

    //=== provider functions ===//
    int							isProviderFile( struct VFile* vFile ) { return vFile->m_ProviderFileType > 0; }

    VirtProviderFile*           findProviderFile( VFile* fp );

	bool						providerDirectoryExists( std::string dirPath );
    uint64_t                    providerFileExists( std::string fileName );

	bool						providerGetFileInfo( std::string fileNameAndPath, VxFileInfoBase& retFileInfo );

    VFile*						providerFileOpen( std::string fileName, std::string mode );
    int							providerFileClose( VFile* fp );
    int							providerFileEof( VFile* fp );
    int							providerFileError( VFile* fp );
    int							providerFileFlush( VFile* fp );

    size_t						providerFileRead( void* buf, size_t size, size_t count, VFile* fp);
    size_t						providerFileWrite( const void* buf, size_t size, size_t count, VFile* fp);

    int							providerFileGetC( VFile* fp );
    char*						providerFileGetS( char* buf, int size, VFile* fp );
    int							providerFilePutC( int ch, VFile* fp );
    int							providerFilePutS( const char* s, VFile* fp );

    int							providerFileGetPos( VFile* fp, fpos_t* pos );
    int							providerFileSetPos( VFile* fp, const fpos_t* pos );
    int							providerFileSeek( VFile* fp, size_t offset, int whence);

    //=== virtual stream functions ===//
	int							isVirtualFile( struct VFile* vFile ) { return vFile->m_VirtFileType > 0; }
	
	VFile*						virtFileOpen( std::string fileName, std::string mode );
	int							virtFileClose( VFile* fp );
	int							virtFileEof( VFile* fp );
	int							virtFileError( VFile* fp );
	int							virtFileFlush( VFile* fp );

	size_t						virtFileRead( void* buf, size_t size, size_t count, VFile* fp);
	size_t						virtFileWrite( const void* buf, size_t size, size_t count, VFile* fp);

	int							virtFileGetC( VFile* fp );
	char*						virtFileGetS( char* buf, int size, VFile* fp );
	int							virtFilePutC( int ch, VFile* fp );
	int							virtFilePutS( const char* s, VFile* fp );

	int							virtFileGetPos( VFile* fp, fpos_t* pos );
	int							virtFileSetPos( VFile* fp, const fpos_t* pos );
	int							virtFileSeek( VFile* fp, size_t offset, int whence);

	bool						waitForStream( int64_t fileOffs, int64_t readLen );
	bool						sendStreamSeek( int64_t newPos );

	bool						verifyCacheData( int64_t fileOffs, uint8_t* buf, int64_t dataLen );

	void						onFileXferPktRxed( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr ) override;

	virtual void				onPktFileGetReply			( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );

	virtual void				onPktFileChunkReq			( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );

	virtual void				onPktFileGetCompleteReq		( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );
	virtual void				onPktFileGetCompleteReply	( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );

	virtual void				onPktFileSendCompleteReq	( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );

	virtual void				onPktFileXferCancel			( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );

	virtual void				onPktFileShareErr			( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );

	virtual void				onPktStreamCtrlReply		( std::shared_ptr<VxSktBase>& sktBase, VxPktHdr* pktHdr );

	P2PEngine&					m_Engine;
	PluginFileShareClient&		m_Plugin;

	std::vector<VirtStreamFile> m_VirtFiles;
	VxMutex						m_StreamMgrMutex;

	VirtStreamFile				m_LiveStream;

	std::atomic_bool			m_IsPlaying;

	std::vector<VirtProviderFile*> m_ProviderFiles;
	VxMutex						m_ProviderMgrMutex;
};

extern VirtStreamMgr& GetVirtStreamMgr( void );
