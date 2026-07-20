#pragma once
//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletPlayerNlcBase.h"

#include "MenuDefs.h"

#include <CoreLib/VxFileLists.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class AppletPlayerNlcUi;
}
QT_END_NAMESPACE

class PlayControlWidget;
class QDir;
class QFileInfo;
class VxMenuButton;

class AppletPlayerNlc : public AppletPlayerNlcBase
{
	Q_OBJECT
public:
	AppletPlayerNlc( AppCommon& app, QWidget* parent );
    virtual ~AppletPlayerNlc() override;

	RenderGlWidget*				getRenderConsumer( void ) override;
	QSlider*					getPlayPosSlider( void ) override;
	QPushButton*				getReplayButton( void ) override;
	VxPushButton*				getPlayPauseButton( void ) override;
	PlayControlWidget*			getPlayControlWidget( void ) override;

	void						onMediaPlayerNlcReady( bool isReady ) override;

	bool						isMediaPlayerReady( bool notifyIfNotReady = false );

protected slots:
	void                        slotMenuItemSelected( int menuId, EMenuItemType menuItemType );

	void                        slotMediaFileComboBoxSelectionChange( int cbIdx );

	void						slotBrowseButtonClick( void );
	void						slotReplayButtonClick( void );
    void                        slotOpenVideoFileButtonClick( void );
    void                        slotOpenAudioFileButtonClick( void );

	void						slotFileWasSelected( QString fileName );

	void						slotExpandWindowChanged( bool isMessengerFrame, bool isMaxScreenSize );

protected:
	void                        setupBottomMenu( VxMenuButton* menuButton );

	void						browseForMovie( void );
	void						onFileSelected( FileInfo& fileInfo );
	void						playSelectedMedia( std::string movieFile );

	int							addMediaFilesToRecentList( QDir& mediaDir );

	void						refreshRecentFilesComboBox( void );
	void						updateRecentListVisibility( void );

	void						setVisible( bool visible ) override;

	//=== vars ===//
	Ui::AppletPlayerNlcUi&		ui;
	static VxFileInfoList		m_RecentFiles;
	bool						m_MediaPlayerReady{ false };
};


