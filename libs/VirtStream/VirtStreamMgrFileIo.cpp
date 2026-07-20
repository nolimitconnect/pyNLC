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
#include <Plugins/PluginFileShareClient.h>

#include <CoreLib/VFile.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>

#include <NetLib/VxSktBase.h>

#ifdef TARGET_OS_WINDOWS
	#include "shlobj.h" // for VxGetMyDocumentsDir
	#include <direct.h>
#else
	#include <dirent.h> // for searching directories
	#include <ctype.h>
	#include <unistd.h> 
	#include <sys/vfs.h>    
	#include <sys/statfs.h> 
#endif

#define VERIFY_CACHE_DATA 0

namespace
{
	
	//============================================================================
	std::wstring Utf8ToWide(const std::string utf8string)
	{
	#ifdef TARGET_OS_WINDOWS
		std::wstring convertedString;
		int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, 0, 0);
		if(requiredSize > 0)
		{
			std::vector<wchar_t> buffer(requiredSize);
			MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, &buffer[0], requiredSize);
			convertedString.assign(buffer.begin(), buffer.end() - 1);
		}

		return convertedString;
	#else
		//TODO Linux version
		size_t asciiSize = utf8string.length();
		wchar_t * buf = new wchar_t[ asciiSize + 2 ];
		buf[0] = 0;
		const char* pTemp = utf8string.c_str();
		for( unsigned int i = 0; i < asciiSize + 1; i++ )
		{
			buf[i] = (wchar_t)pTemp[i];
		}

		std::wstring strResult = buf;
		delete[] buf;
		return strResult;
	#endif // TARGET_OS_WINDOWS
	}
}

//============================================================================
uint64_t VirtStreamMgr::fileExists( const char* fileName )
{
#if defined(TARGET_OS_ANDROID)
    if( fileIsProviderFile( fileName ) )
    {
        return providerFileExists( fileName );
    }
#endif // defined(TARGET_OS_ANDROID)
		int result;
#ifdef TARGET_OS_WINDOWS
	struct __stat64 gStat;
	// Get data associated with the file
	result = _wstat64( Utf8ToWide( fileName ).c_str(), &gStat );
#else
	struct stat64 gStat;
	// Get data associated with the file
	result = stat64( fileName, &gStat );
#endif //TARGET_OS_WINDOWS

	// Check if statistics are valid:
	if( result != 0 )
	{
		//error getting file info
#if defined(DEBUG)
        int errCode = VxGetLastError();
		LogMsg( LOG_DEBUG, "VirtStreamMgr::fileExists Error %d %s", errCode, fileName );
#endif // defined(DEBUG)etCurrentWorkingDirectory
		return 0;
	}
	else
	{
		//return file size
		return gStat.st_size;
	}
}

//============================================================================
bool VirtStreamMgr::directoryExists( const char* dirPath )
{
    if( fileIsProviderFile( dirPath ) )
    {
        return providerDirectoryExists( dirPath );
    }

	char acBuf[ VX_MAX_PATH ];
	strcpy( acBuf, dirPath );
	bool bIsDir = true;
	struct stat oFileStat;

	if( strlen( acBuf ) > 3 )
	{
		//if not root of drive remove the trailing backslash
		if( ('/' == acBuf[ strlen( acBuf ) - 1 ]) ||
			('\\' == acBuf[ strlen( acBuf ) - 1 ]) )
		{
			acBuf[ strlen( acBuf ) - 1 ] = 0;
		}
	}

	memset( &oFileStat, 0, sizeof( struct stat ) );
#ifdef TARGET_OS_WINDOWS
	oFileStat.st_mode = _S_IFDIR; //check for dir not file
	if( 0 == stat( acBuf, &oFileStat ) )
	{
		if( false == ( oFileStat.st_mode & _S_IFDIR ))
		{
			//path is not valid directory
			bIsDir = false;
		}
	}
	else
	{
		bIsDir = false;
	}
#else // LINUX or android
	oFileStat.st_mode = S_IFDIR; //check for dir not file
	if( 0 == stat( acBuf, &oFileStat ) )
	{
		if( false == ( oFileStat.st_mode & S_IFDIR ))
		{
			//path is not valid directory
			bIsDir = false;
		}
	}
    else
	{
		bIsDir = false;
	}
#endif // LINUX
	return bIsDir;
}

//============================================================================
bool VirtStreamMgr::fileIsProviderFile( const char* fileName )
{
#if defined(TARGET_OS_ANDROID)
    const int prefixLen = 10;
    const char* contentPrefix = "content://";
    return fileName && strlen( fileName ) > prefixLen && strncmp( fileName, contentPrefix, prefixLen ) == 0;
#else
    return false;
#endif // defined(TARGET_OS_ANDROID)
}

//============================================================================
bool VirtStreamMgr::getFileInfo( const char* fileNameAndPath, VxFileInfoBase& retFileInfo )
{
    if( fileIsProviderFile( fileNameAndPath ) )
    {
        return providerGetFileInfo( fileNameAndPath, retFileInfo );
    }

    return VxFileUtil::getFileInfo( fileNameAndPath, retFileInfo );
}
	
//============================================================================
bool VirtStreamMgr::seperatePathAndFile( const char* pFullPath,		// path and file name			
										 std::string& strRetPath,	// return path of file
										 std::string& strRetFile )	// return  file name
{
    const std::string speratorStr( "%2F" );

	bool result{ false };
	std::string fullPath( pFullPath );
	if( !fullPath.empty() )
	{
		size_t strPos = fullPath.rfind ( speratorStr );
		if( strPos != std::string::npos )
		{
            strRetFile = fullPath.substr( strPos + speratorStr.length(), fullPath.length() - strPos );
			strRetPath = fullPath.substr( 0, strPos );

		}
		else
		{
			strRetFile = fullPath;
			strRetPath.clear();
		}

		result = true;
	}

	return result;
}

//============================================================================
VFile* VirtStreamMgr::fileOpen( const char* fileNameIn, const char* fileMode )
{
	vx_assert( fileNameIn );
    vx_assert( fileMode );

    if( fileIsProviderFile( fileNameIn ) )
    {
        return providerFileOpen( fileNameIn, fileMode );
    }

    std::string mode( fileMode );
    vx_assert( mode.size() );

    std::string fileName( fileNameIn );

	bool virtStream{ false };
	bool createIfDoesNotExist{ false };
	if( mode.size() > 0 )
	{
		std::size_t foundPos = mode.find_last_of("v");
		if( foundPos != std::string::npos )
		{
			virtStream = true;
			mode = mode.substr( 0, foundPos );
		}

		if( virtStream )
		{
			return virtFileOpen( fileName, mode );
		}

		createIfDoesNotExist = mode.find_last_of("+") != std::string::npos;
	}

	VFile* vFile = (VFile*)malloc( sizeof( VFile ) );
	vx_assert( vFile );
	memset( vFile, 0, sizeof( VFile ) );

	vFile->m_FileLen = VxFileUtil::fileExists( fileName.c_str(), !createIfDoesNotExist );
	FILE* fp = fopen( fileName.c_str(), mode.c_str() );
	vFile->m_Error = VxGetLastError();

	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s filname %s mode %s fp %p", __func__, fileName.c_str(), fileMode, fp );

	if( fp )
	{
		vFile->m_FILE = fp;	
		return vFile;
	}
	else
	{
		free( vFile );
	}

	return 0;
}

//============================================================================
int VirtStreamMgr::fileClose( VFile* fp )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileClose( fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileClose( fp );
	}

	int result{ 0 };
	if( fp->m_FILE )
	{
        result = fclose( fp->m_FILE );
		fp->m_FILE = nullptr;
		fp->m_Error = VxGetLastError();
	}
	else
	{
		LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p possible attempt to close file twice", __func__, fp );
	}

	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	free( fp );
	return result;
}

//============================================================================
int VirtStreamMgr::fileEof( VFile* fp )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileEof( fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileEof( fp );
	}

	int result = feof( fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::fileError( VFile* fp )
{
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return fp->m_Error;
}

//============================================================================
int VirtStreamMgr::fileFlush( VFile* fp )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileFlush( fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileFlush( fp );
	}

	int result = fflush( fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
size_t VirtStreamMgr::fileRead( void* buf, size_t size, size_t count, VFile* fp )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileRead( buf, size, count, fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileRead( buf, size, count, fp );
	}

	int result = fread( buf, size, count, fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
size_t VirtStreamMgr::fileWrite( const void* buf, size_t size, size_t count, VFile* fp )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileWrite( buf, size, count, fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileWrite( buf, size, count, fp );
	}

	size_t result = fwrite( buf, size, count, fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::fileGetC( VFile* fp )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileGetC( fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileGetC( fp );
	}

	int result = fgetc( fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
char*  VirtStreamMgr::fileGetS( char* buf, int size, VFile* fp ) 
{
	if( isVirtualFile( fp ) )
	{
		return virtFileGetS( buf, size, fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileGetS( buf, size, fp );
	}

	char* result = fgets( buf, size, fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::filePutC( int ch, VFile* fp )
{
	if( isVirtualFile( fp ) )
	{
		return virtFilePutC( ch, fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFilePutC( ch, fp );
	}

	int result = fputc( ch, fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::filePutS( const char* s, VFile* fp ) 
{
	if( isVirtualFile( fp ) )
	{
		return virtFilePutS( s, fp );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFilePutS( s, fp );
	}

	int result = fputs( s, fp->m_FILE );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::fileGetPos( VFile* fp, fpos_t* pos )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileGetPos( fp, pos );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileGetPos( fp, pos );
	}

	int result = fgetpos( fp->m_FILE, pos );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::fileSetPos( VFile* fp, const fpos_t* pos )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileSetPos( fp, pos );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileSetPos( fp, pos );
	}

	int result = fsetpos( fp->m_FILE, pos );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::fileSeek( VFile* fp, size_t offset, int whence )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileSeek( fp, offset, whence );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileSeek( fp, offset, whence );
	}

	int result = fseek( fp->m_FILE, offset, whence );
	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
int VirtStreamMgr::fileSeek64( VFile* fp, uint64_t offs )
{
	if( isVirtualFile( fp ) )
	{
		return virtFileSeek( fp, offs, SEEK_SET );
	}
	else if( isProviderFile( fp ) )
	{
		return providerFileSeek( fp, offs, SEEK_SET );
	}

#ifdef TARGET_OS_WINDOWS
	int result = _fseeki64( fp->m_FILE, offs, SEEK_SET );
#else
	int result = fseek( fp->m_FILE, offs, SEEK_SET );
#endif// TARGET_OS_WINDOWS

	fp->m_Error = VxGetLastError();
	LogModule( eLogStreams, LOG_VERBOSE, "VirtStreamMgr::%s fp %p", __func__, fp );
	return result;
}

//============================================================================
//============================================================================
	
//============================================================================
VFile* VirtStreamMgr::virtFileOpen( std::string fileName, std::string fileMode )
{
	VxFileUtil::makeForwardSlashPath( fileName );

	std::string streamFileName;
	VxFileUtil::makeShortFileName( fileName.c_str(), streamFileName );

	std::string assetFileName;
	VxFileUtil::makeShortFileName( m_LiveStream.m_StreamAssetInfo.getAssetName().c_str(), assetFileName );

	if( streamFileName != assetFileName )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s fileName %s does not match asset name %s",
				__func__, fileName.c_str(), m_LiveStream.m_StreamAssetInfo.getAssetName().c_str() );
		return nullptr;
	}

	if( !m_LiveStream.isConnected() )
	{
		LogMsg( LOG_WARN, "VirtStreamMgr::%s user %s is no longer connected", __func__,
				m_Engine.describeUser( m_LiveStream.m_StreamAssetInfo.getDestUserId() ).c_str() );
		return nullptr;
	}

	VFile* vFile = new VFile();
	memset( vFile, 0, sizeof( VFile ) );
	vFile->m_FileLen = m_LiveStream.m_StreamAssetInfo.getAssetLength();
	vFile->m_VirtFileType = (int16_t)m_LiveStream.m_StreamAssetInfo.getAssetType();

	lockStreamMgr();
	if( m_LiveStream.m_VFile )
	{
		delete m_LiveStream.m_VFile;
		m_LiveStream.m_VFile = nullptr;
	}

	m_LiveStream.m_VFile = vFile;
	unlockStreamMgr();
	return vFile;
}

//============================================================================
int VirtStreamMgr::virtFileClose( VFile* fp )
{
	int retVal = -1;
	lockStreamMgr();
	if( fp != m_LiveStream.m_VFile )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
		unlockStreamMgr();
		vx_assert( false );
		return retVal;
	}
		
	delete m_LiveStream.m_VFile;
	m_LiveStream.m_VFile = nullptr;
	m_LiveStream.m_StreamCache.clearCache( false );

	unlockStreamMgr();
	return retVal;
}

//============================================================================
int VirtStreamMgr::virtFileEof( VFile* fp )
{
	lockStreamMgr();
	if( fp != m_LiveStream.m_VFile )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
		unlockStreamMgr();
		vx_assert( false );
		return 0;
	}

	bool eof = m_LiveStream.m_VFile->m_FileOffs == m_LiveStream.m_VFile->m_FileLen;
	unlockStreamMgr();
	return eof;
}

//============================================================================
int VirtStreamMgr::virtFileError( VFile* fp )
{
	int retVal = -1;
	lockStreamMgr();
	if( fp != m_LiveStream.m_VFile )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
		unlockStreamMgr();
		vx_assert( false );
		return retVal;
	}
	
	m_LiveStream.isConnected();
	retVal = m_LiveStream.getError();
	unlockStreamMgr();
	return retVal;
}

//============================================================================
int VirtStreamMgr::virtFileFlush( VFile* fp )
{
	return 0;
}

//============================================================================
size_t VirtStreamMgr::virtFileRead( void* buf, size_t size, size_t count, VFile* fp )
{
	int retVal = -1;

	lockStreamMgr();
	if( fp != m_LiveStream.m_VFile )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
		unlockStreamMgr();
		vx_assert( false );
		return retVal;
	}

	if( !m_LiveStream.isConnected() )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s lost connection", __func__ );
		unlockStreamMgr();
		return -99;
	}

	if( m_LiveStream.getError() )
	{
		retVal = m_LiveStream.getError();
		unlockStreamMgr();
		return retVal;
	}

	int64_t wantReadLen = size * count;
	int64_t readAttemptLen = std::min( wantReadLen, m_LiveStream.m_VFile->m_FileLen - m_LiveStream.m_VFile->m_FileOffs );
	int64_t fileOffs = m_LiveStream.m_VFile->m_FileOffs;
	unlockStreamMgr();

	if( !waitForStream( fileOffs, readAttemptLen ) )
	{
		LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s timeout waiting for stream file %s at offs %" PRId64 " len %" PRId64,
				   __func__, m_LiveStream.m_StreamAssetInfo.getAssetName().c_str(), fileOffs, readAttemptLen );
		if( readAttemptLen <= 0 )
		{
			return 0;
		}
	}

	lockStreamMgr();
	int64_t readLen = 0;
	if( readAttemptLen > 0 )
	{
		const bool tailStartsAtOffset = m_LiveStream.m_FileTail.hasData( fileOffs, 1 ) == 1;
		if( tailStartsAtOffset )
		{
			readLen = m_LiveStream.m_FileTail.readData( fileOffs, (char*)buf, readAttemptLen );
			if( readLen < readAttemptLen )
			{
				readLen += m_LiveStream.m_StreamCache.readData( fileOffs + readLen, &((char*)buf)[readLen], readAttemptLen - readLen );
			}
		}
		else
		{
			readLen = m_LiveStream.m_StreamCache.readData( fileOffs, (char*)buf, readAttemptLen );
			if( readLen < readAttemptLen )
			{
				readLen += m_LiveStream.m_FileTail.readData( fileOffs + readLen, &((char*)buf)[readLen], readAttemptLen - readLen );
			}
		}
	}


	if( readLen > 0 )
	{
#if VERIFY_CACHE_DATA
		verifyCacheData( m_LiveStream.m_VFile->m_FileOffs, (uint8_t *)buf, readAttemptLen );
#endif // VERIFY_CACHE_DATA

		m_LiveStream.m_VFile->m_FileOffs += readLen;
		retVal = 0;
	}

	unlockStreamMgr();
	return retVal ? retVal : readLen;
}

//============================================================================
size_t VirtStreamMgr::virtFileWrite(const void* buf, size_t size, size_t count, VFile* fp)
{
	// not implemented
	return -1;
}

//============================================================================
int VirtStreamMgr::virtFileGetC( VFile* fp )
{
	int retVal = -1;
	lockStreamMgr();
	if( m_LiveStream.m_VFile->m_FileOffs == m_LiveStream.m_VFile->m_FileLen )
	{
		unlockStreamMgr();
		return EOF;
	}

	int64_t fileOffs = m_LiveStream.m_VFile->m_FileOffs;
	unlockStreamMgr();

	if( !waitForStream( fileOffs, 1 ) )
	{

		LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s timeout waiting for stream file %s at offs %" PRId64 " len %" PRId64,
				   m_LiveStream.m_StreamAssetInfo.getAssetName().c_str(), fileOffs, 1 );
		return retVal;
	}

	lockStreamMgr();
	if( fp != m_LiveStream.m_VFile )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
		unlockStreamMgr();
		vx_assert( false );
		return retVal;
	}

	m_LiveStream.isConnected();
	if( m_LiveStream.getError() )
	{
		retVal = m_LiveStream.getError();
		unlockStreamMgr();
		return retVal;
	}

	int64_t readAttemptLen = 1;
	char retChar[1];
	int64_t readLen = m_LiveStream.m_StreamCache.readData( m_LiveStream.m_VFile->m_FileOffs, (char*)retChar, readAttemptLen );
	if( readLen == readAttemptLen )
	{
		m_LiveStream.m_VFile->m_FileOffs += readLen;
		unlockStreamMgr();
		return retChar[0];
	}

	unlockStreamMgr();
	return -1;
}

//============================================================================
char* VirtStreamMgr::virtFileGetS( char* buf, int size, VFile* fp )
{
	int retVal = -1;
	lockStreamMgr();
	int64_t fileOffs = m_LiveStream.m_VFile->m_FileOffs;
	unlockStreamMgr();
	if( !waitForStream( fileOffs, 1 ) )
	{	
		LogModule( eLogStreams, LOG_ERROR, "VirtStreamMgr::%s timeout waiting for stream file %s at offs %" PRId64 " len %" PRId64,
				   m_LiveStream.m_StreamAssetInfo.getAssetName().c_str(), fileOffs, 1 );
		return nullptr;
	}
	
	lockStreamMgr();
	if( fp != m_LiveStream.m_VFile )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
		unlockStreamMgr();
		vx_assert( false );
		return nullptr;
	}

	m_LiveStream.isConnected();
	if( m_LiveStream.getError() )
	{
		unlockStreamMgr();
		return nullptr;
	}
	
	std::string readStr;
	int result = -1;
	int64_t readIdx = m_LiveStream.m_VFile->m_FileOffs;
	bool foundEnd{ false };
	while( !foundEnd && readIdx < m_LiveStream.m_VFile->m_FileLen && readIdx < size )
	{
		char retChar[1];
		int64_t readLen = m_LiveStream.m_StreamCache.readData( readIdx + readStr.size(), (char*)retChar, 1);
		if( readLen )
		{
			readStr.push_back( retChar[0] );
			if( retChar[0] == '\n' )
			{
				readStr.push_back( 0 );
				result = 0;
				foundEnd = true;
				break;
			}
		}
		else
		{
			break;
		}
	}
	
	unlockStreamMgr();
	if( result == 0 )
	{
		memcpy( buf, readStr.c_str(), readStr.length() );
		return buf;
	}

	return nullptr;
}

//============================================================================
int VirtStreamMgr::virtFileGetPos( VFile* fp, fpos_t* pos )
{
    lockStreamMgr();
    if( fp != m_LiveStream.m_VFile )
    {
        LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
        unlockStreamMgr();
        vx_assert( false );
        return -1;
    }

#if defined(TARGET_OS_LINUX)
    fpos_t posConvert;
    posConvert.__pos = m_LiveStream.m_VFile->m_FileOffs;
    *pos = posConvert;
#else
    *pos = m_LiveStream.m_VFile->m_FileOffs;
#endif
    unlockStreamMgr();
    return 0;
}

//============================================================================
int VirtStreamMgr::virtFilePutC(int ch, VFile* fp)
{
	// not implemented
	return -1;
}

//============================================================================
int VirtStreamMgr::virtFilePutS(const char* s, VFile* fp)
{
	// not implemented
	return -1;
}
//============================================================================
int VirtStreamMgr::virtFileSetPos( VFile* fp, const fpos_t* pos )
{
	// not implemented
	return -1;
}

//============================================================================
int VirtStreamMgr::virtFileSeek( VFile* fp, size_t offset, int whence )
{
	lockStreamMgr();
	if( fp != m_LiveStream.m_VFile )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s wrong VFile", __func__ );
		unlockStreamMgr();
		vx_assert( false );
		return -1;
	}

	int64_t origPos = m_LiveStream.m_VFile->m_FileOffs;
	switch( whence )
	{
	case SEEK_SET:
		// Beginning of file
		if( offset >= m_LiveStream.m_VFile->m_FileLen )
		{
			unlockStreamMgr();
			return -1;
		}

		m_LiveStream.m_VFile->m_FileOffs = offset;
		break;

	case SEEK_CUR:
		// Current position of the file pointer
		if( m_LiveStream.m_VFile->m_FileOffs + offset < 0 ||
			m_LiveStream.m_VFile->m_FileOffs + offset >= m_LiveStream.m_VFile->m_FileLen )
		{
			unlockStreamMgr();
			return -1;
		}

		m_LiveStream.m_VFile->m_FileOffs += offset;
		break;

	case SEEK_END:
		m_LiveStream.m_VFile->m_FileOffs = m_LiveStream.m_VFile->m_FileLen + offset;
		break;
	}

	int64_t newPos = m_LiveStream.m_VFile->m_FileOffs;
	if( newPos < 0 )
	{
		m_LiveStream.m_VFile->m_FileOffs = 0;
		LogMsg( LOG_ERROR, "%s invalid pos" PRId64, __func__, newPos );
	}

	unlockStreamMgr();

	if( newPos >= 0 && newPos != origPos && 
		!m_LiveStream.m_StreamCache.hasData( newPos, 1 ) &&
		!m_LiveStream.m_FileTail.hasData( newPos, 1 ) )
	{
		return sendStreamSeek( newPos ) ? 0 : -1;
	}

	return 0;
}

//============================================================================
bool VirtStreamMgr::verifyCacheData( int64_t fileOffs, uint8_t* buf, int64_t dataLen )
{
	if( !dataLen )
	{
		return true;
	}

	bool result{ false };
	FILE* fp = fopen( m_LiveStream.m_StreamAssetInfo.getAssetName().c_str(), "rb" );
	if( fp )
	{
		if( 0 == fseek( fp, fileOffs, SEEK_SET ) )
		{
			uint8_t* fileBuf = new uint8_t[dataLen];
			if( fileBuf )
			{
                if( dataLen == fread( fileBuf, 1, dataLen, fp ) )
				{
					bool match = true;
					for( int i = 0; i < dataLen; i++ )
					{
						if( fileBuf[i] != buf[i] )
						{
							match = false;
							LogMsg( LOG_ERROR, "VirtStreamMgr::%s mismatch offs %" PRId64 " expected 0x%x got 0x%x", __func__,
									fileOffs + i, buf[i], fileBuf[i] );
							break;
						}
					}

					if( match )
					{
						result = true;
					}

				}
			}
		}

		fclose( fp );
	}

	if( !result )
	{
		LogMsg( LOG_ERROR, "VirtStreamMgr::%s verify failed offs %" PRId64 " of file %s", __func__,
									fileOffs, m_LiveStream.m_StreamAssetInfo.getAssetName().c_str() );
	}


	return result;
}
