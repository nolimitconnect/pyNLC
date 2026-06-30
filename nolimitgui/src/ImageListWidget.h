#pragma once
//============================================================================
// Copyright (C) 2019 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#if defined(TARGET_OS_LINUX)
#include <QWidget> // must be declared first or linux Qt will error in qmetatype.h 2167:23: array subscript value 53 is outside the bounds
#endif // defined(TARGET_OS_LINUX)

#include <CoreLib/AssetDefs.h>
#include <CoreLib/VxElapseTimer.h>

#include <QListWidget>

class ImageListRow;
class ThumbnailViewWidget;

class AppCommon;
class P2PEngine;
class ThumbInfo;
class ThumbMgr;
class VxGUID;

class ImageListWidget : public QListWidget
{
	Q_OBJECT

public:
	ImageListWidget( QWidget* parent );

    void                        loadThumbAssets( void );
    void                        clearImages( void );
    void                        clearItems( void );

    void                        addAsset( ThumbInfo* asset );

signals:
	void						signalImageClicked( ThumbnailViewWidget * thumb );

private slots:
	void						slotItemClicked( QListWidgetItem* );
    void						slotImageClicked( ThumbnailViewWidget * thumb );

protected:
    void                        resizeEvent( QResizeEvent* ev );

    bool                        thumbExistsInList( VxGUID& assetId );
    ImageListRow *              getRowWithRoomForThumbnail();

	//=== vars ===//
	AppCommon&					m_MyApp;
	P2PEngine&					m_Engine;
    ThumbMgr&					m_ThumbMgr;

	VxElapseTimer						m_ClickEventTimer; // avoid duplicate clicks
};

