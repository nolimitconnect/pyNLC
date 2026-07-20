//============================================================================
// Copyright (C) 2019 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletFileOfferSelect.h"

#include "AppCommon.h"
#include "AppSettings.h"
#include "MyIcons.h"
#include "AppCommon.h"	
#include "AppSettings.h"

#include "AppletDownloads.h"
#include "ActivityBrowseFiles.h"
#include "ActivityMsgBoxYesNo.h"

#include "FileShareItemWidget.h"
#include "MyIcons.h"
#include "AppGlobals.h"
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

#include "PermissionWidget.h"

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxFileInfo.h>
#include <CoreLib/VxDebug.h>

#include "ui_AppletFileOfferSelect.h"

//============================================================================
AppletFileOfferSelect::AppletFileOfferSelect( AppCommon& app, QWidget* parent, QString launchParam )
    : AppletBase( OBJNAME_APPLET_FILE_OFFER_SELECT, app, parent )
    , ui(*(new Ui::AppletFileOfferSelectUi))
    , m_IsSelectAFileMode( !launchParam.isEmpty() ? true : false )
    , m_FileWasSelected( false )
    , m_eFileFilterType( eFileFilterAll )
{
    setAppletType( eAppletFileOfferSelect );
    ui.setupUi( getContentItemsFrame() );
    setTitleBarText( DescribeApplet( getAppletType() ) ); 
    setPluginType( ePluginTypePersonFileXfer );

    ui.m_OfferSendWidget->setCancelVisible( false );

    m_eFileFilterType = GuiParams::fileFilterToEnum( launchParam );
    setFileFilter( m_eFileFilterType );

    connect( ui.m_BrowseButton, SIGNAL(clicked()), this, SLOT(slotBrowseForFileButClick() ) );
    connect( ui.m_OpenFolderButton, SIGNAL(clicked()), this, SLOT(slotBrowseFolderButClick() ) );

    connect( ui.m_FileFilterSelectWidget, SIGNAL(signalFileFilterChanged(EFileFilterType)), this, SLOT(slotApplyFileFilter(EFileFilterType)) );
    statusMsg( "Requesting File List " );

    slotApplyFileFilter( m_eFileFilterType );
    connectBarWidgets();

    wantFileXferCallbacks( true );
    m_MyApp.activityStateChange( this, true );
}

//============================================================================
AppletFileOfferSelect::~AppletFileOfferSelect()
{
    wantFileXferCallbacks( false );
    m_MyApp.activityStateChange( this, false );
}

//============================================================================
void AppletFileOfferSelect::setPluginType( EPluginType pluginType )
{
    AppletBase::setPluginType( pluginType );
    ui.m_OfferSendWidget->setPluginType( pluginType );
}

//============================================================================
void AppletFileOfferSelect::setUser( GuiUser* guiUser )
{
    AppletBase::setUser( guiUser );
    ui.m_OfferSendWidget->setUser( guiUser );
}

//============================================================================
void AppletFileOfferSelect::statusMsg( QString strMsg )
{
    //LogMsg( LOG_INFO, strMsg.toStdString().c_str() );
    ui.m_StatusMsgLabel->setText( strMsg );
}

//============================================================================
void AppletFileOfferSelect::showEvent( QShowEvent* ev )
{
    AppletBase::showEvent( ev );
    m_MyApp.setIsLibraryActivityActive( true );
    wantFileXferCallbacks( true );
    slotRequestFileList();
}

//============================================================================
void AppletFileOfferSelect::hideEvent( QHideEvent* ev )
{
    wantFileXferCallbacks( false );
    AppletBase::hideEvent( ev );
    m_MyApp.setIsLibraryActivityActive( false );
}

//============================================================================
void AppletFileOfferSelect::callbackToGuiFileList( VxGUID& appInstId, FileInfo& fileInfo )
{
    if( getAppletInstId() == appInstId )
    {
        addFile( fileInfo );
    }
}

//============================================================================
void AppletFileOfferSelect::callbackToGuiFileListCompleted( VxGUID& appInstId )
{
    if( getAppletInstId() == appInstId )
    {
        //setActionEnable( true );
        statusMsg( "List Get Completed" );
    }
}

//============================================================================
void AppletFileOfferSelect::setFileFilter( EFileFilterType fileFilter )
{
    m_eFileFilterType = fileFilter;
    ui.m_FileFilterSelectWidget->setFileFilter( fileFilter );
}

//============================================================================
void AppletFileOfferSelect::slotApplyFileFilter( EFileFilterType fileFilter )
{
    m_eFileFilterType = fileFilter;
    slotRequestFileList();
}

//============================================================================
void AppletFileOfferSelect::slotRequestFileList( void )
{
    clearFileList();
    // will get library and shared files.. even shared files not in library
    m_FromGui.fromGuiGetFileLibraryList( getAppletInstId(), FileFilterToVxFileType( m_eFileFilterType ) );
}

//============================================================================
FileShareItemWidget* AppletFileOfferSelect::fileToWidget( FileInfo& fileInfo )
{
    FileShareItemWidget* item = new FileShareItemWidget( ui.m_FileItemList );
    item->setSizeHint( QSize( (int)(GuiParams::getGuiScale() * 200), GuiParams::getFileListEntryHeight() ) );

    FileItemInfo* poItemInfo = new FileItemInfo( fileInfo );
    item->QListWidgetItem::setData( Qt::UserRole + 1, QVariant( ( quint64 )poItemInfo ) );
    connect( item, SIGNAL(signalFileShareItemClicked(QListWidgetItem*) ), this, SLOT(slotItemClicked(QListWidgetItem*) ) );

    connect( item,
             SIGNAL(signalFileShareItemClicked(QListWidgetItem*)),
             this,
             SLOT(slotListItemClicked(QListWidgetItem*)) );

    connect( item, SIGNAL(signalFileIconClicked(QListWidgetItem*)),
             this, SLOT(slotListFileIconClicked(QListWidgetItem*)) );

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
             SIGNAL(signalShredButtonClicked(QListWidgetItem*)),
             this,
             SLOT(slotListShredIconClicked(QListWidgetItem*)) );

    item->updateWidgetFromInfo();
    return item;
}
//============================================================================
void AppletFileOfferSelect::slotListFileIconClicked( QListWidgetItem* item )
{
    FileItemInfo* fileInfo = ((FileShareItemWidget*)item)->getFileItemInfo();
    if( fileInfo )
    {
        onFileSelected( fileInfo->getFileInfo() );
    }
}

//============================================================================
void AppletFileOfferSelect::slotListShareFileIconClicked( QListWidgetItem* item )
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
            poInfo->toggleIsShared();
            ( ( FileShareItemWidget* )item )->updateWidgetFromInfo();
            m_Engine.fromGuiSetFileIsShared( poInfo->getFileInfo(), poInfo->getIsSharedFile() );
            onFileSelected( poInfo->getFileInfo() );
        }
    }
}

//============================================================================
void AppletFileOfferSelect::slotListLibraryIconClicked( QListWidgetItem* item )
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
            poInfo->toggleIsInLibrary();
            ( ( FileShareItemWidget* )item )->updateWidgetFromInfo();
            m_Engine.fromGuiSetFileIsInLibrary( poInfo->getFileInfo(), poInfo->getIsInLibrary() );
            onFileSelected( poInfo->getFileInfo() );
        }
    }
}

//============================================================================
void AppletFileOfferSelect::slotListPlayIconClicked( QListWidgetItem* item )
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
                onFileSelected( poInfo->getFileInfo() );
            }
            else
            {
                QMessageBox::information( this, QObject::tr( "File Not Found" ), poInfo->getFileNameAndPath().toUtf8().constData(), QMessageBox::Ok );
                ui.m_FileItemList->removeItemWidget( item );
            }
        }
    }
}

//============================================================================
void AppletFileOfferSelect::slotListPlayExternIconClicked( QListWidgetItem* item )
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
                onFileSelected( poInfo->getFileInfo() );
            }
            else
            {
                QMessageBox::information( this, QObject::tr( "File Not Found" ), poInfo->getFileNameAndPath().toUtf8().constData(), QMessageBox::Ok );
                ui.m_FileItemList->removeItemWidget( item );
            }
        }
    }
}

//============================================================================
void AppletFileOfferSelect::slotListShredIconClicked( QListWidgetItem* item )
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
                    onFileRemoved( poInfo->getFileInfo() );
                    ui.m_FileItemList->removeItemWidget( item );
                    m_Engine.fromGuiDeleteFile( assetInfo.getAssetNameAndPath().c_str(), true );
                }
            }
        }
    }
}

//============================================================================
//!	get friend from QListWidgetItem data
FileItemInfo* AppletFileOfferSelect::widgetToFileItemInfo( FileShareItemWidget* item )
{
    return ( FileItemInfo* )item->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
}

//============================================================================
FileShareItemWidget* AppletFileOfferSelect::findListEntryWidget( FileInfo& fileInfo )
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

    return NULL;
}

//============================================================================
void AppletFileOfferSelect::addFile( FileInfo& fileInfo )
{
    FileShareItemWidget* existingItem = findItemByFileName( fileInfo.getFileNameAndPath().c_str() );
    if( existingItem )
    {
        FileItemInfo* poItemInfo = existingItem->getFileItemInfo();
        if( poItemInfo )
        {
            existingItem->update();
        }
    }

    if( !existingItem )
    {
        FileShareItemWidget* item = fileToWidget( fileInfo );
        if( item )
        {
            //LogMsg( LOG_INFO, "AppletFileOfferSelect::addFile: adding widget\n");
            ui.m_FileItemList->addItem( item );
            ui.m_FileItemList->setItemWidget( item, item );
        }
    }
}

//============================================================================
//! user selected menu item
void AppletFileOfferSelect::slotListItemClicked( QListWidgetItem* item )
{
    FileItemInfo* poInfo = ( FileItemInfo* )item->data( Qt::UserRole + 1 ).toLongLong();
    if( poInfo )
    {
        m_FileInfo = poInfo->getFileInfo();
        if( m_IsSelectAFileMode )
        {
            m_FileWasSelected = true;
            accept();
        }
        else
        {
            onFileSelected( m_FileInfo );
        }
    }
}

//============================================================================
//! user double clicked menu item
void AppletFileOfferSelect::slotListItemDoubleClicked( QListWidgetItem* item )
{
    slotListItemClicked( item );
}

//============================================================================
void AppletFileOfferSelect::clearFileList( void )
{
    ui.m_FileItemList->clear();
}

//============================================================================
FileShareItemWidget* AppletFileOfferSelect::findItemByFileName( QString fileName )
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
void AppletFileOfferSelect::slotBrowseForFileButClick( void )
{
    ActivityBrowseFiles dlg( m_MyApp, m_eFileFilterType, this, true );
    dlg.exec();
    if( dlg.getWasFileSelected() )
    {
        onFileSelected( dlg.getSelectedFileInfo() );
    }
}

//============================================================================
void AppletFileOfferSelect::slotBrowseFolderButClick( void )
{
    QString curDir = ui.m_Path->text();
    if( VxFileUtil::directoryExists( curDir.toUtf8().constData() ) )
    {
        ActivityBrowseFiles dlg( m_MyApp, m_eFileFilterType, this, true );
        dlg.setCurrentBrowseDir( curDir );
        dlg.exec();
        if( dlg.getWasFileSelected() )
        {
            onFileSelected( dlg.getSelectedFileInfo() );
        }
    }
}

//============================================================================
bool AppletFileOfferSelect::verifyFile( void )
{
    return true;
}

//============================================================================
void AppletFileOfferSelect::onFileSelected( FileInfo& fileInfo )
{
    if( !fileInfo.m_FileHash.isHashValid() )
    {
        m_MyApp.getFromGuiInterface().fromGuiQueryFileHash( fileInfo );
    }

    OfferBaseInfo offerInfo( fileInfo );
    offerInfo.setPluginType( getPluginType() );
    offerInfo.setOnlineId( m_MyApp.getMyOnlineId() );
    offerInfo.setCreatorId( m_MyApp.getMyOnlineId() );
    offerInfo.setHistoryId( m_MyApp.getMyOnlineId() );
    offerInfo.setDestUserId( m_HisIdent->getMyOnlineId() );
    offerInfo.getOfferId().initializeWithNewVxGUID();
    offerInfo.getAssetUniqueId().assureIsValidGUID();
    offerInfo.setOfferMgr( eOfferMgrHost );

    ui.m_OfferSendWidget->setOfferInfo( offerInfo );

    std::string justFileName = offerInfo.getFileName();
    ui.m_FileNameEdit->setText( justFileName.c_str() );
    statusMsg( justFileName.c_str() );

    std::string fullFileName = offerInfo.getAssetNameAndPath();
    std::string justPath;
    if( 0 == VxFileUtil::seperatePathAndFile( fullFileName, justPath, justFileName ) )
    {
        ui.m_Path->setText( justPath.c_str() );

    }
}

//============================================================================
void AppletFileOfferSelect::onFileRemoved( FileInfo& fileInfo )
{
    if( fileInfo.getFileName().c_str() == ui.m_FileNameEdit->text() )
    {
        ui.m_FileNameEdit->clear();
        ui.m_Path->clear();
        ui.m_OfferSendWidget->clearOffer();
    }
}

//============================================================================
void AppletFileOfferSelect::wantFileXferCallbacks( bool enable )
{
	if( enable != m_FileXferCallbacksRequested )
	{
		m_FileXferCallbacksRequested = enable;
		m_MyApp.getFileXferMgr().wantToGuiFileXferCallbacks( this, enable );
	}
}