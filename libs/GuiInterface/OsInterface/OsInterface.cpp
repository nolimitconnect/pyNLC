//============================================================================
// Copyright (C) 2023 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "OsInterface.h"
#include <GuiInterface/IToGui.h>

#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxElapseTimer.h>

#include "CompileInfo.h"
#include "threads/Thread.h"
#include "platform/xbmc.h"
#include "settings/AdvancedSettings.h"
#include "utils/CPUInfo.h"
#include "platform/Environment.h"
#include "utils/CharsetConverter.h" // Required to initialize converters before usage

#if defined(TARGET_OS_WINDOWS)
#include "threads/platform/win/Win32Exception.h"
#include "platform/win32/CharsetConverter.h"

#include <dbghelp.h>
#include <mmsystem.h>
#include <Objbase.h>
#include <shellapi.h>
#include <WinSock2.h>
#endif //defined(TARGET_OS_WINDOWS)

#if defined(TARGET_OS_ANDROID)
# include "platform/qt/qtandroid/jni/Context.h"
# include "platform/qt/qtandroid/jni/System.h"
# include "platform/qt/qtandroid/jni/ApplicationInfo.h"
# include "platform/qt/qtandroid/jni/JNIFile.h"
# include <android/asset_manager.h>
# include <android/asset_manager_jni.h>
# include "AssetDirectoryList.hh"
#endif // TARGET_OS_ANDROID

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "NlcCoreUtil.h"
#include "platform/win32/WIN32Util.h"

#include "ServiceBroker.h"
#include "settings/SettingsComponent.h"

#include "FileItem.h"

#include <CoreLib/VxGlobals.h>

using namespace XFILE;

extern "C" int XBMC_Run( bool renderGUI );


namespace
{

//============================================================================
bool CopyIfRequiredAssetFile( std::string assetFileName, std::string destFile, bool replaceIfDifferent = true )
{
    bool result = false;
    if( !assetFileName.empty() && !destFile.empty() )
    {
        VxFileUtil::makeForwardSlashPath( assetFileName );
        VxFileUtil::makeForwardSlashPath( destFile );

        int64_t assetLength = VxFileUtil::fileExists( assetFileName.c_str() );
        int64_t destFileLen = VxFileUtil::fileExists( destFile.c_str() );
        if(  assetLength )
        {
            if( !destFileLen || ( replaceIfDifferent && ( assetLength != destFileLen ) ) )
            {
                std::string destDir = VxFileUtil::getJustPath( destFile );
                if( !VxFileUtil::directoryExists( destDir.c_str() ) )
                {
                    VxFileUtil::makeDirectory( destDir.c_str() );
                }

                char* buffer = (char*)malloc( assetLength + 1 );
                VxFileUtil::readWholeFile( assetFileName.c_str(), buffer, ( uint32_t )assetLength );
                buffer[ assetLength ] = 0;
                result = ( 0 == VxFileUtil::writeWholeFile( destFile.c_str(), buffer, ( uint32_t )assetLength ) );
                free( buffer );
            }
        }
        else
        {
            LogMsg( LOG_ERROR, "%s asset file %s does not exist", __func__, destFile.c_str() );
        }

        if( ( 0 != assetLength ) && !VxFileUtil::fileExists( destFile.c_str() ) )
        {
            LogMsg( LOG_ERROR, "Could not create file %s len %d", destFile.c_str(), assetLength );
        }
        else
        {
            result = true;
        }
    }

    return result;
}

//============================================================================
bool CopyIfRequiredAssetDirectory( std::string assetFileDir, std::string destDir, bool replaceIfDifferent = true )
{
    bool result = true;
    if( !assetFileDir.empty() && !destDir.empty() )
    {
        VxFileUtil::assureTrailingDirectorySlash( assetFileDir );
        VxFileUtil::assureTrailingDirectorySlash( destDir );
        VxFileUtil::makeDirectory( destDir );
        std::vector<std::string> fileList;

        if( 0 == VxFileUtil::listFilesInDirectory( assetFileDir.c_str(), fileList ) )
        {
            LogModule( eLogStartup, LOG_DEBUG, "%s copy %zu app assets to read/write location %s -> %s", __func__, fileList.size(), assetFileDir.c_str(), destDir.c_str() );

            for( auto& fileNameAndPath : fileList )
            {
                //! remove the path and return just the file name
                std::string justFileName;
                VxFileUtil::getFileName( fileNameAndPath.c_str(), justFileName );

                result &= CopyIfRequiredAssetFile( assetFileDir + justFileName, destDir + justFileName, replaceIfDifferent );
            }
        }
        else
        {
            result = false;
            LogMsg( LOG_DEBUG, "Could list asset dir %s", assetFileDir.c_str() );
        }
    }
    else
    {
        result = false;
        LogMsg( LOG_DEBUG, "Empty asset directory name" );
    }

    return result;
}

} // anonymouse namespace

#if defined( TARGET_OS_ANDROID )

static AAssetManager* android_asset_manager = NULL;
void android_fopen_set_asset_manager(AAssetManager* manager) {
    android_asset_manager = manager;
}

//============================================================================
bool CopyIfRequiredApkFile( std::string apkFileName, std::string destFile, bool replaceIfDifferent = true )
{
    bool result = false;
    AAssetManager* mgr = android_asset_manager;
    vx_assert(NULL != mgr);
    if( mgr )
    {
        AAsset* apkAssetFile = AAssetManager_open(mgr, apkFileName.c_str(), AASSET_MODE_UNKNOWN);
        vx_assert( apkAssetFile );

        if ( apkAssetFile )
        {
            size_t assetLength = AAsset_getLength( apkAssetFile );
            u_int64_t destFileLen = VxFileUtil::fileExists( destFile.c_str(), false );
            if( !destFileLen || ( replaceIfDifferent && ( assetLength != destFileLen ) ) )
            {
                std::string destDir = VxFileUtil::getJustPath( destFile );
                if( !VxFileUtil::directoryExists( destDir.c_str() ) )
                {
                    VxFileUtil::makeDirectory( destDir.c_str() );
                }

                char* buffer = (char*) malloc(assetLength + 1);
                AAsset_read( apkAssetFile, buffer, assetLength);
                buffer[ assetLength ] = 0;
                result = ( 0 == VxFileUtil::writeWholeFile( destFile.c_str(), buffer, assetLength ) );
                free(buffer);
            }

            if( ( 0 != assetLength ) && !VxFileUtil::fileExists( destFile.c_str() ) )
            {
                LogMsg( LOG_ERROR, "Could not create file %s len %d", destFile.c_str(), assetLength );
            }
            else
            {
                result = true;
            }

            AAsset_close( apkAssetFile );

        }
        else
        {
            LogMsg( LOG_ERROR,  "Asset Manager Cannot open file %s", destFile.c_str() );
        }
    }
    else
    {
        LogMsg( LOG_ERROR,  "Asset Manager Cannot open apk file %s", apkFileName.c_str() );
    }

    return result;
}

//============================================================================
bool CopyIfRequiredApkDirectory( std::string apkFileDir, std::string destDir, bool replaceIfDifferent = true )
{
    bool result = true;
    VxFileUtil::assureTrailingDirectorySlash( destDir );
    if( !VxFileUtil::directoryExists( destDir.c_str() ) )
    {
        VxFileUtil::makeDirectory( destDir.c_str() );
        if( !VxFileUtil::directoryExists( destDir.c_str() ) )
        {
            LogMsg( LOG_ERROR,  "%s could not created directory %s", __func__, destDir.c_str() );
        }
    }

    AAssetManager* assetMgr = android_asset_manager;
    AAssetDir* assetDir = AAssetManager_openDir( assetMgr, apkFileDir.c_str() );
    if( assetDir )
    {
        // LogMsg( LOG_DEBUG, "%s copy apk directory to read/write location  %s", __func__, destDir.c_str() );
        const char* fileName;
        while ((fileName = AAssetDir_getNextFileName(assetDir)) != NULL)
        {
            std::string srcFile = apkFileDir + "/" + fileName;
            std::string destFile = destDir + fileName;
            // LogMsg( LOG_DEBUG, "%s copy apk file %s to read/write location  %s", __func__, srcFile.c_str(), destFile.c_str() );

            //__android_log_print(ANDROID_LOG_DEBUG, "Debug", filename);
            result &= CopyIfRequiredApkFile( srcFile, destFile, replaceIfDifferent );
        }
    }
    else
    {
        result = false;
        LogMsg( LOG_DEBUG, "%s Could not open apk dir %s", __func__, apkFileDir.c_str() );
    }

    return result;
}

#endif // defined( TARGET_OS_ANDROID )


//============================================================================
bool OsInterface::doRun( EMediaModule mediaModule )
{
    LogModule( eLogStartup, LOG_VERBOSE, "OsInterface::doRun");

    if( !IToGui::getIToGui().toGuiGetIsAppModuleRunning( mediaModule ) )
    {
        if( eMediaModulePlayerNlc == mediaModule )
        { 
            #if defined(TARGET_OS_ANDROID)
                int attachedThreadState = CJNIContext::getJniContext().attachThread();
            #endif //  TARGET_OS_ANDROID

            //CAppEnvironment::SetUp(m_CmdLineParams->GetAppParams());

			int runExitCode = XBMC_Run( true );
			setRunResultCode( runExitCode );
            #if defined(TARGET_OS_ANDROID)
                CJNIContext::getJniContext().detachThread( attachedThreadState );
            #endif //  TARGET_OS_ANDROID

            IToGui::getIToGui().toGuiSetIsAppModuleRunning( mediaModule, false );

            //CAppEnvironment::TearDown();
        }
    }

    return true;
}

//=== utilities ===//
//============================================================================

bool OsInterface::initUserPaths( std::string& userWriteablePath )
{
    LogModule( eLogStartup, LOG_VERBOSE, "OsInterface::initUserPaths");

    CSpecialProtocol::SetHomePath( VxGetAppDirectory( eAppDirPlayerNlcData ) );
    CSpecialProtocol::SetTempPath( VxGetAppDirectory( eAppDirAppTempData ) );
    CSpecialProtocol::SetLogPath( VxGetAppDirectory( eAppDirAppLogs ) );
	return true;
}

//============================================================================
bool OsInterface::initDirectories()
{
    LogModule(eLogStartup, LOG_VERBOSE, "OsInterface::initDirectories");

    return true;
}
