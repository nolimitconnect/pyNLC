//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletPlayerNlc.h"

#include "AppletBrowseFiles.h"
#include "AppletMgr.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "BottomBarWidget.h"
#include "GuiHelpers.h"
#include "GuiPlayerMgr.h"
#include "MyIcons.h"
#include "GuiAudioMgr.h"
#include "VxMenuButton.h"

#include <P2PEngine/P2PEngine.h>
#include "MediaPlayerNlc.h"

#include "PlayControlWidget.h"

#include <QDir>

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileIsTypeFunctions.h>
#include <CoreLib/VxGlobals.h>

#include "ui_AppletPlayerNlc.h"

VxFileInfoList AppletPlayerNlc::m_RecentFiles;

RenderGlWidget*		AppletPlayerNlc::getRenderConsumer( void )		{ return ui.m_RenderWidget; }
QSlider*			AppletPlayerNlc::getPlayPosSlider( void )		{ return ui.m_PlayControlWidget->getPlayPosSlider(); }
QPushButton*		AppletPlayerNlc::getReplayButton( void )		{ return ui.m_ReplayButton; }
VxPushButton*		AppletPlayerNlc::getPlayPauseButton( void )		{ return ui.m_PlayControlWidget->getPlayPauseButton(); }
PlayControlWidget*	AppletPlayerNlc::getPlayControlWidget( void )	{ return ui.m_PlayControlWidget; }

//============================================================================
AppletPlayerNlc::AppletPlayerNlc( AppCommon& app, QWidget* parent )
: AppletPlayerNlcBase( OBJNAME_APPLET_PLAYER_NLC, app, parent )
, ui(*(new Ui::AppletPlayerNlcUi()))
{
	setAppletType( eAppletPlayerNlc );
	setTitleBarText( DescribeApplet( m_EAppletType ) );

	ui.setupUi( getContentItemsFrame() );
	ui.m_PlayControlWidget->setVisible( false );

    setMenuBottomVisibility( true );
	ui.m_BrowseButton->setIcon( eMyIconFileBrowseNormal );
	ui.m_BrowseButton->setSquareButtonSize( eButtonSizeSmall );
	ui.m_ReplayButton->setIcon( eMyIconPlayNormal );
	ui.m_ReplayButton->setSquareButtonSize( eButtonSizeSmall );
    ui.m_OpenVideoFileButton->setIcon( eMyIconVideo );
    ui.m_OpenVideoFileButton->setSquareButtonSize( eButtonSizeSmall );
    ui.m_OpenAudioFileButton->setIcon( eMyIconMusic );
    ui.m_OpenAudioFileButton->setSquareButtonSize( eButtonSizeSmall );

    BottomBarWidget * bottomBar = getBottomBarWidget();
    if( bottomBar )
    {
        setupBottomMenu( bottomBar->getMenuButton() );
    }

	ui.m_FilesComboBox->setVisible( false ); // do not show until media player is ready
	ui.m_BrowseButton->setVisible( false );
    ui.m_ReplayButton->setVisible( false );
    ui.m_OpenVideoFileButton->setVisible( false );
    ui.m_OpenAudioFileButton->setVisible( false );

    GuiHelpers::requestFilePermission( eMediaFileAny );

	std::string lastPlayedMovie;
	m_MyApp.getAppSettings().getLastPlayedMovie( lastPlayedMovie );
	if( !lastPlayedMovie.empty() )
	{
		ui.m_LastPlayedFileText->setText( lastPlayedMovie.c_str() );
	}

	ui.m_FilesComboBox->setVisible( false );

	connect( ui.m_BrowseButton, SIGNAL(clicked()), this, SLOT(slotBrowseButtonClick()) );
	connect( ui.m_ReplayButton, SIGNAL(clicked()), this, SLOT(slotReplayButtonClick()) );
    connect( ui.m_OpenVideoFileButton, SIGNAL(clicked()), this, SLOT(slotOpenVideoFileButtonClick()) );
    connect( ui.m_OpenAudioFileButton, SIGNAL(clicked()), this, SLOT(slotOpenAudioFileButtonClick()) );

	connect( ui.m_PlayControlWidget, SIGNAL(signalRestart()), this, SLOT(slotReplayButtonClick()) );
	connect( ui.m_PlayControlWidget, SIGNAL(signalPlayPauseButtonClicked()), this, SLOT(slotPlayPauseButtonClick()) );
	connect( ui.m_PlayControlWidget, SIGNAL(signalStopButtonClicked()), this, SLOT(slotStopButtonClick()) );
	connect( ui.m_PlayControlWidget, SIGNAL(signalSliderChanged(int)), this, SLOT(slotSliderChanged(int)) );
	connect( ui.m_PlayControlWidget, SIGNAL(signalUpdateControlsTimeout()), this, SLOT(slotUpdateControlsTimeout()) );

	connect( ui.m_RenderWidget, SIGNAL(signalLeftMouseButtonClick()), this, SLOT(slotLeftMouseButtonClick()));

	connect( &m_MyApp, SIGNAL(signalExpandWindowChanged(bool,bool)), this, SLOT(slotExpandWindowChanged(bool,bool)));

	onAppletInitialized();
}

//============================================================================
AppletPlayerNlc::~AppletPlayerNlc()
{

}

//============================================================================
void AppletPlayerNlc::setupBottomMenu( VxMenuButton* menuButton )
{
	if( menuButton )
	{
		menuButton->setMenuId( 1 );
		menuButton->addMenuItem( eMenuItemShowRecent );
		menuButton->addMenuItem( eMenuItemShowWatched );
		menuButton->addMenuItem( eMenuItemShowLibrary );
		menuButton->addMenuItem( eMenuItemBrowse );
	}

	connect( menuButton, SIGNAL(signalMenuItemSelected(int,EMenuItemType)),
		this, SLOT(slotMenuItemSelected(int,EMenuItemType)) );
}

//============================================================================
void AppletPlayerNlc::slotMenuItemSelected( int menuId, EMenuItemType menuItemType )
{
	switch( menuItemType )
	{
	case eMenuItemBrowse:
		browseForMovie();
		break;

	default:
		break;
	}
}

//============================================================================
void AppletPlayerNlc::slotMediaFileComboBoxSelectionChange( int cbIdx )
{
	if( cbIdx < m_RecentFiles.size() )
	{
        std::string fullFileName = m_RecentFiles.getFileNameAndPathAtIndex( cbIdx );
		if( !fullFileName.empty() )
		{
            playSelectedMedia( fullFileName );
		}
	}
	else
	{
		LogMsg( LOG_ERROR, "%s invalid combo box index", __func__ );
	}
}

//============================================================================
void AppletPlayerNlc::slotBrowseButtonClick( void )
{
	browseForMovie();
}

//============================================================================
void AppletPlayerNlc::browseForMovie( void )
{
	stopMediaIfPlaying();

	QString launchParam( "Single File" );
	ActivityBase* actBase = m_MyApp.getAppletMgr().launchApplet( eAppletBrowseFiles, getParentPageFrame(), launchParam);
    AppletBrowseFiles* fileBrowser = dynamic_cast<AppletBrowseFiles*>(actBase);
    if( fileBrowser )
    {
		std::string fullFileName = ui.m_LastPlayedFileText->text().toUtf8().constData();
		if( !fullFileName.empty() )
		{
			std::string filePath;
			std::string	fileName;
                                                        
			VxFileUtil::seperatePathAndFile( fullFileName.c_str(), filePath, fileName );
			fileBrowser->setCurrentDirectory( filePath.c_str() );
		}
		else
		{
			fileBrowser->setCurrentDirectory( VxGetAppDirectory( eAppDirRootDataStorage ).c_str() );
		}

		fileBrowser->setFileFilter( eFileFilterVideo );
		connect( fileBrowser, SIGNAL(signalFileWasSelected(QString)), this, SLOT(slotFileWasSelected(QString)) );
    }
}

//============================================================================
void AppletPlayerNlc::onFileSelected( FileInfo& fileInfo )
{
	if( VXFILE_TYPE_AUDIO == fileInfo.getFileType() || VXFILE_TYPE_VIDEO == fileInfo.getFileType() )
	{
        std::string fullFileName = fileInfo.getFileNameAndPath();

        playSelectedMedia( fullFileName );
	}
	else
	{
		QMessageBox::information( this, QObject::tr("Unknown Media File Type"), fileInfo.getFileName().c_str(), QMessageBox::Ok );
	}
}

//============================================================================
void AppletPlayerNlc::onMediaPlayerNlcReady( bool isReady )
{
	m_MediaPlayerReady = isReady;
    m_MyApp.getAudioMgr().setPlayerNlcActive(m_MediaPlayerReady);
	updateRecentListVisibility();
	if( m_MediaPlayerReady )
	{       
		getRenderConsumer()->showAppIcon();
	}

	AppletPlayerNlcBase::onMediaPlayerNlcReady( isReady );
}

//============================================================================
void AppletPlayerNlc::slotReplayButtonClick( void )
{
	if( m_AssetInfo.isStream() )
	{
		AppletPlayerNlcBase::slotReplayButtonClick();
		return;
	}

	std::string movieFile = ui.m_LastPlayedFileText->text().toUtf8().constData();
	if( VxFileUtil::fileExists( movieFile.c_str() ) )
	{
        playSelectedMedia( movieFile );
	}
}

//============================================================================
void AppletPlayerNlc::playSelectedMedia( std::string fileNameAndPath )
{
#if defined(TARGET_OS_WINDOWS)
	try
	{
#endif // defined(TARGET_OS_WINDOWS)
		stopMediaIfPlaying();

		if( VxFileUtil::fileExists( fileNameAndPath.c_str() ) )
		{
			//startBusySpinner( getPlayControlWidget() );

			m_RecentFiles.moveToTopOfList( fileNameAndPath );

			std::vector<VxFileInfoBase>& fileList = m_RecentFiles.getFileInfoList();
			if( !fileList.size() )
			{
				QMessageBox::information( this, QObject::tr( "Media Player empty file list" ), fileNameAndPath.c_str(), QMessageBox::Ok );
				return;
			}

			std::string justFileName = fileList.at(0).getFileName();
			QSize screenSize = getRenderConsumer()->getRenderWindowSize();

			// does not work but do anyway so NLC icon is not showing when playing audio
			if(fileList.at(0).getIsAudioFile())
			{
				// set file icon to screen for audio
				QImage iconImage = m_MyApp.getMyIcons().getIconPixmap( eMyIconMusic, screenSize ).toImage();
				getRenderConsumer()->setLastRenderedImage( iconImage );
			}

			//m_RecentFiles.dumpToLog( LOG_VERBOSE );
			refreshRecentFilesComboBox();

			ui.m_LastPlayedFileText->setText( fileNameAndPath.c_str() );
			m_MyApp.getAppSettings().setLastPlayedMovie( fileNameAndPath );
			if( !m_MyApp.getPlayerMgr().playFile( fileNameAndPath.c_str(), 0, false, false, getParentPageFrame() ) )
			{
				QMessageBox::information( this, QObject::tr( "Media Player could not play file" ), fileNameAndPath.c_str(), QMessageBox::Ok );
			}
		}
		else
		{
			QMessageBox::information( this, QObject::tr( "File does not exist" ), fileNameAndPath.c_str(), QMessageBox::Ok );
		}
#if defined(TARGET_OS_WINDOWS)
	}
	catch( ... )
	{
		LogMsg( LOG_ERROR, "%s Exception playing file %s", __func__, fileNameAndPath.c_str() );
		QMessageBox::information( this, QObject::tr( "Exception playing" ), fileNameAndPath.c_str(), QMessageBox::Ok );
	}
#endif // defined(TARGET_OS_WINDOWS)
}

//============================================================================
int AppletPlayerNlc::addMediaFilesToRecentList( QDir& mediaDir )
{
	int addedFileCnt{ 0 };
	mediaDir.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::Readable );
	mediaDir.setSorting( QDir::Name );

    GuiHelpers::listFilesInFolder( mediaDir, m_RecentFiles.getFileInfoList() );

    return m_RecentFiles.size();
}

//============================================================================
void AppletPlayerNlc::refreshRecentFilesComboBox( void )
{
	ui.m_FilesComboBox->blockSignals(true);

	ui.m_FilesComboBox->clear();

    for( auto fileInfo : m_RecentFiles.getFileInfoList() )
	{
        ui.m_FilesComboBox->addItem( fileInfo.getFileName().c_str() );
	}

	ui.m_FilesComboBox->setCurrentIndex( 0 );
	updateRecentListVisibility();

	ui.m_FilesComboBox->blockSignals(false);
}

//============================================================================
bool AppletPlayerNlc::isMediaPlayerReady( bool notifyIfNotReady ) 
{ 
	if( !m_MediaPlayerReady && notifyIfNotReady )
	{
		QMessageBox::information( this, QObject::tr( "Media Player not read" ), QObject::tr( "Try again when Media Player is ready" ), QMessageBox::Ok );
	}

	return m_MediaPlayerReady;  
}

//============================================================================
void AppletPlayerNlc::updateRecentListVisibility( void )
{
	ui.m_BrowseButton->setVisible( m_MediaPlayerReady );
	ui.m_ReplayButton->setVisible( m_MediaPlayerReady );
    ui.m_OpenVideoFileButton->setVisible( m_MediaPlayerReady );
    ui.m_OpenAudioFileButton->setVisible( m_MediaPlayerReady );
}

//============================================================================
void AppletPlayerNlc::slotFileWasSelected( QString fileName )
{
    playSelectedMedia(fileName.toUtf8().constData());
}

//============================================================================
void AppletPlayerNlc::slotOpenVideoFileButtonClick( void )
{
    std::string lastVideoFileDir;
    m_MyApp.getAppSettings().getLastVideoFileDir( lastVideoFileDir );
    QString startDir;
    if( !lastVideoFileDir.empty() )
    {
        startDir = lastVideoFileDir.c_str();
    }
    
    std::string fileNameAndPath;
    if( GuiHelpers::browseForFile( this, eMediaFileVideo, fileNameAndPath, startDir ) )
    {
        FileInfo fileInfo;
        if( VxFileUtil::getFileInfo( fileNameAndPath.c_str(), fileInfo ) )
        {
            m_MyApp.getEngine().fromGuiSetFileIsInLibrary( fileInfo, true );

			std::string filePath = fileInfo.getFilePath();
            if( !filePath.empty() )
            {
                m_MyApp.getAppSettings().setLastVideoFileDir( filePath );
            }
            
            m_RecentFiles.moveToTopOfList( fileInfo );
            refreshRecentFilesComboBox();

            playSelectedMedia( fileNameAndPath );
        }
        else
        {
            QMessageBox::information( this, QObject::tr( "Could not read file" ), QObject::tr( "Failed to get file info" ), QMessageBox::Ok );
        }
    }
}

//============================================================================
void AppletPlayerNlc::slotOpenAudioFileButtonClick( void )
{
    std::string lastAudioFileDir;
    m_MyApp.getAppSettings().getLastVideoFileDir( lastAudioFileDir );
    QString startDir;
    if( !lastAudioFileDir.empty() )
    {
        startDir = lastAudioFileDir.c_str();
    }
    
    std::string fileNameAndPath;
    if( GuiHelpers::browseForFile( this, eMediaFileAudio, fileNameAndPath, startDir ) )
    {
        FileInfo fileInfo;
        if( VxFileUtil::getFileInfo( fileNameAndPath.c_str(), fileInfo ) )
        {
            m_MyApp.getEngine().fromGuiSetFileIsInLibrary( fileInfo, true );

            std::string filePath = fileInfo.getFilePath();
            if( !filePath.empty() )
            {
                m_MyApp.getAppSettings().setLastAudioFileDir( filePath );
            }
            
            m_RecentFiles.moveToTopOfList( fileInfo );
            refreshRecentFilesComboBox();

            playSelectedMedia( fileNameAndPath );
        }
        else
        {
            QMessageBox::information( this, QObject::tr( "Could not read file" ), QObject::tr( "Failed to get file info" ), QMessageBox::Ok );
        }
    }
}

//============================================================================
void AppletPlayerNlc::slotExpandWindowChanged( bool isMessengerFrame, bool isMaxScreenSize )
{
	LogModule( eLogPlayerNlc, LOG_VERBOSE, "%s messenger frame ? %d is max screen size ? %d", __func__, isMessengerFrame, isMaxScreenSize );
	if( isMessengerFrame && isMaxScreenSize )
	{
		ActivityBase::hide();
	}
	else
	{
		ActivityBase::show();
	}
}

//============================================================================
void AppletPlayerNlc::setVisible( bool visible )
{
	AppletPlayerNlcBase::setVisible( visible );
}
