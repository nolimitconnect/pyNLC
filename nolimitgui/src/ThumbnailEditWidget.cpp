//============================================================================
// Copyright (C) 2019 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "ThumbnailEditWidget.h"
#include "AppletGalleryEmoticon.h"
#include "AppletGalleryThumb.h"
#include "AppletSnapshot.h"
#include "AppletMgr.h"

#include "AppCommon.h"
#include "GuiHelpers.h"
#include "GuiParams.h"

#include <P2PEngine/P2PEngine.h>
#include <ThumbMgr/ThumbInfo.h>
#include <ThumbMgr/ThumbMgr.h>

#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxTime.h>

#include <GuiInterface/ICamCapture.h>

#include <VxVideoLib/VxVideoLib.h>

#include <QMessageBox>
#include <QPainter>

#include "ui_ThumbnailEditWidget.h"

void                        ThumbnailEditWidget::setIsUserPickedImage( bool userPicked )         { ui.m_ThumbnailViewWidget->setIsUserPickedImage( userPicked ); }
bool                        ThumbnailEditWidget::getIsUserPickedImage( void )                    { return ui.m_ThumbnailViewWidget->getIsUserPickedImage(); }

void                        ThumbnailEditWidget::setThumnailIsCircular( bool isCircle )          { m_ThumbnailIsCircular = isCircle;  ui.m_ThumbnailViewWidget->setThumnailIsCircular( isCircle ); }
bool                        ThumbnailEditWidget::getThumbnailIsCircular( void )                  { return m_ThumbnailIsCircular; }

bool                        ThumbnailEditWidget::saveToPngFile( QString& fileName )              { return ui.m_ThumbnailViewWidget->saveToPngFile( fileName ); }

//============================================================================
ThumbnailEditWidget::ThumbnailEditWidget( QWidget* parent )
    : QWidget( parent )
    , ui(*(new Ui::ThumnailEditWidgetUi))
    , m_MyApp( GetAppInstance() )
    , m_ThumbMgr( m_MyApp.getEngine().getThumbMgr() )
{
    m_ParentApplet = GuiHelpers::findParentApplet( parent );
    ui.setupUi( this );
    QSize frameSize( GuiParams::getThumbnailSize().width() + 20, GuiParams::getThumbnailSize().height() + 20 );
    ui.m_ThumbnailFrame->setFixedSize( frameSize );

    connect( ui.m_ThumbGalleryButton, SIGNAL(clicked()), this, SLOT(slotThumbGalleryClick() ) );
    connect( ui.m_EmoticonGalleryButton, SIGNAL(clicked()), this, SLOT(slotEmoticonGalleryClick() ) );
    connect( ui.m_TakeSnapshotButton, SIGNAL(clicked()), this, SLOT(slotSnapShotButClick() ) );
    connect( ui.m_BrowsePictureButton, SIGNAL(clicked()), this, SLOT(slotBrowseButClick() ) );

    m_CameraSourceAvail = ICamCapture::getICamCapture().isCamCaptureAvailable();
}

//============================================================================
bool ThumbnailEditWidget::loadFromAsset( ThumbInfo* thumbAsset )
{
    bool loadOk = false;
    if( thumbAsset )
    {
        loadOk = ui.m_ThumbnailViewWidget->loadFromFile( thumbAsset->getAssetNameAndPath().c_str() );
        if( loadOk )
        {
            setAssetId( thumbAsset->getAssetUniqueId() );
        }
    }

    return loadOk;
}

//============================================================================
bool ThumbnailEditWidget::generateThumbAsset( ThumbInfo& assetInfoOut )
{
    bool assetGenerated = false;
    if( !m_AsssetId.isValid() )
    {
        m_AsssetId.initializeWithNewVxGUID();
    }

    VxGUID assetGuid = m_AsssetId;
    if( !assetGuid.isValid() )
    {
        VxGUID::generateNewVxGUID( assetGuid );
    }

    QString fileName = GuiHelpers::generateThumbFileName( assetGuid );
    if( !fileName.isEmpty() && saveToPngFile( fileName ) && VxFileUtil::fileExists( fileName.toUtf8().constData() ) )
    {
        if( GuiHelpers::addThumbAsset( m_MyApp, fileName, assetGuid, assetInfoOut ) )
        { 
            assetGenerated = true;
        }
        else
        {
            QString msgText = QObject::tr( "Could not get thumbnail file info" );
            QMessageBox::information( this, QObject::tr( "Error occured creating thumbnail asset " ) + fileName, msgText );
        }
    }
    else
    {
        QString msgText = QObject::tr( "Could not save thumbnail image" );
        QMessageBox::information( this, QObject::tr( "Error occured saving thumbnail to file " ) + fileName, msgText );
    }

    return assetGenerated;
}

//============================================================================
bool ThumbnailEditWidget::updateThumbAsset( ThumbInfo& thumbInfo )
{
    vx_assert( thumbInfo.isValid() );
    bool assetUpdated = false;
    VxGUID assetGuid = thumbInfo.getAssetUniqueId();
    if( assetGuid.isValid() )
    {
        QString fileName = thumbInfo.getAssetNameAndPath().c_str();
        if( !VxFileUtil::fileExists( fileName.toUtf8().constData() ) )
        {
            if( GuiHelpers::createThumbFileName( assetGuid, fileName ) )
            {
                thumbInfo.setAssetNameFromFileNameAndPath( fileName.toUtf8().constData() );
            }
        }

        if( saveToPngFile( fileName ) && VxFileUtil::fileExists( fileName.toUtf8().constData() ) )
        {
            thumbInfo.setModifiedTime( GetTimeStampMs() );
            if( m_ThumbMgr.fromGuiThumbUpdated( thumbInfo ) )
            {
                assetUpdated = true;
            }
            else
            {
                QString msgText = QObject::tr( "Could not update thumbnail asset" );
                QMessageBox::information( this, QObject::tr( "Error occured update thumbnail asset " ) + fileName, msgText );
            }
        }
        else
        {
            QString msgText = QObject::tr( "Could not save updated thumbnail image" );
            QMessageBox::information( this, QObject::tr( "Error occured saving thumbnail to file " ) + fileName, msgText );
        }
    }
    else
    {
        QString msgText = QObject::tr( "thumbnail id was invalid" );
        QMessageBox::information( this, QObject::tr( "Error occured updatin thumbnail " ), msgText );
    }

    return assetUpdated;
}

//============================================================================
void ThumbnailEditWidget::slotSnapShotButClick( void )
{
    if( m_CameraSourceAvail )
    {
        AppletSnapshot * appletSnapshot = dynamic_cast< AppletSnapshot * >( m_MyApp.getAppletMgr().launchApplet( eAppletSnapshot, m_ParentApplet ) );
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
void ThumbnailEditWidget::slotJpgSnapshot( uint8_t* pu8JpgData, uint32_t u32DataLen, int iWidth, int iHeight )
{
    QPixmap bitmap;
    if( bitmap.loadFromData( pu8JpgData, u32DataLen, "JPG" ) )
    {
        VxGUID nullGUID;
        ui.m_ThumbnailViewWidget->setThumbnailId( nullGUID );
        ui.m_ThumbnailViewWidget->setThumbnailImage( bitmap );
        ui.m_ThumbnailViewWidget->setIsUserPickedImage( true );
        emit signalImageChanged();
    }
}

//============================================================================
void ThumbnailEditWidget::slotImageSnapshot( QImage snapshotImage )
{
    if( !snapshotImage.isNull() )
    {
        VxGUID nullGUID;
        ui.m_ThumbnailViewWidget->setThumbnailId( nullGUID );
        ui.m_ThumbnailViewWidget->setThumbnailImage( QPixmap::fromImage( snapshotImage ) );
        ui.m_ThumbnailViewWidget->setIsUserPickedImage( true );
        emit signalImageChanged();
    }
}

//============================================================================
void ThumbnailEditWidget::slotBrowseButClick( void )
{
    ui.m_ThumbnailViewWidget->browseForImage();
    if( ui.m_ThumbnailViewWidget->getIsUserPickedImage() )
    {
        emit signalImageChanged();
    }
}

//============================================================================
QPixmap ThumbnailEditWidget::makeCircleImage( QPixmap& pixmap )
{
    QPixmap target( pixmap.width(), pixmap.height() );
    target.fill( Qt::transparent );

    QPainter painter( &target );

    // Set clipped region (circle) in the center of the target image
    QRegion clipRegion( QRect( 0, 0, pixmap.width(), pixmap.height() ), QRegion::Ellipse );
    painter.setClipRegion( clipRegion );

    painter.drawPixmap( 0, 0, pixmap );  
    return target;
}

//============================================================================
void ThumbnailEditWidget::slotThumbGalleryClick( void )
{
    AppletGalleryThumb * galleryThumb = dynamic_cast< AppletGalleryThumb * >( m_MyApp.getAppletMgr().launchApplet( eAppletGalleryThumb, m_ParentApplet ) );
    if( galleryThumb )
    {
        connect( galleryThumb, SIGNAL(signalThumbSelected( AppletBase *, ThumbnailViewWidget * ) ), this, SLOT(slotThumbSelected( AppletBase *, ThumbnailViewWidget * ) ) );
    }
}

//============================================================================
void ThumbnailEditWidget::slotEmoticonGalleryClick( void )
{
    AppletGalleryEmoticon * galleryEmoticon = dynamic_cast< AppletGalleryEmoticon* >( m_MyApp.getAppletMgr().launchApplet( eAppletGalleryEmoticon, m_ParentApplet ) );
    if( galleryEmoticon )
    {
        connect( galleryEmoticon, SIGNAL(signalThumbSelected( AppletBase *, ThumbnailViewWidget * ) ), this, SLOT(slotThumbSelected( AppletBase *, ThumbnailViewWidget * ) ) );
    }
}

//============================================================================
void ThumbnailEditWidget::slotThumbSelected( AppletBase * thumbGallery, ThumbnailViewWidget * thumb )
{
    if( thumbGallery && thumb )
    {
        VxGUID assetGuid = thumb->getThumbnailId();
        ThumbInfo* thumbAsset = dynamic_cast<ThumbInfo*>(m_ThumbMgr.findAsset( assetGuid ));
        if( thumbAsset )
        {
            if( loadFromAsset( thumbAsset ) )
            {
                setAssetId( assetGuid );
            }
        }

        disconnect( thumbGallery, SIGNAL(signalThumbSelected( AppletBase *, ThumbnailViewWidget * ) ), this, SLOT(slotThumbSelected( AppletBase *, ThumbnailViewWidget * ) ) );
        thumbGallery->closeApplet();
        emit signalImageChanged();
    }
}

//============================================================================
bool ThumbnailEditWidget::loadThumbnail( VxGUID& assetId )
{
    bool result = false;
    if( assetId.isValid() )
    {
        ThumbInfo* thumbAsset = dynamic_cast<ThumbInfo*>(m_ThumbMgr.findAsset( assetId ));
        if( thumbAsset )
        {
            if( loadFromAsset( thumbAsset ) )
            {
                setAssetId( assetId );
                result = true;
            }
            else
            {
                LogMsg( LOG_VERBOSE, "ThumbnailEditWidget::loadThumbnail failed to load thumb %s", assetId.toOnlineIdString().c_str() );
            }
        }
    }

    return result;
}

//============================================================================
VxGUID ThumbnailEditWidget::updateAndGetThumbnailId( void )
{
    bool assetExists = isAssetIdValid();
    if( assetExists )
    {
        ThumbInfo* existingAsset =  dynamic_cast<ThumbInfo*>(m_ThumbMgr.findAsset( getAssetId() ));
        if( existingAsset )
        {
            return existingAsset->getAssetUniqueId();
        }
        else
        {
            assetExists = false;
        }
    }

    if( !assetExists && getIsUserPickedImage() )
    {
        ThumbInfo assetInfo;
        if( generateThumbAsset( assetInfo ) )
        {
            return assetInfo.getAssetUniqueId();
        }
    }

    VxGUID nullGuid;
    return nullGuid;
}

//============================================================================
void ThumbnailEditWidget::clearAssetId( void )                            
{ 
    m_AsssetId.clearVxGUID(); 
    ui.m_ThumbnailViewWidget->clear();
}