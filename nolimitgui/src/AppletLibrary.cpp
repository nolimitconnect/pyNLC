//============================================================================
// Copyright (C) 2019 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletLibrary.h"

#include "ActivityBrowseFiles.h"
#include "ActivityMsgBoxYesNo.h"

#include "AppCommon.h"
#include "AppGlobals.h"
#include "AppSettings.h"

#include "AppletAboutFile.h"
#include "AppletMgr.h"

#include "FileShareItemWidget.h"
#include "FileItemInfo.h"
#include "FileActionMenu.h"
#include "GuiHelpers.h"
#include "GuiParams.h"
#include "GuiPlayerMgr.h"

#include <AssetBase/AssetPlaySession.h>
#include <AssetMgr/AssetMgr.h>
#include <AssetMgr/AssetInfo.h>
#include <P2PEngine/P2PEngine.h>

#include <PktLib/VxSearchDefs.h>
#include <NetLib/VxFileXferInfo.h>

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxFileInfo.h>
#include <CoreLib/VxDebug.h>

#include "ui_AppletLibrary.h"

//============================================================================
AppletLibrary::AppletLibrary( AppCommon& app, QWidget* parent, QString launchParam )
    : AppletBase( OBJNAME_APPLET_LIBRARY, app, parent )
    , ui(*(new Ui::AppletLibraryUi))
    , m_ePluginType( ePluginTypeInvalid )
    , m_IsSelectAFileMode( !launchParam.isEmpty() ? true : false )
{
    setAppletType( eAppletLibrary );
    ui.setupUi( getContentItemsFrame() );
    setTitleBarText( DescribeApplet( m_EAppletType ) );

    m_eFileFilterType = getAppletFileFilter( getAppletType() );
    setFileFilter( m_eFileFilterType );

#if defined(TARGET_OS_ANDROID)
    // android does not allow normal file browsing without special permissions
    ui.m_FileMediaSelectWidget->setScanFolderVisible( false );
#else
    connect( ui.m_FileMediaSelectWidget, SIGNAL(signalFileFolderSelected()), this, SLOT(slotFileFolderSelected()) );
#endif // defined(TARGET_OS_ANDROID)

    connect( ui.m_FileMediaSelectWidget, SIGNAL(signalFileMediaSelected(EMediaFileType)), this, SLOT(slotFileMediaSelected(EMediaFileType)) );

    connect( ui.m_FileFilterSelectWidget, SIGNAL(signalFileFilterChanged(EFileFilterType)), this, SLOT(slotApplyFileFilter(EFileFilterType)) );

    statusMsg( QObject::tr( "Requesting Library File List " ) );

    slotApplyFileFilter( m_eFileFilterType );
    connectBarWidgets();

    wantFileXferCallbacks( true );
    m_MyApp.activityStateChange( this, true );
}

//============================================================================
AppletLibrary::~AppletLibrary()
{
    wantFileXferCallbacks( false );
    m_MyApp.activityStateChange( this, false );
}

//============================================================================
void AppletLibrary::showEvent( QShowEvent* ev )
{
    AppletBase::showEvent( ev );
    m_MyApp.setIsLibraryActivityActive( true );
    wantFileXferCallbacks( true );
    slotRequestFileList();
}

//============================================================================
void AppletLibrary::hideEvent( QHideEvent* ev )
{
    if( m_IsScanningFolder )
    {
        m_IsScanningFolder = false;
        m_FromGui.fromGuiScanFolderCancel( getAppletInstId() );
        ui.m_FileMediaSelectWidget->setScanCancelEnable( m_IsScanningFolder );
    }

    wantFileXferCallbacks( false );
    AppletBase::hideEvent( ev );
    m_MyApp.setIsLibraryActivityActive( false );
}

//============================================================================
void AppletLibrary::callbackToGuiFileList( VxGUID& appInstId, FileInfo& fileInfo )
{
    if( appInstId == getAppletInstId() )
    {
        if( ui.m_FileItemList->count() == 0 )
        {
            updateStorageSpace( fileInfo.getFileNameAndPath() );
        }

        addFile( fileInfo );
    }
}

//============================================================================
void AppletLibrary::callbackToGuiFileListCompleted( VxGUID& appInstId )
{
    if( appInstId == getAppletInstId() )
    {
        statusMsg( QObject::tr( "List Get Completed" ) );
    }
}

//============================================================================
void AppletLibrary::callbackToGuiFolderScan( VxGUID& appInstId, FileInfo& fileInfo )
{
    if( appInstId == getAppletInstId() )
    {
        updateFromFileInfo( fileInfo, false );
        m_FromGui.fromGuiScanItemReceived( getAppletInstId() );
    }
}

//============================================================================
void AppletLibrary::callbackToGuiFolderScanCompleted( VxGUID& appInstId, bool wasCanceled )
{
    if( appInstId == getAppletInstId() )
    {
        m_IsScanningFolder = false;
        ui.m_FileMediaSelectWidget->setScanCancelEnable( m_IsScanningFolder );
        if( wasCanceled )
        {
            statusMsg( QObject::tr( "Folder Scan Canceled" ) );
        }
        else
        {
            statusMsg( QObject::tr( "Folder Scan Completed" ) );
        }
    }
}

//============================================================================
void AppletLibrary::toGuiFileDeleted( QString& fileName )
{
    FileShareItemWidget* poWidget;
    int iIdx = 0;
    while( iIdx < ui.m_FileItemList->count() )
    {
        poWidget = (FileShareItemWidget*)ui.m_FileItemList->item( iIdx );
        if( poWidget )
        {
            FileItemInfo* poFileInfo = (FileItemInfo*)poWidget->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
            if( poFileInfo && (poFileInfo->getFileNameAndPath() == fileName) )
            {
                poWidget->deleteLater();
                break;
            }
        }

        iIdx++;
    }

    updateStorageSpace( fileName.toUtf8().constData() );
}

//============================================================================
void AppletLibrary::setFileFilter( EFileFilterType fileFilter )
{
    m_eFileFilterType = fileFilter;
    ui.m_FileFilterSelectWidget->setFileFilter( m_eFileFilterType );
}

//============================================================================
void AppletLibrary::slotApplyFileFilter( EFileFilterType fileFilter )
{
    m_eFileFilterType = fileFilter;
    slotRequestFileList();
}

//============================================================================
void AppletLibrary::slotRequestFileList( void )
{
    clearFileList();
    setAppletFileFilter( getAppletType(), m_eFileFilterType );
    m_FromGui.fromGuiGetFileLibraryList( getAppletInstId(), FileFilterToVxFileType( m_eFileFilterType ) );
}

//============================================================================
FileShareItemWidget* AppletLibrary::fileToWidget( FileInfo& fileInfo )
{
    FileShareItemWidget* item = new FileShareItemWidget( ui.m_FileItemList );
    item->setSizeHint( QSize( (int)(GuiParams::getGuiScale() * 200), GuiParams::getFileListEntryHeight() ) );

    FileItemInfo* poItemInfo = new FileItemInfo( fileInfo );
    item->QListWidgetItem::setData( Qt::UserRole + 1, QVariant( ( quint64 )poItemInfo ) );

    connect( item,
             SIGNAL(signalFileShareItemClicked(QListWidgetItem*)),
             this,
             SLOT(slotListItemClicked(QListWidgetItem*)) );

    connect( item,
             SIGNAL(signalFileIconClicked(QListWidgetItem*)),
             this,
             SLOT(slotListFileIconClicked(QListWidgetItem*)) );

    connect( item,
             SIGNAL(signalPlayButtonClicked(QListWidgetItem*)),
             this,
             SLOT(slotListPlayIconClicked(QListWidgetItem*)) );
        
    connect( item,
             SIGNAL(signalPlayExternButtonClicked(QListWidgetItem*)),
             this,
             SLOT(slotListPlayExternIconClicked(QListWidgetItem*)) );

    connect( item,
             SIGNAL(signalLibraryButtonClicked(QListWidgetItem*)),
             this,
             SLOT(slotListLibraryIconClicked(QListWidgetItem*)) );

    connect( item,
             SIGNAL(signalFileShareButtonClicked(QListWidgetItem*)),
             this,
             SLOT(slotListShareFileIconClicked(QListWidgetItem*)) );

    connect( item,
            SIGNAL(signalAboutFileButtonClicked(QListWidgetItem*)),
            this,
            SLOT(slotListAboutFileClicked(QListWidgetItem*)) );

    connect( item,
             SIGNAL(signalShredButtonClicked(QListWidgetItem*)),
             this,
             SLOT(slotListShredIconClicked(QListWidgetItem*)) );

    item->updateWidgetFromInfo();
    return item;
}

//============================================================================
void AppletLibrary::slotListFileIconClicked( QListWidgetItem* item )
{
    slotListPlayIconClicked( item );
}

//============================================================================
void AppletLibrary::slotListShareFileIconClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( ( FileShareItemWidget* )item )->getFileItemInfo();
    if( poInfo )
    {
        if( VXFILE_TYPE_DIRECTORY == poInfo->getFileType() )
        {
        }
        else
        {
            // is file
            bool wasInUse = poInfo->getIsInUse();
            bool isShared = poInfo->toggleIsShared();
            if( !wasInUse && isShared && !poInfo->getThumbId().isValid() )
            {
                // new instertion. generate thumbnail if available
                generateThumb( poInfo );
            }
            
            m_Engine.fromGuiSetFileIsShared( poInfo->getFileInfo(), poInfo->getIsSharedFile() );
            if( !poInfo->getIsInUse() )
            {
                poInfo->getThumbId().clear();
            }

            ( (FileShareItemWidget*)item )->updateWidgetFromInfo();
        }
    }
}

//============================================================================
void AppletLibrary::slotListLibraryIconClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( (FileShareItemWidget*)item )->getFileItemInfo();
    if( poInfo )
    {
        if( VXFILE_TYPE_DIRECTORY == poInfo->getFileType() )
        {
        }
        else
        {
            // is file
            bool wasInUse = poInfo->getIsInUse();
            bool inLibrary = poInfo->toggleIsInLibrary();
            if( !wasInUse && inLibrary && !poInfo->getThumbId().isValid() )
            {
                // new instertion. generate thumbnail if available
                generateThumb( poInfo );
            }

            m_Engine.fromGuiSetFileIsInLibrary( poInfo->getFileInfo(), inLibrary );
            if( !poInfo->getIsInUse() )
            {
                poInfo->getThumbId().clear();
            }

            ( (FileShareItemWidget*)item )->updateWidgetFromInfo();
        }
    }
}

//============================================================================
void AppletLibrary::slotListAboutFileClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( (FileShareItemWidget*)item )->getFileItemInfo();
    if( poInfo )
    {
        if( VXFILE_TYPE_DIRECTORY == poInfo->getFileType() )
        {
        }
        else
        {
            AppletAboutFile* aboutFile = dynamic_cast<AppletAboutFile*>( m_MyApp.getAppletMgr().launchApplet( eAppletAboutFile, getParentPageFrame() ) );
            if( aboutFile )
            {
                aboutFile->setFileInfo( poInfo->getFileInfo() );
            }
        }
    }
}

//============================================================================
void AppletLibrary::handleMissingFilePlayAttempt( QListWidgetItem* item, FileItemInfo* poInfo )
{
    if( !poInfo )
    {
        return;
    }

    QString missingPath = poInfo->getFileNameAndPath();
    QString confirmText = QObject::tr( "File was not found:\n%1\n\nRemove this item from My Library and asset database?" ).arg( missingPath );
    QMessageBox::StandardButton button = QMessageBox::question(
        this,
        QObject::tr( "File Not Found" ),
        confirmText,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes );

    if( QMessageBox::Yes != button )
    {
        return;
    }

    // Always clear library membership for this file path.
    FileInfo missingFileInfo = poInfo->getFileInfo();
    missingFileInfo.setIsInLibrary( false );
    m_Engine.fromGuiSetFileIsInLibrary( missingFileInfo, false );

    // Remove asset record if it exists so the stale entry is removed from asset manager/database.
    AssetBaseInfo* assetInfo = m_Engine.getAssetMgr().findAsset( missingFileInfo.getFileNameAndPath() );
    if( assetInfo )
    {
        m_Engine.fromGuiAssetAction( eAssetActionRemoveFromAssetMgr, *assetInfo, 0 );
    }

    int row = ui.m_FileItemList->row( item );
    if( row >= 0 )
    {
        QListWidgetItem* removedItem = ui.m_FileItemList->takeItem( row );
        delete removedItem;
    }
}

//============================================================================
void AppletLibrary::slotListPlayIconClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( ( FileShareItemWidget* )item )->getFileItemInfo();
    if( poInfo )
    {
        if( VXFILE_TYPE_DIRECTORY == poInfo->getFileType() )
        {
        }
        else
        {
            bool newAsset{ false };
            AssetInfo assetInfo;
            if( poInfo->toAsssetInfo( m_MyApp, assetInfo, &newAsset ) )
            {
                if( newAsset )
                {
                    assetInfo.setLocationFlags( ASSET_LOC_FLAG_LIBRARY );
                    m_MyApp.getEngine().fromGuiAssetAction( eAssetActionAddToAssetMgr, assetInfo );
                }

                AssetPlaySession playSession( assetInfo );
                m_MyApp.getPlayerMgr().playMedia( playSession, false, 0, getParentPageFrame() );
            }
            else
            {
                handleMissingFilePlayAttempt( item, poInfo );
            }
        }
    }
}

//============================================================================
void AppletLibrary::slotListPlayExternIconClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( ( FileShareItemWidget* )item )->getFileItemInfo();
    if( poInfo )
    {
        if( VXFILE_TYPE_DIRECTORY == poInfo->getFileType() )
        {
        }
        else
        {
            bool newAsset{ false };
            AssetInfo assetInfo;
            if( poInfo->toAsssetInfo( m_MyApp, assetInfo, &newAsset ) )
            {
                if( newAsset )
                {
                    assetInfo.setLocationFlags( ASSET_LOC_FLAG_LIBRARY );
                    m_MyApp.getEngine().fromGuiAssetAction( eAssetActionAddToAssetMgr, assetInfo );
                }

                AssetPlaySession playSession( assetInfo );
                m_MyApp.getPlayerMgr().playMedia( playSession, true, 0, getParentPageFrame() );
            }
            else
            {
                handleMissingFilePlayAttempt( item, poInfo );
            }
        }
    }
}

//============================================================================
void AppletLibrary::slotListShredIconClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( ( FileShareItemWidget* )item )->getFileItemInfo();
    if( poInfo )
    {
        if( VXFILE_TYPE_DIRECTORY == poInfo->getFileType() )
        {
        }
        else
        {
            // shred file
            AssetInfo assetInfo;
            if( poInfo->toAsssetInfo( m_MyApp, assetInfo ) )
            {
                if( confirmDeleteFile( assetInfo, true ) )
                {
                    ui.m_FileItemList->removeItemWidget( item );
                    m_Engine.fromGuiDeleteFile( assetInfo.getAssetNameAndPath().c_str(), true );
                }
            }
        }
    }
}

//============================================================================
//!	get friend from QListWidgetItem data
FileItemInfo* AppletLibrary::widgetToFileItemInfo( FileShareItemWidget* item )
{
    return ( FileItemInfo* )item->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
}

//============================================================================
FileShareItemWidget* AppletLibrary::findListEntryWidget( FileInfo& fileInfo )
{
    int iIdx = 0;
    FileShareItemWidget* poWidget;
    while( iIdx < ui.m_FileItemList->count() )
    {
        poWidget = ( FileShareItemWidget* )ui.m_FileItemList->item( iIdx );
        if( poWidget )
        {
            FileItemInfo* poFileInfo = ( FileItemInfo* )poWidget->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
            if( poFileInfo && ( poFileInfo->getFileInfo().getFileNameAndPath() == fileInfo.getFileNameAndPath() ) )
            {
                return poWidget;
            }
        }

        iIdx++;
    }

    return nullptr;
}

//============================================================================
void AppletLibrary::addFile( FileInfo& fileInfo )
{
    FileShareItemWidget* existingItem = findItemByFileName( fileInfo.getFileNameAndPath().c_str() );
    if( existingItem )
    {
        FileItemInfo* poItemInfo = existingItem->getFileItemInfo();
        if( poItemInfo )
        {
            poItemInfo->setFileInfo( fileInfo );
            existingItem->update();
        }
    }

    if( !existingItem )
    {
        FileShareItemWidget* item = fileToWidget( fileInfo );
        if( item )
        {
            insertItemInFileNameOrder( item, fileInfo.getFileName() );
        }
    }
}

//============================================================================
//! user selected menu item
void AppletLibrary::slotListItemClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( FileItemInfo* )item->data( Qt::UserRole + 1 ).toLongLong();
    if( poInfo )
    {
        FileInfo& fileInfo = poInfo->getFileInfo();
        if( m_IsSelectAFileMode )
        {
            m_FileWasSelected = true;
            m_SelectedFileType = fileInfo.getFileType();
            m_SelectedFileName = fileInfo.getFileNameAndPath().c_str();
            m_SelectedFileLen = fileInfo.getFileLength();
            m_SelectedFileIsShared = poInfo->getIsSharedFile();
            m_SelectedFileIsInLibrary = poInfo->getIsInLibrary();
            accept();
        }
        else
        {
            playFile( fileInfo.getFileNameAndPath().c_str(), 0, false, false );
        }
    }
}

//============================================================================
//! user double clicked menu item
void AppletLibrary::slotListItemDoubleClicked( QListWidgetItem* item )
{
    slotListItemClicked( item );
}

//============================================================================
void AppletLibrary::clearFileList( void )
{
    ui.m_FileItemList->clear();
}

//============================================================================
FileShareItemWidget* AppletLibrary::findItemByFileName( QString fileName )
{
    int iIdx = 0;
    FileShareItemWidget* poWidget;
    while( iIdx < ui.m_FileItemList->count() )
    {
        poWidget = (FileShareItemWidget*)ui.m_FileItemList->item( iIdx );
        if( poWidget )
        {
            FileItemInfo* poFileInfo = (FileItemInfo*)poWidget->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
            if( poFileInfo && (poFileInfo->getFileNameAndPath() == fileName) )
            {
                return poWidget;
            }
        }

        iIdx++;
    }

    return nullptr;
}

//============================================================================
void AppletLibrary::updateStorageSpace( std::string fileNameAndPath )
{
	uint64_t diskFreeSpace = m_Engine.fromGuiGetDiskFreeSpace( fileNameAndPath.c_str() );
	if( ( 0 != diskFreeSpace ) && ( diskFreeSpace < 1000000000 ) )
	{
        m_MyApp.toGuiUserMessage( "Storage Space is low %s", GuiParams::describeFileLength( diskFreeSpace ).toUtf8().constData() );
	}

    ui.m_StatusLabel->setText( QObject::tr( "Storage Space Available: " ) + GuiParams::describeFileLength( diskFreeSpace ) );
}

//============================================================================
void AppletLibrary::statusMsg( QString strMsg )
{
    ui.m_StatusLabel->setText( strMsg );
}

//============================================================================
void AppletLibrary::slotFileMediaSelected( EMediaFileType mediaFileType )
{
    browseForFile( mediaFileType );
}

//============================================================================
void AppletLibrary::browseForFile( EMediaFileType mediaFileType )
{
    std::string lastDir;
    switch( mediaFileType )
    {
    case eMediaFileVideo:
        m_MyApp.getAppSettings().getLastLibraryVideoDir( lastDir );
        break;
    case eMediaFileAudio:
        m_MyApp.getAppSettings().getLastLibraryAudioDir( lastDir );
        break;
    case eMediaFileImage:
        m_MyApp.getAppSettings().getLastLibraryImageDir( lastDir );
        break;
    default:
        QMessageBox::information( this, QObject::tr( "Error" ), QObject::tr( "Unknown Media Type" ), QMessageBox::Ok );
        return;
    }

    QString startDir;
    if( !lastDir.empty() )
    {
        startDir = lastDir.c_str();
    }
    
    FileInfo fileInfo;
    if( GuiHelpers::browseForFile( this, mediaFileType, fileInfo, startDir ) )
    {
        std::string fileNameAndPath = fileInfo.getFileNameAndPath();
        std::string justPath = fileInfo.getFilePath();
        std::string justName = fileInfo.getFileName();
        if( !justPath.empty() )
        {
            switch( mediaFileType )
            {
            case eMediaFileVideo:
                m_MyApp.getAppSettings().setLastLibraryVideoDir( justPath );
                break;
            case eMediaFileAudio:
                m_MyApp.getAppSettings().setLastLibraryAudioDir( justPath );
                break;
            case eMediaFileImage:
                m_MyApp.getAppSettings().setLastLibraryImageDir( justPath );
                break;
            default:
                QMessageBox::information( this, QObject::tr( "Error" ), QObject::tr( "Unknown Media Type" ), QMessageBox::Ok );
                return;
            }
        }

        updateFromFileInfo( fileInfo, true );
    }
}

//============================================================================
void AppletLibrary::wantFileXferCallbacks( bool enable )
{
    if( m_FileXferCallbacksRequested != enable )
    {
        m_FileXferCallbacksRequested = enable;
        m_MyApp.getFileXferMgr().wantToGuiFileXferCallbacks( this, enable );
    }
}

//============================================================================
void AppletLibrary::generateThumb( FileItemInfo* poInfo )
{
    FileInfo fileInfo = poInfo->getFileInfo();
    if( !fileInfo.getThumbId().isValid() )
    {
        QString thumbFileName;
        if( GuiHelpers::generateMediaThumbnail( fileInfo, thumbFileName ) )
        {
            ThumbInfo thumbInfo;
            if( !GuiHelpers::addThumbAsset( m_MyApp, thumbFileName, fileInfo.getThumbId(), thumbInfo ) )
            {
                QString msgText = QObject::tr( "Could not get thumbnail file info" );
                QMessageBox::information( this, QObject::tr( "Error occured creating thumbnail asset " ) + thumbFileName, msgText );
                fileInfo.getThumbId().clear();
            }
            else
            {
                poInfo->setThumbId( fileInfo.getThumbId() );
            }
        }
    }
}

//============================================================================
void AppletLibrary::slotFileFolderSelected( void )
{
    if( m_IsScanningFolder )
    {
        m_IsScanningFolder = false;
        ui.m_FileMediaSelectWidget->setScanCancelEnable( m_IsScanningFolder );
        m_FromGui.fromGuiScanFolderCancel( getAppletInstId() );
    }
    else
    {
        std::string addFileDir;
        m_MyApp.getAppSettings().getLastFolderScanDir( addFileDir );
        QString curDir;
        if( !addFileDir.empty() )
        {
            curDir = addFileDir.c_str();
        }

        std::string scanDir = GuiHelpers::browseForDirectory( addFileDir.c_str(), this );
        if( scanDir.empty() )
        {
            return;
        }

        m_MyApp.getAppSettings().setLastFolderScanDir( addFileDir );
        statusMsg( QObject::tr( "Scaning folder" ) );
        m_IsScanningFolder = true;
        ui.m_FileMediaSelectWidget->setScanCancelEnable( m_IsScanningFolder );
        m_FromGui.fromGuiScanFolderForMedia( getAppletInstId(), scanDir, VXFILE_TYPE_AUDIO_VIDEO_PHOTO );
    }
}

//============================================================================
void AppletLibrary::updateFromFileInfo( FileInfo& fileInfo, bool showUserPopup )
{
    // see if file is already a asset
    AssetMgr& assetMgr = m_MyApp.getEngine().getAssetMgr();
    assetMgr.lockResources();
    AssetBaseInfo* assetInfo = assetMgr.findAsset( fileInfo.getFileNameAndPath() );
    if( assetInfo )
    {
        if( assetInfo->isInLibrary() )
        {
            assetMgr.unlockResources();
            if( showUserPopup )
            {
                QMessageBox::information( this, QObject::tr( "Already in library" ), QObject::tr( "File is already in library " ), QMessageBox::Ok );
            }
        }
        else
        {
            assetInfo->setIsInLibrary( true );
            FileInfo fileInfoInLibrary = assetInfo->getFileInfo();
            if( !fileInfoInLibrary.getThumbId().isValid() )
            {
                QString thumbFileName;
                if( GuiHelpers::generateMediaThumbnail( fileInfoInLibrary, thumbFileName ) )
                {
                    assetInfo->setThumbId( fileInfoInLibrary.getThumbId() );
                    ThumbInfo thumbInfo;
                    if( !GuiHelpers::addThumbAsset( m_MyApp, thumbFileName, assetInfo->getFileInfo().getThumbId(), thumbInfo ) )
                    {
                        if( showUserPopup )
                        {
                            QString msgText = QObject::tr( "Could not get thumbnail file info" );
                            QMessageBox::information( this, QObject::tr( "Error occured creating thumbnail asset " ) + thumbFileName, msgText );
                        }

                        assetInfo->getFileInfo().getThumbId().clear();
                    }
                    else
                    {
                        assetMgr.updateDatabase( assetInfo );
                    }
                }
            }

            assetMgr.unlockResources();

            addFile( fileInfoInLibrary );
        }
    }
    else
    {
        assetMgr.unlockResources();

        AssetInfo newAsset( fileInfo );
        newAsset.setIsInLibrary( true );

        if( !newAsset.getThumbId().isValid() )
        {
            QString thumbFileName;
            if( GuiHelpers::generateMediaThumbnail( fileInfo, thumbFileName ) )
            {
                newAsset.setThumbId( fileInfo.getThumbId() );
                ThumbInfo thumbInfo;
                if( !GuiHelpers::addThumbAsset( m_MyApp, thumbFileName, newAsset.getFileInfo().getThumbId(), thumbInfo ) )
                {
                    if( showUserPopup )
                    {
                        QString msgText = QObject::tr( "Could not get thumbnail file info" );
                        QMessageBox::information( this, QObject::tr( "Error occured creating thumbnail asset " ) + thumbFileName, msgText );
                    }

                    assetInfo->getFileInfo().getThumbId().clear();
                    newAsset.getThumbId().clear();
                }
            }
        }

        AssetBaseInfo* createdAsset{ nullptr };
        bool result = assetMgr.addAsset( newAsset, createdAsset );
        if( result && createdAsset )
        {
            FileInfo fileInfoInLibrary = createdAsset->getFileInfo();
            addFile( fileInfoInLibrary );
        }
        else
        {
            if( showUserPopup )
            {
                QMessageBox::information( this, QObject::tr( "File Error" ), QObject::tr( "Could not add file to library " ), QMessageBox::Ok );
            }
        }
    }
}

//============================================================================
void AppletLibrary::insertItemInFileNameOrder( FileShareItemWidget* item, std::string fileName )
{
    if( 0 == ui.m_FileItemList->count() )
    {
        ui.m_FileItemList->addItem( item );
        ui.m_FileItemList->setItemWidget( item, item );
        return;
    }

    int iIdx{ 0 };
    while( iIdx < ui.m_FileItemList->count() )
    {
        FileShareItemWidget* listWidget = (FileShareItemWidget*)ui.m_FileItemList->item( iIdx );
        if( listWidget )
        {
            FileItemInfo* poFileInfo = (FileItemInfo*)listWidget->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
            if( poFileInfo  )
            {
                std::string listFileName = poFileInfo->getFileInfo().getFileName();
                if( listFileName > fileName )
                {
                    ui.m_FileItemList->insertItem( iIdx, item );
                    ui.m_FileItemList->setItemWidget( item, item );
                    return;
                }
            }
        }

        iIdx++;
    }

    ui.m_FileItemList->addItem( item );
    ui.m_FileItemList->setItemWidget( item, item );
}
