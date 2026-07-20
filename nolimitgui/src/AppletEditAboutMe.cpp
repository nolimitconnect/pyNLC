//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "AppletEditAboutMe.h"

#include "AppletAboutMeClient.h"
#include "AppletMgr.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "MyIcons.h"
#include "AppletSnapshot.h"
#include "AppGlobals.h"
#include "AppCommon.h"
#include "AccountMgr.h"
#include "GuiHelpers.h"
#include "GuiParams.h"

#include <QMessageBox>
#include <QUuid>
#include <QFileDialog>

#include <P2PEngine/EngineSettings.h>
#include <P2PEngine/P2PEngine.h>

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>

#include <GuiInterface/ICamCapture.h>

#include "ui_AppletEditAboutMe.h"

//============================================================================
AppletEditAboutMe::AppletEditAboutMe( AppCommon& app, QWidget* parent )
: AppletBase( OBJNAME_APPLET_EDIT_ABOUT_ME, app, parent )
, ui(*(new Ui::AppletEditAboutMeUi))
{
    setAppletType( eAppletEditAboutMe );
    ui.setupUi( getContentItemsFrame() );
	setTitleBarText( DescribeApplet( m_EAppletType ) );
    ui.m_ViewAboutMePageButton->setFixedSize( eButtonSizeMedium );
    ui.m_ViewAboutMePageButton->setIcon( getMyIcons().getPluginIcon( ePluginTypeAboutMePageServer ) );
    ui.m_PermissionWidget->setPluginType( ePluginTypeAboutMePageServer );
    ui.m_PermissionWidget->getPluginSettingsButton()->setVisible( false );
 
    m_MyIdent = m_MyApp.getAppGlobals().getMyNetIdent();
    m_strOrigOnlineName = m_MyIdent->getOnlineName();
    m_strOrigMoodMessage = m_MyIdent->getOnlineDescription();

    m_strUserSepecificDataDir = VxGetUserSpecificDataDirectory();
    m_strDefaultPicPath = VxGetAppDirectory( eAppDirAboutMePageServer );
    VxFileUtil::makeDirectory( m_strDefaultPicPath.c_str() );
    m_strDefaultPicPath += "me.png";
    uint32_t u32FileLen = VxFileUtil::getFileLen( m_strDefaultPicPath.c_str() );
    if( u32FileLen > 0 )
    {
        QString filename = m_strDefaultPicPath.c_str();
        QPixmap oBitmap;
        if( false == oBitmap.load( filename ) )
        {
            QString msgText = QObject::tr( "Failed To Read Image File " ) + filename;
            QMessageBox::critical( this, QObject::tr( "Error Reading Image" ), msgText );
        }
        else
        {
            ui.m_PictureOfMeFrame->setPixmap( oBitmap );
        }
    }
    else
    {
        QPixmap oBitmap;
        if( false == oBitmap.load( ":/AppRes/Resources/me.png" ) )
        {

            QString msgText = QObject::tr( "Failed To Read Image File " );
            QMessageBox::critical( this, QObject::tr( "Error Reading Image" ), msgText );
        }
        else
        {
            ui.m_PictureOfMeFrame->setPixmap( oBitmap );
        }

        m_bUsingDefaultImage = true;
    }

    connect( ui.m_BrowsePictureButton, SIGNAL(clicked()), this, SLOT(onBrowseButClick()) );
    connect( ui.m_TakeSnapshotButton, SIGNAL(clicked()), this, SLOT(onSnapshotButClick()) );
    connect( ui.m_ApplyAboutMeButton, SIGNAL(clicked()), this, SLOT(onApplyAboutMeButClick()) );
    connect( ui.m_ViewAboutMePageButton, SIGNAL(clicked()), this, SLOT(slotViewAboutMeButClick()) );

    m_CameraSourceAvail = ICamCapture::getICamCapture().isCamCaptureAvailable();

	m_MyApp.activityStateChange( this, true );
}

//============================================================================
//! browse for picture of me
void AppletEditAboutMe::onBrowseButClick( void )
{
    QString startPath = QDir::current().path();
    std::string lastGalleryPath;
    m_MyApp.getAppSettings().getLastGalleryDir( lastGalleryPath );
    if( ( 0 != lastGalleryPath.length() )
        && ( VxFileUtil::directoryExists( lastGalleryPath.c_str() ) ) )
    {
        startPath = lastGalleryPath.c_str();
    }

    // Get a filename from the file dialog.
    QString filename = QFileDialog::getOpenFileName( this,
                                                     QObject::tr( "Open Image" ),
                                                     startPath,
                                                     SUPPORTED_IMAGE_FILES );
    if( filename.length() > 0 )
    {
        QPixmap oBitmap;
        if( false == oBitmap.load( filename ) )
        {
            QString msgText = QObject::tr( "Failed To Read Image File " ) + filename;
            QMessageBox::critical( this, QObject::tr( "Error Reading Image" ), msgText );
        }
        else
        {
            std::string justFileName;
            VxFileUtil::seperatePathAndFile( filename.toUtf8().constData(), lastGalleryPath, justFileName );
            if( ( 0 != lastGalleryPath.length() )
                && ( VxFileUtil::directoryExists( lastGalleryPath.c_str() ) ) )
            {
                m_MyApp.getAppSettings().setLastGalleryDir( lastGalleryPath );
            }

            updateSnapShot( oBitmap );
        }
    }
}

//============================================================================
//! Implement the OnClickListener callback    
void AppletEditAboutMe::onSnapshotButClick( void )
{
    if( m_CameraSourceAvail )
    {
        AppletSnapshot* appletSnapshot = dynamic_cast< AppletSnapshot* >( m_MyApp.getAppletMgr().launchApplet( eAppletSnapshot, this ) );
        if( appletSnapshot )
        {
            //connect( appletSnapshot, SIGNAL(signalJpgSnapshot( uint8_t*, uint32_t, int, int ) ), this, SLOT(slotJpgSnapshot( uint8_t*, uint32_t, int, int ) ) );
            connect( appletSnapshot, SIGNAL(signalSnapshotImage( QImage ) ), this, SLOT(slotImageSnapshot( QImage ) ) );
        }
    }
    else
    {
        QMessageBox::warning( this, QObject::tr( "Camera Capture" ), QObject::tr( "No Camera Source Available." ) );
    }
}

//============================================================================
void AppletEditAboutMe::slotImageSnapshot( QImage snapshotImage )
{
    if( !snapshotImage.isNull() )
    {
        QPixmap pixmap = QPixmap::fromImage( snapshotImage );
        LogMsg( LOG_VERBOSE, "AppletEditAboutMe::slotImageSnapshot w %d h %d", pixmap.width(), pixmap.height() );
        if( !pixmap.isNull() )
        {
            updateSnapShot( pixmap );
        }
        else
        {
            QString msgText = QObject::tr( "Failed to read snapshot " );
            QMessageBox::critical( this, QObject::tr( "Error Reading snapshot" ), msgText );
        }
    }
}

//============================================================================
//! Implement the OnClickListener callback    
void AppletEditAboutMe::onApplyAboutMeButClick( void )
{
    bool saveError{ false };
    std::string strUserProfileDir = VxGetAppDirectory( eAppDirAboutMePageServer );

    saveContentToDb();

    if( m_bUserPickedImage || m_bUsingDefaultImage )
    {
        // save image to web page
        QPixmap bitmap = ui.m_PictureOfMeFrame->pixmap();
        if( !bitmap.isNull())
        {
            QString picPath = strUserProfileDir.c_str();
            picPath += "me.png";
            bool isOk = bitmap.save( picPath, "PNG" );

            if( !isOk )
            {
                QString msgText = QObject::tr( "Failed to write into " ) + picPath;
                QMessageBox::critical( this, QObject::tr( "Error Writing" ), msgText );
                saveError = true;
            }
        }
        else
        {
            LogMsg( LOG_ERROR, "Failed to save picture of me" );
            saveError = true;
        }
    }

    m_Engine.fromGuiUpdateWebPageProfile( strUserProfileDir.c_str(),
                                          m_UserProfile.m_strGreeting.toUtf8(),
                                          m_UserProfile.m_strAboutMe.toUtf8(),
                                          m_UserProfile.m_strUrl1.toUtf8(),
                                          m_UserProfile.m_strUrl2.toUtf8(),
                                          m_UserProfile.m_strUrl3.toUtf8(),
                                          m_UserProfile.m_strDonation.toUtf8() );
    if( !saveError )
    {
        // assume user has changed something so has about me content
        UpdateHasAboutMeContent( m_MyApp.getEngine(), true );
    }

    QString titleText = QObject::tr( "About Me Change" );
    QString msgText = QObject::tr( "Applied About Me Changes " );
    msgText += saveError ? QObject::tr( "Failed" ) : QObject::tr( "Success" );
    QMessageBox::information( this, titleText, msgText);
}

//============================================================================
void AppletEditAboutMe::updateSnapShot( QPixmap& pixmap )
{
    QPixmap scaledPixmap = pixmap.scaled( GuiParams::getSnapshotDesiredSize() );
    ui.m_PictureOfMeFrame->setPixmap( scaledPixmap );
    m_bUserPickedImage = true;
}

//============================================================================
//! validate user input
QString AppletEditAboutMe::validateString( QString charSeq )
{
    //return charSeq.toString();    	
    return charSeq;
}

//============================================================================
//! load user profile data from database
void AppletEditAboutMe::loadContentFromDb( void )
{
    m_MyApp.getAccountMgr().getUserProfile( *m_MyApp.getAppGlobals().getMyNetIdent(), m_UserProfile );
    ui.m_AboutMeEdit->setPlainText( m_UserProfile.m_strAboutMe );
    ui.m_GreetingEdit->setText( m_UserProfile.m_strGreeting );
    ui.m_FavoriteWebsite1Edit->setText( m_UserProfile.m_strUrl1 );
    ui.m_FavoriteWebsite2Edit->setText( m_UserProfile.m_strUrl2 );
    ui.m_FavoriteWebsite3Edit->setText( m_UserProfile.m_strUrl3 );
    ui.m_DonationEdit->setPlainText( m_UserProfile.m_strDonation );
}

//============================================================================
//! save user profile data to database
void AppletEditAboutMe::saveContentToDb( void )
{
    m_UserProfile.m_strAboutMe = ui.m_AboutMeEdit->toPlainText();
    m_UserProfile.m_strGreeting = ui.m_GreetingEdit->text();
    m_UserProfile.m_strUrl1 = ui.m_FavoriteWebsite1Edit->text();
    m_UserProfile.m_strUrl2 = ui.m_FavoriteWebsite2Edit->text();
    m_UserProfile.m_strUrl3 = ui.m_FavoriteWebsite3Edit->text();
    m_UserProfile.m_strDonation = ui.m_DonationEdit->toPlainText();
    m_MyApp.getAccountMgr().updateUserProfile( *m_MyApp.getAppGlobals().getMyNetIdent(), m_UserProfile );
}

//============================================================================
void AppletEditAboutMe::slotViewAboutMeButClick( void )
{
    AppletAboutMeClient* applet = dynamic_cast<AppletAboutMeClient*>(m_MyApp.launchApplet( eAppletAboutMeClient, getParentPageFrame() ));
    if( applet )
    {
        applet->setIdentity( m_MyApp.getUserMgr().getMyIdent() );
    }
}
