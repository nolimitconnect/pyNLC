//============================================================================
// Copyright (C) 2024 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VirtStorageProvider.h"

#include <P2PEngine/P2PEngine.h>
#include <Plugins/FileInfo.h>

#include <CoreLib/VirtFileMgr.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxAndroid.h>


//============================================================================
VirtStorageProvider& GetVirtStorageProvider( void )
{
    static VirtStorageProvider g_VirtStorageProvider;
    return g_VirtStorageProvider;
}

//============================================================================
void VirtStorageProvider::fromGuiBrowseFiles( VxGUID& appInstId, std::string& folderNameIn, uint8_t fileFilterMask )
{
#if !defined(TARGET_OS_ANDROID)
    GetPtoPEngine().getFromGuiInterface().fromGuiBrowseFiles( appInstId, folderNameIn, fileFilterMask );
    return;
#endif // !defined(TARGET_OS_ANDROID)

    std::string folderName(folderNameIn);
    VxFileUtil::removeTrailingDirectorySlash(folderName);
    //VxFileUtil::encodePercentEncodingOfSlash(folderName);

	std::vector<FileInfo> fileList;
	if( 0 == fileFilterMask )
	{
		fileFilterMask = VXFILE_TYPE_ALLNOTEXE | VXFILE_TYPE_DIRECTORY;
	}

    VxGUID onlineId = GetPtoPEngine().getMyOnlineId();
    std::vector<VxFileInfoBase> fileInfoList;
    if( 0 != GetVirtFileMgr().listProviderFilesAndFolders( folderName.c_str(), fileInfoList, fileFilterMask ) )
    {
        LogMsg( LOG_WARN, "%s failed to list provider files for %s", __func__, folderName.c_str() );
    }

    LogMsg( LOG_VERBOSE, "%zu files in dir %s", fileInfoList.size(), folderName.c_str() );
    for( auto fileInfoBase : fileInfoList )
    {
        if( fileInfoBase.isDirectory() )
        {
            LogMsg( LOG_VERBOSE, "Directory %s", fileInfoBase.getFileName().c_str() );

            if( fileFilterMask & VXFILE_TYPE_DIRECTORY )
            {
                VxFileUtil::assureTrailingDirectorySlash( fileInfoBase.getFileNameAndPath() );
                FileInfo dirInfo( fileInfoBase );
                dirInfo.setOnlineId( onlineId );
                GetPtoPEngine().getToGui().toGuiFileList( appInstId, dirInfo );
            }
        }
        else if( fileInfoBase.isExecutableFile() )
        {
            LogMsg( LOG_VERBOSE, "Executable ignored File %s", fileInfoBase.getFileName().c_str() );
        }
        else if( fileInfoBase.fileIsAvailable() )
        {
            FileInfo fileInfo( fileInfoBase );
            fileInfo.setOnlineId( onlineId );
            fileInfo.setIsInLibrary( GetPtoPEngine().fromGuiGetFileIsInLibrary( fileInfo ) );
            fileInfo.setIsSharedFile( GetPtoPEngine().fromGuiGetIsFileShared( fileInfo ) );
        }
        else
        {
            LogMsg( LOG_VERBOSE, "NOT Readable File %s", fileInfoBase.getFileName().c_str() );
        }
    }

    GetPtoPEngine().getToGui().toGuiFileListCompleted( appInstId );
}

//============================================================================
bool VirtStorageProvider::requestPermission( const std::string& permissionName ) // returns false if user denies permission to use android hardware
{
    return VxAndroid::requestPermission( permissionName.c_str() );
}

