//============================================================================
// Copyright (C) 2015 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "FileActionMenu.h"

#include "AppGlobals.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "ActivityMsgBoxYesNo.h"
#include "ActivityMessageBox.h"

#include "GuiHelpers.h"
#include "GuiParams.h"
#include "GuiPlayerMgr.h"
#include "MyIcons.h"

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/ObjectCommonDefs.h>
#include <PktLib/VxCommon.h>
#include <CoreLib/VxDebug.h>

#include "ui_FileActionMenu.h"

//============================================================================
FileActionMenu::FileActionMenu( AppCommon&		app, 
								QWidget*		parent, 
								FileInfo&		fileInfo, 
								bool			isShared, 
								bool			isInLibrary,
								GuiUser*		selectedFriend )
: ActivityBase( OBJNAME_FILE_ACTION_MENU, app, parent, eAppletMessengerFrame, true )
, ui(*(new Ui::FileActionMenuClass))
, m_FileInfo( fileInfo )
, m_IsShared( isShared )
, m_IsInLibrary( isInLibrary )
, m_SelectedFriend( selectedFriend )
, m_iMenuItemHeight( 34 )
{
	ui.setupUi(this);
	slotRepositionToParent();

    connect(ui.m_MenuItemList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(itemClicked(QListWidgetItem*)));

	setupFileInfo();
	setupMenuItems();
}

//============================================================================
FileActionMenu::~FileActionMenu()
{
}

//============================================================================
void FileActionMenu::setTitle( QString strTitle )
{
	ui.m_TitleLabel->setText(strTitle);
}

//============================================================================
void FileActionMenu::setupFileInfo( void )
{
	ui.m_FileIconButton->setIcon( getMyIcons().getFileIcon( m_FileInfo.getFileType() ) );

	QString fileName = m_FileInfo.getFileNameAndPath().c_str();
	QString path = m_FileInfo.getFilePath().c_str();

	ui.m_FileNameLabel->setText( fileName );
	ui.m_FileSizeLabel->setText( GuiParams::describeFileLength( m_FileInfo.getFileLength() ) );
	ui.m_FilePathLabel->setTextBreakAnywhere( path, 3 );
}

//============================================================================
void FileActionMenu::setupMenuItems( void )
{
	int menuIndex = 0;
	addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconPlayNormal ), QObject::tr("Open File") );
	m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionOpen ) );
	menuIndex++;
	if( m_SelectedFriend )
	{
		if( m_SelectedFriend->isMyAccessAllowedFromHim( ePluginTypePersonFileXfer ) )
		{
			QString menuText = "Send File To ";
			menuText += m_SelectedFriend->getOnlineName().c_str();
			addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconSendFileNormal ), menuText );
			m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionSendToFriend ) );
		}
		else
		{
			addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconSendFileDisabled ), QObject::tr("Insufficient Permission To Send File") );
			m_MenuList.push_back( FileMenuItemAction( menuIndex, eFileMenuActionNone ) );
		}

		menuIndex++;
	}

	if( m_MyApp.getEngine().getMyPktAnnounce().getPluginPermission( ePluginTypeFileShareServer ) )
	{
		addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconShareFilesCancel ), QObject::tr("File Sharing Is Disabled") );
		m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionNone ) );
	}
	else if( m_IsShared )
	{
		addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconShareFilesNormal ), QObject::tr("Stop Sharing This File")  );
		m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionRemoveShare ) );
	}
	else
	{
		addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconShareFilesCancel), QObject::tr("Start Sharing This File")  );
		m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionAddShare ) );
	}

	menuIndex++;
	if( m_IsInLibrary )
	{
		addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconLibraryNormal ), QObject::tr("Remove From My Library")  );
		m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionRemoveFromLibrary ) );
	}
	else
	{
		addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconLibraryCancel ), QObject::tr("Add To My Library") );
		m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionAddToLibrary ) );
	}

	menuIndex++;
	addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconTrash ), QObject::tr("Delete File") );
	m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionDelete ) );

	menuIndex++;
	addMenuItem( menuIndex, getMyIcons().getIcon( eMyIconShredderNormal ), QObject::tr("Shred File") );
	m_MenuList.push_back( FileMenuItemAction(menuIndex, eFileMenuActionShred ) );
}

//============================================================================
void FileActionMenu::addMenuItem( int iItemId, QIcon& oIcon, QString strMenuItemText )
{
	QListWidgetItem* poMenuItem = new QListWidgetItem( strMenuItemText );
	poMenuItem->setIcon( oIcon );
	poMenuItem->setData( Qt::UserRole, iItemId );
	ui.m_MenuItemList->addItem( poMenuItem );
}

//============================================================================
void FileActionMenu::itemClicked(QListWidgetItem*item)
{
	// resignal with id set by user
	int iItemId = item->data(Qt::UserRole).toInt();
	std::vector<FileMenuItemAction>::iterator iter;
	for( iter = m_MenuList.begin(); iter != m_MenuList.end(); ++iter )
	{
		if( iItemId != (*iter).getMenuIndex() )
		{
			continue;
		}

		EFileMenuAction menuAction = (*iter).getMenuAction();
		switch( menuAction )
		{
		case eFileMenuActionOpen:
			m_MyApp.getPlayerMgr().playFile( m_FileInfo.getFileNameAndPath().c_str(), 0, false, false, getParentPageFrame() );
			break;

		case eFileMenuActionSendToFriend:
			if( m_SelectedFriend->isMyAccessAllowedFromHim( ePluginTypePersonFileXfer ) )
			{
				if( !m_MyApp.getOfferMgr().fromGuiMakePluginOffer( this, ePluginTypePersonFileXfer, m_SelectedFriend, m_FileInfo ) )
				{
                    ActivityMessageBox errMsgBox( m_MyApp, this, LOG_INFO, "Could Not Offer file because %s is unavailable", m_SelectedFriend->getOnlineName().c_str() );
					errMsgBox.exec();
				}
			}
			break;

		case eFileMenuActionAddShare:
			m_Engine.fromGuiSetFileIsShared( m_FileInfo, true );
			break;

		case eFileMenuActionRemoveShare:
			m_Engine.fromGuiSetFileIsShared( m_FileInfo, false );
			break;

		case eFileMenuActionAddToLibrary:
			m_Engine.fromGuiSetFileIsInLibrary( m_FileInfo, true );
			break;

		case eFileMenuActionRemoveFromLibrary:
			m_Engine.fromGuiSetFileIsInLibrary( m_FileInfo, false );
			break;

		case eFileMenuActionDelete:
			if( confirmDeleteFile( false ) )
			{
				int result = m_Engine.fromGuiDeleteFile( m_FileInfo.getFileNameAndPath(), false );
				if( result )
				{
					QString title = "Could Not Delete File";
					QString bodyText =  QString("OS returned error %1 accessing file %2 for deletion").arg(result).arg(m_FileInfo.getFileNameAndPath().c_str());
					ActivityMsgBoxYesNo dlg( m_MyApp, &m_MyApp, title, bodyText );
					dlg.hideCancelButton();
					dlg.exec();
				}
			}
			break;

		case eFileMenuActionShred:
			if( confirmDeleteFile( true ) )
			{
				int result = m_Engine.fromGuiDeleteFile( m_FileInfo.getFileNameAndPath(), true );
				if( result )
				{
					QString title = "Could Not Shred File";
					QString bodyText =  QString("OS returned error %1 accessing file %2 for shredding").arg(result).arg(m_FileInfo.getFileNameAndPath().c_str());
					ActivityMsgBoxYesNo dlg( m_MyApp, &m_MyApp, title, bodyText );
					dlg.hideCancelButton();
					dlg.exec();
				}
			}
			break;
		default:
			break;
		}

		break;
	}

	closeApplet();
}

//============================================================================
bool FileActionMenu::confirmDeleteFile( bool shredFile )
{
	return GuiHelpers::confirmDeleteFile( m_MyApp, getContentItemsFrame(), shredFile );
}
