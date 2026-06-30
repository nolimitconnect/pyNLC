#pragma once
//============================================================================
// Copyright (C) 2010 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "ActivityBase.h"

#include "FileItemInfo.h"
#include "ToGuiFileXferInterface.h"

#include <CoreLib/VxElapseTimer.h>
#include <CoreLib/VxFileTypeMasks.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class BrowseFilesWidget;
}
QT_END_NAMESPACE

class FileShareItemWidget;
class FromGuiInterface;
class FromEngineInterface;
class P2PEngine;
class QTimer;
class QListWidgetItem;

class ActivityBrowseFiles : public ActivityBase, public ToGuiFileXferInterface
{
	Q_OBJECT
public:

	ActivityBrowseFiles( AppCommon& app, EFileFilterType fileFilter, QWidget* parent = nullptr, bool isSelectAFileMode = false );
	virtual ~ActivityBrowseFiles();

    // overrides required for dialogs with there own title bar and bottom bar widgets
	TitleBarWidget*				getTitleBarWidget( void ) override;
	BottomBarWidget*			getBottomBarWidget( void ) override;

public:
	void						setFileFilter( EFileFilterType eFileFilter );

	FileShareItemWidget*		fileToWidget( FileInfo& fileInfo );

	bool						getWasFileSelected( void )						{ return m_FileWasSelected; }
	FileInfo&					getSelectedFileInfo( void )						{ return m_SelectedFileInfo; }

	void						setCurrentBrowseDir( QString browseDir );

protected slots:
	void						slotUpDirectoryClicked( void );
	void						slotBrowseButtonClicked( void );

	void						slotRequestFileList( void );
	void						slotApplyFileFilter( EFileFilterType fileFilter );

	void						slotListItemClicked( QListWidgetItem* item );
	void						slotListItemDoubleClicked( QListWidgetItem* item );
	void						slotListFileIconClicked( QListWidgetItem* item );
	void						slotListShareFileIconClicked( QListWidgetItem* item );
	void						slotListLibraryIconClicked( QListWidgetItem* item );
	void						slotListPlayIconClicked( QListWidgetItem* item );
	void						slotListPlayExternIconClicked( QListWidgetItem* item );
	void						slotListShredIconClicked( QListWidgetItem* item );
	void						slotAddAllButtonClicked( void );
	
protected:
    void						showEvent( QShowEvent* ev ) override;
    void						hideEvent( QHideEvent* ev ) override;

	void						callbackToGuiFileList( VxGUID& appInstId, FileInfo& fileInfo ) override;
	void						callbackToGuiFileListCompleted( VxGUID& appInstId ) override;
	void						toGuiFileDeleted( QString& fileName ) override;

	void						fromListWidgetRequestFileList( void );
	
	void						setActionEnable( bool enable );
	void						addFile( FileInfo& fileInfo );

	void						clearFileList( void );

	void						showAddAllToLibrary( bool visible );

	void                        updateStorageSpace( std::string fileName );

	bool						fileExistsInList( QString fileName );

	void						wantFileXferCallbacks( bool enable );

	Ui::BrowseFilesWidget&		ui;

	std::string					m_CurBrowseDirectory;
	std::string					m_LastBrowseDir;

	QTimer *					m_WidgetClickEventFixTimer;
	VxElapseTimer						m_ClickToFastTimer;

	bool						m_bFetchInProgress{ false };
	bool						m_IsSelectAFileMode{ false };
	bool						m_FileWasSelected{ false };
	FileInfo					m_SelectedFileInfo;

	EFileFilterType				m_eFileFilterType{ eFileFilterAll };
	bool                        m_FileXferCallbacksRequested{ false };
};


