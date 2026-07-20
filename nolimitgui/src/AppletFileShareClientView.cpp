//============================================================================
// Copyright (C) 2013 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletFileShareClientView.h"

#include "ActivityMessageBox.h"
#include "ActivityMsgBoxYesNo.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "AppletAboutFile.h"
#include "AppletDownloads.h"
#include "AppletMgr.h"
#include "AppletPopupMenu.h"
#include "AppGlobals.h"

#include "GuiFileXferSession.h"
#include "GuiHelpers.h"
#include "GuiParams.h"
#include "GuiPlayerMgr.h"

#include "FileListReplySession.h"
#include "FileXferWidget.h"
#include "PermissionWidget.h"

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileInfo.h>
#include <CoreLib/VxFileUtil.h>

#include <P2PEngine/P2PEngine.h>
#include <PktLib/VxSearchDefs.h>
#include <NetLib/VxFileXferInfo.h>

#include "ui_AppletFileShareClientView.h"

//============================================================================
AppletFileShareClientView::AppletFileShareClientView( AppCommon& app, QWidget*	parent )
: AppletPeerBase( OBJNAME_ACTIVITY_TO_FRIEND_VIEW_SHARED_FILES, app, parent )
, ui(*(new Ui::AppletFileShareClientViewUi))
{
	m_LclSessionId.initializeWithNewVxGUID();
    setPluginType( ePluginTypeFileShareClient );
    setAppletType( eAppletFileShareClientView );
    ui.setupUi( getContentItemsFrame() );
    setTitleBarText( DescribeApplet( m_EAppletType ) );

    connectBarWidgets();

    connect( ui.FileItemList, SIGNAL(itemClicked(QListWidgetItem*)),						this, SLOT(slotItemClicked(QListWidgetItem*)));
    connect( ui.FileItemList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),					this, SLOT(slotItemClicked(QListWidgetItem*)));
    connect( ui.m_FileFilterSelectWidget, SIGNAL(signalFileFilterChanged(EFileFilterType)), this, SLOT(slotApplyFileFilter(EFileFilterType)) );

	m_MyApp.activityStateChange( this, true );
	wantActivityCallbacks( true );
	wantFileXferCallbacks( true );
	m_MyApp.getThumbMgr().wantGuiThumbCallbacks( this, true );

	checkDiskSpace();
}

//============================================================================
AppletFileShareClientView::~AppletFileShareClientView()
{
	if( m_HisOnlineId.isValid() )
	{
		m_MyApp.getEngine().fromGuiDownloadFileListCancel( getPluginType(), m_HisOnlineId, m_LclSessionId );
	}

	m_MyApp.getThumbMgr().wantGuiThumbCallbacks( this, false );
	wantFileXferCallbacks( false );
	wantActivityCallbacks( false );
	m_MyApp.activityStateChange( this, false );
}

//============================================================================
void AppletFileShareClientView::setIdentity( GuiUser* guiUser )
{
	if( guiUser )
	{
		ui.m_IdentWidget->setupIdentLogic();
		ui.m_IdentWidget->updateIdentity( guiUser );
		m_HisOnlineId = guiUser->getMyOnlineId();
		if( !m_MyApp.getEngine().fromGuiDownloadFileList( getPluginType(), m_HisOnlineId, m_LclSessionId, FileFilterToVxFileType( m_eFileFilterType ) ) )
		{
			GuiHelpers::errorMsgBox( eErrMsgUserUnavailable, this, guiUser );
			close();
		}
	}
}

//============================================================================
void AppletFileShareClientView::toGuiFileListReply( FileListReplySession* replySession )
{
	addFile( replySession->getIdent(), replySession->getPluginType(), replySession->getFileInfo() );
}

//============================================================================
void AppletFileShareClientView::toGuiSearchResultFileSearch( GuiUser* guiUser, EPluginType pluginType, VxGUID& lclSessionId, FileInfo& fileInfo )
{
	addFile( guiUser, pluginType, fileInfo );
}

//============================================================================
void AppletFileShareClientView::toGuiFileXferState( EPluginType pluginType, VxGUID& lclSessionId, EXferDirection xferDir, EXferState xferState, EXferError xferErr, int param1 )
{
    if( eXferDirectionRx == xferDir )
    {
        FileXferWidget* item = findListEntryWidget( lclSessionId );
        if( item )
        {
            if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_DEBUG, "AppletFileShareClientView::%s session %s dir %s state %s error %s param %d",
                      __func__, lclSessionId.toHexString().c_str(), DescribeXferDirection( xferDir ), DescribeXferState( xferState ), DescribeXferError( xferErr ), param1 );
            item->setXferState( xferState, xferErr, param1 );
        }
		else
		{
			LogMsg( LOG_ERROR, "AppletFileShareClientView::%s item for session id %s not found", 
					__func__, lclSessionId.toHexString().c_str() );
		}
    }
}

//============================================================================
void AppletFileShareClientView::toGuiFileDownloadStart( GuiFileXferSession* xferSessionIn )
{
	GuiFileXferSession* xferSession = findSession( xferSessionIn->getLclSessionId() );
	if( xferSession )
	{
		if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_VERBOSE, "AppletFileShareClientView::%s session id %s file %s",
				   __func__, xferSessionIn->getLclSessionId().toHexString().c_str(), xferSessionIn->getFileName().toUtf8().constData() );
        xferSession->setXferState( eXferStateInDownloadXfer, eXferErrorNone, 0 );
		FileXferWidget* item = findListEntryWidget( xferSession->getLclSessionId() );
		if( item )
		{
			item->updateWidgetFromInfo();
		}
		else
		{
			LogMsg( LOG_ERROR, "AppletFileShareClientView::%s item for session id %s not found", 
					__func__, xferSessionIn->getLclSessionId().toHexString().c_str() );
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "AppletFileShareClientView::%s session id %s not found", 
				__func__, xferSessionIn->getLclSessionId().toHexString().c_str() );
	}
}

//============================================================================
GuiFileXferSession* AppletFileShareClientView::findSession( VxGUID lclSessionId )
{
	int iCnt = ui.FileItemList->count();
	for( int iRow = 0; iRow < iCnt; iRow++ )
	{
		QListWidgetItem* item =  ui.FileItemList->item( iRow );
		GuiFileXferSession* poCurInfo = (GuiFileXferSession*)item->data( Qt::UserRole + 1).toULongLong();
		if( poCurInfo->getLclSessionId() == lclSessionId )
		{
			return poCurInfo;
		}
	}

	if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_WARN, "AppletFileShareClientView::%s session id %s not found", __func__, lclSessionId.toHexString().c_str() );
	return NULL;
}

//============================================================================
void AppletFileShareClientView::toGuiFileDownloadComplete( EPluginType pluginType, VxGUID& lclSessionId, QString newFileName, EXferError xferError )
{
	GuiFileXferSession* xferSession = findSession( lclSessionId );
	if( xferSession )
	{
		if( !newFileName.isEmpty() )
		{
			xferSession->setFileNameAndPath( newFileName.toUtf8().constData() );
		}

        xferSession->setXferState( eXferStateCompletedDownload, xferError, 100 );
		FileXferWidget* item = findListEntryWidget( lclSessionId );
		if( item )
		{
			item->updateWidgetFromInfo();
		}
	}
	else
	{
		if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_WARN, "AppletFileShareClientView::%s session id %s not found", 
				__func__, lclSessionId.toHexString().c_str() );
	}
}

//============================================================================
void AppletFileShareClientView::statusMsg( QString strMsg )
{
	ui.m_StatusMsgLabel->setText( strMsg );
}

//============================================================================
void AppletFileShareClientView::slotApplyFileFilter( EFileFilterType fileFilter )
{
	m_eFileFilterType = fileFilter;
	int iIdx = 0;
	FileXferWidget* poWidget;
	while( iIdx < ui.FileItemList->count() )
	{
		poWidget = (FileXferWidget*)ui.FileItemList->item(iIdx);
		if( poWidget )
		{
			GuiFileXferSession* poFileInfo = (GuiFileXferSession*)poWidget->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
			updateListEntryWidget( poWidget, poFileInfo );
		}

		iIdx++;
	}
}

//============================================================================
FileXferWidget* AppletFileShareClientView::fileToWidget( GuiUser* guiUser, EPluginType pluginType, FileInfo& fileInfo )
{
	FileXferWidget* item = new FileXferWidget(ui.FileItemList);
	item->setSizeHint( QSize( (int)(GuiParams::getGuiScale() * 200), GuiParams::getFileListEntryHeight() ) );

    VxGUID lclSessionId;
    lclSessionId.initializeWithNewVxGUID();
    GuiFileXferSession* xferSession = new GuiFileXferSession( eXferDirectionRx, pluginType, guiUser, lclSessionId, fileInfo );
	xferSession->setXferState( eXferStateDownloadNotStarted );
	if( fileInfo.isStremable() )
	{
		xferSession->setStreamingEnable( true );
	}

	xferSession->setWidget( item );
    item->QListWidgetItem::setData( Qt::UserRole + 1, QVariant((quint64)xferSession) );

    connect( item, SIGNAL(signalFileXferItemClicked(QListWidgetItem*)),		this, SLOT(slotItemClicked(QListWidgetItem*)) );
    connect( item, SIGNAL(signalFileIconButtonClicked(QListWidgetItem*)),	this, SLOT(slotItemClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalThumbButtonClicked(QListWidgetItem*)),		this, SLOT(slotItemClicked(QListWidgetItem*)) );

	connect( item, SIGNAL(signalAcceptButtonClicked(QListWidgetItem*)),		this, SLOT(slotAcceptButtonClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalCancelButtonClicked(QListWidgetItem*)),		this, SLOT(slotCancelButtonClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalStreamButtonClicked(QListWidgetItem*)),		this, SLOT(slotStreamButtonClicked(QListWidgetItem*)) );

	connect( item, SIGNAL(signalPlayButtonClicked(QListWidgetItem*)),		this, SLOT(slotPlayButtonClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalPlayExternButtonClicked(QListWidgetItem*)),	this, SLOT(slotPlayExternButtonClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalLibraryButtonClicked(QListWidgetItem*)),	this, SLOT(slotLibraryButtonClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalFileShareButtonClicked(QListWidgetItem*)),	this, SLOT(slotFileShareButtonClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalAboutFileButtonClicked(QListWidgetItem*)),  this, SLOT(slotAboutFileButtonClicked(QListWidgetItem*)) );
	connect( item, SIGNAL(signalShredButtonClicked(QListWidgetItem*)),		this, SLOT(slotShredButtonClicked(QListWidgetItem*)) );

	updateListEntryWidget( item, xferSession );

	return item;
}

//============================================================================
void AppletFileShareClientView::updateListEntryWidget( FileXferWidget* item, GuiFileXferSession* xferSession )
{
	if( !item || !xferSession )
	{
		return;
	}

	item->updateWidgetFromInfo();
	if( 0 == ( xferSession->getFileType() & FileFilterToVxFileType( m_eFileFilterType ) ) )
	{
		item->setVisible( false );
	}
	else
	{
		item->setVisible( true );
	}

	if( xferSession->getXferState() != eXferStateDownloadNotStarted && xferSession->getXferState() != eXferStateUploadNotStarted )
	{
		int percent = m_FromGui.fromGuiGetFileDownloadState( xferSession->getFileHashId().getHashData() );
		if( percent < 0 )
		{
			item->setFileIconButtonEnabled( true );
			item->setFileProgressBarValue( 0 );
		}
		else
		{
			item->setFileIconButtonEnabled( false );
			item->setFileProgressBarValue( percent );
		}
	}
}

//============================================================================
GuiFileXferSession* AppletFileShareClientView::widgetToFileItemInfo( FileXferWidget* item )
{
	return (GuiFileXferSession*)item->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
}

//============================================================================
FileXferWidget* AppletFileShareClientView::findListEntryWidget( VxGUID lclSessionId )
{
	int iIdx = 0;
	FileXferWidget* poWidget;
	while( iIdx < ui.FileItemList->count() )
	{
		poWidget = (FileXferWidget*)ui.FileItemList->item(iIdx);
		if( poWidget )
		{
			GuiFileXferSession* poFileInfo = (GuiFileXferSession*)poWidget->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
			if( poFileInfo && ( poFileInfo->getLclSessionId() == lclSessionId ) )
			{
				return poWidget;
			}
		}

		iIdx++;
	}

	return NULL;
}

//============================================================================
void AppletFileShareClientView::addFile( GuiUser* guiUser, EPluginType pluginType, FileInfo& fileInfo )
{
    if( fileInfo.getFileLength() && !fileInfo.getFileNameAndPath().empty() )
	{
        FileXferWidget* item = fileToWidget( guiUser, pluginType, fileInfo );
		if( item )
		{
			ui.FileItemList->addItem( item );
			ui.FileItemList->setItemWidget( item, item );
			GuiFileXferSession* xferSession = widgetToFileItemInfo( item );
			if( xferSession )
			{
				EXferState xferState = xferSession->getXferState();
				if( eXferStateDownloadNotStarted == xferState && xferSession->isStremable() )
				{
					xferSession->setStreamingEnable( true );
					item->updateWidgetFromInfo();
					if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_VERBOSE, "AppletFileShareClientView::%s %s lcl session id %s streamable", __func__,
							   fileInfo.getFileName().c_str(), xferSession->getLclSessionId().toHexString().c_str() );
				}
				else if( eXferStateUploadNotStarted == xferState || eXferStateDownloadNotStarted == xferState)
				{
					std::string fileName = xferSession->getFileNameAndPath().toUtf8().constData();
                    int64_t fileLen = (int64_t)VxFileUtil::fileExists( fileName.c_str(), false );
					if( fileLen && fileLen == fileInfo.getFileLength() )
					{
						if( eXferStateUploadNotStarted == xferState )
						{
							xferSession->setXferState( eXferStateCompletedUpload, eXferErrorNone, 100 );
							item->updateWidgetFromInfo();
						}
						else
						{
							xferSession->setXferState( eXferStateCompletedDownload, eXferErrorNone, 100 );
							item->updateWidgetFromInfo();
						}
					}
				}	
			}
		}
	}
	else
	{
		statusMsg( "File List Complete" );
	}
}

//============================================================================
void AppletFileShareClientView::slotItemClicked(QListWidgetItem* item)
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data( Qt::UserRole + 1 ).toLongLong();
	if( !xferSession )
	{
		LogMsg( LOG_ERROR, "AppletFileShareClientView::%s null xferSession", __func__ );
		vx_assert( false );
		return;
	}

	FileInfo& fileInfo = xferSession->getFileInfo();
	AppletAboutFile* aboutFile = dynamic_cast<AppletAboutFile*>( m_MyApp.getAppletMgr().launchApplet( eAppletAboutFile, getParentPageFrame() ) );
	if( aboutFile )
	{
		aboutFile->setFileInfo( fileInfo );
	}
}

//============================================================================
void AppletFileShareClientView::slotCancelButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data(Qt::UserRole + 1).toLongLong();
	if( !xferSession )
	{
		LogMsg( LOG_ERROR, "AppletFileShareClientView::%s null xferSession", __func__ );
		vx_assert( false );
		return;
	}

	switch( xferSession->getXferState() )
	{
	case eXferStateDownloadNotStarted:
		beginDownload( xferSession, item );
		break;

	case eXferStateCompletedUpload:
	case eXferStateCompletedDownload:
		removeDownload( xferSession, item );
		break;
	case eXferStateWaitingOfferResponse:
	case eXferStateInUploadQue:
	case eXferStateBeginUpload:
	case eXferStateInUploadXfer:
	case eXferStateUserCanceledUpload:
	case eXferStateUploadOfferRejected:
	case eXferStateUploadError:
		cancelUpload( xferSession, item );
		break;

	case eXferStateInDownloadQue:
	case eXferStateBeginDownload:
	case eXferStateInDownloadXfer:
	case eXferStateUserCanceledDownload:
	case eXferStateDownloadError:
		cancelDownload( xferSession, item );
	default:
		break;
	}
}

//============================================================================
void AppletFileShareClientView::slotStreamButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data(Qt::UserRole + 1).toLongLong();
	if( !xferSession )
	{
		LogMsg( LOG_ERROR, "%s null xferSession", __func__ );
		vx_assert( false );
		return;
	}

	if( !xferSession || !xferSession->getIdent() )
	{
		LogMsg( LOG_ERROR, "%s invalid param", __func__ );
		vx_assert( false );
		return;
	}

	if(	-1 != m_FromGui.fromGuiGetFileDownloadState( xferSession->getFileHashId().getHashData() ) )
	{
		ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, "File cannot be streamed while downloading" );
		errMsgBox.exec();
		return;
	}

	if( eXferStateDownloadNotStarted != xferSession->getXferState() )
	{
		ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, "File cannot be streamed while in state %s", DescribeXferState( xferSession->getXferState() ) );
		errMsgBox.exec();
		return;
	}

	EXferState xferState = xferSession->getXferState();
	AssetBaseInfo assetInfo;

	xferSession->getAssetInfo( assetInfo, true );

	assetInfo.setPluginType( ePluginTypeFileShareClient );
	assetInfo.setDestUserId( xferSession->getIdent()->getMyOnlineId() );

	if(LogEnabled(eLogFileXfer)) LogModule( eLogFileXfer, LOG_VERBOSE, "AppletFileShareClientView::%s session id %s file %s playStream",
				   __func__, xferSession->getLclSessionId().toHexString().c_str(), xferSession->getFileName().toUtf8().constData() );

	// Each explicit play click gets a new stream session id so stream lifecycle is strictly session-based.
	VxGUID streamSessionId;
	streamSessionId.initializeWithNewVxGUID();
	xferSession->setLclSessionId( streamSessionId );

	if( m_MyApp.getPlayerMgr().playStream( assetInfo, streamSessionId, 0, getParentPageFrame() ) )
	{
		((FileXferWidget*)item)->setXferState( eXferStateStreaming, eXferErrorNone, 0 );
	}
	else
	{
		ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, QObject::tr("Failed to play stream ") + assetInfo.getFileName().c_str() );
		errMsgBox.exec();
	}
}

//============================================================================
void AppletFileShareClientView::beginDownload( GuiFileXferSession* xferSession, QListWidgetItem* item )
{
	if( !xferSession || !xferSession->getIdent() )
	{
		LogMsg( LOG_ERROR, "%s invalid param", __func__ );
		vx_assert( false );
		return;
	}

	if(	-1 != m_FromGui.fromGuiGetFileDownloadState( xferSession->getFileHashId().getHashData() ) )
	{
		ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, "File is already downloading" );
		errMsgBox.exec();
	}
	else
	{
		GuiUser* guiUser = xferSession->getIdent();
		if( false == xferSession->getLclSessionId().isValid() )
		{
			xferSession->getLclSessionId().initializeWithNewVxGUID();
		}

		FileInfo& fileInfo = xferSession->getFileInfo();
		// make sure is correct user to transfer from
		fileInfo.setOnlineId( guiUser->getMyOnlineId() );
		fileInfo.setXferSessionId( xferSession->getLclSessionId() );

		EXferError xferError = m_FromGui.fromGuiFileXferControl( xferSession->getPluginType(), eXferActionDownload, fileInfo );
		if( eXferErrorNone != xferError )
		{
			ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, GuiParams::describeEXferError( xferError ) );
			errMsgBox.exec();
		}
		else
		{
			m_MyApp.getFileXferMgr().beginDownload( getAppletType(), xferSession );
		}
	}
}

//============================================================================
void AppletFileShareClientView::removeDownload( GuiFileXferSession* xferSession, QListWidgetItem* item  )
{
	// do not remove because we are viewing shared file
	( (FileXferWidget*)item )->resetXferState();
}

//============================================================================
void AppletFileShareClientView::cancelDownload( GuiFileXferSession* xferSession, QListWidgetItem* item )
{
    ((FileXferWidget*)item)->resetXferState();
	m_MyApp.getFileXferMgr().cancelDownload( getAppletType(), xferSession );
    if( xferSession->isStream() )
	{
		xferSession->setIsStream( false );
	}
	else
	{
		std::string fileName = xferSession->getFileInfo().getFileNameAndPath();
		if( VxFileUtil::fileExists( fileName.c_str() ) )
		{
			m_MyApp.getEngine().fromGuiDeleteFile( fileName, true );
		}
	}

	removeDownload( xferSession, item );
}

//============================================================================
void AppletFileShareClientView::cancelUpload( GuiFileXferSession* xferSession, QListWidgetItem* item )
{
    xferSession->setXferState( eXferStateUserCanceledUpload, eXferErrorNone, 0 );
    ((FileXferWidget*)item)->setXferState( eXferStateUserCanceledUpload, eXferErrorNone, 0 );
	m_MyApp.getFileXferMgr().cancelUpload( getAppletType(), xferSession );

	removeDownload( xferSession, item );
}

//============================================================================
void AppletFileShareClientView::slotPlayButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data(Qt::UserRole + 1).toLongLong();
	if( xferSession )
	{
		this->playFile( xferSession->getFileNameAndPath(), 0, false, false );
	}
}

//============================================================================
void AppletFileShareClientView::slotPlayExternButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data(Qt::UserRole + 1).toLongLong();
	if( xferSession )
	{
		this->playFile( xferSession->getFileNameAndPath(), 0, false, true );
	}
}

//============================================================================
void AppletFileShareClientView::slotLibraryButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data(Qt::UserRole + 1).toLongLong();
	if( xferSession )
	{
		bool inLibary = xferSession->getIsInLibrary();
		inLibary = !inLibary;
		xferSession->setIsInLibrary( inLibary );
		m_Engine.fromGuiSetFileIsInLibrary( xferSession->getFileInfo(), inLibary );
		((FileXferWidget*)item)->updateWidgetFromInfo();
	}
}

//============================================================================
void AppletFileShareClientView::slotFileShareButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data(Qt::UserRole + 1).toLongLong();
	if( xferSession )
	{
		bool isShared = xferSession->getIsSharedFile();
		isShared = !isShared;
		xferSession->setIsSharedFile( isShared );
		m_Engine.fromGuiSetFileIsShared( xferSession->getFileInfo(), isShared );
		((FileXferWidget*)item)->updateWidgetFromInfo();
	}
}


//============================================================================
void AppletFileShareClientView::slotAboutFileButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
	if( xferSession )
	{
		AppletAboutFile* aboutFile = dynamic_cast<AppletAboutFile*>( m_MyApp.getAppletMgr().launchApplet( eAppletAboutFile, getParentPageFrame() ) );
		if( aboutFile )
		{
			aboutFile->setFileInfo( xferSession->getFileInfo() );
		}
	}
}

//============================================================================
void AppletFileShareClientView::slotShredButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->data(Qt::UserRole + 1).toLongLong();
	if( xferSession )
	{
		QString fileName = xferSession->getFileNameAndPath();
		if( confirmDeleteFile( true ) )
		{
			removeDownload( xferSession, item );
			m_Engine.fromGuiDeleteFile( fileName.toUtf8().constData(), true );
		}
	}
}

//============================================================================
bool AppletFileShareClientView::confirmDeleteFile( bool shredFile )
{
	return GuiHelpers::confirmDeleteFile( m_MyApp, getContentItemsFrame(), shredFile );
}

//============================================================================
void AppletFileShareClientView::slotDownloadFileSelected( int iMenuId, QWidget* popupMenu )
{
	if( m_SelectedFileInfo && m_Friend )
	{
		if(	-1 != m_FromGui.fromGuiGetFileDownloadState( m_SelectedFileInfo->getFileHashId().getHashData() ) )
		{
			ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, "File is already downloading" );
			errMsgBox.exec();
		}
		else
		{
			if( false == m_SelectedFileInfo->getLclSessionId().isValid() )
			{
				m_SelectedFileInfo->getLclSessionId().initializeWithNewVxGUID();
			}

			FileInfo& fileInfo = m_SelectedFileInfo->getFileInfo();
			fileInfo.setOnlineId( m_Friend->getMyOnlineId() );

			EXferError xferError = m_FromGui.fromGuiFileXferControl( ePluginTypeFileShareServer, eXferActionDownload, fileInfo );

			if( eXferErrorNone != xferError )
			{
				ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, GuiParams::describeEXferError( xferError ) );
				errMsgBox.exec();
			}
		}
	}
}

//============================================================================
void AppletFileShareClientView::clearFileList( void )
{
	ui.FileItemList->clear();
}

//============================================================================
void AppletFileShareClientView::moveUpOneFolder( void )
{
	if( m_strCurrentDirectory.length() )
	{
		char * pBuf = new char[ m_strCurrentDirectory.length() + 1 ];
		strcpy( pBuf, m_strCurrentDirectory.c_str() );
		char * pTemp = strrchr( pBuf, '/' );
		if( pTemp )
		{
			pTemp[0] = 0;
		}

		m_strCurrentDirectory = pBuf;
        delete[] pBuf;
	}
}

//============================================================================
void AppletFileShareClientView::slotAcceptButtonClicked( QListWidgetItem* item )
{
	GuiFileXferSession* xferSession = (GuiFileXferSession*)item->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
	if( xferSession )
	{
		m_MyApp.getFileXferMgr().acceptDownload( getAppletType(), xferSession );
		ui.FileItemList->removeItemWidget( item );
	}
}

//============================================================================
void AppletFileShareClientView::wantFileXferCallbacks( bool enable )
{
	if( enable != m_FileXferCallbacksRequested )
	{
		m_FileXferCallbacksRequested = enable;
		m_MyApp.getFileXferMgr().wantToGuiFileXferCallbacks( this, enable );
	}
}

//============================================================================
void AppletFileShareClientView::callbackThumbAdded( GuiThumb* guiThumb )
{
	VxGUID thumbId = guiThumb->getThumbId();
	int iIdx = 0;
	FileXferWidget* poWidget;
	while( iIdx < ui.FileItemList->count() )
	{
		poWidget = (FileXferWidget*)ui.FileItemList->item( iIdx );
		if( poWidget && !poWidget->hasThumbImage() )
		{
			GuiFileXferSession* xferSession = (GuiFileXferSession*)poWidget->QListWidgetItem::data( Qt::UserRole + 1 ).toULongLong();
			if( xferSession->getThumbId() == thumbId )
			{
				QImage qImage;
				bool validImage = guiThumb->createImage( qImage );
				if( validImage )
				{
					poWidget->setThumbImage( qImage );
				}
			}
		}

		iIdx++;
	}

}