//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "GuiHelpers.h"

#include "GuiParams.h"
#include "GuiUser.h"

#include "ActivityBase.h"
#include "ActivityMsgBoxYesNo.h"
#include "AppletBase.h"
#include "AppletInviteCreate.h"
#include "AppletMgr.h"
#include "AppCommon.h"
#include "AppDefs.h"
#include "AppSettings.h"
#include "AppTranslate.h"
#include "AssetSendMgr.h"
#include "HomeWindow.h"
#include "MyIconsDefs.h"
#include "PluginSettingsWidget.h"
#include "VxFrame.h"

#include <CoreLib/ObjectCommon.h>
#include <CoreLib/ObjectCommonDefs.h>

#include <CoreLib/VFile.h>
#include <CoreLib/VxFileIsTypeFunctions.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxParse.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxDebug.h>

#include <P2PEngine/P2PEngine.h>
#include <VxVideoLib/VxVideoLib.h>

#include <QDesktopServices>
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QStandardPaths>
#include <QUrl>

#include <QFileSystemModel>

//============================================================================
std::string GuiHelpers::browseForDirectory( QString startDir, QWidget* parent )
{
    QFileDialog dialog( parent, QObject::tr("Open Folder"), startDir );

	dialog.setFileMode( QFileDialog::Directory );
    dialog.setOptions( QFileDialog::ShowDirsOnly );
    //dialog.setDirectory( startDir );

    QString selectedDir;
	QStringList fileNames;
	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();
		if( fileNames.size() )
		{
			selectedDir = fileNames[0];
		}
	}

    if( selectedDir.isEmpty() )
    {
        return "";
    }

    std::string folder = selectedDir.toUtf8().constData();

    VxFileUtil::makeForwardSlashPath( folder );
    VxFileUtil::assureTrailingDirectorySlash( folder );

    //listFilesInFolder( folder );

	return folder;
}

//============================================================================
void GuiHelpers::listFilesInFolder( QDir& folder, std::vector<VxFileInfoBase>& retFileList, uint8_t fileFilterMask )
{
    QFileInfoList fileInfoList = folder.entryInfoList();
    LogMsg( LOG_VERBOSE, "%zu files in dir %s", fileInfoList.size(), folder.path().toUtf8().constData() );
    for( auto fileListInfo : fileInfoList )
    {
        VxFileInfoBase fileInfo;
        if( qtFileInfoToVxFileInfo( fileListInfo, fileInfo, fileFilterMask ) )
        {
            retFileList.emplace_back(fileInfo);
        }
    }
}

//============================================================================
bool GuiHelpers::qtFileInfoToVxFileInfo( const QFileInfo& fileInfo, VxFileInfoBase& retFileInfo, uint8_t fileFilterMask )
{
    bool isAvailable{false};
    std::string fileName = fileInfo.fileName().toUtf8().constData();

    if( fileName.empty() )
    {
        return false;
    }

    if( fileInfo.isDir() )
    {
        if( fileFilterMask & VXFILE_TYPE_DIRECTORY )
        {
            std::string fileNameAndPath = fileInfo.path().toUtf8().constData();
            VxFileUtil::assureTrailingDirectorySlash( fileName );
            LogMsg( LOG_VERBOSE, "Directory %s", fileName.c_str() );
            isAvailable = true;
            retFileInfo = VxFileInfoBase( fileName.c_str(), fileNameAndPath.c_str(), 0,  VXFILE_TYPE_DIRECTORY );
        }
    }
    else if( fileInfo.isExecutable() )
    {
        LogMsg( LOG_VERBOSE, "Executable ignored File %s", fileName.c_str() );
    }
    else if( fileInfo.isReadable() )
    {
        int64_t fileLen = fileInfo.size();

        if( fileLen )
        {
            uint8_t fileType = VxFileNameToFileType( fileName );
            std::string fileNameAndPath = fileInfo.path().toUtf8().constData();
            if( fileLen && ( fileType & fileFilterMask ) )
            {
                retFileInfo = VxFileInfoBase( fileName.c_str(), fileNameAndPath.c_str(), fileLen, fileType );
            }
        }
        else
        {
            LogMsg( LOG_VERBOSE, "Could Not Resolve file length of file %s", fileName.c_str() );
        }
    }
    else
    {
        LogMsg( LOG_VERBOSE, "NOT Readable File %s", fileName.c_str() );
    }


    return isAvailable;
}

//============================================================================
void dumpFileInfo(QString selectedFile)
{
    QFileInfo fileInfo(selectedFile);

    std::string fileNameAndPath = selectedFile.toUtf8().constData();
    std::string fileName = fileInfo.fileName().toUtf8().constData();
    std::string filePath = fileInfo.dir().path().toUtf8().constData();
    int64_t fileLen = fileInfo.size();

    LogMsg(LOG_VERBOSE, "dumpFileInfo %s \n display name %s \n len %lld \n path %s ",
           fileNameAndPath.c_str(), fileName.c_str(), fileLen,
           filePath.c_str() );
}

//============================================================================
void listFiles( QDir browseDir, uint8_t fileFilterMask = VXFILE_TYPE_AUDIO_VIDEO_PHOTO )
{
    //QDir browseDir( filePath.c_str() );

    QFileInfoList fileInfoList = browseDir.entryInfoList();
    LogMsg( LOG_VERBOSE, "%zu files in dir %s", fileInfoList.size(), browseDir.path().toUtf8().constData() );
    for( auto fileListInfo : fileInfoList )
    {
        std::string fileName = fileListInfo.fileName().toUtf8().constData();
        std::string fileNameAndPath = fileListInfo.canonicalFilePath().toUtf8().constData();
        if( fileName.empty() )
        {
            continue;
        }

        if( fileListInfo.isDir() )
        {
            if( fileFilterMask & VXFILE_TYPE_DIRECTORY )
            {
                VxFileUtil::assureTrailingDirectorySlash( fileNameAndPath );
                LogMsg( LOG_VERBOSE, "Directory %s", fileNameAndPath.c_str() );

            }
        }
        else if( fileListInfo.isExecutable() )
        {
            LogMsg( LOG_VERBOSE, "Executable ignored File %s", fileName.c_str() );
        }
        else if( fileListInfo.isReadable() )
        {
            int64_t fileLen = fileListInfo.size();

            if( fileLen )
            {
                uint8_t fileType = VxFileNameToFileType( fileName );
                if( fileLen && ( fileType & fileFilterMask ) )
                {
                    if( GuiHelpers::testCanReadFile( fileNameAndPath ) )
                    {
                        LogMsg( LOG_VERBOSE, "Readable File %s len %" PRId64 " type 0x%x ", fileName.c_str(), fileLen, fileType );
                    }
                    else
                    {
                        LogMsg( LOG_VERBOSE, "NOT Readable File %s len %" PRId64 " type 0x%x ", fileName.c_str(), fileLen, fileType );
                    }
                }
            }
            else
            {
                LogMsg( LOG_VERBOSE, "Could Not Resolve file length of file %s", fileName.c_str() );
            }
        }
        else
        {
            LogMsg( LOG_VERBOSE, "NOT Readable File %s", fileName.c_str() );
        }
    }
}

//============================================================================
int countDirEntries( QDir testDir )
{
    QFileInfoList fileInfoList = testDir.entryInfoList();
    if(fileInfoList.size())
    {
        //listFiles(testDir);
        LogMsg( LOG_DEBUG, "Files count %zu found dir %s", fileInfoList.size(), testDir.path().toUtf8().constData() );
        return fileInfoList.size();
    }
    else
    {
        LogMsg( LOG_DEBUG, "No files found dir %s", testDir.path().toUtf8().constData() );
        return 0;
    }
}

//============================================================================
int countPathEntries( QString testPath )
{
    QDir testDir(testPath);
    return countDirEntries(testDir);
}

//============================================================================
bool GuiHelpers::browseForFile( QWidget* parent, enum EMediaFileType mediaFileType, FileInfo& retFileInfo, QString startDir )
{
    VxFileInfoBase fileInfoBase;
    bool result = browseForFile( parent, mediaFileType, fileInfoBase, startDir );
    if( !result )
    {
        return false;
    }

    FileInfo fileInfo( fileInfoBase );
    fileInfo.setOnlineId( GetAppInstance().getMyOnlineId() );

    retFileInfo = fileInfo;
    return true;
}

//============================================================================
bool GuiHelpers::browseForFile( QWidget* parent, enum EMediaFileType mediaFileType, VxFileInfoBase& retFileInfo, QString startDir )
{
    std::string fileNameAndPath;
    if( browseForFile( parent, mediaFileType, fileNameAndPath, startDir ) )
    {
        return VxFileUtil::getFileInfo( fileNameAndPath.c_str(), retFileInfo );
    }

    return false;
}

//============================================================================
bool GuiHelpers::browseForFile( QWidget* parent, enum EMediaFileType mediaFileType, std::string& retFileNameAndPath, QString startDir )
{
    bool filePerm = requestFilePermission(mediaFileType);
    if( !requestFilePermission(mediaFileType))
    {
        return false;
    }

    QString title = QObject::tr( "Select Media File" );
    QString supportedFileTypes = fileMaskToFileFilter( VXFILE_TYPE_AUDIO_VIDEO_PHOTO );
    bool strictDesktopTypeOnlyFilter = false;
    QString defaultDir = QDir::homePath();

    switch( mediaFileType )
    {
    case eMediaFileVideo:
        title = QObject::tr( "Select Video File" );
        // Desktop video picker should expose only video extensions (no All files option).
        supportedFileTypes = "Video (";
        supportedFileTypes += fileExtensionToFilter( VxGetFileExtensionsFromFileType( VXFILE_TYPE_VIDEO ).c_str() );
        supportedFileTypes += ")";
        strictDesktopTypeOnlyFilter = true;
        defaultDir = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath());
        break;

    case eMediaFileAudio:
        title = QObject::tr( "Select Audio File" );
        supportedFileTypes = "Audio (";
        supportedFileTypes += fileExtensionToFilter( VxGetFileExtensionsFromFileType( VXFILE_TYPE_AUDIO ).c_str() );
        supportedFileTypes += ")";
        strictDesktopTypeOnlyFilter = true;
        defaultDir = QStandardPaths::standardLocations(QStandardPaths::MusicLocation).value(0, QDir::homePath());
        break;

    case eMediaFileImage:
        title = QObject::tr( "Select Image File" );
        supportedFileTypes = "Image (";
        supportedFileTypes += fileExtensionToFilter( VxGetFileExtensionsFromFileType( VXFILE_TYPE_PHOTO ).c_str() );
        supportedFileTypes += ")";
        strictDesktopTypeOnlyFilter = true;
        defaultDir = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QDir::homePath());
        break;

    default:
        break;
    }

    QFileDialog fileDialog( parent, title, startDir );
    fileDialog.setAcceptMode( QFileDialog::AcceptOpen );
    fileDialog.setFileMode( QFileDialog::FileMode::ExistingFile );
    fileDialog.setOptions( fileDialog.options() | QFileDialog::ReadOnly );

#if !defined(TARGET_OS_ANDROID)
    // does not work in android.. some are greyed out even though have correct extension
    if( strictDesktopTypeOnlyFilter )
    {
        QStringList onlyMediaTypeFilter;
        onlyMediaTypeFilter << supportedFileTypes;
        fileDialog.setNameFilters( onlyMediaTypeFilter );
        fileDialog.selectNameFilter( supportedFileTypes );
    }
    else
    {
        fileDialog.setNameFilter( supportedFileTypes );
    }
#endif // !defined(TARGET_OS_ANDROID)

    QString selectedFile;
    if (fileDialog.exec() == QDialog::Accepted)
    {
        QStringList fileNames = fileDialog.selectedFiles();
        if( fileNames.size() )
        {
            selectedFile = fileNames[0];
        }
    }

    if( selectedFile.isEmpty() )
    {
        return false;
    }

    QFileInfo fileInfo(selectedFile);
    dumpFileInfo(selectedFile);

    QDir fileDir = fileInfo.dir();

    countDirEntries(fileDir);
    countPathEntries(fileDir.path());

    std::string fullFileName = selectedFile.toUtf8().constData();

    VxFileUtil::makeForwardSlashPath( fullFileName );

    if( !testCanReadFile( fullFileName ) )
    {
        QMessageBox warnStorage( QMessageBox::Icon::Information, QObject::tr("Cannot Read File"), fullFileName.c_str(), QMessageBox::Ok);
        warnStorage.exec();
        return false;
    }

    retFileNameAndPath = fullFileName;
    return !retFileNameAndPath.empty();
}

//============================================================================
uint64_t GuiHelpers::testCanReadFile( std::string fullFileName )
{
    if( VxFileUtil::fileIsProviderFile( fullFileName.c_str() ) )
    {
        VFile* vFile = VxFileUtil::fileOpen( fullFileName.c_str(), "rb" );
        if( vFile )
        {
            VFileClose( vFile );
            return 1;
        }

        return 0;
    }

    uint64_t fileLen = VxFileUtil::fileExists(fullFileName.c_str());
    if( fileLen )
    {
        VFile* vFile = VxFileUtil::fileOpen( fullFileName.c_str(), "rb" );
        if( vFile )
        {
            VFileClose( vFile );
            return fileLen;
        }
    }

    return 0;
}

//============================================================================
QString GuiHelpers::fileMaskToFileFilter( uint8_t fileMask )
{
    QString filter = "All files (*.*)";
    std::string fileExtensions = VxGetFileExtensionsFromFileType( fileMask );
    if( fileMask == VXFILE_TYPE_AUDIO_VIDEO_PHOTO )
	{
        filter += ";;Media (";
        filter += fileExtensionToFilter( fileExtensions.c_str() );
        filter += ")";
	}
	else if( fileMask == VXFILE_TYPE_ALLNOTEXE )
	{
        filter += ";;Known (";
        filter += fileExtensionToFilter( fileExtensions.c_str() );
        filter += ")";
	}
	else if( fileMask == VXFILE_TYPE_ANY || !fileMask )
	{
		return filter;
	}
	else
	{
		if( fileMask == VXFILE_TYPE_PHOTO )
		{
            filter += ";;Image (";
            filter += fileExtensionToFilter( fileExtensions.c_str() );
            filter += ")";
		}
		else if( fileMask == VXFILE_TYPE_AUDIO )
		{
            filter += ";;Audio (";
            filter += fileExtensionToFilter( fileExtensions.c_str() );
            filter += ")";
		}
		else if( fileMask == VXFILE_TYPE_VIDEO )
		{
	        filter += ";;Video (";
            filter += fileExtensionToFilter( fileExtensions.c_str() );
            filter += ")";
		}
		else if( fileMask == VXFILE_TYPE_DOC )
		{
	        filter += ";;Document (";
            filter += fileExtensionToFilter( fileExtensions.c_str() );
            filter += ")";
		}
		else if( fileMask == VXFILE_TYPE_ARCHIVE_OR_CDIMAGE )
		{
	        filter += ";;Archive (";
            filter += fileExtensionToFilter( fileExtensions.c_str() );
            filter += ")";
		}
		else if( fileMask == VXFILE_TYPE_EXECUTABLE )
		{
	        filter += ";;Executable (";
            filter += fileExtensionToFilter( fileExtensions.c_str() );
            filter += ")";
		}
	}

    return filter;
}

//============================================================================
QString GuiHelpers::fileExtensionToFilter( QString fileExtensions )
{
    QString filter;
    QStringList extList = fileExtensions.split( ',' );
    for( auto ext : extList )
    {
        if( !filter.isEmpty() )
        {
            filter += " ";
        }

        filter += "*.";
        filter += ext;
    }

    return filter;
}

//============================================================================
bool GuiHelpers::copyResourceToOnDiskFile( QString resourcePath, QString fileNameAndPath )
{
	bool resourceCopied = false;

	QFile resfile( resourcePath );
	QFile onDiskFile( fileNameAndPath );
	if( resfile.open( QIODevice::ReadOnly ) )
	{
		if( onDiskFile.open( QIODevice::ReadWrite ) )
		{
			if( 0 == onDiskFile.write( resfile.readAll() ) )
			{
				qWarning() << "Could not write app resource file " << fileNameAndPath;
			}
			else
			{
				resourceCopied = true;
			}

			onDiskFile.close();
		}
		else
		{
			qWarning() << "Could not open app resource file for writing " << fileNameAndPath;
		}

		resfile.close();
	}
	else
	{
		qWarning() << "Could not open resource file " << resourcePath;
	}

	return resourceCopied;
}

//============================================================================
EApplet GuiHelpers::getAppletThatPlaysFile( AppCommon& myApp, AssetBaseInfo& assetInfo )
{
    return getAppletThatPlaysFile( myApp, 
                                   GuiParams::assetTypeToFileType( assetInfo.getAssetType() ), 
                                   assetInfo.getAssetNameAndPath().c_str(), 
                                   assetInfo.getAssetUniqueId(),
                                   assetInfo.isStream() );
}

//============================================================================
EApplet GuiHelpers::getAppletThatPlaysFile( AppCommon& myApp, uint8_t fileType, QString fullFileName, VxGUID& assetId, bool isStream )
{
    EApplet applet = eAppletUnknown;
    if( fileType & VXFILE_TYPE_VIDEO )
    {
        if( myApp.getFromGuiInterface().fromGuiIsNoLimitVideoFile( fullFileName.toUtf8().constData() ) )
        {
            applet = eAppletPlayerCamClip;
        }
        else
        {
            applet = eAppletPlayerNlc;
        }
    }
    else if( fileType & VXFILE_TYPE_PHOTO )
    {
        applet = eAppletPlayerPhoto;
    }
    else if( fileType & VXFILE_TYPE_AUDIO )
    {
        applet = eAppletPlayerNlc;
    }

    return applet;
}

//============================================================================
int GuiHelpers::calculateTextHeight( QFontMetrics& fontMetrics, QString textStr )
{
	int textHeight = 0;
	if( textStr.isEmpty() )
	{
		textHeight = fontMetrics.height();
	}
	else
	{
		int lineCount = textStr.count( QLatin1Char('\n') ) + 1;
		textHeight = lineCount * fontMetrics.height();
	}

	return textHeight;
}

//============================================================================
bool GuiHelpers::isAppletAService( EApplet applet )
{
    return ( ( eAppletServiceAboutMe == applet )
             || ( eAppletServiceShareFiles == applet )
             || ( eAppletServiceShareWebCam == applet )
             || ( eAppletServiceAboutMe == applet )
             || ( eAppletServiceStoryboard == applet )
             );
}

//============================================================================
bool GuiHelpers::isAppletAClient( EApplet applet )
{
    return ( ( eAppletAboutMeClient == applet )
             || ( eAppletAvatarImageClient == applet )
             || ( eAppletConnectionTestClient == applet )
             || ( eAppletGroupClient == applet )
             || ( eAppletRandomConnectClient == applet )
             || ( eAppletClientShareFiles == applet )
             || ( eAppletCamClient == applet )
             || ( eAppletStoryboardClient == applet )
             );
}

//============================================================================
EApplet GuiHelpers::hostTypeToHostClientApplet( EHostType hostType )
{
    EApplet applet{ eAppletUnknown };
    switch( hostType )
    {
    case eHostTypeChatRoom:
        applet = eAppletChatRoomClient;
        break;

    case eHostTypeGroup:
        applet = eAppletGroupClient;
        break;

    case eHostTypeRandomConnect:
        applet = eAppletRandomConnectClient;
        break;

    default:
        break;
    }

    return applet;
}

//============================================================================
EPluginType GuiHelpers::getAppletAssociatedPlugin( EApplet applet )
{
    EPluginType pluginType = ePluginTypeInvalid;
    switch( applet )
    {
    case eAppletAboutMeClient:              return ePluginTypeAboutMePageClient;
    case eAppletCamClient:                  return ePluginTypeCamServer;
    case eAppletAvatarImageClient:          return ePluginTypeClientPeerUser;
    case eAppletConnectionTestClient:       return ePluginTypeClientConnectTest;
    case eAppletGroupClient:                return ePluginTypeHostGroup;

    case eAppletHostNetworkClient:          return ePluginTypeHostNetwork;
    case eAppletRandomConnectClient:        return ePluginTypeClientRandomConnect;
    case eAppletClientShareFiles:           return ePluginTypeFileShareServer;

    case eAppletFileShareClientView:        return ePluginTypeFileShareClient;

    case eAppletServiceAboutMe:              return ePluginTypeAboutMePageServer;

    case eAppletServiceConnectionTest:       return ePluginTypeHostConnectTest;
    case eAppletChatRoomHostAdmin:           return ePluginTypeHostChatRoom;
    case eAppletGroupHostAdmin:              return ePluginTypeHostGroup;
    case eAppletServiceHostNetwork:          return ePluginTypeHostNetwork;
    case eAppletServiceShareFiles:           return ePluginTypeFileShareServer;
    case eAppletServiceShareWebCam:          return ePluginTypeCamServer;
    case eAppletServiceStoryboard:           return ePluginTypeStoryboardServer;

    case eAppletSettingsAboutMe:            return ePluginTypeAboutMePageServer;
    case eAppletSettingsAvatarImage:        return ePluginTypeHostPeerUser;
    case eAppletSettingsWebCamServer:       return ePluginTypeCamServer;
    case eAppletSettingsConnectTest:        return ePluginTypeHostConnectTest;
    case eAppletSettingsShareFiles:         return ePluginTypeFileShareServer;
    case eAppletSettingsFileXfer:           return ePluginTypePersonFileXfer;
    case eAppletSettingsFriendRequest:      return ePluginTypePersonFileXfer;

    case eAppletSettingsHostChatRoom:       return ePluginTypeHostChatRoom;
    case eAppletSettingsHostGroup:          return ePluginTypeHostGroup;
    case eAppletSettingsHostNetwork:        return ePluginTypeHostNetwork;
    case eAppletSettingsHostRandomConnect:  return ePluginTypeHostRandomConnect;

    case eAppletSettingsMessenger:          return ePluginTypeMessenger;
    case eAppletSettingsRandomConnect:      return ePluginTypeClientRandomConnect;

    case eAppletSettingsStoryboard:         return ePluginTypeStoryboardServer;
    case eAppletSettingsTruthOrDare:        return ePluginTypeTruthOrDare;
    case eAppletSettingsVideoPhone:         return ePluginTypeVideoChat;
    case eAppletSettingsVoicePhone:         return ePluginTypeVoicePhone;


    default:
        break;
    }

    return pluginType;
}

//============================================================================
EApplet GuiHelpers::pluginTypeToEditApplet( EPluginType pluginType )
{
    EApplet appletType = eAppletUnknown;

    switch( pluginType )
    {
    case ePluginTypeAboutMePageServer:      return eAppletEditAboutMe;
    case ePluginTypeHostPeerUser:           return eAppletEditAvatarImage;
    case ePluginTypeStoryboardServer:       return eAppletEditStoryboard;
    case ePluginTypeCamServer:              return eAppletUnknown;
    case ePluginTypeHostConnectTest:        return eAppletSettingsConnectTest;
    case ePluginTypeFileShareServer:        return eAppletUnknown;
    case ePluginTypePersonFileXfer:         return eAppletUnknown;
    case ePluginTypeHostGroup:              return eAppletUnknown;
    case ePluginTypeHostNetwork:            return eAppletUnknown;
    case ePluginTypeClientRandomConnect:    return eAppletUnknown;
    case ePluginTypeHostRandomConnect:      return eAppletUnknown;
    default:
        break;
    }

    return appletType;
}

//============================================================================
EApplet GuiHelpers::pluginTypeToSettingsApplet( EPluginType pluginType )
{
    EApplet appletType = eAppletUnknown;

    switch( pluginType )
    {
    case ePluginTypeAboutMePageServer:      return eAppletSettingsAboutMe;
    case ePluginTypeHostPeerUser:           return eAppletSettingsAvatarImage;
    case ePluginTypeCamServer:              return eAppletSettingsWebCamServer;
    case ePluginTypeHostConnectTest:        return eAppletSettingsConnectTest;
    case ePluginTypeFileShareServer:        return eAppletSettingsShareFiles;
    case ePluginTypePersonFileXfer:         return eAppletSettingsFileXfer;

    case ePluginTypeHostChatRoom:           return eAppletSettingsHostChatRoom;
    case ePluginTypeHostGroup:              return eAppletSettingsHostGroup;
    case ePluginTypeHostNetwork:            return eAppletSettingsHostNetwork;
    case ePluginTypeHostRandomConnect:      return eAppletSettingsHostRandomConnect;

    case ePluginTypeMessenger:              return eAppletSettingsMessenger;
    case ePluginTypePushToTalk:             return eAppletSettingsPushToTalk;
    case ePluginTypeClientRandomConnect:    return eAppletSettingsRandomConnect;

    case ePluginTypeStoryboardServer:       return eAppletSettingsStoryboard;
    case ePluginTypeTruthOrDare:            return eAppletSettingsTruthOrDare;
    case ePluginTypeVideoChat:             return eAppletSettingsVideoPhone;
    case ePluginTypeVoicePhone:             return eAppletSettingsVoicePhone;

    case ePluginTypeFriendRequest:          return eAppletSettingsFriendRequest;

    default:
        break;
    }

    return appletType;
}

//============================================================================
EApplet GuiHelpers::pluginTypeToViewApplet( EPluginType pluginType )
{
    EApplet appletType = eAppletUnknown;

    switch( pluginType )
    {
    case ePluginTypeAboutMePageServer:      return eAppletEditAboutMe;
    case ePluginTypeHostPeerUser:           return eAppletEditAvatarImage;
    case ePluginTypeStoryboardServer:       return eAppletEditStoryboard;
    case ePluginTypeCamServer:              return eAppletUnknown;
    case ePluginTypeHostConnectTest:        return eAppletSettingsConnectTest;
    case ePluginTypeFileShareServer:        return eAppletUnknown;
    case ePluginTypePersonFileXfer:         return eAppletUnknown;
    case ePluginTypeHostGroup:              return eAppletUnknown;
    case ePluginTypeHostNetwork:            return eAppletUnknown;
    case ePluginTypeClientRandomConnect:    return eAppletUnknown;
    case ePluginTypeHostRandomConnect:      return eAppletUnknown;

    default:
        break;
    }

    return appletType;
}

//============================================================================
EApplet GuiHelpers::pluginTypeToUserApplet( EPluginType pluginType )
{
    EApplet appletType = eAppletUnknown;

    switch( pluginType )
    {
    case ePluginTypeAboutMePageServer:     return eAppletEditAboutMe;
    case ePluginTypeAboutMePageClient:     return eAppletAboutMeClient;
    case ePluginTypeHostPeerUser:          return eAppletEditAvatarImage;
    //case ePluginTypeCamServer:              return eAppletSettingsWebCamServer;
    // case ePluginTypeHostConnectTest:     return eAppletSettingsConnectTest;
    //case ePluginTypeFileShareServer:             return eAppletShareFiles;
    // case ePluginTypePersonFileXfer:               return eAppletSettingsFileXfer;

    case ePluginTypeHostChatRoom:           return eAppletSettingsHostChatRoom;
    case ePluginTypeHostGroup:              return eAppletSettingsHostGroup;
    case ePluginTypeHostNetwork:            return eAppletSettingsHostNetwork;
    case ePluginTypeHostRandomConnect:      return eAppletSettingsHostRandomConnect;

    case ePluginTypeStoryboardServer:       return eAppletEditStoryboard;
    case ePluginTypeStoryboardClient:       return eAppletStoryboardClient;

    //case ePluginTypeMessenger:              return eAppletMultiMessenger;
    //case ePluginTypeTruthOrDare:            return eAppletPeerTruthOrDare;
    //case ePluginTypeVideoChat:             return eAppletPeerVideoPhone;
    //case ePluginTypeVoicePhone:             return eAppletPeerVoicePhone;

    default:
        break;
    }

    return appletType;
}

//============================================================================
EApplet GuiHelpers::pluginTypeToSessionApplet( EPluginType pluginType )
{
    EApplet appletType = eAppletUnknown;

    switch( pluginType )
    {
    case ePluginTypeMessenger:              return eAppletMultiMessenger;
    case ePluginTypeTruthOrDare:            return eAppletPeerTruthOrDare;
    case ePluginTypeVideoChat:             return eAppletPeerVideoPhone;
    case ePluginTypeVoicePhone:             return eAppletPeerVoicePhone;

    default:
        break;
    }

    return appletType;
}


//============================================================================
QFrame* GuiHelpers::pluginTypeToDefaultContentFrame( EPluginType pluginType )
{
    EApplet appletType = pluginTypeToSessionApplet( pluginType );
    if( eAppletUnknown != appletType )
    {
        QString contentFrameObjName = OBJNAME_FRAME_LAUNCH_PAGE;
        if( ePluginTypeMessenger == pluginType )
        {
            contentFrameObjName = OBJNAME_FRAME_MESSAGER_PAGE;
        }

        return findContentFrame( contentFrameObjName );
    }

    return nullptr;
}

//============================================================================
QFrame* GuiHelpers::pluginTypeToOppositeDefaultContentFrame( EPluginType pluginType )
{
    EApplet appletType = pluginTypeToSessionApplet( pluginType );
    if( eAppletUnknown != appletType )
    {
        QString contentFrameObjName = OBJNAME_FRAME_MESSAGER_PAGE;
        if( ePluginTypeMessenger == pluginType )
        {
            contentFrameObjName = OBJNAME_FRAME_LAUNCH_PAGE;
        }

        return findContentFrame( contentFrameObjName );
    }

    return nullptr;
}

//============================================================================
EMyIcons GuiHelpers::pluginTypeToSettingsIcon( EPluginType pluginType )
{
    EMyIcons iconType = eMyIconUnknown;

    switch( pluginType )
    {
    case ePluginTypeAboutMePageServer:      return eMyIconSettingsAboutMe;
    case ePluginTypeHostPeerUser:           return eMyIconSettingsAvatarImage;
    case ePluginTypeCamServer:              return eMyIconSettingsShareWebCam;
    case ePluginTypeHostConnectTest:        return eMyIconSettingsConnectionTest;
    case ePluginTypeFileShareServer:        return eMyIconSettingsShareFiles;
    case ePluginTypePersonFileXfer:         return eMyIconSettingsFileXfer;
    case ePluginTypeHostGroup:              return eMyIconSettingsHostGroup;

    case ePluginTypeHostNetwork:            return eMyIconSettingsHostNetwork;
    case ePluginTypeMessenger:              return eMyIconSettingsMessenger;
    case ePluginTypeClientRandomConnect:    return eMyIconSettingsRandomConnect;
    case ePluginTypeHostRandomConnect:      return eMyIconSettingsRandomConnectRelay;
    case ePluginTypeStoryboardServer:       return eMyIconSettingsShareStoryboard;
    case ePluginTypeTruthOrDare:            return eMyIconSettingsTruthOrDare;
    case ePluginTypeVideoChat:             return eMyIconSettingsVideoPhone;
    case ePluginTypeVoicePhone:             return eMyIconSettingsVoicePhone;
    case ePluginTypePushToTalk:             return eMyIconSettingsPushToTalk;

    default:
        break;
    }

    return iconType;
}

//============================================================================
bool GuiHelpers::isPluginSingleSession( EPluginType pluginType )
{
    return ::IsPluginSingleSession( pluginType );
}

//============================================================================
//! which plugins to show in permission list
bool GuiHelpers::isPluginAPrimaryService( EPluginType pluginType )
{
    bool isPrimaryPlugin = false;
    switch( pluginType )
    {
    case ePluginTypeAboutMePageServer:
    case ePluginTypeVoicePhone:
    case ePluginTypeVideoChat:
    case ePluginTypeTruthOrDare:
    case ePluginTypeMessenger:
    case ePluginTypeCamServer:
    case ePluginTypeStoryboardServer:
    case ePluginTypeFileShareServer:
    case ePluginTypePersonFileXfer:
    case ePluginTypeHostGroup:
    case ePluginTypeHostChatRoom:
    case ePluginTypeHostNetwork:
    case ePluginTypeHostRandomConnect:
    case ePluginTypeHostConnectTest:
    case ePluginTypePushToTalk:
    case ePluginTypeFriendRequest:
        isPrimaryPlugin = true;
        break;

    case ePluginTypeClientConnectTest:
    default:
        break;
    }

    return isPrimaryPlugin;
}

//============================================================================
bool GuiHelpers::getSecondaryPlugins( EPluginType pluginType, QVector<EPluginType> secondaryPlugins )
{
    secondaryPlugins.clear();
    switch( pluginType )
    {
    case ePluginTypeHostNetwork:
        secondaryPlugins.emplace_back( ePluginTypeHostConnectTest );
        break;

    case ePluginTypeHostChatRoom:
        secondaryPlugins.emplace_back( ePluginTypeHostConnectTest );
        break;

    case ePluginTypeHostGroup:
        secondaryPlugins.emplace_back( ePluginTypeHostConnectTest );
        break;

    case ePluginTypeHostRandomConnect:
        secondaryPlugins.emplace_back( ePluginTypeHostConnectTest );
        break;

    default:
        break;
    }

    return !secondaryPlugins.isEmpty();
}

//============================================================================
bool GuiHelpers::isMessagerFrame( QWidget* curWidget )
{
    bool isMessengerFrame{ false };
    
    QObject * curParent = curWidget;
    QString messengerPageObjName = OBJNAME_FRAME_MESSAGER_PAGE;

    while( curParent )
    {
        QString objName = curParent->objectName();
        if( objName == messengerPageObjName )
        {
            isMessengerFrame = true;
            break;
        }


        curParent = dynamic_cast<QObject *>( curParent->parent() );
        if( !curParent )
        {
            break;
        }
    }

    return isMessengerFrame;

}

//============================================================================
QFrame* GuiHelpers::getParentPageFrame( QWidget* curWidget )
{
    QFrame* pageFrame = nullptr;
    QObject * curParent = curWidget;

    QString launchPageObjName = OBJNAME_FRAME_LAUNCH_PAGE;
    QString messengerPageObjName = OBJNAME_FRAME_MESSAGER_PAGE;

    while( curParent )
    {
        QString objName = curParent->objectName();
        if( ( objName == launchPageObjName ) || ( objName == messengerPageObjName ) )
        {
            pageFrame = dynamic_cast<QFrame*>( curParent );
            if( pageFrame )
            {
                break;
            }
        }

        if( !curParent->parent() )
        {
            LogMsg( LOG_WARNING, "GuiHelpers::%s %s has no parent", __func__, objName.toUtf8().constData() );
        }

        curParent = dynamic_cast<QObject *>( curParent->parent() );
    }

    if( !pageFrame )
    {
        LogMsg( LOG_ERROR, "GuiHelpers::%s NOT FOUND", __func__ );
        // vx_assert( false ); // popup dialags from title bar have HomeWindow as parent so QFrame will not be found
    }

    return pageFrame;
}

//============================================================================
QString GuiHelpers::getParentPageFrameName( QWidget* curWidget )
{
    QString frameName;

    QFrame* parentFrame = getParentPageFrame( curWidget );
    if( parentFrame )
    {
        frameName = parentFrame->objectName();
        if( frameName != OBJNAME_FRAME_LAUNCH_PAGE && frameName != OBJNAME_FRAME_MESSAGER_PAGE )
        {
            LogMsg( LOG_ERROR, "GuiHelpers::getParentPageFrameName invalid frame name" );
            frameName.clear();
        }
    }

    return frameName;
}

//============================================================================
QFrame* GuiHelpers::getMessengerPageFrame( QWidget* curWidget )
{
    QFrame* pageFrame = nullptr;
    QObject * curParent = curWidget;

    QString launchPageObjName = OBJNAME_FRAME_LAUNCH_PAGE;
    QString messengerPageObjName = OBJNAME_FRAME_MESSAGER_PAGE;

    while( curParent )
    {
        QString objName = curParent->objectName();
        if( (objName == launchPageObjName) || (objName == messengerPageObjName) )
        {
            if( objName == messengerPageObjName )
            {
                pageFrame = dynamic_cast<QFrame*>(curParent);
                if (pageFrame)
                {
                    break;
                }
            }
            else
            {
                bool foundMessengerFrame = false;
                QWidget* baseFrame = dynamic_cast<QWidget*>(curParent->parent());
                if( baseFrame )
                {
                    QObjectList childList = baseFrame->children();
                    for( auto iter = childList.begin();  iter != childList.end(); ++iter )
                    {
                        QFrame*childFrame = dynamic_cast<QFrame*>(*iter);
                        if( childFrame && childFrame->objectName() == messengerPageObjName )
                        {
                            pageFrame = childFrame;
                            foundMessengerFrame = true;
                            break;
                        }
                    }
                }

                if( foundMessengerFrame )
                {
                    break;
                }
            }
        }

        if( !curParent->parent() )
        {
            LogMsg( LOG_WARNING, "Object %s has no parent", objName.toUtf8().constData() );
        }

        curParent = dynamic_cast<QObject *>(curParent->parent());
    }

    return pageFrame;
}

//============================================================================
QFrame* GuiHelpers::getLaunchPageFrame( QWidget* curWidget )
{
    QFrame* pageFrame = nullptr;
    QObject * curParent = curWidget;

    QString launchPageObjName = OBJNAME_FRAME_LAUNCH_PAGE;
    QString messengerPageObjName = OBJNAME_FRAME_MESSAGER_PAGE;

    while (curParent)
    {
        QString objName = curParent->objectName();
        if ((objName == launchPageObjName) || (objName == messengerPageObjName))
        {
            if( objName == launchPageObjName )
            {
                pageFrame = dynamic_cast<QFrame*>(curParent);
                if (pageFrame)
                {
                    break;
                }
            }
            else
            {
                bool foundLaunchFrame = false;
                QWidget* baseFrame = dynamic_cast<QWidget*>(curParent->parent());
                if( baseFrame )
                {
                    QObjectList childList = baseFrame->children();
                    for( auto iter = childList.begin();  iter != childList.end(); ++iter )
                    {
                        QFrame* childFrame = dynamic_cast<QFrame*>(*iter);
                        if( childFrame && childFrame->objectName() == launchPageObjName )
                        {
                            pageFrame = childFrame;
                            foundLaunchFrame = true;
                            break;
                        }
                    }
                }

                if( foundLaunchFrame )
                {
                    break;
                }
            }
        }

        if (!curParent->parent())
        {
            LogMsg( LOG_WARNING, "Object %s has no parent", objName.toUtf8().constData() );
        }

        curParent = dynamic_cast<QObject *>(curParent->parent());
    }

    return pageFrame;
}

//============================================================================
QFrame* GuiHelpers::getOppositePageFrame( QWidget* curWidget )
{
    QFrame* pageFrame = nullptr;
    QObject * curParent = curWidget;

    QString launchPageObjName = OBJNAME_FRAME_LAUNCH_PAGE;
    QString messengerPageObjName = OBJNAME_FRAME_MESSAGER_PAGE;

    while (curParent)
    {
        QString objName = curParent->objectName();
        if ((objName == launchPageObjName) || (objName == messengerPageObjName))
        {
            QString otherPageObjeName = (objName == launchPageObjName) ? messengerPageObjName : launchPageObjName;

            bool foundOtherFrame = false;
            QWidget* baseFrame = dynamic_cast<QWidget*>(curParent->parent());
            if( baseFrame )
            {
                QObjectList childList = baseFrame->children();
                for( auto iter = childList.begin();  iter != childList.end(); ++iter )
                {
                    QFrame* childFrame = dynamic_cast<QFrame*>(*iter);
                    if( childFrame && childFrame->objectName() == otherPageObjeName && childFrame == GetAppInstance().getHomeWindow().getMessengerParentFrame() )
                    {
                        pageFrame = childFrame;
                        foundOtherFrame = true;
                        break;
                    }
                }
            }

            if( foundOtherFrame )
            {
                break;
            }
        }

        if (!curParent->parent())
        {
            LogMsg( LOG_WARNING, "Object %s has no parent", objName.toUtf8().constData() );
        }

        curParent = dynamic_cast<QObject *>(curParent->parent());
    }

    return pageFrame;
}

//============================================================================
VxFrame* GuiHelpers::getVxFrame( QWidget* curWidget )
{
    VxFrame* vxFrame{ nullptr };
    QObject * curParent = curWidget;

    static QString launchPageObjName = OBJNAME_FRAME_LAUNCH_PAGE;
    static QString messengerPageObjName = OBJNAME_FRAME_MESSAGER_PAGE;

    while (curParent)
    {
        QString objName = curParent->objectName();
        if ((objName == launchPageObjName) || (objName == messengerPageObjName))
        {
            vxFrame = dynamic_cast<VxFrame*>(curParent);
            if( !vxFrame )
            {
                LogMsg( LOG_WARNING, "%s could not dynamic cast VxFrame", __func__ );
                return nullptr;
            }

            break;
        }

        if (!curParent->parent())
        {
            LogMsg( LOG_WARNING, "Object %s has no parent", objName.toUtf8().constData() );
        }

        curParent = dynamic_cast<QObject *>(curParent->parent());
    }

    return vxFrame;
}

#if 0
//============================================================================
QFrame* GuiHelpers::getVxFrameContentItemsFrame( QWidget* curWidget )
{
    VxFrame* vxFrame = getVxFrame( curWidget );
    if( vxFrame )
    {
        return vxFrame->getContentItemsFrame();
    }

    return nullptr;
}

//============================================================================
TitleBarWidget* GuiHelpers::getVxFrameTitleBarWidget( QWidget* curWidget )
{
    VxFrame* vxFrame = getVxFrame( curWidget );
    if( vxFrame )
    {
        return vxFrame->getTitleBarWidget();
    }

    return nullptr;
}

//============================================================================
BottomBarWidget* GuiHelpers::getVxFrameBottomBarWidget( QWidget* curWidget )
{
    VxFrame* vxFrame = getVxFrame( curWidget );
    if( vxFrame )
    {
        return vxFrame->getBottomBarWidget();
    }

    return nullptr;
}
#endif // 0

//============================================================================
QFrame* GuiHelpers::findContentFrame( QString& contentFrameObjName )
{
    if( OBJNAME_FRAME_MESSAGER_PAGE == contentFrameObjName )
    {
        return GetAppInstance().getHomeWindow().getMessengerParentFrame();
    }
    else
    {
        return GetAppInstance().getHomeWindow().getLaunchPageFrame();
    }
}

//============================================================================
AppletBase * GuiHelpers::findParentApplet( QWidget* parent )
{
    AppletBase * appletBase = nullptr;
    if( parent )
    {
        appletBase = dynamic_cast< AppletBase * >( parent );
        while( parent && !appletBase )
        {
            parent = dynamic_cast< QWidget* >( parent->parent() );
            if( parent )
            {
                appletBase = dynamic_cast< AppletBase * >( parent );
            }
        }
    }

    return appletBase;
}

//============================================================================
bool GuiHelpers::validateUserName( QWidget* curWidget, QString strUserName )
{
    if( strUserName.contains( "NoLimitConnect" )
        || strUserName.contains( "nolimitconnect" )
        || strUserName.contains( "No Limit Connect" )
        || strUserName.contains( "no limit connect" ) )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Invalid User Name" ), QObject::tr( "User Name cannot have NoLimitConnect in name." ) );
        return false;
    }

    if( strUserName.contains( "'" ) || strUserName.contains( "\"" ) )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Invalid User Name" ), QObject::tr( "User Name cannot have special character quote." ) );
        return false;
    }

    if( strUserName.contains( "," ) )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Invalid User Name" ), QObject::tr( "User Name cannot have comma." ) );
        return false;
    }

    if( strUserName.contains( "(" ) || strUserName.contains( ")" ) )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Invalid User Name" ), QObject::tr( "User Name cannot have special character parentheses." ) );
        return false;
    }

    if( strUserName.contains( "/" ) || strUserName.contains( "\\" ) )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Invalid User Name" ), QObject::tr( "User Name cannot have special character slashes." ) );
        return false;
    }

    if( strUserName.length() > 31 )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Invalid User Name" ), QObject::tr( "User Name is too long (maximum 31 chars)." ) );
        return false;
    }

    if( strUserName.length() < 4 )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Invalid User Name" ), QObject::tr( "User Name is too short (minimum 4 chars)." ) );
        return false;
    }

    return true;
}

//============================================================================
bool GuiHelpers::validateMoodMessage( QWidget* curWidget, QString strMoodMsg )
{
    //if( strMoodMsg.contains( "'" ) )
    //{
    //    QMessageBox::warning( curWidget, QObject::tr( "Application" ), QObject::tr( "Mood Message cannot have special character quote." ) );
    //    return false;
    //}

    int iLen = strMoodMsg.length();
    if( iLen > 31 )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Application" ), QObject::tr( "Mood Message is too long (maximum 31 chars)" ) );
        return false;
    }

    return true;
}

//============================================================================
bool GuiHelpers::validateAge( QWidget* curWidget, int age )
{
    if( age < 0 )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Age Verify" ), QObject::tr( "Invalid Age" ) );
        return false;
    }

    if( age > 120 )
    {
        QMessageBox::warning( curWidget, QObject::tr( "Age Verify" ), QObject::tr( "Age Connot be greater than 120 years old" ) );
        return false;
    }

    return true;
}

//============================================================================
void GuiHelpers::fillGender( QComboBox * comboBox )
{
    if( comboBox )
    {
        comboBox->clear();
        for( int i = 0; i < eMaxGenderType; i++ )
        {
            comboBox->addItem( GuiParams::describeGender( (EGenderType)i ) );
        }
    }
}

//============================================================================
EGenderType GuiHelpers::getGender( QComboBox * comboBox )
{
    return (EGenderType)comboBox->currentIndex();
}

//============================================================================
bool GuiHelpers::setGender( QComboBox * comboBox, EGenderType gender)
{
    if( comboBox )
    {
        comboBox->setCurrentIndex(genderToIndex(gender));
        return true;
    }
    else
    {
        return false;
    }
}

//============================================================================
uint8_t GuiHelpers::genderToIndex( EGenderType gender )
{
    if( ( gender >= 0 ) && ( gender < eMaxGenderType ) )
    {
        return ( uint8_t )gender;
    }

    return 0;
}

//============================================================================
void GuiHelpers::fillAge( QComboBox * comboBox )
{
    if( comboBox )
    {
        comboBox->clear();
        for( int i = 0; i < eMaxAgeType; i++ )
        {
            comboBox->addItem(  GuiParams::describeAge( (EAgeType)i ) );
        }
    }
}

//============================================================================
EAgeType GuiHelpers::getAge( QComboBox * comboBox )
{
    if( comboBox )
    {
        return (EAgeType)comboBox->currentIndex();
    }
    else
    {
        return eAgeTypeUnspecified;
    }
}

//============================================================================
bool GuiHelpers::setAge( QComboBox * comboBox, EAgeType ageType)
{
    if( comboBox )
    {
        comboBox->setCurrentIndex(ageToIndex(ageType));
        return true;
    }
    else
    {
        return false;
    }
}

//============================================================================
uint8_t GuiHelpers::ageToIndex( EAgeType age )
{
    if( ( age >= 0 ) && ( age < eMaxAgeType ) )
    {
        return ( uint8_t )age;
    }

    return 0;
}

//============================================================================
void GuiHelpers::fillContentRating( QComboBox * comboBox )
{
    if( comboBox )
    {
        comboBox->clear();
        for( int i = 0; i < eMaxContentRating; i++ )
        {
            comboBox->addItem(  GuiParams::describeContentRating( (EContentRating)i ) );
        }
    }
}

//============================================================================
EContentRating GuiHelpers::getContentRating( QComboBox * comboBox )
{
    if( comboBox )
    {
        return (EContentRating)comboBox->currentIndex();
    }
    else
    {
        return eContentRatingUnspecified;
    }
}

//============================================================================
bool GuiHelpers::setContentRating( QComboBox * comboBox, EContentRating contentRating)
{
    if( comboBox )
    {
        comboBox->setCurrentIndex(contentRatingToIndex(contentRating));
        return true;
    }
    else
    {
        return false;
    }
}

//============================================================================
uint8_t GuiHelpers::contentRatingToIndex( EContentRating rating )
{
    if( ( rating >= 0 ) && ( rating < eMaxContentRating ) )
    {
        return ( uint8_t )rating;
    }

    return 0;
}

//============================================================================
void GuiHelpers::fillContentCatagory( QComboBox * comboBox )
{
    if( comboBox )
    {
        comboBox->clear();
        for( int i = 0; i < eMaxContentCatagory; i++ )
        {
            comboBox->addItem(  GuiParams::describeContentCatagory( ( EContentCatagory )i ) );
        }
    }
}

//============================================================================
uint8_t GuiHelpers::contentCatagoryToIndex( EContentCatagory rating )
{
    if( ( rating >= 0 ) && ( rating < eMaxContentCatagory ) )
    {
        return ( uint8_t )rating;
    }

    return 0;
}

//============================================================================
void GuiHelpers::fillLanguage( QComboBox * comboBox )
{
    if( comboBox )
    {
        static const ELanguageType kSupportedLanguages[] =
        {
            eLangEnglish,
            eLangGerman,
            eLangChinese,
            eLangSpanish,
            eLangFrench,
            eLangArabic,
            eLangHindi,
            eLangPortuguese,
            eLangJapanese,
            eLangKorean,
            eLangRussian,
            eLangThai,
            eLangIndonesian,
        };

        comboBox->clear();
        for( ELanguageType language : kSupportedLanguages )
        {
            comboBox->addItem( GuiParams::describeLanguage( language ), (int)language );
        }
    }
}

//============================================================================
ELanguageType GuiHelpers::getLanguage( QComboBox * comboBox )
{
    if( comboBox )
    {
        QVariant langData = comboBox->currentData();
        if( langData.isValid() )
        {
            return (ELanguageType)langData.toInt();
        }

        int currentIdx = comboBox->currentIndex();
        if( ( 0 <= currentIdx ) && ( currentIdx < eMaxLanguageType ) )
        {
            return (ELanguageType)currentIdx;
        }

        return eLangUnspecified;
    }
    else
    {
        return eLangUnspecified;
    }
}

//============================================================================
bool GuiHelpers::setLanguage( QComboBox * comboBox, ELanguageType language)
{
    if( comboBox )
    {
        int itemIdx = comboBox->findData( (int)language );
        if( 0 <= itemIdx )
        {
            comboBox->setCurrentIndex( itemIdx );
        }
        else
        {
            comboBox->setCurrentIndex( languageToIndex( language ) );
        }

        return true;
    }
    else
    {
        return false;
    }
}

//============================================================================
uint16_t GuiHelpers::languageToIndex( ELanguageType language )
{
    switch( language )
    {
    case eLangEnglish:      return 0;
    case eLangGerman:       return 1;
    case eLangChinese:      return 2;
    case eLangSpanish:      return 3;
    case eLangFrench:       return 4;
    case eLangArabic:       return 5;
    case eLangHindi:        return 6;
    case eLangPortuguese:   return 7;
    case eLangJapanese:     return 8;
    case eLangKorean:       return 9;
    case eLangRussian:      return 10;
    case eLangThai:         return 11;
    case eLangIndonesian:   return 12;
    default:
        return 0;
    }
}

//============================================================================
void GuiHelpers::fillPermissionComboBox( QComboBox * permissionComboBox )
{
    if( permissionComboBox )
    {
        permissionComboBox->clear();
        permissionComboBox->addItem( GuiParams::describePermissionLevel( eFriendStateAdmin ) );
        permissionComboBox->addItem( GuiParams::describePermissionLevel( eFriendStateFriend ) );
        permissionComboBox->addItem( GuiParams::describePermissionLevel( eFriendStateGuest ) );
        permissionComboBox->addItem( GuiParams::describePermissionLevel( eFriendStateAnonymous ) );
        permissionComboBox->addItem( GuiParams::describePermissionLevel( eFriendStateIgnore ) );
    }
}

//============================================================================
EFriendState GuiHelpers::comboIdxToFriendState( int comboIdx )
{
    switch( comboIdx )
    {
    case 0:
        return eFriendStateAdmin;
    case 1:
        return eFriendStateFriend;
    case 2:
        return eFriendStateGuest;
    case 3:
        return eFriendStateAnonymous;
    default:
        return eFriendStateIgnore;
    }   
}

//============================================================================
int GuiHelpers::friendStateToComboIdx( EFriendState friendState )
{
    switch( friendState )
    {
    case eFriendStateAdmin:
        return 0;
    case eFriendStateFriend:
        return 1;
    case eFriendStateGuest:
        return 2;
    case eFriendStateAnonymous:
        return 3;
    default:
        return 4;
    }
}

//============================================================================
void GuiHelpers::fillExpireTimeComboBox( QComboBox* comboBox )
{
    if( comboBox )
    {
        comboBox->clear();
        for( int i = eExpireTimeWhenResponseRxed; i < eMaxExpireTime; i++ )
        {
            comboBox->addItem( GuiParams::describeExpireTime( (EExpireTime)i ) );
        }
    }
}

//============================================================================
int GuiHelpers::expireTimeComboIdxToSeconds( int comboIdx )
{
    return GuiParams::getExpireTimeSeconds( (EExpireTime)comboIdx );
}

//============================================================================
void GuiHelpers::fillJoinRequest( QComboBox* comboBox )
{
    if( comboBox )
    {
        comboBox->clear();
        comboBox->addItem( GuiParams::describeJoinState( eJoinStateJoinRequested ) );
        comboBox->addItem( GuiParams::describeJoinState( eJoinStateJoinIsGranted ) );
        comboBox->addItem( GuiParams::describeJoinState( eJoinStateJoinDenied ) );
    }
}

//============================================================================
EJoinState GuiHelpers::comboIdxToJoinState( int comboIdx )
{
    switch( comboIdx )
    {
    case 0: return eJoinStateJoinRequested;
    case 1: return eJoinStateJoinIsGranted;
    default: return  eJoinStateJoinDenied;
    }
}

//============================================================================
uint8_t GuiHelpers::joinRequestToIndex( EJoinState joinState )
{
    switch( joinState )
    {
    case eJoinStateJoinRequested:
        return 0;
    case eJoinStateJoinIsGranted:
        return 1;
    default:
        return 2;
    }
}

//============================================================================
void GuiHelpers::setValuesFromIdentity( QWidget* curWidget, VxNetIdent* ident, QComboBox *  ageCombo, QComboBox * genderCombo, QComboBox * languageCombo, QComboBox * contentCombo )
{
    if( curWidget && ident && ageCombo && genderCombo && languageCombo && contentCombo )
    {
        ageCombo->setCurrentIndex( ident->getAgeType() );
        genderCombo->setCurrentIndex( ident->getGender() );
        languageCombo->setCurrentIndex( ident->getPrimaryLanguage() );
        contentCombo->setCurrentIndex( ident->getPreferredContent() );
    }
}

//============================================================================
void GuiHelpers::setIdentityFromValues( QWidget* curWidget, VxNetIdent* ident, QComboBox * age, QComboBox * genderCombo, QComboBox * languageCombo, QComboBox * contentCombo )
{
    if( curWidget && ident && age && genderCombo && languageCombo && contentCombo )
    {
        int ageValue = age->currentIndex();
        if( ( 0 > ageValue ) || ( eMaxAgeType <= ageValue ) )
        {
            ageValue = 0;
        }

        ident->setAgeType( (EAgeType)ageValue );

        int genderValue = genderCombo->currentIndex();
        if( ( 0 > genderValue ) || ( eMaxGenderType <= genderValue ) )
        {
            genderValue = 0;
        }

        ident->setGender( (EGenderType)genderValue );

        int languageValue = languageCombo->currentIndex();
        if( ( 0 > languageValue ) || ( eMaxLanguageType <= genderValue ) )
        {
            languageValue = 0;
        }

        ident->setPrimaryLanguage( (ELanguageType)languageValue );

        int contentValue = contentCombo->currentIndex();
        if( ( 0 > contentValue ) || ( eMaxContentRating <= genderValue ) )
        {
            contentValue = 0;
        }

        ident->setPreferredContent( (EContentRating)contentValue );
    }
}

//============================================================================
ActivityBase* GuiHelpers::findParentActivity( QWidget* widget )
{
    ActivityBase* actBase = nullptr;
    QObject * objWidget = widget;
    while( objWidget )
    {
        ActivityBase* actTemp = dynamic_cast<ActivityBase*>( objWidget );
        if( actTemp )
        {
            actBase = actTemp;
            break;
        }

        objWidget = objWidget->parent();
    }

    return actBase;
}

//============================================================================
QWidget* GuiHelpers::findAppletContentFrame( QWidget* widget )
{
    ActivityBase* actBase = findLaunchWindow( widget );
    if( actBase )
    {
        return actBase->getContentItemsFrame();
    }

    return nullptr;
}

//============================================================================
QWidget* GuiHelpers::findParentPage( QWidget* parent ) // this should return home or messenger page
{
    // from title bar find the Home page or messenger
    QFrame* pageFrame = nullptr;
    QObject* curParent = parent;

    QString launchPageObjName = OBJNAME_FRAME_LAUNCH_PAGE;
    QString messengerPageObjName = OBJNAME_FRAME_MESSAGER_PAGE;

    while( curParent )
    {
        QString objName = curParent->objectName();
        if( (objName == launchPageObjName) || (objName == messengerPageObjName) )
        {
            pageFrame = dynamic_cast<QFrame*>(curParent);
            if( pageFrame )
            {
                break;
            }
        }

        if( !curParent->parent() )
        {
            LogMsg( LOG_WARNING, "Object %s has no parent", objName.toUtf8().constData() );
            break;
        }
        else
        {
            curParent = curParent->parent();
        }
    }

    return pageFrame;
}

//============================================================================
QWidget* GuiHelpers::findParentContentFrame( QWidget* parent )
{
    ActivityBase* actBase = findParentActivity( parent );
    if( actBase )
    {
        return actBase->getContentItemsFrame();
    }
    else
    {
        return findParentPage( parent );
    }

    return nullptr;
}

//============================================================================
ActivityBase* GuiHelpers::findLaunchWindow( QWidget* widget )
{
    QObject * objWidget = findParentActivity( widget );
    QObject * prevWidget = objWidget;
    while( objWidget )
    {
        if( dynamic_cast<VxFrame*>( objWidget ) )
        {
            return dynamic_cast<ActivityBase*>( prevWidget );
        }

        prevWidget = objWidget;
        objWidget = objWidget->parent();
    }

    return nullptr;
}

//============================================================================
bool GuiHelpers::widgetToPluginSettings( EPluginType pluginType, PluginSettingsWidget* settingsWidget, PluginSetting& pluginSetting )
{
    bool result = false;
    if( ePluginTypeInvalid != pluginType && settingsWidget )
    {
        pluginSetting.setPluginType( pluginType );
        pluginSetting.setContentRating( (EContentRating)settingsWidget->getContentRatingComboBox()->currentIndex() );
        pluginSetting.setLanguage( (ELanguageType)settingsWidget->getLanguageComboBox()->currentIndex() );
        pluginSetting.setGender( (EGenderType)settingsWidget->getGenderComboBox()->currentIndex() );
        pluginSetting.setAgeType( (EAgeType)settingsWidget->getAgeComboBox()->currentIndex() );
        pluginSetting.setPluginUrl( settingsWidget->getServiceUrlEdit()->text().toUtf8().constData() );

        pluginSetting.setTitle( settingsWidget->getServiceTitleEdit()->text().toUtf8().constData() );
        pluginSetting.setGreetingMsg( settingsWidget->getGreetingEdit()->toPlainText().toUtf8().constData() );
        pluginSetting.setRejectMsg( settingsWidget->getRejectEdit()->toPlainText().toUtf8().constData() );

        pluginSetting.setThumnailId( settingsWidget->getThumbnailChooseWidget()->updateAndGetThumbnailId(), settingsWidget->getThumbnailChooseWidget()->getThumbnailIsCircular() );

        QString description =settingsWidget->getServiceDescriptionEdit()->toPlainText().trimmed();
        if( !description.isEmpty() )
        {
            pluginSetting.setDescription( description.toUtf8().constData() );
        }
        else
        {
            pluginSetting.setDescription( "" );
        }

        result = true;
    }

    return result;
}

//============================================================================
bool GuiHelpers::pluginSettingsToWidget( EPluginType pluginType, PluginSetting& pluginSetting, PluginSettingsWidget* settingsWidget )
{
    bool result = false;
    if( ePluginTypeInvalid != pluginType && settingsWidget )
    {
        settingsWidget->getAgeComboBox()->setCurrentIndex( GuiHelpers::ageToIndex( pluginSetting.getAgeType() ) );
        settingsWidget->getGenderComboBox()->setCurrentIndex( GuiHelpers::genderToIndex( pluginSetting.getGender() ) );
        settingsWidget->getContentRatingComboBox()->setCurrentIndex( GuiHelpers::contentRatingToIndex( pluginSetting.getContentRating() ) );
        GuiHelpers::setLanguage( settingsWidget->getLanguageComboBox(), pluginSetting.getLanguage() );

        settingsWidget->getServiceUrlEdit()->setText( pluginSetting.getPluginUrl().c_str() );

        settingsWidget->getServiceTitleEdit()->setText( pluginSetting.getTitle().c_str() );
        settingsWidget->getServiceDescriptionEdit()->appendPlainText( pluginSetting.getDescription().c_str() );
        settingsWidget->getGreetingEdit()->appendPlainText( pluginSetting.getGreetingMsg().c_str() );
        settingsWidget->getRejectEdit()->appendPlainText( pluginSetting.getRejectMsg().c_str() );

        settingsWidget->getThumbnailChooseWidget()->loadThumbnail( pluginSetting.getThumnailId(), pluginSetting.getThumbnailIsCircular() );
 
        result = true;
    }

    return result;
}

//============================================================================
bool GuiHelpers::createThumbFileName( VxGUID& assetGuid, QString& retFileName )
{
    bool validFileName = false;
    if( assetGuid.isValid() )
    {
        retFileName = VxGetAppDirectory( eAppDirThumbs ).c_str();
        if( !retFileName.isEmpty() )
        {
            retFileName += assetGuid.toHexString().c_str();
            retFileName += ".nlt"; // use extension not known as image so thumbs will not be scanned by android image gallery etc
            validFileName = true;
        }
    }

    return validFileName;
}

//============================================================================
bool GuiHelpers::makeCircleImage( QImage& image )
{
    QPixmap target( image.width(), image.height() );
    target.fill( Qt::transparent );

    QPainter painter( &target );

    // Set clipped region (circle) in the center of the target image
    QRegion clipRegion( QRect( 0, 0, image.width(), image.height() ), QRegion::Ellipse );
    painter.setClipRegion( clipRegion );

    painter.drawImage( 0, 0, image );
    image = target.toImage();
    return !image.isNull();
}

//============================================================================
bool GuiHelpers::makeCircleImage( QPixmap& targetPixmap )
{
    QPixmap target( targetPixmap.width(), targetPixmap.height() );
    target.fill( Qt::transparent );

    QPainter painter( &target );

    // Set clipped region (circle) in the center of the target image
    QRegion clipRegion( QRect( 0, 0, targetPixmap.width(), targetPixmap.height() ), QRegion::Ellipse );
    painter.setClipRegion( clipRegion );

    painter.drawPixmap( 0, 0, targetPixmap );
    targetPixmap = target;
    return !targetPixmap.isNull();
}

//============================================================================
uint64_t GuiHelpers::saveToPngFile( QImage& image, QString& fileName ) // returns file length
{
    bool result = !image.isNull() && !fileName.isEmpty();
    if( result )
    {
        result = image.save( fileName, "PNG" );
    }
    else
    {
        LogMsg( LOG_ERROR, "GuiHelpers::saveToPngFile Invalid Param" );
        return 0;
    }

    if( result )
    {
        return VxFileUtil::fileExists( fileName.toUtf8().constData() );
    }

    return 0;
}

//============================================================================
uint64_t GuiHelpers::saveToPngFile( QPixmap& bitmap, QString& fileName ) // returns file length
{
    bool result = !bitmap.isNull() && !fileName.isEmpty();
    if( result )
    {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        if( !bitmap.isNull() )
        {
            result = bitmap.save( fileName, "PNG" );
#else
        const QPixmap* bitmap = m_ThumbPixmap;
        if( bitmap )
        {
            result = bitmap->save( fileName, "PNG" );
#endif // QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        }
    }
    else
    {
        LogMsg( LOG_ERROR, "GuiHelpers::%s Invalid Param", __func__ );
        return 0;
    }

    if( result )
    {
        return VxFileUtil::fileExists( fileName.toUtf8().constData() );
    }  

    return 0;
}

//============================================================================
bool GuiHelpers::generateMediaThumbnail( FileInfo& fileInfo, QString& retThumbFileName )
{
    bool result{ false };
    std::string fileName = fileInfo.getFileNameAndPath();
    EAssetType assetType = fileInfo.getAssetType();

    if( eAssetTypePhoto == assetType )
    {
        result = generateThumbFromImageFile( fileName, fileInfo.getThumbId(), retThumbFileName );
    }
    else if( eAssetTypeVideo == assetType || eAssetTypeAudio == assetType )
    {
        if( VxFileUtil::replaceExtension( fileName, "png" ) )
        {
            if( VxFileUtil::fileExists( fileName.c_str() ) )
            {
                result = generateThumbFromImageFile( fileName, fileInfo.getThumbId(), retThumbFileName );
            }
        }
    }
    else
    {
        if(LogEnabled(eLogThumbnail))LogModule( eLogThumbnail, LOG_WARN, "GuiHelpers::%s not a media type", __func__ );
    }

    return result;
}

//============================================================================
bool GuiHelpers::generateThumbFromImageFile( std::string fileName, VxGUID& thumbId, QString& retThumbFileName )
{
    QImage image( fileName.c_str() );

    if( image.isNull() ) 
    {
        if( LogEnabled( eLogThumbnail ) )LogModule( eLogThumbnail, LOG_WARN, "GuiHelpers::%s failed to load %s", __func__, fileName.c_str() );
        return false;
    }

    QSize thumbSize( GuiParams::getThumbnailSize() );
    image = image.scaled( thumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    if( image.isNull() )
    {
        if( LogEnabled( eLogThumbnail ) )LogModule( eLogThumbnail, LOG_WARN, "GuiHelpers::%s failed to scale %s", __func__, fileName.c_str() );
        return false;
    }

    VxGUID tmpThumbId = thumbId;
    if( !tmpThumbId.isValid() )
    {
        tmpThumbId.initializeWithNewVxGUID();
    }
    
    QString thumbFileName = generateThumbFileName( tmpThumbId );
    if( saveToPngFile( image, thumbFileName ) && VxFileUtil::fileExists( thumbFileName.toUtf8().constData() ) )
    {
        thumbId = tmpThumbId;
        retThumbFileName = thumbFileName;
        return true;
    }

    return false;
}

//============================================================================
QString GuiHelpers::generateThumbFileName( VxGUID& thumbId )
{
    QString retFileName = VxGetAppDirectory( eAppDirThumbs ).c_str();
    retFileName += thumbId.toHexString().c_str();
    retFileName += ".nlt"; // use extension not known as image so thumbs will not be scanned by android image gallery etc
    return retFileName;
}

//============================================================================
bool GuiHelpers::addThumbAsset( AppCommon& myApp, QString& thumbFileName, VxGUID thumbId, ThumbInfo& assetInfoOut )
{
    bool assetGenerated{ false };
    VxFileInfo fileInfo;
    if( VxFileUtil::getFileInfo( thumbFileName.toUtf8().constData(), fileInfo ) )
    {
        fileInfo.setFileType( VXFILE_TYPE_OTHER );
        ThumbInfo thumbInfo( fileInfo );
        thumbInfo.setAssetUniqueId( thumbId );
        thumbInfo.setCreatorId( myApp.getEngine().getMyOnlineId() );
        thumbInfo.setCreationTime( GetTimeStampMs() );
        if( myApp.getEngine().getThumbMgr().fromGuiThumbCreated(thumbInfo) )
        {
            assetGenerated = true;
            assetInfoOut = thumbInfo;
        }
        else
        {
            QString msgText = QObject::tr( "Could not create thumbnail asset" );
            QMessageBox::information( &myApp.getHomeWindow(), QObject::tr("Error occured creating thumbnail asset ") + thumbFileName, msgText);
        }
    }
    else
    {
        QString msgText = QObject::tr( "Could not get thumbnail file info" );
        QMessageBox::information( &myApp.getHomeWindow(), QObject::tr( "Error occured creating thumbnail asset " ) + thumbFileName, msgText );
    }

    return assetGenerated;
}

//============================================================================
void GuiHelpers::fillHostType( QComboBox* comboBox, bool excludePeerHost )
{
    comboBox->addItem( GuiParams::describeHostType( eHostTypeGroup ) );
    comboBox->addItem( GuiParams::describeHostType( eHostTypeChatRoom ) );
    comboBox->addItem( GuiParams::describeHostType( eHostTypeRandomConnect ) );
    comboBox->addItem( GuiParams::describeHostType( eHostTypeNetwork ) );
    comboBox->addItem( GuiParams::describeHostType( eHostTypeConnectTest ) );

    if( !excludePeerHost )
    { 
        comboBox->addItem( GuiParams::describeHostType( eHostTypePeerUser ) );
    }
}

//============================================================================
EHostType GuiHelpers::comboIdxToHostType( int comboIdx )
{
    switch( comboIdx )
    {
    case 0:
        return eHostTypeGroup;
    case 1:
        return eHostTypeChatRoom;
    case 2:
        return eHostTypeRandomConnect;
    case 3:
        return eHostTypeNetwork;
    case 4:
        return eHostTypeConnectTest;
    case 5:
        return eHostTypePeerUser;

    default:
        return eHostTypeUnknown;
    }
}

//============================================================================
QMessageBox::StandardButton GuiHelpers::errorMsgBox (EErrMsgType errMsgType, QWidget* parent, GuiUser* guiUser )
{
    std::string userName = guiUser ? guiUser->getOnlineName().c_str() : "Unknown";
    QMessageBox::StandardButton buttonResult{ QMessageBox::NoButton };
    switch( errMsgType )
    {
    case eErrMsgUserUnavailable:
        buttonResult = QMessageBox::information( parent, QObject::tr( "User Unavailable" ), 
            QObject::tr( "User " ) + userName.c_str() + QObject::tr( " Is Unavailable" ), QMessageBox::Ok );
        break;

    case eErrMsgUserIsOffline:
        buttonResult = QMessageBox::information( parent, QObject::tr( "User is offline" ),
            QObject::tr( "User is no longer connected" ), QMessageBox::Ok );
        break;

    case eErrMsgAlreadyInSession:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Already In A Session" ),
            QObject::tr( "Already in session. Please close existing session and try again" ), QMessageBox::Ok );
        break;

    case eErrMsgSessionNotFound:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Session not found" ),
            QObject::tr( "Session not found" ), QMessageBox::Ok );
        break;

    case eErrMsgOfferSent:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Sent User An Offer" ),
            QObject::tr( "Offer Was Sent To  " ) + userName.c_str(), QMessageBox::Ok );
        break;

    case eErrMsgOfferSendFailed:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Send Offer Failed" ),
            QObject::tr( "Offer Send Failed  " ), QMessageBox::Ok );
        break;

    case eErrMsgNotConnectedToHost:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Not Connected" ),
            QObject::tr( "You are not connected to host " ), QMessageBox::Ok );
        break;

    case eErrMsgNoUserSelectedToSendTo:
        buttonResult = QMessageBox::information( parent, QObject::tr( "No User Selected" ),
            QObject::tr( "You must select a user to send to " ), QMessageBox::Ok );
        break;

    case eErrMsgPurgeEverythingWarning:
        buttonResult = QMessageBox::question( parent, QObject::tr( "Purge Everthing Warning" ),
            QObject::tr( "This action will delete everything not locked including Identity, Downloaded Files, Storyboard, Message History Etc.\nThe action cannot be undone\nAre you very sure you want to proceed?" ) );
        break;

    case eErrMsgVoiceMessageTooShort:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Voice Message Too Short" ),
            QObject::tr( "The Voice Message was too short and will not be sent." ) );
        break;

    case eErrMsgVideoClipTooShort:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Video clip Too Short" ),
            QObject::tr( "The Video Clip was too short and will not be sent." ) );
        break;

    case eErrMsgVideoClipFailedToStart:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Video record failed to start" ),
            QObject::tr( "Video record failed to start." ) );
        break;

    default:
        buttonResult = QMessageBox::information( parent, QObject::tr( "Unknown Error" ),
            QObject::tr( "Unknown Error " ) + QString::number(errMsgType) + QObject::tr( " for user " ) + userName.c_str(), QMessageBox::Ok );
        break;
    }

    return buttonResult;
}

//============================================================================
void GuiHelpers::showFileNameEmptyError( QWidget* parent )
{
    QString deniedPermTitle = QObject::tr( "File Not Found" );
    QString deniedPermMsg = QObject::tr( "File Name Is Empty" );
    QMessageBox warnStorage( QMessageBox::Icon::Information, deniedPermMsg, deniedPermMsg, QMessageBox::Ok );
    warnStorage.exec();
}

//============================================================================
void GuiHelpers::showFilePermissionError( QWidget* parent )
{
    QString deniedPermTitle = QObject::tr("Access File Permissions Denied By User");
    QString deniedPermMsg = QObject::tr("Access File Permissions Denied By User");
    QMessageBox warnStorage( QMessageBox::Icon::Information, deniedPermMsg, deniedPermMsg, QMessageBox::Ok);
    warnStorage.exec();
}

//============================================================================
void GuiHelpers::showApplicationNotReadyError( bool appReadyButNetworkNotReady, QWidget* parent )
{
    QString notReadyTitle = QObject::tr("Application Not Ready");
    QString notReadyMsg = appReadyButNetworkNotReady ? QObject::tr("Cannot launch applet until network is available") : QObject::tr("Cannot Launch Applet Until Application Has Initialized");
    QMessageBox warnStorage( QMessageBox::Icon::Information, notReadyTitle, notReadyMsg, QMessageBox::Ok);
    warnStorage.exec();
}

//============================================================================
void GuiHelpers::showRequiresOpenPort( QWidget* parent )
{
    QString title = QObject::tr("Requires Open Port");
    QString msg = QObject::tr("Action requires a open port");
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok);
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showInviteInvalidError( QWidget* parent )
{
    QString title = QObject::tr("Invalid Invite");
    QString msg = QObject::tr("The invite is invalid");
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok);
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showInviteMyselfError( QWidget* parent )
{
    QString title = QObject::tr("Invalid Invite");
    QString msg = QObject::tr("Cannot accept invite from myself");
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok);
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showUserNotFoundError( QWidget* parent )
{
    QString title = QObject::tr("Invalid User");
    QString msg = QObject::tr("Could not find the user");
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok);
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showUserNotOnlineError( QWidget* parent )
{
    QString title = QObject::tr("User Not Online");
    QString msg = QObject::tr("The user is not currently online");
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok);
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showRequiresFriendshipError( QWidget* parent )
{
    QString title = QObject::tr("Friendship Level To Low");
    QString msg = QObject::tr("Requires friendship of friend or higher");
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok);
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showInvalidHostIdError( QWidget* parent )
{
    QString title = QObject::tr( "Invalid Host Id" );
    QString msg = QObject::tr( "Host Id has not been set"  );
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showInvalidUrlrror( QWidget* parent )
{
    QString title = QObject::tr( "Invalid URL" );
    QString msg = QObject::tr( "The host url is not valid" );
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showInvalidHostTypeError( QWidget * parent )
{
    QString title = QObject::tr( "Invalid Host Type" );
    QString msg = QObject::tr( "The host url does not have a vailid host type" );
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showNoMembersOnlineError( QWidget* parent )
{
    QString title = QObject::tr( "No Members Online" );
    QString msg = QObject::tr( "There are no members online to send to" );
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showAddAssetFailedError()
{
    QString title = QObject::tr( "Failed to add asset" );
    QString msg = QObject::tr( "Failed to add asset. Please check disk space" );
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showFailedToSendMemberError( QString userName )
{
    LogMsg( LOG_ERROR, "GuiHelpers::%s user=%s", __func__, userName.toUtf8().constData() );
    QString title = QObject::tr( "Failed to send " );
    QString msg = QObject::tr( "Failed to send to " ) + userName;
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showCannotSendReason( ECanSendState canSendState )
{
    if( canSendState == ECanSendState::eNoMembersToSendTo )
    {
        GuiHelpers::showCannotSendReason( QObject::tr( "No members to send to" ) );
    }
    else if( canSendState == ECanSendState::eAdminIsOffline )
    {
        GuiHelpers::showCannotSendReason( QObject::tr( "Admin is offline" ) );
    }
    else if( canSendState == ECanSendState::eCannotSendToSelf )
    {
        GuiHelpers::showCannotSendReason( QObject::tr( "Cannot send to self" ) );
    }
    else if( canSendState == ECanSendState::eInvalidHostOrState )
    {
        GuiHelpers::showCannotSendReason( QObject::tr( "Invalid host or state" ) );
    }
    else
    {
        GuiHelpers::showCannotSendReason( QObject::tr( "Unknown reason" ) );
    }
}

//============================================================================
void GuiHelpers::showCannotSendReason( QString reason )
{
    LogMsg( LOG_ERROR, "GuiHelpers::%s reason=%s", __func__, reason.toUtf8().constData() );
    QString title = QObject::tr( "Cannot send " );
    QString msg = QObject::tr( "Cannot send reason: " ) + reason;
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::showHostIsDisabledError( EHostType hostType )
{
    QString title = QObject::tr( "Failed to add asset" );
    QString msg = QObject::tr( "Failed to add asset. Please check disk space" );
    QMessageBox warnMsg( QMessageBox::Icon::Information, title, msg, QMessageBox::Ok );
    warnMsg.exec();
}

//============================================================================
void GuiHelpers::processQtEvents( int ms )
{
	QCoreApplication::processEvents( QEventLoop::AllEvents, ms );
}

//============================================================================
void GuiHelpers::showCreateInvite( EHostType hostType, QWidget* parent )
{
    if( !GetAppInstance().getIsMyPortOpen() )
	{
		GuiHelpers::showRequiresOpenPort();
		return;
	}

	ActivityBase* inviteBase = GetAppInstance().getAppletMgr().launchApplet( eAppletInviteCreate, parent );
	AppletInviteCreate* appletInvite = dynamic_cast<AppletInviteCreate*>(inviteBase);
	if( appletInvite )
	{
		appletInvite->setInviteType( hostType );
	}
}

//============================================================================
bool GuiHelpers::confirmDeleteFile( AppCommon& appCommon, QFrame* contentFrame, bool shredFile, QString fileName )
{
    
    bool isConfirmDisabled = appCommon.getAppSettings().getIsConfirmDeleteDisabled();
    if( isConfirmDisabled )
    {
        return true;
    }

    bool acceptAction = true;
    QString title = shredFile ? QObject::tr( "Confirm Shred File" ) : QObject::tr( "Confirm Delete File" );
    QString bodyText = "";
    if( shredFile )
    {
        bodyText = QObject::tr( "Are You Sure You Want To Write Random Data Into The File Then Delete From The Device?" );
    }
    else
    {
        bodyText = QObject::tr( "Are You Sure To Delete The File From The Device?" );
    }

    if( !fileName.isEmpty() )
    {
        bodyText += "\n";
        bodyText += fileName;
    }

    ActivityMsgBoxYesNo dlg( appCommon, contentFrame, title, bodyText );
    dlg.makeNeverShowAgainVisible( false );
    if( false == ( QDialog::Accepted == dlg.exec() ) )
    {
        acceptAction = false;
    }

    if( dlg.wasNeverShowAgainChecked() )
    {
        appCommon.getAppSettings().setIsConfirmDeleteDisabled( true );
    }

    return acceptAction;
}
