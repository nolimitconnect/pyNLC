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

#include "AppletBase.h"

#include "FileItemInfo.h"
#include "ToGuiFileXferInterface.h"

#include <CoreLib/VxElapseTimer.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class AppletBrowseFilesUi;
}
QT_END_NAMESPACE

class FileShareItemWidget;
class FromGuiInterface;
class FromEngineInterface;
class P2PEngine;
class QListWidgetItem;
class QTimer;

class AppletBrowseFiles : public AppletBase, public ToGuiFileXferInterface
{
	Q_OBJECT
public:

	AppletBrowseFiles( AppCommon& app, QWidget* parent, QString launchParam );
	virtual ~AppletBrowseFiles() override;

public:
	void						setFileFilter( EFileFilterType eFileFilter );
	void						setCurrentDirectory( QString browseDir );

	FileShareItemWidget*		fileToWidget( FileInfo& fileInfo );

	bool						getWasFileSelected( void )						{ return m_FileWasSelected; }
	uint8_t						getSelectedFileType( void )						{ return m_SelectedFileType; }
	QString						getSelectedFileName( void )						{ return m_SelectedFileName; }
	uint64_t					getSelectedFileLen( void )						{ return m_SelectedFileLen; }
	bool						getSelectedFileIsShared( void )					{ return m_SelectedFileIsShared; }
	bool						getSelectedFileIsInLibrary( void )				{ return m_SelectedFileIsInLibrary; }

signals:
	void						signalFileWasSelected( QString fileName );

protected slots:
	void						slotUpDirectoryClicked( void );
	void						slotBrowseButtonClicked( void );
    void                        slotAddAllButtonClicked( void );

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
	
protected:
    virtual void				showEvent( QShowEvent* ev ) override;
    virtual void				hideEvent( QHideEvent* ev ) override;

    virtual void				callbackToGuiFileList( VxGUID& appInstId, FileInfo& fileInfo ) override;
	virtual void				callbackToGuiFileListCompleted( VxGUID& appInstId ) override;

	void						fromListWidgetRequestFileList( void );
	void						setActionEnable( bool enable );
	void						addFile( FileInfo& fileInfo );

	void						clearFileList( void );

	void						setCurrentBrowseDir( QString browseDir );

	void						showAddAllToLibrary( bool visible );

	bool						fileExistsInList( QString fileName );

	Ui::AppletBrowseFilesUi&	ui;

	std::string					m_CurBrowseDirectory;
	bool						m_bFetchInProgress{ false };
	QTimer*						m_WidgetClickEventFixTimer{ nullptr };
	bool						m_IsSelectAFileMode{ false };
	bool						m_FileWasSelected{ false };
	uint8_t						m_SelectedFileType{ 0 };
	QString						m_SelectedFileName;
	uint64_t					m_SelectedFileLen{ 0 };
	bool						m_SelectedFileIsShared{ false };
	bool						m_SelectedFileIsInLibrary{ false };

	EFileFilterType				m_eFileFilterType{ eFileFilterAll };
	VxElapseTimer						m_ClickToFastTimer;
};


