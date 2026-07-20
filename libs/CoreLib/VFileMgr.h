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

// DO NOT USE FOR NLC.. for local testing only.. NLC uses VirtStreamMgr with streaming support

#include <CoreLib/VirtFileMgr.h>
#include <CoreLib/VxMutex.h>

#include <vector>
#include <atomic>

class P2PEngine;
class VirtProviderFile;
class VirtStreamFile;
class VxFileInfoBase;

class VFileMgr : public VirtFileMgr
{
public:
    VFileMgr() = default;

    bool                        directoryExists( const char* dirPath ) override;
    uint64_t                    fileExists( const char* fileNameAndPath ) override;
    bool                        fileIsProviderFile( const char* fileNameAndPath ) override;

    bool                        getFileInfo( const char* fileNameAndPath, VxFileInfoBase& fileInfoBase ) override;

    bool						seperatePathAndFile( const char* fileNameAndPath,		// path and file name
                                                     std::string& strRetPath,           // return path of file
                                                     std::string& strRetFile ) override;	// return file name

    VFile*						fileOpen( const char* fileNameAndPath, const char* fileMode ) override;
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

protected:
	void						lockProviderMgr() { m_ProviderMgrMutex.lock(); }
	void						unlockProviderMgr() { m_ProviderMgrMutex.unlock(); }

    //=== provider functions ===//
    int							isProviderFile( struct VFile* vFile ) { return vFile->m_ProviderFileType > 0; }

    VirtProviderFile*           findProviderFile( VFile* fp );

	bool						providerDirectoryExists( std::string dirPath );
    uint64_t                    providerFileExists( std::string fileNameAndPath );
    bool						providerGetFileInfo( std::string fileNameAndPath, VxFileInfoBase& retFileInfo );

    VFile*						providerFileOpen( std::string fileNameAndPath, std::string mode );
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

	bool						verifyCacheData( int64_t fileOffs, uint8_t* buf, int64_t dataLen );

	std::vector<VirtProviderFile*> m_ProviderFiles;
	VxMutex						m_ProviderMgrMutex;
};

extern VirtFileMgr& GetVirtFileMgr();
