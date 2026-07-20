//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VFileMgr.h"

#include <VirtStream/VirtProviderFile.h>

#include <CoreLib/AssetDefs.h>
#include <CoreLib/VxAndroid.h>
#include <CoreLib/VxDefs.h>
#include <CoreLib/VFile.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxGUID.h>

//============================================================================
VirtProviderFile* VFileMgr::findProviderFile( VFile* fp )
{

    auto iter = std::find_if(m_ProviderFiles.begin(), m_ProviderFiles.end(),
                             [&](VirtProviderFile* file) { return file->m_VFile == fp; });
    if( iter != m_ProviderFiles.end() )
    {
        return *iter;
    }


    return nullptr;
}

//============================================================================
bool VFileMgr::providerDirectoryExists( std::string dirPath )
{
    return VxAndroid::directoryExists( dirPath.c_str() );
}

//============================================================================
uint64_t VFileMgr::providerFileExists( std::string fileName )
{
    uint64_t fileLen{0};

    VirtProviderFile* providerFile = new VirtProviderFile( fileName );
    if( providerFile->openReadOnly() )
    {
        fileLen = providerFile->size();
        providerFile->closeFile();
    }

    delete providerFile;


    return fileLen;
}

//============================================================================
VFile* VFileMgr::providerFileOpen( std::string fileNameIn, std::string fileMode )
{
    VirtProviderFile* providerFile = new VirtProviderFile( fileNameIn );
    if( providerFile->openReadOnly() )
    {
		VFile* vFile = new VFile();
		memset( vFile, 0, sizeof( VFile ) );
        vFile->m_ProviderFileType = 1;
        vFile->m_FileLen = providerFile->size();

        providerFile->m_FileMode = fileMode;
        providerFile->m_FileName = fileNameIn;
        providerFile->m_VFile = vFile;

        lockProviderMgr();
        m_ProviderFiles.emplace_back(providerFile);
        unlockProviderMgr();

        return vFile;
    }

    delete providerFile;



	return nullptr;
}

//============================================================================
int VFileMgr::providerFileClose( VFile* fp )
{
	int retVal = -1;

	lockProviderMgr();
    auto iter = std::find_if(m_ProviderFiles.begin(), m_ProviderFiles.end(), 
                              [&](VirtProviderFile* file) { return file->m_VFile == fp; });
    if( iter != m_ProviderFiles.end() )
    {
        VirtProviderFile* providerFile = *iter;
        providerFile->closeFile();
        delete providerFile;
		m_ProviderFiles.erase( iter );
        retVal = 0;
    }

	unlockProviderMgr();


	return retVal;
}

//============================================================================
int VFileMgr::providerFileEof( VFile* fp )
{
    bool eof{ false };

	lockProviderMgr();
    VirtProviderFile* providerFile = findProviderFile( fp );
    if( !providerFile )
	{
		LogMsg( LOG_ERROR, "VFileMgr::%s wrong VFile", __func__ );
		unlockProviderMgr();
		vx_assert( false );
		return 0;
	}

    eof = fp->m_FileOffs == fp->m_FileLen;
	unlockProviderMgr();


	return eof;
}

//============================================================================
int VFileMgr::providerFileError( VFile* fp )
{
	int retVal = -1;
	lockProviderMgr();
    VirtProviderFile* providerFile = findProviderFile( fp );
    if( !providerFile )
	{
		LogMsg( LOG_ERROR, "VFileMgr::%s wrong VFile", __func__ );
		unlockProviderMgr();
		vx_assert( false );
		return retVal;
	}
	
	unlockProviderMgr();
	return 0;
}

//============================================================================
int VFileMgr::providerFileFlush( VFile* fp )
{
	return 0;
}

//============================================================================
size_t VFileMgr::providerFileRead( void* buf, size_t size, size_t count, VFile* fp )
{
	int retVal = -1;

	lockProviderMgr();
    VirtProviderFile* providerFile = findProviderFile( fp );
    if( !providerFile )
	{
		LogMsg( LOG_ERROR, "VFileMgr::%s wrong VFile", __func__ );
		unlockProviderMgr();
		vx_assert( false );
		return retVal;
	}

    if( !providerFile->isOpen() )
    {
        LogMsg( LOG_ERROR, "VFileMgr::%s file not open", __func__ );
        unlockProviderMgr();
        vx_assert( false );
        return retVal;
    }

	int64_t wantReadLen = size * count;
    int64_t readAttemptLen = std::min( wantReadLen, fp->m_FileLen - fp->m_FileOffs );

    int64_t readLen = providerFile->read( (char*)buf, readAttemptLen );
    if( readLen > 0 )
    {
        fp->m_FileOffs += readLen;
        retVal = 0;
    }

	unlockProviderMgr();
	return retVal ? retVal : readLen;

}

//============================================================================
size_t VFileMgr::providerFileWrite(const void* buf, size_t size, size_t count, VFile* fp)
{
	// not implemented
	return -1;
}

//============================================================================
int VFileMgr::providerFileGetC( VFile* fp )
{
    if( fp->m_FileOffs == fp->m_FileLen )
    {
        return EOF;
    }

    char retChar[1];
    retChar[0] = 0;
    int readLen = providerFileRead( retChar, 1, 1, fp );

    return readLen == 1 ? retChar[0] : -1;
}

//============================================================================
char* VFileMgr::providerFileGetS( char* buf, int size, VFile* fp )
{

	lockProviderMgr();
    VirtProviderFile* providerFile = findProviderFile( fp );
    if( !providerFile )
    {
        LogMsg( LOG_ERROR, "VFileMgr::%s wrong VFile", __func__ );
        unlockProviderMgr();
        vx_assert( false );
        return nullptr;
    }
	
	std::string readStr;
	int result = -1;
    int64_t readIdx = fp->m_FileOffs;
	bool foundEnd{ false };
    while( !foundEnd && readIdx < fp->m_FileLen && readIdx < size )
	{
		char retChar[1];
        int64_t readLen = providerFile->read( (char*)retChar, 1 );
        if( readLen == 1 )
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
	
	unlockProviderMgr();
	if( result == 0 )
	{
		memcpy( buf, readStr.c_str(), readStr.length() );
		return buf;
	}


	return nullptr;
}

//============================================================================
int VFileMgr::providerFileGetPos( VFile* fp, fpos_t* pos )
{
    lockProviderMgr();
    VirtProviderFile* providerFile = findProviderFile( fp );
    if( !providerFile )
    {
        LogMsg( LOG_ERROR, "VFileMgr::%s wrong VFile", __func__ );
        unlockProviderMgr();
        vx_assert( false );
        return -1;
    }

#if defined(TARGET_OS_LINUX)
    fpos_t posConvert;
    posConvert.__pos = fp->m_FileOffs;
    *pos = posConvert;
#else
    *pos = fp->m_FileOffs;
#endif
    unlockProviderMgr();
    return 0;
}

//============================================================================
int VFileMgr::providerFilePutC(int ch, VFile* fp)
{
	// not implemented
	return -1;
}

//============================================================================
int VFileMgr::providerFilePutS(const char* s, VFile* fp)
{
	// not implemented
	return -1;
}
//============================================================================
int VFileMgr::providerFileSetPos( VFile* fp, const fpos_t* pos )
{
	// not implemented
	return -1;
}

//============================================================================
int VFileMgr::providerFileSeek( VFile* fp, size_t offset, int whence )
{
    int result = -1;

	lockProviderMgr();
    VirtProviderFile* providerFile = findProviderFile( fp );
    if( !providerFile )
    {
        LogMsg( LOG_ERROR, "VFileMgr::%s wrong VFile", __func__ );
        unlockProviderMgr();
        vx_assert( false );
        return -1;
    }

    int64_t origPos = fp->m_FileOffs;
	switch( whence )
	{
	case SEEK_SET:
		// Beginning of file
        if( offset >= fp->m_FileLen )
		{
			unlockProviderMgr();
			return -1;
		}

        fp->m_FileOffs = offset;
		break;

	case SEEK_CUR:
		// Current position of the file pointer
        if( fp->m_FileOffs + offset < 0 ||
            fp->m_FileOffs + offset >= fp->m_FileLen )
		{
			unlockProviderMgr();
			return -1;
		}

        fp->m_FileOffs += offset;
		break;

	case SEEK_END:
        fp->m_FileOffs = fp->m_FileLen + offset;
		break;
	}

    int64_t newPos = fp->m_FileOffs;
	if( newPos < 0 )
	{
        fp->m_FileOffs = 0;
		LogMsg( LOG_ERROR, "%s invalid pos" PRId64, __func__, newPos );
	}

    if( providerFile->seek( newPos ) )
    {
        result = 0;
    }

	unlockProviderMgr();


    return result;
}

//============================================================================
int VFileMgr::listProviderFilesAndFolders( const char* srcDir, std::vector<VxFileInfo>& fileList, uint8_t fileFilterMask )
{
    fileList.clear();

    std::string folderName( srcDir );

    if( 0 == fileFilterMask )
    {
        fileFilterMask = VXFILE_TYPE_ALLNOTEXE | VXFILE_TYPE_DIRECTORY;
    }

    std::vector<VxAndroidPathInfo> pathInfoList;
    if( 0 != VxAndroid::listDirectory( srcDir, pathInfoList ) )
    {
        return -1;
    }

    LogMsg( LOG_VERBOSE, "VFileMgr::%s %zu files in dir %s", __func__, pathInfoList.size(), folderName.c_str() );
    for( const auto& pathInfo : pathInfoList )
    {
        VxFileInfo vxFileInfo;

        if( pathInfo.m_IsDirectory )
        {
            if( fileFilterMask & VXFILE_TYPE_DIRECTORY )
            {
                std::string fileName = pathInfo.m_FileNameAndPath;
                VxFileUtil::assureTrailingDirectorySlash( fileName );
                vxFileInfo.setFileName( fileName );
                vxFileInfo.setFileNameAndPath( fileName );
                vxFileInfo.setFileType( VXFILE_TYPE_DIRECTORY );
                fileList.push_back( vxFileInfo );
            }
            continue;
        }

        if( pathInfo.m_IsExecutable )
        {
            continue;
        }

        if( !pathInfo.m_IsReadable )
        {
            continue;
        }

        if( pathInfo.m_FileLength <= 0 )
        {
            continue;
        }

        vxFileInfo.setFileName( pathInfo.m_FileNameAndPath );
        vxFileInfo.setFileNameAndPath( pathInfo.m_FileNameAndPath );
        vxFileInfo.setFileType( VxFileNameToFileType( pathInfo.m_FileNameAndPath ) );
        vxFileInfo.setFileLength( pathInfo.m_FileLength );
        fileList.push_back( vxFileInfo );
    }

	return 0;

}
