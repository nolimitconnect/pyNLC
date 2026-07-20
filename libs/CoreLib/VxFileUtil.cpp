//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxFileUtil.h"

#include "VirtFileMgr.h"
#include "VxCrypto.h"
#include "VxDebug.h"
#include "VxFileCopier.h"
#include "VxFileIsTypeFunctions.h"
#include "VxGlobals.h"
#include "VxParse.h"
#include "VxUrl.h"

#include "SHA1.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#ifdef TARGET_OS_WINDOWS
	#include "shlobj.h" // for VxGetMyDocumentsDir and GetFullPathNameA
	#include <direct.h>
#define S_IRUSR 0000400
#define S_IWUSR 0000200
#define S_IXUSR 0000100
#define	S_IRWXG	0000070			/* RWX mask for group */
#define S_IRGRP 0000040
#define S_IWGRP 0000020
#define S_IXGRP 0000010
#define	S_IRWXO	0000007			/* RWX mask for other */
#define S_IROTH 0000004
#define S_IWOTH 0000002
#define S_IXOTH 0000001

#if !S_IRWXU
# define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif
#if !S_IRWXG
# define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#endif
#if !S_IRWXO
# define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#endif

#else
	#include <dirent.h> // for searching directories
	#include <ctype.h>
	#include <unistd.h> 
	#include <sys/vfs.h>    
	#include <sys/statfs.h> 
#endif

#include <algorithm>
#include <regex>

#define PATH_SEP_CHAR   '/'
#define PATH_ALT_SEP_CHAR   '\\'

static char GetPathSeperator( void )
{
#ifdef TARGET_OS_WINDOWS
    return PATH_ALT_SEP_CHAR;
#else
    return PATH_SEP_CHAR;
#endif // TARGET_OS_WINDOWS
}

const char* TEST_WRITEABLE_FILE_NAME = "nlctest.nlc";


//============================================================================
size_t FindLastPathSeperator( std::string& path )
{
    return path.find_last_of( GetPathSeperator() );
}

//============================================================================
bool AddExtraLongPathPrefix( std::string& path )
{
    const char* str = path.c_str();
    if( path.length() < 4 || str[ 0 ] != '\\' || str[ 1 ] != '\\' || str[ 3 ] != '\\' || str[ 2 ] != '?' )
    {
        path.insert( 0, "\\\\?\\" );
        return true;
    }

    return false;
}

//============================================================================
bool RemoveExtraLongPathPrefix( std::string& path )
{
    const char* str = path.c_str();
    if( path.length() >= 4 && str[ 0 ] == '\\' && str[ 1 ] == '\\' && str[ 3 ] == '\\' && str[ 2 ] == '?' )
    {
        path.erase( 0, 4 );
        return true;
    }

    return false;
}

//============================================================================
#ifdef TARGET_OS_WINDOWS
std::string WindowsRelativeToAbsolutePath( std::string& path )
{
    std::string resultPath = path;
    char pathBuf[ MAX_PATH * 2];
    if( path.length() && path.length() <= MAX_PATH )
    {
        strcpy( pathBuf, path.c_str() );
        if( WindowsRelativeToAbsolutePath( pathBuf, sizeof( pathBuf ) ) )
        {
            resultPath = pathBuf;
        }
    }

    return resultPath;
}

extern "C" bool WindowsRelativeToAbsolutePath( char * pathBuf, int bufLen )
{
    if( !pathBuf || bufLen < 1 )
    {
        LogMsg( LOG_ERROR, "WindowsRelativeToAbsolutePath invalid path" );
        return false;
    }

    std::string fullPath = pathBuf;
    if( fullPath.size() < 3 )
    {
        // not long enough to have relative path but is not a failure
        return true;
    }

    std::string justPath = fullPath;
    std::string fileName;
    char pathSeperator = GetPathSeperator();
    bool removedRelative = false;
    bool endsWithSlash = false;

    if( !VxFileUtil::directoryExists( justPath.c_str() ) )
    {
        // remove file name
        size_t lastSep = FindLastPathSeperator( fullPath );
        if( lastSep != std::string::npos )
        {
             pathSeperator = fullPath.at( lastSep );

            if( lastSep == fullPath.size() - 1 )
            {
                endsWithSlash = true;
                // remove slash at end of path
                justPath = fullPath.substr( 0, lastSep );
            }
           
            justPath = fullPath.substr( 0, lastSep );
            fileName = fullPath.substr( lastSep + 1, fullPath.size() );
        }
    }

    bool hasRelativePath = true;
    for( int i = 0; hasRelativePath && i < 4; i++ )
    {
        hasRelativePath = false;
        // make into full path by removing relative ..
        if( justPath.find( ".." ) != std::string::npos )
        {
            // expand relative path to full path
            AddExtraLongPathPrefix( justPath );
            const unsigned int bufSize = GetFullPathNameA( justPath.c_str(), 0, nullptr, nullptr );
            if( bufSize != 0 )
            {
                char * buf = new char[ bufSize ];
                if( GetFullPathNameA( justPath.c_str(), bufSize, buf, nullptr ) <= bufSize - 1 )
                {
                    std::string absolutePath( buf );
                    RemoveExtraLongPathPrefix( absolutePath );
                    justPath = absolutePath;
                    hasRelativePath = true;
                    removedRelative = true;
                }

                delete[] buf;
            }
        }
    }

    if( removedRelative )
    {
        fullPath = justPath;
        fullPath += pathSeperator;
        if( fileName.size() )
        {
            // add back the file name to path
            fullPath += fileName;
        }

        if( endsWithSlash )
        {
            fullPath += pathSeperator;
        }

        if( fullPath.size() >= bufLen )
        {
            // too long for buffer
            LogMsg( LOG_ERROR, "WindowsRelativeToAbsolutePath Buffer too short" );
            return false;
        }

        strcpy( pathBuf, fullPath.c_str() );
    }

    return true;
}
#endif // TARGET_OS_WINDOWS

//========================================================================
std::string VxFileUtil::makeKodiPath( const char* path )
{
	std::string kodiPath = path;
#if defined(TARGET_OS_WINDOWS)
	makeBackwardSlashPath( kodiPath );
#else
	makeForwardSlashPath( kodiPath );
#endif // defined(TARGET_OS_WINDOWS)

	removeTrailingDirectorySlash( kodiPath );

	return kodiPath;
}

//============================================================================
std::string VxFileUtil::readVersionFile( std::string& versionFileName )
{
    std::string versionContents;

    char * pvBuf = nullptr;
    uint32_t u32LenOfFile = 0;
    int32_t rc = VxFileUtil::readWholeFile( versionFileName.c_str(), (void**)&pvBuf, &u32LenOfFile );
    if( 0 == rc )
    {
        if( u32LenOfFile > 2 && u32LenOfFile < 32 && strlen(pvBuf) > 2 && strlen(pvBuf) < 32 )
        {
            versionContents = pvBuf;
        }

        delete pvBuf;
    }

    return versionContents;
}

//============================================================================
bool VxFileUtil::isDotDotDirectory( const char* fileName )
{
	int nameLen = strlen( fileName );
	if( ( ( 1 == nameLen ) && ( '.' == fileName[0] ) )
		|| ( ( 2 == nameLen ) && ( '.' == fileName[0] ) && ( '.' == fileName[1] ) ) )
	{
		return true;
	}

	return false;
}

//============================================================================
bool VxFileUtil::isDotDotDirectory( const wchar_t * fileName )
{
	int nameLen = wstrlen( fileName );
	if( ( ( 1 == nameLen ) && ( '.' == fileName[0] ) )
		|| ( ( 2 == nameLen ) && ( '.' == fileName[0] ) && ( '.' == fileName[1] ) ) )
	{
		return true;
	}

	return false;
}

//============================================================================
// strip last directory in string
std::string VxFileUtil::moveUpADirectory( std::string& folderPathIn )
{
    if( folderPathIn.empty() )
    {
        LogMsg( LOG_ERROR, "VxFileUtil::moveUpADirectory: empty folderPathIn" );
        return folderPathIn;
    }

    std::string folderPathOut = folderPathIn;
    bool hasTrailingSlash = '/' == folderPathOut[ folderPathOut.length() - 1 ];
    if( hasTrailingSlash )
    {
        folderPathOut = folderPathOut.substr( 0, folderPathOut.length() - 1 );
    }

    size_t lastSlashPos = folderPathOut.find_last_of('/');
    if( lastSlashPos == std::string::npos )
    {
        LogMsg( LOG_ERROR, "VxFileUtil::moveUpADirectory: no directory to move up to" );
        return folderPathIn;
    }

    folderPathOut = folderPathOut.substr( 0, lastSlashPos );
    if( hasTrailingSlash )
    {
        folderPathOut += '/';
    }

    return folderPathOut;
}

//============================================================================
// append file name to path.. account for url etc
std::string VxFileUtil::addFileToFolder( std::string& strFolder,  std::string& strFile)
{
	if( VxUrl::isURL( strFolder ) )
	{
		VxUrl url( strFolder );
		if( url.getFileName() != strFolder )
		{
            url.setFileName( strFile );
			return url.getUrl();
		}
	}

	std::string strResult = strFolder;
	if (!strResult.empty())
	{
		assureTrailingDirectorySlash( strResult );
	}

	// Remove any slash at the start of the file
	if( strFile.size() && ( ( '/' == strFile[0] )  || ( '\\' == strFile[0] ) ) )
		strResult += strFile.substr(1);
	else
		strResult += strFile;

	makeForwardSlashPath( strResult );

	return strResult;
}

//============================================================================
int32_t VxFileUtil::getCurrentWorkingDirectory( std::string strRetDir )
{
	char* buffer;
#ifdef TARGET_OS_WINDOWS
	if( (buffer = _getcwd( nullptr, 0 )) == nullptr )
#else
	if( (buffer = getcwd( nullptr, 0 )) == nullptr )
#endif
	{
		strRetDir = "";
        LogMsg( LOG_INFO, "%s: getcwd error", __func__ );
		return -1;
	}
	else
	{
		strRetDir = buffer;
        free(buffer);
		return 0;
	}
}

//============================================================================
int32_t VxFileUtil::setCurrentWorkingDirectory( const char* pDir )
{
#ifdef TARGET_OS_WINDOWS
	return _chdir(pDir);
#else
	return chdir(pDir);
#endif
}

//============================================================================
bool VxFileUtil::fileIsProviderFile( const char* fileName )
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
bool VxFileUtil::getFileInfo( const char* fileNameAndPath, VxFileInfoBase& retFileInfo )
{
    if(!fileNameAndPath)
    {
        LogMsg( LOG_ERROR, "%s: null file name", __func__ );
        return true;
    }

    if( fileIsProviderFile( fileNameAndPath ) )
    {
        return VFileGetFileInfo( fileNameAndPath, retFileInfo );
    }

    char acBuf[ VX_MAX_PATH ];
    strcpy( acBuf, fileNameAndPath );
    bool isDir = true;
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
            isDir = false;
        }
    }
    else
    {
        isDir = false;
    }
#else // LINUX or android
    oFileStat.st_mode = S_IFDIR; //check for dir not file
    if( 0 == stat( acBuf, &oFileStat ) )
    {
        if( false == ( oFileStat.st_mode & S_IFDIR ))
        {
            //path is not valid directory
            isDir = false;
        }
    }
    else
    {
        isDir = false;
    }
#endif // LINUX

    if( isDir )
    {
        retFileInfo.setFileName( fileNameAndPath );
        retFileInfo.setFileNameAndPath( fileNameAndPath );
        retFileInfo.setFileLength( 0 );
        retFileInfo.setFileType( VXFILE_TYPE_DIRECTORY );
        return true;
    }

    uint64_t fileLen = fileExists( fileNameAndPath );
    if(!fileLen)
    {
        LogMsg(LOG_ERROR, "%s %s does not exist", __func__, fileNameAndPath );
        return false;
    }

    std::string	filePath;
    std::string	justFileName;
    //! separate Path and file name into separate strings
    if( 0 != seperatePathAndFile( fileNameAndPath, filePath, justFileName ) )
    {
        LogMsg(LOG_ERROR, "%s %s failed to separate name and path", __func__, fileNameAndPath );
        return false;
    }

    retFileInfo.setFileName( justFileName );
    retFileInfo.setFileNameAndPath( fileNameAndPath );
    retFileInfo.setFileLength( fileLen );
    retFileInfo.setFileType( VxFileUtil::fileExtensionToFileTypeFlag( retFileInfo.getFileName().c_str() ) );
    return true;
}

//============================================================================
//! returns file size or 0 if doesn't exist or zero length
uint64_t VxFileUtil::fileExists( const char* fileName, bool printLogIfDoesNotExist )
{
	// may be called before ptop engine exists which VFileExists requires
	// so only call the virtual version if is a android provider file
	if( fileIsProviderFile( fileName ) )
	{
		uint64_t fileLen = VFileExists( fileName );
		if( !fileLen )
		{
			int errCode = VxGetLastError();
			if( errCode && printLogIfDoesNotExist )
			{
				LogMsg( LOG_DEBUG, "VxFileUtil::%s File Exists Error %d %s", __func__, errCode, fileName );
			}
		}

		return fileLen;
	}

	int result;
#ifdef TARGET_OS_WINDOWS
	struct __stat64 gStat;
	// Get data associated with the file
	result = _wstat64( Utf8ToWide( fileName ).c_str(), &gStat );
#else
	struct stat gStat;
	// Get data associated with the file
    result = stat( fileName, &gStat );
#endif //TARGET_OS_WINDOWS

	// Check if statistics are valid:
	if( result != 0 )
	{
		//error getting file info
#if defined(DEBUG)
        int errCode = VxGetLastError();
		if( printLogIfDoesNotExist )
		{
			LogMsg( LOG_DEBUG, "VxFileUtil::%s File Exists Error %d %s", __func__, errCode, fileName );
		}
#endif // defined(DEBUG)
		return 0;
	}
	else
	{
		//return file size
		return gStat.st_size;
	}
}

//============================================================================
//! return true if directory exists
bool VxFileUtil::directoryExists( const char* dirPath )
{
	if( fileIsProviderFile( dirPath ) )
	{
		return VFileDirectoryExists( dirPath );
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
uint64_t VxFileUtil::getFileLen( const char* pFileName, bool printLogIfDoesNotExist )			
{ 
	return fileExists( pFileName, printLogIfDoesNotExist ); 
}

//============================================================================
int32_t VxFileUtil::makeDirectory( std::string& strDirectoryPath )	
{ 
	return makeDirectory( strDirectoryPath.c_str()); 
}

//============================================================================
void VxFileUtil::assureTrailingDirectorySlash( std::string& strDirectoryPath )
{
	if( strDirectoryPath.length() )
	{
		const char* name = strDirectoryPath.c_str();
		if( '/' != name[ strDirectoryPath.length() - 1 ] )
		{
			strDirectoryPath += "/";
		}
	}
}

//============================================================================
void VxFileUtil::removeTrailingDirectorySlash( std::string& strDirectoryPath )
{
	if( strDirectoryPath.length() )
	{
		const char* name = strDirectoryPath.c_str();
		if( ( '/' == name[strDirectoryPath.length() - 1] ) || ( '\\' == name[strDirectoryPath.length() - 1] ) )
		{
			strDirectoryPath = strDirectoryPath.substr( 0, strDirectoryPath.length() - 1 );
		}
	}
}

//============================================================================
std::string VxFileUtil::makeUniqueFileName( const char* fileName )
{
	std::string retFileName			= fileName;
	std::string fileNamePart		= "";
	std::string fileExtensionPart	= "";

	seperateFileNameAndExtension( retFileName, fileNamePart, fileExtensionPart );
	//if( 0 != fileExtensionPart.length() )
	{
		int fileIncBinary = 1;
		char fileIncBuf[16];
		while( fileExists( retFileName.c_str() ) )
		{
			sprintf( fileIncBuf, "(%d)", fileIncBinary );
			fileIncBinary++;
			retFileName = fileNamePart;
			retFileName += fileIncBuf;
			retFileName += fileExtensionPart;
		}
	}

	return retFileName;
}

//============================================================================
//! Make all directories that don't exist in a given path
int32_t VxFileUtil::makeDirectory( const char* pDirectoryPath )
{
    vx_assert( pDirectoryPath );
    if( !pDirectoryPath )
    {
        LogMsg( LOG_ERROR, "makeDirectory null path");
        return -1;
    }

    int pathLen = strlen(pDirectoryPath);
    vx_assert( pathLen < VX_MAX_PATH );
    if( ( pathLen >= VX_MAX_PATH ) || ( pathLen < 2 ) )
    {
        LogMsg( LOG_ERROR, "makeDirectory invalid path");
        return -2;
    }

    bool createdDirectories = false;
    char tempDir[VX_MAX_PATH * 2];
    //make a copy
    strcpy(tempDir, pDirectoryPath);
    for(int i = 0; i < pathLen; i++)
    {
        if( tempDir[i] == '\\' )
        {
             tempDir[i] = '/';
        }
    }

    //if no '/' at end put it there
    if( '/' != tempDir[strlen(tempDir) - 1] )
    {
        strcat(tempDir , "/");
    }

    //make the path
    char * pTemp = tempDir;
    #ifdef TARGET_OS_WINDOWS
		// for windows .. if root drive skip it
		if( ':' == pTemp[1] )
		{
			pTemp = strchr( pTemp, '/' );
			if( pTemp )
			{
				pTemp++;
			}
		}
	#endif //TARGET_OS_WINDOWS
    if( pTemp && strlen( pTemp ) )
    {
		while((pTemp = strtok(pTemp, "/" )))
		{
            //look for drive letter or root path
            if(0 == strlen(tempDir))
            {
                continue;
            }

            if(!directoryExists(tempDir))
            {
                //make directory
                int errCode = mkdir_os( tempDir, S_IRWXU | S_IRWXG | S_IRWXO );
                if( 0 != errCode)
                {
                    LogMsg( LOG_INFO, "CoreLib:FailedToMakeDir err %d dir %s", VxGetLastError(), tempDir );
                    return -3;
                }
                else
                {
                    // LogMsg( LOG_VERBOSE, "created directory %s", tempDir);
                    createdDirectories = true;
                }
            }

            //move pTemp up
            pTemp = tempDir + strlen(tempDir);
            //put the '/' back
            tempDir[strlen(tempDir)] = '/';
		}
   }

   if( createdDirectories )
   {
       LogMsg( LOG_VERBOSE, "created directory %s", tempDir);
   }

   return 0;
}

//============================================================================
//! read a line from file into buffer and null terminate it
int32_t VxFileUtil::readLine( VFile *pgFile, char *pBuf, int iBufLen )
{
    int c;
    int i= 0;

    //=== Read a line  ===//
	// skip over \n or \r if is the first character
	c = VFileGetC( pgFile );
	if( c == '\n' || c == '\r' )
	{
		c = VFileGetC( pgFile );
	}

    while( c != '\n' && c != '\r' )
    {
        if( c == EOF )
		{
			pBuf[i] = 0;
            return EOF;
		}
        if( i < iBufLen )
		{
            pBuf[i++] = (char)c;
		}
		else
		{
			LogMsg( LOG_INFO, "Read Line Buffer Overflow" );
			return EOF; // not enough room in buffer
		}

		c = VFileGetC( pgFile );
    }

    pBuf[i] = '\0';
    return 0;
}

//============================================================================
//! open a file and log error message if fails
VFile * VxFileUtil::fileOpen( const char* pFileName, const char* pFileMode )
{
    VFile * retval;
    retval = VFileOpen( pFileName, pFileMode );
	if( nullptr == retval )
	{
		LogMsg( LOG_INFO, "fileOpen:Could not open file %s", pFileName );
	}

    return retval;
}

//============================================================================
//! File seek..NOTE: only seeks from beginning of file
int32_t VxFileUtil::fileSeek ( VFile * poFile, uint32_t u32Pos )
{
	return VFileSeek64( poFile, u32Pos );
}
//============================================================================
//! File seek..NOTE: only seeks from beginning of file
int32_t VxFileUtil::fileSeek ( VFile * poFile, uint64_t u64Pos )
{
	return VFileSeek64( poFile, u64Pos );
}

//============================================================================
int32_t VxFileUtil::copyFile( const char* pOldPath, const char* pNewPath )
{
	#ifdef TARGET_OS_WINDOWS
		if( CopyFileW( Utf8ToWide( pOldPath ).c_str(), Utf8ToWide( pNewPath ).c_str(), false ) )
		{
			return 0;
		}
		return VxGetLastError();
	#else // LINUX
		deleteFile( pNewPath );
		char as8Buf[ VX_MAX_PATH * 2 ];
		sprintf( as8Buf, "cp %s %s", pOldPath, pNewPath );
		system( as8Buf );
		unlink( pOldPath );
		return 0;
	#endif // LINUX
}
//============================================================================
//! copy all files and directories to destination directory
int32_t VxFileUtil::recursiveCopyDirectory( const char* pSrcDir, const char* pDestDir, int64_t& totalCopied )
{
    int32_t rc = 0;
    totalCopied = 0;
    if( pDestDir && pSrcDir )
    {
        LogMsg( LOG_INFO, "recursive copy files from %s to %s", pSrcDir, pDestDir );

        std::string curPath = pSrcDir;
        std::string srcDir = pSrcDir;
        std::string destDir = pDestDir;
        std::vector<VxFileInfo> retFileList;

        if( !srcDir.empty() && !destDir.empty() )
        {
            //makeForwardSlashPath( srcDir );
            //makeForwardSlashPath( destDir );

            //assureTrailingDirectorySlash( srcDir );
            //assureTrailingDirectorySlash( destDir );

            if( directoryExists( srcDir.c_str() ) && directoryExists( destDir.c_str() ) )
            {
                VxFileCopier dirCopier;
                rc = dirCopier.copyDirectory( curPath, srcDir, destDir, retFileList, true );
                if( !rc )
                {
                    for( VxFileInfo& fileInfo : retFileList )
                    {
                        totalCopied += fileInfo.getFileLength();
                    }
                }
            }
            else
            {
                rc = EBADF;
                LogMsg( LOG_ERROR, "recursiveCopyDirectory invalid directory" );
            }
        }
        else
        {
            rc = EBADF;
            LogMsg( LOG_ERROR, "recursiveCopyDirectory empty directory path" );
        }
    }
    else
    {
        rc = EBADF;
        LogMsg( LOG_ERROR, "recursiveCopyDirectory null param" );
    }

    return rc;
}

//============================================================================
int32_t VxFileUtil::deleteFile( const char* pFileName )
{
	#ifdef TARGET_OS_WINDOWS
		return _unlink( pFileName );
	#else // LINUX
		return unlink( pFileName );
	#endif // LINUX
}

//============================================================================
int32_t VxFileUtil::renameFile( const char* pFileOldName, const char* pFileNewName )
{
	#ifdef TARGET_OS_WINDOWS
		return rename( pFileOldName, pFileNewName );
	#else // LINUX
		return rename( pFileOldName, pFileNewName );
	#endif // LINUX
}

//============================================================================
//! copy files to destination directory then delete the source files
int32_t VxFileUtil::moveFiles( char * pDestDir, char * pSrcDir )
{
	// make directory if doesn't exits
	makeDirectory( pDestDir );
	// loop through files in source directory and move them

	LogMsg( LOG_INFO, "VxMoveFile: moving files from %s to %s", pSrcDir, pDestDir );

	#ifdef TARGET_OS_WINDOWS
		std::wstring strSrcDir = Utf8ToWide( pSrcDir );
		std::wstring strDestDir = Utf8ToWide( pDestDir );

		wchar_t as8DestDir[ VX_MAX_PATH * 2 ];
		wchar_t destFile[ VX_MAX_PATH * 2 ];
		wstrcpy( as8DestDir, strDestDir.c_str() );

		// build path and wild card
		wchar_t srcDir[ VX_MAX_PATH * 2 ];
		wchar_t srcFile[ VX_MAX_PATH * 2 ];
		wstrcpy( srcDir, Utf8ToWide( pSrcDir ).c_str() );
		wstrcat( srcDir, L"\\*.*" );
		// start working for files
		WIN32_FIND_DATAW FindFileData;

		HANDLE hFind = FindFirstFileW( srcDir, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			// done with listing
			return 0;
		}

		bool bFinished = false;
		while ( false == bFinished)
		{
			// skip . and .. files; otherwise, we'd
			// recur infinitely!
			if( isDotDotDirectory( FindFileData.cFileName ) )
			{
				if( false == FindNextFileW(hFind, &FindFileData)  )
					break;
				continue;
			}

			// make source file name
			wstrcpy( srcFile, strSrcDir.c_str()  );
			wstrcat( srcFile, L"\\" );
			wstrcat( srcFile, FindFileData.cFileName );
			// make destination file name
			wstrcpy( destFile, strDestDir.c_str() );
			wstrcat( destFile, L"\\" );
			wstrcat( destFile, FindFileData.cFileName );

			struct _stat m;
			if( 0 != _wstat( srcFile, &m) )
			{
				LogMsg( LOG_ERROR, "VxFileUtil::%s ERROR %d", __func__, VxGetLastError() );
			}
			else
			{
				if( _S_IFDIR & m.st_mode )
				{
					// its a directory
				}
				else
				{
					// its a file
					if( 0 == copyFile( WideToUtf8( srcFile ).c_str(), WideToUtf8( destFile ).c_str() ) )
					{
						deleteFile( WideToUtf8( srcFile ).c_str() );
					}
				}
			}

			if( false == FindNextFileW(hFind, &FindFileData)  )
			{
				// done with listing
				FindClose(hFind);
				return 0;
			}
		}
		return 0;
	#else //LINUX
		char as8DestDir[ VX_MAX_PATH * 2 ];
		char as8DestFile[ VX_MAX_PATH * 2 ];
		strcpy( as8DestDir, pDestDir );

		// build path and wild card
		char as8SrcDir[ VX_MAX_PATH * 2 ];
		char as8SrcFile[ VX_MAX_PATH * 2 ];
		strcpy( as8SrcDir, pSrcDir );
		// find the files in the directory
		DIR *pDir;
		struct dirent *pFileEnt;
		if( directoryExists( as8SrcDir ) )
		{
			//LogMsg( LOG_INFO, "VxMoveFile: directory %s exists.. opening dir", as8SrcDir );
			//ok directory exists!
			if(!(nullptr == (pDir = opendir(as8SrcDir))))
			{
				//pDir is open
				while( 0 != (pFileEnt = readdir(pDir)))
				{
					//LogMsg( LOG_INFO, "VxMoveFile: found file %s", pFileEnt->d_name );
					//got a file or directory
					if( isDotDotDirectory(  pFileEnt->d_name ) ) 
					{
						// skip . and ..
						//LogMsg( LOG_INFO, "VxMoveFile: skipping file %s", pFileEnt->d_name );
						continue;
					}

					// valid directory entry
					// make source file name
					strcpy( as8SrcFile, as8SrcDir );
					if( '/' != as8SrcFile[ strlen( as8SrcFile ) - 1 ] )
					{
						strcat( as8SrcFile, "/" );
					}
					strcat( as8SrcFile, pFileEnt->d_name );
					LogMsg( LOG_INFO, "VxMoveFile: made src file %s", as8SrcFile );
					// make destination file name
					strcpy( as8DestFile, as8DestDir );
					if( '/' != as8DestFile[ strlen( as8DestFile ) - 1 ] )
					{
						strcat( as8DestFile, "/" );
					}
					strcat( as8DestFile, pFileEnt->d_name );
					LogMsg( LOG_INFO, "VxMoveFile: made dest file %s", as8DestFile );
					//=== Last Modification Date ===//
					struct stat m;
					if( 0 != stat( as8SrcFile, &m) )
					{
						///ERROR how do we handle
						LogMsg( LOG_INFO, "Unable to stat file %s", as8SrcFile );
						continue;
					}

					if( pFileEnt->d_type == DT_DIR )
					{
						// its a directory
						LogMsg( LOG_INFO, "file %s is directory", as8SrcFile );
						continue;
					}

					// its a file
					LogMsg( LOG_INFO, "VxMoveFile: moving file %s to file %s", as8SrcFile, as8DestFile );
					if( 0 == copyFile( as8SrcFile, as8DestFile ) )
					{
						deleteFile( as8SrcFile );
					}
					else
					{
						LogMsg( LOG_INFO, "VxMoveFile: FAILED moving file %s to file %s", as8SrcFile, as8DestFile );
					}
				}
				// end of listing
				// done with listing
				closedir(pDir);
				return 0;
			}
			else
			{
				LogMsg( LOG_INFO, "moveFiles: could not open directory %s ", as8SrcDir );
			}
		}
		else
		{
			LogMsg( LOG_INFO, "moveFiles: directory %s does not exist ", as8SrcDir );
		}
		return 0;
	#endif //LINUX
}

//============================================================================
int32_t VxFileUtil::moveAFile( const char* srcFile, const char* destFile )
{
	int result = rename( srcFile, destFile );
	int32_t rc = 0;
	if( result )
	{
		rc = VxGetLastError();
        if( 0 == rc )
		{
			rc = result;
		}
	}

	// might be the destination is not on the same drive
	if( rc )
	{
		rc = copyFile( srcFile, destFile );
		if( 0 == rc )
		{
			// successful copy.. delete old file
			deleteFile( srcFile );
		}
	}

	return rc;
}

//============================================================================
//! separate file name into file name and extension strings
bool VxFileUtil::seperateFileNameAndExtension(	std::string &	fileNameWithExt,		// file name with extension				
												std::string &	strRetFileNamePart,		// return file name part without .ext
												std::string &	strRetExtensionPart )	// return .ext part
{
	strRetFileNamePart		= fileNameWithExt;
	strRetExtensionPart		= "";
	char as8Buf[ VX_MAX_PATH ];
	strcpy( as8Buf, fileNameWithExt.c_str() );
	makeForwardSlashPath( as8Buf );
	char * period = strrchr( as8Buf, '.' );
	if( period )
	{
		char * forwardSlash = strrchr( period, '/' );
		if( 0 == forwardSlash )
		{
			strRetExtensionPart = period;
			period[0]			= 0;
			strRetFileNamePart	= as8Buf;
			return true;
		}
		else
		{
            LogMsg( LOG_ERROR, "%s.. no extension..has slash", __func__ );
		}
	}
	else
	{
        LogMsg( LOG_ERROR, "%s.. no extension", __func__ );
	}

	return false;
}

//============================================================================
bool VxFileUtil::replaceExtension( std::string& fileName, std::string newExtension )
{
	if( newExtension.empty() || fileName.empty() )
	{
		LogMsg( LOG_ERROR, "%s empty param", __func__ );
		return false;
	}

	std::size_t foundPos = fileName.find_last_of( "." );
	if( std::string::npos == foundPos || 0 == foundPos )
	{
		LogMsg( LOG_ERROR, "%s last . not found %s", __func__, fileName.c_str() );
		return false;
	}

	foundPos++; // move past .
	size_t lenToEnd = fileName.length() - foundPos;
	if( lenToEnd <= 0 )
	{
		LogMsg( LOG_ERROR, "%s len to end invalid", __func__ );
		return false;
	}

	fileName.replace( foundPos, lenToEnd, newExtension.c_str() );
	return true;
}

//============================================================================
//! separate Path and file name into separate strings
int32_t VxFileUtil::seperatePathAndFile(	std::string &	strFullPath,	// path and file name			
										std::string &	strRetPath,		// return path to file
										std::string &	strRetFile )	// return file name
{
	return seperatePathAndFile( strFullPath.c_str(), strRetPath, strRetFile );
}

//============================================================================
//! separate Path and file name into separate strings
int32_t VxFileUtil::seperatePathAndFile(	const char*	pFullPath,		// path and file name			
											std::string &	strRetPath,		// return path to file
											std::string &	strRetFile )	// return file name
{
	if( VFileIsProviderFile( pFullPath ) )
	{
		return GetVirtFileMgr().seperatePathAndFile( pFullPath, strRetPath, strRetFile ) ? 0 : -1;
	}

	char as8Buf[ VX_MAX_PATH ];
	strcpy( as8Buf, pFullPath );
	makeForwardSlashPath( as8Buf );
	char * pForwardSlash = strrchr( as8Buf, '/' );
	if( pForwardSlash )
	{
		// get file name
		strRetFile = &pForwardSlash[1];
		// get path
		pForwardSlash[1] = 0;
		strRetPath = as8Buf;
		return 0;
	}
	else
	{
		strRetPath = "";
		strRetFile = pFullPath;
	}

	return 0;
}

//============================================================================
//! remove the path and return just the file name
void	VxFileUtil::getFileName(	const char*	pFullPath,				// file name may be full or just file name
										std::string&	strRetJustFileName )	// return file name
{
	std::string	strRetPath;
	int32_t rc = seperatePathAndFile(	pFullPath,				// path and file name			
									strRetPath,				// return path to file
									strRetJustFileName );	// return file name
	if( rc )
	{
		LogMsg( LOG_ERROR, "getFileName: error %d file %s", rc, pFullPath );
		strRetJustFileName = pFullPath;
	}
}

//============================================================================
//! remove the file name and return just the path
std::string VxFileUtil::getJustPath( std::string fullPath )	// file name and path
{
    size_t i = fullPath.rfind( '/', fullPath.length() );
    if ( i != std::string::npos )
    {
        return( fullPath.substr( 0, i) );
    }

    return("");
}

//============================================================================
//! get the . extension of file name
void VxFileUtil::getFileExtension(	std::string&	strFileName,	// file name with extension
									std::string&	strRetExt )		// return extension ( ie "myfile.etm" would return etm"
{
	int iIdx;
	if( -1 != (iIdx = StdStringReverseFind( strFileName, '.') ) )
	{
		strRetExt = &(((const char*)strFileName.c_str())[iIdx + 1]);
	}
	else
	{
		//ErrMsgBox("No File Extension Found %s", csFileName.c_str() );
		strRetExt = "";
	}
}

//============================================================================
//! flip back slashes into forward slashes
void VxFileUtil::makeForwardSlashPath( std::string & csFilePath )
{
	if( csFilePath.empty() )
	{
		return;
	}

	for( size_t i = 0; i < csFilePath.size(); i++ )
	{
		if( '\\' == csFilePath[i] )
		{
			csFilePath[i] = '/';
		}
	}
}

//============================================================================
//! flip back slashes into forward slashes
void VxFileUtil::makeForwardSlashPath( char * pFilePath )
{
	size_t iLen = strlen( pFilePath );
	for( size_t i = 0; i < iLen; i++ )
	{
		if( '\\' == pFilePath[i] )
			pFilePath[i] = '/';
	}
}

//============================================================================
//! flip back slashes into forward slashes
void VxFileUtil::makeBackwardSlashPath( std::string & csFilePath )
{
	if( csFilePath.empty() )
	{
		return;
	}

	for( size_t i = 0; i < csFilePath.size(); i++ )
	{
		if( '/' == csFilePath[i] )
		{
			csFilePath[i] = '\\';
		}
	}
}

//============================================================================
//! flip back slashes into forward slashes
void VxFileUtil::makeBackwardSlashPath( char * pFilePath )
{
	size_t iLen = strlen( pFilePath );
	for( size_t i = 0; i < iLen; i++ )
	{
		if( '/' == pFilePath[i] )
			pFilePath[i] = '\\';
	}
}

//============================================================================
//! return true if last char is '/' else '\\'
bool VxFileUtil::doesPathEndWithSlash( const char* pFileName )
{
	int iStrLen = strlen( pFileName );
	if( ('/' == pFileName[iStrLen-1]) ||
		('\\' == pFileName[iStrLen-1]) )
	{
		return true;
	}
	return false;
}

//============================================================================
//! append slash if needed
void VxFileUtil::assurePathEndWithSlash( std::string &csFileName )
{
    if (!doesPathEndWithSlash(csFileName.c_str()))
    {
        csFileName += '/';
    }
}

//============================================================================
//! return true if is a root path like C:\dir or /dir
bool VxFileUtil::isFullPath( const char* pFileName )
{
	bool isFullPath = false;
	if( pFileName )
	{
		int iStrLen = strlen( pFileName );
		if( iStrLen )
		{
			if('/' == pFileName[0] )
			{
				// linux full path
				isFullPath = true;
			}
			else if( iStrLen > 1 )
			{
				if( ':' == pFileName[ 1 ] )
				{
					// windows full path
					isFullPath = true;
				}
			}
		}
	}
	return isFullPath;
}

//============================================================================
//! Make full path to execute directory if full path was not specified
//! NOTE: be careful .. assumes pFileName has enough space for full path and file name
void VxFileUtil::makeFullPath( char * pFileName )
{
	if( false == isFullPath(pFileName) )
	{
		// add path to the data directory
		std::string csFullPath;
		getExecuteDirectory( csFullPath );
		csFullPath += pFileName;
		strcpy( pFileName, csFullPath.c_str() );
	}
}

//============================================================================
//! Make full path to given directory if full path was not specified.. make path if does not exist
void VxFileUtil::makeFullPath( const char* pShortFileName, const char* pDownloadDir, std::string & strRetPath )
{
	if( isFullPath( pShortFileName ) )
	{
		strRetPath = pShortFileName;
	}
	else
	{
		strRetPath = pDownloadDir;
		if( false == doesPathEndWithSlash(pDownloadDir))
		{
			strRetPath += "/";
		}

		strRetPath += pShortFileName;
	}
}

//============================================================================
//! Make short FileName.. if pDownloadDir and full path contains pDownloadDir then will be path in that dir else just filename
bool VxFileUtil::makeShortFileName( const char* pFullFileName, std::string & strRetShortName, const char* pDownloadDir )
{
	bool bUsedDownloadDir = false;
	if( isFullPath( pDownloadDir ) )
	{
		int iDirStrLen = strlen( pDownloadDir );
		int iFileStrLen = strlen( pFullFileName );
		if( iFileStrLen > iDirStrLen )
		{
			if( 0 == strncmp( pDownloadDir, pFullFileName, iDirStrLen ) )
			{
				strRetShortName = &pFullFileName[ iDirStrLen ];
				bUsedDownloadDir = true;
			}
			else
			{
				getFileName( pFullFileName, strRetShortName);
			}
		}
		else
		{
			// return just file name
			getFileName( pFullFileName, strRetShortName);
		}
	}
	else if( isFullPath( pFullFileName ) )
	{
		// return just file name
		std::string strFileName;
		std::string strPath;
		int32_t rc = seperatePathAndFile(	pFullFileName,	// path and file name			
			strPath,		// return path to file
			strFileName );	// return file name
		if( 0 == rc )
		{
			//! separate Path and file name into separate strings
			strRetShortName = strFileName;
		}
		else
		{
			LogMsg( LOG_ERROR, "makeShortFileName: invalid file %s", pFullFileName );
			strRetShortName = pFullFileName; 
		}
	}
	else
	{
		strRetShortName = pFullFileName; 
	}
	return bUsedDownloadDir;
}

//============================================================================
//! Get execution full path
int32_t	VxFileUtil::getExecuteFullPathAndName( std::string& strRetExePathAndFileName )
{
	std::string strRetExeDir;
	std::string strRetExeFileName;
	int32_t rc = getExecutePathAndName( strRetExeDir, strRetExeFileName );
	strRetExePathAndFileName = strRetExeDir + strRetExeFileName;
	return rc;
}

//============================================================================
//! Get directory we execute from
int32_t	VxFileUtil::getExecuteDirectory( std::string& strRetExeDir )
{
	std::string strRetExeFileName;
	return getExecutePathAndName( strRetExeDir, strRetExeFileName );
}

//============================================================================
//! Get execution path and file name
int32_t	VxFileUtil::getExecutePathAndName( std::string& strRetExeDir, std::string& strRetExeFileName )
{
#ifdef TARGET_OS_WINDOWS
	wchar_t pRetBuf[ VX_MAX_PATH ];
	int iRetStrLen = GetModuleFileNameW( nullptr, pRetBuf, VX_MAX_PATH );
	if( 0 != iRetStrLen )
	{
		// remove file name
		wchar_t * pTemp = wstrrchr( pRetBuf, '\\' );
		if( pTemp )
		{
			* pTemp = 0;
			pTemp++;
			strRetExeFileName = WideToUtf8( pTemp );
		}
		// remove debug path if exists
		pTemp = wstrrchr( pRetBuf, '\\' );
		if( pTemp )
		{
#ifdef _DEBUG
			if( 0 == wstrcmp( pTemp, L"\\DEBUG" ) ||
				0 == wstrcmp( pTemp, L"\\Debug" ) )
			{
				*pTemp = 0;
			}
#endif
		}
		// make sure has the final slash
		if( pRetBuf[ wstrlen( pRetBuf ) - 1 ] != '\\' )
		{
			wstrcat( pRetBuf, L"\\" );
		}
		// flip the slashes
		size_t uiStrLen = wstrlen( pRetBuf );
		for( size_t i = 0; i < uiStrLen; i++ )
		{
			if( L'\\' == pRetBuf[ i ] )
			{
				pRetBuf[ i ] = L'/';
			}
		}

		strRetExeDir = WideToUtf8( pRetBuf );
		return 0;
	}
	else
	{
		LogMsg( LOG_INFO, "Error %d occurred getting module directory", VxGetLastError());
		return -1;
	}

#else // LINUX
	char pRetBuf[ VX_MAX_PATH ];
	int iByteCount;
	char* pTempBuf;
	iByteCount = readlink("/proc/self/exe", pRetBuf, VX_MAX_PATH);
	if(-1 == iByteCount)
	{
		LogMsg( LOG_INFO, "Error %d occured getting module directory", VxGetLastError());
		return -1;
	}

    if( iByteCount >= VX_MAX_PATH )
    {
        LogMsg( LOG_INFO, "Error %d occured iByteCount %d", VxGetLastError(), iByteCount );
        return -1;
    }

	pRetBuf[iByteCount] = '\0';

	if(nullptr == (pTempBuf = strrchr(pRetBuf,'/')))
	{
		LogMsg( LOG_INFO, "Error %d occured getting module directory", VxGetLastError());
		return -1;
	}

    strRetExeFileName = &pTempBuf[1];
	pTempBuf[1] = '\0';
	strRetExeDir = pRetBuf;
#endif // LINUX
	return 0;
}

//============================================================================
bool VxFileUtil::fileNameWildMatch( const char  * pMatchName, const char* pWildName )
{
//  test if a file name matches a file name pattern.
//   handles * and ? wildcard characters.

    int   bNoMatch = false;
    int   bDone = false;
    while (false == bDone)
    {
        switch (*pWildName)
        {
            case '*':
                // skip to last . or end of Name
				if(strchr(pWildName, '.'))
				{
					while(strchr(pWildName, '.'))
					{
						pWildName = strchr(pWildName, '.');
						pWildName++;
					}
					//pWildName--;
				}
				else
				{
					while (* pWildName)
						pWildName++;
				}
                // skip to last . or end of Name
				if( strchr(pMatchName, '.'))
				{
					while( strchr(pMatchName, '.'))
					{
						pMatchName = strchr(pMatchName, '.');
						pMatchName++;
					}
					//pMatchName--;
				}
				else
				{
					while (* pMatchName)
						pMatchName++;
				}
                break;

            case '?':
                pWildName++;
                pMatchName++;
                break;

            case 0:
                if (*pMatchName != 0)
                    bNoMatch = true;
                bDone = true;
                break;

            default:
                if (toupper(* pWildName) == toupper(* pMatchName))
                {
                    pWildName++;
                    pMatchName++;
                }
                else
                {
                    bNoMatch = true;
                    bDone = true;
                }
                break;
        }
    }
	if( bNoMatch )
		return false ;
	return true;
}

//============================================================================
//! allocate memory and read whole file into memory
//! NOTE: USER MUST DELETE THE RETURED POINTER OR MEMORY LEAK WILL OCCURE
int32_t	VxFileUtil::readWholeFile(	const char*		pFileName,			// file to read	
									void **			ppvRetBuf,			// return allocated buffer it was read into
                                    uint32_t *		pu32RetLenOfFile )	// return length of file
{
	int32_t rc = 0;
	uint32_t u32Len = (uint32_t)getFileLen( pFileName );
	if( 0 == u32Len )
	{
		rc = VxGetLastError();
		if( 0 == rc )
		{
			rc = -1;
		}
		LogMsg( LOG_ERROR, "readWholeFile: Could not open file %s", pFileName );
		return rc;
	}
	else
	{
		char * pTemp = new char[ u32Len + 16 ];
		int32_t rc = readWholeFile( pFileName, pTemp, u32Len, nullptr );
		if( rc )
		{
			// error occurred so delete so no memory leak
			delete[] pTemp;
			* ppvRetBuf = 0;
			* pu32RetLenOfFile = 0;
		}
		else
		{
			// make sure is null terminated
			pTemp[ u32Len ] = 0;
			* ppvRetBuf = pTemp;
			* pu32RetLenOfFile = u32Len;
		}
		return rc;
	}
}

//============================================================================
//! read whole file of known length into existing buffer
//! NOTE assumes buffer has enough room for the whole file
int32_t VxFileUtil::readWholeFile(	const char*		pFileName,				// file to read
									void *			pvBuf,					// buffer to read into
                                    uint32_t		u32LenToRead,			// length to read ( assumes is same as file length
                                    uint32_t*		pu32RetAmountRead )		// return length actually read if not null
{
	int32_t rc = 0;
	if( pu32RetAmountRead  )
	{
		* pu32RetAmountRead = 0;
	}

	if( fileIsProviderFile( pFileName ) )
	{
		VFile* poFile = VFileOpen( pFileName, "rb" );
		if( nullptr == poFile )
		{
			rc = VxGetLastError();
			LogMsg( LOG_INFO, "readWholeFile: error %d opening file %s", rc, pFileName );
			return rc;
		}

		size_t iResult = VFileRead( pvBuf, 1, u32LenToRead, poFile );
		VFileClose( poFile );
		if( iResult > 0 )
		{
			if( pu32RetAmountRead )
			{
				*pu32RetAmountRead = (uint32_t)iResult;
			}
			// null terminate it
			if( u32LenToRead > iResult )
			{
				((char*)pvBuf)[iResult] = 0;
			}
			return 0;
		}

		rc = VxGetLastError();
		if( 0 == rc )
		{
			rc = -1;
		}

		LogMsg( LOG_INFO, "readWholeFile: error %d reading file %s", rc, pFileName );
		return rc;
	}

	FILE* poFile = fopen( pFileName, "rb" );
	if( nullptr == poFile )
	{
		rc = VxGetLastError();
		LogMsg( LOG_INFO, "readWholeFile: error %d opening file %s", rc, pFileName );
		return rc;
	}

	size_t iResult = fread( pvBuf, 1, u32LenToRead, poFile );
	fclose( poFile );
	if( iResult > 0 )
	{
		if( pu32RetAmountRead )
		{
			*pu32RetAmountRead = (uint32_t)iResult;
		}
		// null terminate it
		if( u32LenToRead > iResult )
		{
			((char*)pvBuf)[iResult] = 0;
		}
		return 0;
	}

	rc = VxGetLastError();
	if( 0 == rc )
	{
		rc = -1;
	}

	LogMsg( LOG_INFO, "readWholeFile: error %d reading file %s", rc, pFileName );
	return rc;
}

//============================================================================
//! allocate memory and read whole file into memory and decrypt
//! NOTE: USER MUST DELETE THE RETURED POINTER OR MEMORY LEAK WILL OCCURE
int32_t	VxFileUtil::readWholeFile(	VxKey *				poKey,				// key to decrypt with
									const char*			pFileName,			// file to read	
									void **				ppvRetBuf,			// return allocated buffer it was read into
									uint32_t *			pu32RetLenOfFile )	// return length of file
{
	uint32_t		u32FileLen;

	int32_t rc = readWholeFile( pFileName,
		                      ppvRetBuf,
							  pu32RetLenOfFile );
	if( rc )
	{
		return rc;
	}

	u32FileLen = *pu32RetLenOfFile;
	vx_assert( u32FileLen );
	vx_assert( 0 == (u32FileLen & 0x0f ) );
	VxSymDecrypt( poKey, (char *)*ppvRetBuf, u32FileLen );
	return 0;
}

//============================================================================
//! write all of data to a file
int32_t	VxFileUtil::writeWholeFile(	const char*		pFileName,			// file to write to
									void *			pvBuf,				// data to write
                                    uint32_t		u32LenOfData )		// data length
{
	int32_t rc = 0;

	if( fileIsProviderFile( pFileName ) )
	{
		VFile* poFile = VFileOpen( pFileName, "wb+" );
		if( nullptr == poFile )
		{
			rc = VxGetLastError();
			if( 0 == rc )
			{
				rc = -1;
			}
			LogMsg( LOG_ERROR, "writeWholeFile: ERROR %d Could not write file %s %d bytes", rc, pFileName, u32LenOfData );
		}
		else
		{
			uint32_t u32Result = (uint32_t)VFileWrite( pvBuf, 1, u32LenOfData, poFile );
			if( u32Result != u32LenOfData )
			{
				rc = VxGetLastError();
				if( 0 == rc )
				{
					rc = -1;
				}

				LogMsg( LOG_ERROR, "writeWholeFile: ERROR %d Could not write file %s %d bytes", rc, pFileName, u32LenOfData );
			}

			VFileClose( poFile );
		}
		return rc;
	}

	FILE* poFile = fopen( pFileName, "wb+" );
	if( nullptr == poFile )
	{
		rc = VxGetLastError();
		if( 0 == rc )
		{
			rc = -1;
		}
		LogMsg( LOG_ERROR, "writeWholeFile: ERROR %d Could not write file %s %d bytes", rc, pFileName, u32LenOfData );
	}
	else
	{
		uint32_t u32Result = (uint32_t)fwrite( pvBuf, 1, u32LenOfData, poFile );
		if( u32Result != u32LenOfData )
		{
			rc = VxGetLastError();
			if( 0 == rc )
			{
				rc = -1;
			}

			LogMsg( LOG_ERROR, "writeWholeFile: ERROR %d Could not write file %s %d bytes", rc, pFileName, u32LenOfData );
		}

		fclose( poFile );
	}

	return rc;
}

//============================================================================
//! encrypt and write all of data to a file
int32_t VxFileUtil::writeWholeFile(	VxKey *			poKey,				// key to encrypt with
									const char*		pFileName,			// file to write to
									void *			pvBuf,				// data to write
                                    uint32_t		u32LenOfData )		// data length
{
	vx_assert( u32LenOfData );
	vx_assert( VxIsEncryptable( u32LenOfData ) );
	// make copy first
	char * pBuf = new char[ u32LenOfData ];
	memcpy( pBuf, pvBuf, u32LenOfData );
	VxSymEncrypt( poKey, (char *)pBuf, u32LenOfData );
	return writeWholeFile( pFileName, pBuf, u32LenOfData );
}

//============================================================================
int32_t VxFileUtil::listFilesInDirectory(	const char*					pSrcDir,
										std::vector<std::string>&	fileList )
{
	vx_assert( pSrcDir );
#ifdef TARGET_OS_WINDOWS
	std::wstring strSrcDir = Utf8ToWide( pSrcDir );
	// build path and wild card
	wchar_t srcDir[ VX_MAX_PATH * 2 ];
	wchar_t srcFile[ VX_MAX_PATH * 2 ];
	wstrcpy( srcDir, strSrcDir.c_str() );
	wstrcat( srcDir, L"\\*.*" );

	// start working for files
	WIN32_FIND_DATAW FindFileData;

	HANDLE hFind = FindFirstFileW( srcDir, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		// done with listing
		return 0;
	}
	bool bFinished = false;
	while ( false == bFinished)
	{
		// skip . and .. files; otherwise, we'd
		// recur infinitely!
		if( isDotDotDirectory(  FindFileData.cFileName ) )
		{
			if( false == FindNextFileW(hFind, &FindFileData)  )
				break;
			continue;
		}

		// make source file name
		wstrcpy( srcFile, strSrcDir.c_str()  );
		wstrcat( srcFile, FindFileData.cFileName );

		struct _stat m;
		if( 0 != _wstat( srcFile, &m) )
		{
			LogMsg( LOG_ERROR, "VxFileUtil::%s ERROR %d", __func__, VxGetLastError() );
		}
		else
		{
			if( _S_IFDIR & m.st_mode )
			{
				// its a directory
			}
			else
			{
				// its a file
				fileList.push_back( WideToUtf8( srcFile ).c_str() );
			}
		}

		if( false == FindNextFileW(hFind, &FindFileData)  )
		{
			// done with listing
			FindClose(hFind);
			return 0;
		}
	}
	return 0;
#else //LINUX
	// build path and wild card
	char as8SrcDir[ VX_MAX_PATH * 2 ];
	char as8SrcFile[ VX_MAX_PATH * 2 ];
	strcpy( as8SrcDir, pSrcDir );
	// find the files in the directory
	DIR *pDir;
	struct dirent *pFileEnt;
	if( directoryExists( as8SrcDir ) )
	{
		//LogMsg( LOG_INFO, "listFilesInDirectory:  directory %s exists.. opening dir", as8SrcDir );
		//ok directory exists!
		if(!(nullptr == (pDir = opendir(as8SrcDir))))
		{
			//pDir is open
			while( 0 != (pFileEnt = readdir(pDir)))
			{
				//LogMsg( LOG_INFO, "listFilesInDirectory: found file %s", pFileEnt->d_name );
				//got a file or directory
				if( isDotDotDirectory(  pFileEnt->d_name ) )
				{
					// skip . and ..
					//LogMsg( LOG_INFO, "listFilesInDirectory: skipping file %s", pFileEnt->d_name );
					continue;
				}

				// valid directory entry
				// make source file name
				strcpy( as8SrcFile, as8SrcDir );
				if( '/' != as8SrcFile[ strlen( as8SrcFile ) - 1 ] )
				{
					strcat( as8SrcFile, "/" );
				}

				strcat( as8SrcFile, pFileEnt->d_name );
				//LogMsg( LOG_INFO, "listFilesInDirectory:  found file %s", as8SrcFile );
				//=== Last Modification Date ===//
				struct stat m;
				if( 0 != stat( as8SrcFile, &m) )
				{
					///ERROR how do we handle
					LogMsg( LOG_INFO, "listFilesInDirectory: Unable to stat file %s", as8SrcFile );
					continue;
				}
				
				if( pFileEnt->d_type == DT_DIR )
				{
					// its a directory
					LogMsg( LOG_INFO, "listFilesInDirectory: file %s is directory", as8SrcFile );
					continue;
				}

				// its a file
				fileList.push_back( as8SrcFile );
			}
			// end of listing
			// done with listing
			closedir(pDir);
			return 0;
		}
		else
		{
			LogMsg( LOG_INFO, "listFilesInDirectory:  could not open directory %s ", as8SrcDir );
		}
	}
	else
	{
		LogMsg( LOG_INFO, "listFilesInDirectory:  directory %s does not exist ", as8SrcDir );
	}
	return 0;
#endif //LINUX

}

//============================================================================
int32_t VxFileUtil::listFilesAndFolders( const char* pSrcDir, std::vector<VxFileInfoBase>& fileList, uint8_t fileFilterMask )
{
	vx_assert( pSrcDir );

	if( fileIsProviderFile( pSrcDir ) )
	{
		return GetVirtFileMgr().listProviderFilesAndFolders( pSrcDir, fileList, fileFilterMask );
	}

#ifdef TARGET_OS_WINDOWS
	std::wstring strSrcDir = Utf8ToWide( pSrcDir );
	// build path and wild card
	wchar_t srcDir[ VX_MAX_PATH * 2 ];
	wchar_t srcFile[ VX_MAX_PATH * 2 ];
	wstrcpy( srcDir, strSrcDir.c_str() );
	wstrcat( srcDir, L"\\*.*" );

	// start working for files
	WIN32_FIND_DATAW FindFileData;

	HANDLE hFind = FindFirstFileW( srcDir, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		// done with listing
		return 0;
	}
	bool bFinished = false;
	while( false == bFinished )
	{
		// skip . and .. files; otherwise, we'd
		// recur infinitely!
		if( isDotDotDirectory(  FindFileData.cFileName ) )
		{
			if( false == FindNextFileW(hFind, &FindFileData)  )
				break;
			continue;
		}

		// make source file name
		wstrcpy( srcFile, strSrcDir.c_str()  );
		wstrcat( srcFile, FindFileData.cFileName );

		struct _stat oStat;
		if( 0 != _wstat( srcFile, &oStat) )
		{
			//LogMsg( LOG_ERROR, "VxFileUtil::listFilesAndFolders ERROR %d", VxGetLastError() );
		}
		else
		{
			std::string fileNameAndPath = WideToUtf8( srcFile ).c_str();
			std::string fileNameOnly;
			std::string filePath;
			if( _S_IFDIR & oStat.st_mode )
			{
				fileNameOnly = fileNameAndPath;
			}
			else
			{
				VxFileUtil::seperatePathAndFile( fileNameAndPath, filePath, fileNameOnly );
			}

            VxFileInfoBase fileInfo( fileNameOnly.c_str(), fileNameAndPath.c_str(), (int64_t)oStat.st_size );

			if( _S_IFDIR & oStat.st_mode )
			{
				// its a directory
				fileInfo.setFileType( VXFILE_TYPE_DIRECTORY );
				fileInfo.assureTrailingDirectorySlash();
			}
			else
			{
				// its a file
				fileInfo.setFileType( fileExtensionToFileTypeFlag( fileInfo.getFileName().c_str() ) );
			}

			if( 0 != ( fileInfo.getFileType() & fileFilterMask ) )
			{
				fileList.push_back( fileInfo );
			}
		}

		if( false == FindNextFileW(hFind, &FindFileData)  )
		{
			// done with listing
			FindClose(hFind);
			return 0;
		}
	}

	return 0;
#else //LINUX
	// build path and wild card
	char as8SrcDir[ VX_MAX_PATH * 2 ];
	char as8SrcFile[ VX_MAX_PATH * 2 ];
	strcpy( as8SrcDir, pSrcDir );
	// find the files in the directory
	DIR *pDir;
	struct dirent *pFileEnt;
	if( directoryExists( as8SrcDir ) )
	{
		//LogMsg( LOG_INFO, "listFilesAndFolders:  directory %s exists.. opening dir", as8SrcDir );
		//ok directory exists!
		if(!(nullptr == (pDir = opendir(as8SrcDir))))
		{
			//pDir is open
			while( 0 != (pFileEnt = readdir(pDir)))
			{
				//LogMsg( LOG_INFO, "listFilesAndFolders: found file %s", pFileEnt->d_name );
				//got a file or directory
				if( isDotDotDirectory(  pFileEnt->d_name ) )
				{
					// skip . and ..
					//LogMsg( LOG_INFO, "listFilesAndFolders: skipping file %s", pFileEnt->d_name );
					continue;
				}

				// valid directory entry
				// make source file name
				strcpy( as8SrcFile, as8SrcDir );
				if( '/' != as8SrcFile[ strlen( as8SrcFile ) - 1 ] )
				{
					strcat( as8SrcFile, "/" );
				}

				strcat( as8SrcFile, pFileEnt->d_name );
				//LogMsg( LOG_INFO, "listFilesAndFolders:  found file %s", as8SrcFile );
				//=== Last Modification Date ===//
				struct stat oStat;
				if ( 0 != stat( as8SrcFile, &oStat ) )
				{
					///ERROR how do we handle
					LogMsg( LOG_ERROR, "listFilesAndFolders: ERROR %d when stat file %s", VxGetLastError(), as8SrcFile );
					continue;
				}

                VxFileInfoBase fileInfo( pFileEnt->d_name, as8SrcFile );
                fileInfo.setFileLength( oStat.st_size );

				if( pFileEnt->d_type == DT_DIR )
				{
					// its a directory
                    fileInfo.setFileType( VXFILE_TYPE_DIRECTORY );
					fileInfo.assureTrailingDirectorySlash();
					LogMsg( LOG_INFO, "listFilesAndFolders: Is Directory %s", fileInfo.getFileName().c_str() );
				}
				else
				{
					// its a file
					LogMsg( LOG_INFO, "listFilesAndFolders: Is File %s", fileInfo.getFileName().c_str() );
                    fileInfo.setFileType( fileExtensionToFileTypeFlag( fileInfo.getFileName().c_str() ) );
				}

				if ( 0 != ( fileInfo.getFileType() & fileFilterMask ) )
				{
					fileList.push_back( fileInfo );
				}
				else
				{
					LogMsg( LOG_INFO, "listFilesAndFolders: Skip file type 0x%x not in filter mask 0x%x File %s", fileInfo.getFileType(), fileFilterMask, fileInfo.getFileName().c_str() );			
				}
			}

			// end of listing
			// done with listing
			closedir(pDir);
			return 0;
		}
		else
		{
			LogMsg( LOG_INFO, "listFilesInDirectory:  could not open directory %s", as8SrcDir );
		}
	}
	else
	{
		LogMsg( LOG_INFO, "listFilesInDirectory:  directory %s does not exist", as8SrcDir );
	}

	return 0;
#endif //LINUX
}

//============================================================================
bool VxFileUtil::deleteFilesInFolder( std::string fileFolder, bool folderNameEndsWithOnlineId )
{
	bool result = !fileFolder.empty() && directoryExists( fileFolder.c_str() );
	if( !result )
	{
		LogMsg( LOG_ERROR, "deleteFilesInFolder directory does not exist" );
		return result;
	}

	if( folderNameEndsWithOnlineId )
	{
		std::string noTrailingSlash = fileFolder;
		removeTrailingDirectorySlash( noTrailingSlash );
		std::size_t foundPos = noTrailingSlash.find_last_of( "/" );
		if( std::string::npos == foundPos )
		{
			LogMsg( LOG_ERROR, "deleteFilesInFolder directory %s does not contain backslash", fileFolder.c_str() );
			return false;
		}

		VxGUID onlineId;
		onlineId.fromVxGUIDHexString( &( noTrailingSlash.c_str()[foundPos + 1]) );
		if( !onlineId.isValid() )
		{
			LogMsg( LOG_ERROR, "deleteFilesInFolder directory %s does not end with online id", fileFolder.c_str() );
			return false;
		}
	}

	std::vector<std::string> fileList;
	listFilesInDirectory( fileFolder.c_str(), fileList );
	for( auto& fileName : fileList )
	{
		result &= 0 == deleteFile( fileName.c_str() );
	}

	return result;
}

//============================================================================
// Extract base name and numeric suffix if it exists
std::pair<std::string, int> parseFileNameIncrement( const std::string& baseName ) 
{
	std::regex re( R"(^(.*)\s\((\d+)\)$)" );
	std::smatch match;

	if( std::regex_match( baseName, match, re ) ) 
	{
		std::string name = match[1];
		int number = std::stoi( match[2] );
		return { name, number };
	}

	return { baseName, 0 };
}

//============================================================================
bool VxFileUtil::incrementFileName( std::string& strFileName )
{
	std::string fileNamePart;
	std::string extensionPart;
	seperateFileNameAndExtension(	strFileName,		// file name with extension				
		fileNamePart,		// return file name part without .ext
		extensionPart );	// return .ext part
	if( fileNamePart.empty() )
	{
		return false;
	}

	// Parse for existing (n) suffix
	std::pair<std::string, int> suffixPair = parseFileNameIncrement( fileNamePart );
	std::string namePart = suffixPair.first;

	int incNum = suffixPair.second;
	if( 0 == incNum )
	{
		// does not have a (n) suffix
		strFileName = fileNamePart + " (1)" + extensionPart;
	}
	else
	{
		if( namePart.empty() )
		{
			return false;
		}

		incNum++;
		strFileName = namePart + " (" + std::to_string( incNum ) + ")" + extensionPart;
	}

	return true;
}

//============================================================================
uint8_t VxFileUtil::fileExtensionToFileTypeFlag( const char* pFileName )
{
	uint8_t fileType = VXFILE_TYPE_OTHER;
	if( pFileName )
	{
		std::string fileName = pFileName;
		if( fileName.size() )	
		{
			if ( '/' == pFileName[fileName.size() - 1] )
			{
				fileType = VXFILE_TYPE_DIRECTORY;
			}
			else
			{
				std::string fileExt = "";
				getFileExtension( fileName, fileExt );
				if( fileExt.size() )
				{
					fileType = VxFileExtensionToFileTypeFlag( fileExt.c_str() );
				}							
			}
		}
	}
	
	return fileType;
}

//============================================================================
uint64_t VxFileUtil::getDiskFreeSpace( const char* filePath  )
{
	uint64_t totalSpace;
	uint64_t freeSpace = 0;
	getDiskSpace( filePath, totalSpace, freeSpace );
	return freeSpace;
}

//============================================================================
bool VxFileUtil::getDiskSpace( const char* filePath, uint64_t& totalDiskSpace, uint64_t& diskSpaceAvail )
{
	totalDiskSpace = 0;
	diskSpaceAvail = 0;

#ifdef WIN32
	std::string dir = filePath;
	if( dir.length() > 2 )
	{
		dir = dir.substr( 0, 2 );

		ULARGE_INTEGER FreeBytesAvailable;
		ULARGE_INTEGER TotalNumberOfBytes;
		ULARGE_INTEGER TotalNumberOfFreeBytes;

        BOOL result = GetDiskFreeSpaceExA(	dir.c_str(),
											&FreeBytesAvailable,
											&TotalNumberOfBytes,
											&TotalNumberOfFreeBytes
											);
		if( result )
		{
			totalDiskSpace = TotalNumberOfBytes.QuadPart;
			diskSpaceAvail = FreeBytesAvailable.QuadPart;
			return true;
		}
		else
		{
			LogMsg( LOG_ERROR, "getDiskSpace %s error %d", filePath, GetLastError() );
		}
	}
	return false;
#else
	struct statfs sfs;
	if (statfs(filePath, &sfs) == 0)
	{
		totalDiskSpace = (uint64_t)sfs.f_blocks * sfs.f_bsize;
		diskSpaceAvail = (uint64_t)sfs.f_bavail * sfs.f_bsize;
		return true;
	}
/*#else
	struct statvfs sfs;
	if (statvfs(disk, &sfs) == 0)
	{
		totalDiskSpace = (uint64_t)sfs.f_blocks * sfs.f_bsize;
		diskSpaceAvail = (uint64_t)sfs.f_bavail * sfs.f_bsize;
		return true;
	}
#endif*/
#endif
	return false;
}

//============================================================================
bool VxFileUtil::u64ToHexAscii( uint64_t fileLen, std::string& retHexAscii  )
{
	return dataToHexAscii( (uint8_t *)&fileLen, 8, retHexAscii );
}

//============================================================================
bool VxFileUtil::hexAsciiToU64( const char* hexAscii, uint64_t& retFileLen  )
{
	return hexAsciiToData( hexAscii, (uint8_t *)&retFileLen, 8  ); 
}

//============================================================================
bool VxFileUtil::dataToHexAscii( uint8_t * dataBuf, int dataLen, std::string& retHexAscii  )
{
	if( ( 0 == dataBuf ) || ( 0 == dataLen ) )
	{
		retHexAscii = "";
		return false;
	}

	char * hexBuf = new char[ dataLen * 2 + 1 ];
	int strIdx = 0;
	for( int i = 0; i < dataLen; ++i )
	{
		hexBuf[ strIdx++ ] = binaryToHexChar( dataBuf[ i ] >> 4 );
		hexBuf[ strIdx++ ] = binaryToHexChar( dataBuf[ i ] );
	}

	hexBuf[ strIdx ] = 0;
	retHexAscii = hexBuf;
	delete[] hexBuf;
	return true;
}

//============================================================================
bool VxFileUtil::hexAsciiToData( const char* hexAscii, uint8_t * retDataBuf, int dataLen  )
{
	int strLen = std::min( (int)strlen( hexAscii ), dataLen);
	uint8_t u8Byte;
	int byteIdx = 0;
	for( int i = 0; i < strLen; ++i )
	{
		u8Byte = (charToHexBinary(*hexAscii))<<4;
		hexAscii++;
		u8Byte |= charToHexBinary(*hexAscii);
		hexAscii++;
		retDataBuf[ byteIdx++ ] = u8Byte;
		if( byteIdx >= dataLen )
		{
			break;
		}
	}

	return true;
}

//============================================================================
uint8_t  VxFileUtil::charToHexBinary( char cVal )
{
	if( cVal >= '0' && cVal <= '9' )
	{
		return cVal & 0x0f;
	}
	else if( cVal >= 'A' && cVal <= 'F' )
	{
		return cVal - 55;
	}
	else if( cVal >= 'a' && cVal <= 'f' )
	{
		return cVal - 87;
	}
	else
	{
		LogMsg( LOG_ERROR, "VxGUID::charToHex invalid char 0x%2.2x", cVal );
		return 0;
	}
}

//============================================================================
char VxFileUtil::binaryToHexChar( uint8_t u8Val )
{
	uint8_t byteVal = u8Val & 0x0f;
	if( byteVal < 10 )
	{
		return byteVal + '0';
	}

	return ( byteVal - 9 ) + 'A';
}

//============================================================================
bool VxFileUtil::testIsWritablePath( std::string writeablePath )
{
	bool result{ false };
	assurePathEndWithSlash( writeablePath );
	makeDirectory( writeablePath );
	writeablePath += TEST_WRITEABLE_FILE_NAME;

	// Virtual file system is not available yet.. use normal file io
	FILE* fileHandle = fopen( writeablePath.c_str(), "wb+" );
	if( fileHandle )
	{
		const char* testStr = "1234\n";
		char buf[10];
		strcpy( buf, testStr );
		int writeCnt = fwrite( buf, 1, strlen( testStr ), fileHandle );
		if( writeCnt == strlen( testStr ) )
		{
			result = true;
		}

		fclose( fileHandle );
		deleteFile( writeablePath.c_str() );
	}
	else
	{
        LogMsg( LOG_WARN, "VxFileUtil::testIsWritablePath errno %d could not open file %s", VxGetLastError(), writeablePath.c_str() );
	}

	return result;
}

//============================================================================
std::string VxFileUtil::describeDiskSpace( std::string pathOnDisk )
{
	uint64_t totalDiskSpace{ 0 };
	uint64_t diskSpaceAvail{ 0 };
	bool resultOk = getDiskSpace( pathOnDisk.c_str(), totalDiskSpace, diskSpaceAvail );
	if( resultOk )
	{
        char resultSpace[256];
        sprintf( resultSpace, "Avail %s of %s total disk space", describeFileSize( diskSpaceAvail ).c_str(),
                describeFileSize( totalDiskSpace ).c_str() );
        return resultSpace;
	}
	else
	{
		char resultErr[128];
		sprintf( resultErr, "disk space error %d", VxGetLastError() );
		LogMsg( LOG_ERROR, "VxFileUtil::describeDiskSpace %s", resultErr );
		return resultErr;
	}
}

//============================================================================
std::string VxFileUtil::describeFileSize( uint64_t fileLen )
{
	double dSize = (double)fileLen;
	char as8Buf[32];
	char as8Suffix[32];
	if( fileLen < 1000 )
	{
		sprintf( as8Buf, "%d", (uint32_t)fileLen );
		strcpy( as8Suffix, "  B" );
	}
	else if( fileLen < 1000000 )
	{
		sprintf( as8Buf, "%3.1f", dSize/1000 );
		strcpy( as8Suffix, " KB" );
	}
	else if( fileLen < 1000000000 )
	{
		sprintf( as8Buf, "%3.1f", dSize/1000000 );
		strcpy( as8Suffix, " MB" );
	}
	else if( fileLen < 1000000000000 )
	{
		sprintf( as8Buf, "%3.1f", dSize/1000000000 );
		strcpy( as8Suffix, " GB" );
	}
	else if( fileLen < 1000000000000000 )
	{
		sprintf( as8Buf, "%3.1f", dSize/1000000000000 );
		strcpy( as8Suffix, " TB" );
	}
	else if( fileLen < 1000000000000000000 )
	{
		sprintf( as8Buf, "%3.1f", dSize/1000000000000000 );
		strcpy( as8Suffix, " PB" );
	}
	else
	{
		strcpy( as8Buf, "???" );
		strcpy( as8Suffix, " ??" );
	}

	char textBuf[32];
	textBuf[0] = 0;
	size_t iLen = strlen( as8Buf );
	if( iLen > 4 )
	{
		// too long.. remove the decimal point
		char * pTemp = strchr( as8Buf, '.' );
		if( pTemp )
		{
			pTemp[0] = pTemp[2];
			pTemp[1] = 0;
		}
		iLen = strlen( as8Buf );
	}
	if( iLen < 4 )
	{
		// fill in leading spaces
		for( size_t i = 0; i < ( 4 - iLen ); i++ )
		{
			textBuf[i] = ' ';
			textBuf[i+1] = 0;
		}
	}
	strcat( textBuf, as8Buf );
	strcat( textBuf, as8Suffix );
	return textBuf;
}
