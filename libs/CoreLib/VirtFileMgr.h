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

#include <CoreLib/VirtFile.h>

#include <string>
#include <vector>

class VxFile;
class VxFileInfoBase;

class VirtFileMgr
{
public:
	virtual bool                directoryExists( const char* dirPath ) = 0;
    virtual uint64_t			fileExists( const char* fileNameAndPath ) = 0;
    virtual bool				fileIsProviderFile( const char* fileNameAndPath ) = 0;

    virtual bool                getFileInfo( const char* fileNameAndPath, VxFileInfoBase& retFileInfo ) = 0;

    virtual bool				seperatePathAndFile( const char* fileNameAndPath,		// path and file name
													 std::string& strRetPath,	// return path to file
													 std::string& strRetFile ) = 0;	// return file name

    virtual VFile*				fileOpen( const char* fileNameAndPath, const char* fileMode ) = 0;
	virtual int					fileClose( VFile* vFile ) = 0;
	virtual int					fileEof( VFile* fp ) = 0;
	virtual int					fileError( VFile* fp ) = 0;
	virtual int					fileFlush( VFile* fp ) = 0;
	virtual size_t				fileRead( void* buf, size_t size, size_t count, VFile* fp ) = 0;
	virtual size_t				fileWrite( const void* buf, size_t size, size_t count, VFile* fp ) = 0;
	virtual int					fileGetC( VFile* fp ) = 0;
	virtual char*				fileGetS( char* buf, int size, VFile* fp ) = 0;
	virtual int					filePutC( int ch, VFile* fp ) = 0;
	virtual int					filePutS( const char* s, VFile* fp ) = 0;
	virtual int					fileGetPos( VFile* fp, fpos_t* pos ) = 0;
	virtual int					fileSetPos( VFile* fp, const fpos_t* pos ) = 0;
	virtual int					fileSeek( VFile* fp, size_t offset, int whence ) = 0;
	virtual int					fileSeek64( VFile* fp, uint64_t offs ) = 0;

    virtual int					listProviderFilesAndFolders( const char* srcDir, std::vector<VxFileInfoBase>& fileList, uint8_t fileFilterMask ) = 0;
};

extern VirtFileMgr& GetVirtFileMgr();
