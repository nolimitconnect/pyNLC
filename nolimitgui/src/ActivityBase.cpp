//============================================================================
// Copyright (C) 2014 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "ActivityBase.h"

#include "ActivityMsgBoxYesNo.h"
#include "AppCommon.h"
#include "AppGlobals.h"
#include "AppSettings.h"
#include "ActivityMessageBox.h"
#include "IdentWidget.h"

#include "GuiHelpers.h"
#include "GuiOfferSession.h"
#include "GuiPlayerMgr.h"
#include "GuiParams.h"
#include "HomeWindow.h"

#include "MyIcons.h"
#include "SoundFxMgr.h"
#include "WaitingSpinnerWidget.h"
#include "VxPushButton.h"

#include <P2PEngine/P2PEngine.h>

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxGlobals.h>

#include <QRect>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QResizeEvent>
#include <QStandardPaths>

#include <stdio.h>
#include <array>

#include "ui_ActivityBase.h"

const int RESIZE_WINDOW_COMPLETED_TIMEOUT = 500;

//============================================================================
ActivityBase::ActivityBase( const char* objName, AppCommon& app, QWidget* parent, EApplet eAppletType, bool isDialog, bool isPopup, bool fullWindowSize )
: QDialog( parent, Qt::Widget )
, ObjectCommon( objName )
, ui(*(new Ui::ActivityBaseClass))
, m_MyApp( app )
, m_UserMgr( app.getUserMgr() )
, m_Engine( app.getEngine() )
, m_FromGui( m_Engine.getFromGuiInterface() )
, m_ParentWidget( parent )
, m_EAppletType( eAppletType )
, m_ePluginType( ePluginTypeInvalid )
, m_ResizingTimer( new QTimer(this) )
, m_DelayedCloseTimer( new QTimer(this) )
, m_IsDialog( isDialog )
, m_IsPopup( isPopup )
, m_FullWindowSize( fullWindowSize )
{
    vx_assert( objName );
    setObjectName( objName );
	m_AppInstId.initializeWithNewVxGUID();

	VxAppStyle::clearFocusFrameWidget();

	if( 0xcdcdcdcdcdcdcdcd == (uint64_t)parent )
	{
		vx_assert( false );
		LogMsg( LOG_FATAL, "ActivityBase::ActivityBase: Bad Param");
	}

    m_IsDialog = isDialog 
                || ( eAppletUnknown == eAppletType ) 
                || ( eAppletActivityDialog == eAppletType ); // do not setup base class ui in the case of activity dialog because of conflict with dialog ui
    if( !m_IsDialog && !m_IsPopup )
    {
        m_WindowFlags = Qt::SubWindow;
        setWindowFlags( m_WindowFlags );
        ui.setupUi( this );
    }
    else if( m_IsPopup )
    {
        //m_WindowFlags = Qt::Popup;
        m_WindowFlags = Qt::Dialog | Qt::WindowStaysOnTopHint;
        setWindowFlags( m_WindowFlags );

        connect( &m_MyApp, SIGNAL(signalMainWindowMoved()), this, SLOT(slotRepositionToParent()) );

        LogMsg( LOG_DEBUG, "ActivityBase::ActivityBase: Activity Popup %s\n", objectName().toUtf8().constData() );
    }
    else if( parent )
    {
        // dialog needs to cover parent

        //m_WindowFlags = Qt::Window;
        //m_WindowFlags = Qt::SubWindow;
        //m_WindowFlags = Qt::Popup;
        m_WindowFlags = Qt::Dialog | Qt::FramelessWindowHint;
        //m_WindowFlags = Qt::Sheet;
        //m_WindowFlags = Qt::Drawer;
        //m_WindowFlags = Qt::Tool;
        //m_WindowFlags = Qt::ForeignWindow;
        //m_WindowFlags = Qt::CoverWindow;
        setWindowFlags( m_WindowFlags );
        m_MyApp.getAppTheme().applyTheme( this );
        if( !m_FullWindowSize )
        {
            m_ParentWidget = getParentPageFrame();
        }

        connect( &m_MyApp, SIGNAL(signalMainWindowMoved() ), this, SLOT(slotRepositionToParent() ) );

        LogMsg( LOG_DEBUG, "ActivityBase::ActivityBase: Activity Dialog %s\n", objectName().toUtf8().constData() );
    }

	connect( &m_MyApp,			SIGNAL(signalMainWindowResized() ),			this, SLOT(slotRepositionToParent() ) );
    connect( &m_MyApp,          SIGNAL(signalMainWindowMoved() ),              this, SLOT(slotRepositionToParent() ) );

	connect( &m_MyApp,			SIGNAL(signalStatusMsg(QString)),				this, SLOT(slotStatusMsg(QString)) );
	connect( this,				SIGNAL(signalShowShouldExitMsgBox(QString)),	this, SLOT(slotShowShouldExitMsgBox(QString)), Qt::QueuedConnection );
	connect( this,				SIGNAL(finished(int)),							this, SLOT(slotActivityFinished(int)) );

	connect( m_ResizingTimer,		SIGNAL(timeout()), this, SLOT(slotResizeWindowTimeout()) );
	connect( m_DelayedCloseTimer,	SIGNAL(timeout()), this, SLOT(slotBackButtonClicked()), Qt::QueuedConnection );

	connect(	this, 
				SIGNAL(signalPlayNotifySound()), 
				&m_MyApp.getSoundFxMgr(), 
				SLOT(slotPlayNotifySound()) );

	connect(	this, 
				SIGNAL(signalPlayShredderSound()), 
				&m_MyApp.getSoundFxMgr(), 
				SLOT(slotPlayShredderSound()) );

    if( !m_IsDialog && !m_IsPopup )
    {
        // if dialog then have to wait for dialog sets up title and bottom bar widgets before connecting them
        connectBarWidgets();
    }
}

//============================================================================
void ActivityBase::connectBarWidgets( bool removeConnections )
{
	connectTitleBarWidget( getTitleBarWidget(), removeConnections );
	connectBottomBarWidget( getBottomBarWidget(), removeConnections );
	if( !removeConnections )
	{
		updateExpandWindowIcon();
		if( m_IsDialog || m_IsPopup )
		{
			slotRepositionToParent();
		}
	}
}

//============================================================================
void ActivityBase::connectTitleBarWidget( TitleBarWidget * titleBar, bool removeConnections )
{
    //=== title bar connections ====//
	if( !removeConnections )
	{
		connect( titleBar, SIGNAL(signalPowerButtonClicked()), this, SLOT(slotPowerButtonClicked()) );
		connect( titleBar, SIGNAL(signalCameraSnapshotButtonClicked()), this, SLOT(slotCameraSnapshotButtonClicked()) );
		connect( titleBar, SIGNAL(signalCamPreviewClicked()), this, SLOT(slotCamPreviewClicked()) );
		connect( titleBar, SIGNAL(signalMenuTopButtonClicked()), this, SLOT(slotMenuTopButtonClicked()) );
		connect( titleBar, SIGNAL(signalBackButtonClicked()), this, SLOT(slotBackButtonClicked()) );
		VxPushButton* titleButton = titleBar->getAppIconPushButton();
        if( titleButton )
        {
            connect( titleButton, SIGNAL(signalAppIconSpecialClick() ), this, SLOT(slotAppIconSpecialClick() ) );
        }
	}
	else
	{
		disconnect( titleBar, SIGNAL(signalPowerButtonClicked()), this, SLOT(slotPowerButtonClicked()) );
		disconnect( titleBar, SIGNAL(signalCameraSnapshotButtonClicked()), this, SLOT(slotCameraSnapshotButtonClicked()) );
		disconnect( titleBar, SIGNAL(signalCamPreviewClicked()), this, SLOT(slotCamPreviewClicked()) );
		disconnect( titleBar, SIGNAL(signalMenuTopButtonClicked()), this, SLOT(slotMenuTopButtonClicked()) );
		disconnect( titleBar, SIGNAL(signalBackButtonClicked()), this, SLOT(slotBackButtonClicked()) );
		VxPushButton * titleButton = titleBar->getAppIconPushButton();
        if( titleButton )
        {
            disconnect( titleButton, SIGNAL(signalAppIconSpecialClick()), this, SLOT(slotAppIconSpecialClick()) );
        }
	}
}

//============================================================================
void ActivityBase::connectBottomBarWidget( BottomBarWidget * bottomBar, bool removeConnections )
{
    //=== bottom bar signals ===// 
	if( !removeConnections )
	{
		connect( bottomBar, SIGNAL(signalMenuBottomButtonClicked()), this, SLOT(slotMenuBottomButtonClicked()) );
		connect( bottomBar, SIGNAL(signalExpandWindowButtonClicked()), this, SLOT(slotExpandWindowButtonClicked()) );
	}
	else
	{
		disconnect( bottomBar, SIGNAL(signalMenuBottomButtonClicked()), this, SLOT(slotMenuBottomButtonClicked()) );
		disconnect( bottomBar, SIGNAL(signalExpandWindowButtonClicked()), this, SLOT(slotExpandWindowButtonClicked()) );
	}
}

//============================================================================
VxPushButton* ActivityBase::getAppIconPushButton( void )
{ 
	return getTitleBarWidget()->getAppIconPushButton();
}

//============================================================================
bool ActivityBase::isAlwaysAvailableApplet( void )
{
	return getAppletType() == eAppletMultiMessenger ||
		getAppletType() == eAppletDownloads ||
		getAppletType() == eAppletUploads;
}

//============================================================================
bool ActivityBase::isMessengerReady( void )
{
	return m_MyApp.isMessengerReady();
}

//============================================================================
bool ActivityBase::isMessagerFrame( void )
{
    return GuiHelpers::isMessagerFrame( this );
}

//============================================================================
bool ActivityBase::isHomeFrame( void )
{
	return !GuiHelpers::isMessagerFrame( this );
}

//============================================================================
bool ActivityBase::isPluginEnabled( EPluginType pluginType )
{
	return m_Engine.getMyPktAnnounce().isPluginEnabled( pluginType );
}

//============================================================================
bool ActivityBase::getIsMaxScreenSize( void )
{
     return m_MyApp.getIsMaxScreenSize( isMessagerFrame() );
}

//============================================================================
void ActivityBase::setIsMaxScreenSize( bool fullScreen )
{
    m_MyApp.setIsMaxScreenSize( isMessagerFrame(), fullScreen );
}

//============================================================================
// only available for applets
QFrame* ActivityBase::getContentItemsFrame( void )
{
    return ui.m_ContentItemsFrame;
}

//============================================================================
// only available for applets
QFrame* ActivityBase::getContentFrameOfLaunchFrame( void )
{
    return GuiHelpers::getLaunchPageFrame( ui.m_ContentItemsFrame );
}

//============================================================================
// only available for applets
QFrame* ActivityBase::getContentFrameOfMessengerFrame( void )
{
    return GuiHelpers::getMessengerPageFrame( ui.m_ContentItemsFrame );
}

//============================================================================
// only available for applets
QFrame* ActivityBase::getContentFrameOfOppositePageFrame( void )
{
    return GuiHelpers::getOppositePageFrame( ui.m_ContentItemsFrame );
}

//============================================================================
// get home page activity ( Launch or Messenger Page )
QWidget* ActivityBase::getParentPageFrame( void )
{
    return GuiHelpers::getParentPageFrame( this );
}

//============================================================================
QString ActivityBase::getParentPageFrameName( void )
{
    return GuiHelpers::getParentPageFrameName( this );
}

//============================================================================
// overridden in dialogs
TitleBarWidget * ActivityBase::getTitleBarWidget( void )
{
    return ui.m_TitleBarWidget;
}

//============================================================================
// overridden in dialogs
BottomBarWidget * ActivityBase::getBottomBarWidget( void )
{
    return ui.m_BottomBarWidget;
}

//============================================================================
void ActivityBase::setupStyledDlg(	GuiUser*           poFriend,
									IdentWidget *		poIdentWidget,
									EPluginType			pluginType,
									VxPushButton *		poPermissionButton,
									QLabel *			poPermissionLabel )
{
	if( poIdentWidget )
	{
		poIdentWidget->setIdentMenuButtonVisible( false );
		poIdentWidget->updateIdentity( poFriend );
	}

	if( poPermissionButton )
	{
		EPluginAccess ePluginAccess = poFriend->getMyAccessPermissionFromHim(pluginType);

		poPermissionButton->setIcon( getMyIcons().getPluginIcon( pluginType, ePluginAccess ) );
		QString strAction = GuiParams::describePluginAction( poFriend, pluginType, ePluginAccess );
		poPermissionLabel->setText( strAction );
	}
}

//============================================================================
void ActivityBase::setTitleBarText( QString titleText )
{
	getTitleBarWidget()->setTitleBarText( titleText );
}

//============================================================================
QString ActivityBase::getTitleBarText( void )
{
	return getTitleBarWidget()->getTitleBarText();
}

//============================================================================  
void ActivityBase::setStatusText( QString statusMsgText )
{
	m_MyApp.toGuiStatusMessage( statusMsgText.toUtf8().constData() );
}

//============================================================================  
void ActivityBase::setNewParent( QWidget* parent )
{
	bool dlgIsVisible = isVisible();
	m_ParentWidget = parent;
	setParent( parent );
	if( dlgIsVisible )
	{
		show();
	}

	repositionToParent();
}

//============================================================================  
void ActivityBase::checkDiskSpace( void )
{
	uint64_t diskFreeSpace = m_Engine.fromGuiGetDiskFreeSpace();
	if( ( 0 != diskFreeSpace ) && ( diskFreeSpace < 1000000000 ) )
	{
        m_MyApp.toGuiUserMessage( "Disk Space is low %s", GuiParams::describeFileLength( diskFreeSpace ).toUtf8().constData() );
	}
}

//============================================================================
MyIcons& ActivityBase::getMyIcons( void )
{
	return m_MyApp.getMyIcons();
}

//============================================================================
void ActivityBase::slotStatusMsg( QString strMsg )
{
	//LogMsg( LOG_INFO, strMsg.toStdString().c_str() );
	if( 0 != m_StatusMsgLabel )
	{
		m_StatusMsgLabel->setText( strMsg );
	}
}

//============================================================================
void ActivityBase::slotShowShouldExitMsgBox( QString exitReason )
{
	QMessageBox::information( this, QObject::tr("Session End Message"), exitReason, QMessageBox::Ok );
	accept();
}

//============================================================================
bool ActivityBase::playFile( QString fileName, int pos0to100000, bool isStream, bool useExternPlayer )
{
	return m_MyApp.getPlayerMgr().playFile( fileName, pos0to100000, isStream, useExternPlayer, getParentPageFrame() );
}

//============================================================================
void ActivityBase::playSound( ESndDef sndDef )
{
	m_MyApp.playSound( sndDef );
}

//============================================================================
void ActivityBase::okMessageBox( QString title, QString msg )
{
	QMessageBox::information( this, title, msg, QMessageBox::Ok );
}

//============================================================================
void ActivityBase::okMessageBox2( QString title, const char* msg, ... )
{
	std::array<char,2048> szBuffer;
	va_list argList;
	va_start( argList, msg );
	vsnprintf( szBuffer.data(), 2048, msg, argList);
	va_end( argList );
	szBuffer.data()[2047] = 0;

	okMessageBox( title, szBuffer.data() );
}

//============================================================================
bool ActivityBase::yesNoMessageBox( QString title, QString msg )
{
	if( QMessageBox::Yes == QMessageBox::question( this, title, msg, QMessageBox::Yes | QMessageBox::No ) )
	{
		return true;
	}

	return false;
}

//============================================================================
bool ActivityBase::yesNoMessageBox2( QString title, const char* msg, ... )
{
	std::array<char,2048> szBuffer;
	va_list argList;
	va_start( argList, msg );
	vsnprintf( szBuffer.data(), 2048, msg, argList);
	va_end( argList );
	szBuffer.data()[2047] = 0;

	return yesNoMessageBox( title, szBuffer.data() );
}

//============================================================================
void ActivityBase::errMessageBox( QString title, QString msg )
{
	QMessageBox::warning( this, title, msg, QMessageBox::Ok );
}

//============================================================================
void ActivityBase::errMessageBox2( QString title, const char* msg, ... )
{
	if( 0 != msg )
	{
		std::array<char,2048> szBuffer;
		va_list argList;
		va_start( argList, msg );
		vsnprintf( szBuffer.data(), 2048, msg, argList);
		va_end( argList );
		szBuffer.data()[2047] = 0;

		errMessageBox( title, szBuffer.data() );
	}
}

//============================================================================
void ActivityBase::hideEvent( QHideEvent* hideEvent )
{
	QDialog::hideEvent( hideEvent );
	if( isAlwaysAvailableApplet() )
	{
		connectBarWidgets( true );
	}
}

//============================================================================
void ActivityBase::showEvent( QShowEvent* showEvent )
{
#if defined(TARGET_OS_ANDROID)
    // not sure why but android seems to loose the theme text color so reapply
    m_MyApp.getAppTheme().applyTheme( this );
#endif // defined(TARGET_OS_ANDROID)

    if( m_ParentWidget )
	{
		slotRepositionToParent();
	}

	QDialog::showEvent( showEvent );
#if defined(TARGET_OS_ANDROID)
    // not sure why but android seems to loose the theme text color so reapply
    m_MyApp.getAppTheme().applyTheme( this );
#endif // defined(TARGET_OS_ANDROID)

    if( !m_InitialFocusWasSet )
    {
        m_InitialFocusWasSet = true;
        if( getTitleBarWidget() )
        {
            getTitleBarWidget()->getBackButton()->setFocus();
        }
    }

	emit signalDialogWasShown();
#if defined(TARGET_OS_ANDROID)
    // not sure why but android seems to loose the theme text color so reapply
    m_MyApp.getAppTheme().applyTheme( this );
#endif // defined(TARGET_OS_ANDROID)
	if( isAlwaysAvailableApplet() )
	{
		connectBarWidgets( false );
		setTitleBarText( DescribeApplet( m_EAppletType ) );
	}
}

//============================================================================
void ActivityBase::changeEvent( QEvent* event )
{
	QDialog::changeEvent( event );
	if( event && ( QEvent::LanguageChange == event->type() ) )
	{
		retranslateUi();
	}
}

//============================================================================
void ActivityBase::retranslateUi( void )
{
	if( !m_IsDialog && !m_IsPopup )
	{
		ui.retranslateUi( this );
	}

	const QString titleText = getTranslatedTitleBarText();
	if( !titleText.isEmpty() && getTitleBarWidget() )
	{
		setTitleBarText( titleText );
	}
}

//============================================================================
QString ActivityBase::getTranslatedTitleBarText( void ) const
{
	if( eAppletUnknown == m_EAppletType )
	{
		return QString();
	}

	return DescribeApplet( m_EAppletType );
}

//============================================================================
void ActivityBase::resizeEvent( QResizeEvent* ev )
{
	QDialog::resizeEvent( ev );
    updateExpandWindowIcon();
	emit signalActivityBaseWasResized();
	m_ResizingWindowSize = ev->size();
	if( !m_IsResizing )
	{
		m_IsResizing = true;
		onResizeBegin( m_ResizingWindowSize );
	}

	onResizeEvent( m_ResizingWindowSize );
	m_ResizingTimer->setSingleShot( true );
	m_ResizingTimer->start( RESIZE_WINDOW_COMPLETED_TIMEOUT );
}

//============================================================================
void ActivityBase::closeEvent( QCloseEvent * ev )
{
	onCloseEvent();
	m_MyApp.activityStateChange( this, false );
	QDialog::closeEvent( ev );
}

//============================================================================
void ActivityBase::slotExpandWindowButtonClicked( void )
{
    m_MyApp.setIsMaxScreenSize( isMessagerFrame(), !m_MyApp.getIsMaxScreenSize( isMessagerFrame() ) );
    updateExpandWindowIcon();
}

//============================================================================
void ActivityBase::updateExpandWindowIcon( void )
{
    if( !m_IsDialog && !m_IsPopup )
    {
        if( m_MyApp.getIsMaxScreenSize( isMessagerFrame() ) )
        {
            getBottomBarWidget()->setExpandWindowButtonIcon( eMyIconWindowShrink );
        }
        else
        {
            getBottomBarWidget()->setExpandWindowButtonIcon( eMyIconWindowExpand );
        }
    }
    else if( getBottomBarWidget() && getBottomBarWidget()->isVisible() )
    {
        if( m_MyApp.getIsMaxScreenSize( isMessagerFrame() ) )
        {
            getBottomBarWidget()->setExpandWindowButtonIcon( eMyIconWindowShrink );
        }
        else
        {
            getBottomBarWidget()->setExpandWindowButtonIcon( eMyIconWindowExpand );
        }
    }
}

//============================================================================
void ActivityBase::slotRepositionToParent( void )
{
	repositionToParent();
}

//============================================================================
void ActivityBase::slotActivityFinished( int finishResult )
{
	onActivityFinish();
}

//============================================================================
void ActivityBase::repositionToParent( void )
{
	if( m_ParentWidget )
	{
		if( 0xcdcdcdcdcdcdcdcd == (uint64_t)m_ParentWidget )
		{
			vx_assert( false );
		}

		QRect parentRect = m_ParentWidget->geometry();
		if( ( 0 >= parentRect.width() )
			|| ( 0 >= parentRect.height() ) )
		{
			// invalid window rectangle
			return;
		}

        if( m_IsPopup )
        {
            QRect parentRect( m_ParentWidget->mapToGlobal( QPoint( 0, 0 ) ), m_ParentWidget->size() );

            move( QStyle::alignedRect( Qt::LeftToRight, Qt::AlignCenter, size(), parentRect ).topLeft() );
            LogMsg( LOG_DEBUG, "Popup %s size %d %d", getObjName(), parentRect.width(), parentRect.height() );
        }
        else if( m_IsDialog )
        {
            QRect parentRect( m_ParentWidget->mapToGlobal( QPoint( 0, 0 ) ), m_ParentWidget->size() );
            // LogMsg( LOG_DEBUG, "Dialog %s size %d %d", getObjName(), parentRect.width(), parentRect.height() );
            setGeometry( parentRect );
        }
        else
        {
            parentRect.setRight( parentRect.right() - parentRect.left() );
            parentRect.setLeft( 0 );
            parentRect.setBottom( parentRect.bottom() - parentRect.top() );
            parentRect.setTop( 0 );

            //LogMsg( LOG_DEBUG, "Reposition to x=%d y=%d w=%d h=%d %s parent %s\n",
            //    parentRect.left(), parentRect.top(), parentRect.width(), parentRect.height(), getObjName(), m_ParentWidget->objectName().toUtf8().constData() );
            setGeometry( parentRect );
            // LogMsg( LOG_DEBUG, "Normal %s size %d %d", getObjName(), parentRect.width(), parentRect.height() );
        }
	}
    else
    {
        LogMsg( LOG_WARNING, "Object %s has no parent page", objectName().toUtf8().constData() );
    }
}

//============================================================================
//=== title bar button visiblility ====//
//============================================================================
void ActivityBase::setPowerButtonVisibility( bool visible )
{
	getTitleBarWidget()->setPowerButtonVisibility( visible );
}

//============================================================================
void ActivityBase::setNetStatusVisibility( bool visible )
{
	getTitleBarWidget()->setNetStatusVisibility( visible );
}

//============================================================================
void ActivityBase::setOfferListButtonVisibility( bool visible )
{
	getTitleBarWidget()->setOfferListButtonVisibility( visible );
}

//============================================================================
void ActivityBase::setHostJoinRequestListButtonVisibility( bool visible )
{
	getTitleBarWidget()->setHostJoinRequestListButtonVisibility( visible );
}

//============================================================================
void ActivityBase::setCameraButtonVisibility( bool visible )
{
	getTitleBarWidget()->setCameraButtonVisibility( visible );
}

//============================================================================
void ActivityBase::setCamPreviewVisibility( bool visible )
{
	getTitleBarWidget()->setCamPreviewVisibility( visible );
}

//============================================================================
void ActivityBase::setCamViewerCountVisibility( bool visible )
{
	getTitleBarWidget()->setCamViewerCountVisibility( visible );
}

//============================================================================
void ActivityBase::setMenuTopButtonVisibility( bool visible )
{
	getTitleBarWidget()->setMenuTopButtonVisibility( visible );
}

//============================================================================
void ActivityBase::setMenuListButtonVisibility( bool visible )
{
	getTitleBarWidget()->setMenuListButtonVisibility( visible );
}

//============================================================================
void ActivityBase::setBackButtonVisibility( bool visible )
{
	getTitleBarWidget()->setBackButtonVisibility( visible );
}

//=== bottom bar button visibility ===// 
//============================================================================
void ActivityBase::setMenuBottomVisibility( bool visible )
{
	getBottomBarWidget()->setMenuBottomVisibility( visible );
}

//============================================================================
void ActivityBase::setExpandWindowVisibility( bool visible )
{
	getBottomBarWidget()->setExpandWindowVisibility( visible );
}

//============================================================================
//=== title bar button icons ====//
//============================================================================
void ActivityBase::setPowerButtonIcon( EMyIcons myIcon )
{
	getTitleBarWidget()->setPowerButtonIcon( myIcon );
}

//============================================================================
void ActivityBase::setMicrophoneIcon( EMyIcons myIcon )
{
	getTitleBarWidget()->setMicrophoneIcon( myIcon );
}

//============================================================================
void ActivityBase::setSpeakerIcon( EMyIcons myIcon )
{
	getTitleBarWidget()->setSpeakerIcon( myIcon );
}

//============================================================================
void ActivityBase::setCameraIcon( EMyIcons myIcon )
{
	getTitleBarWidget()->setCameraIcon( myIcon );
}

//============================================================================
void ActivityBase::setTopMenuButtonIcon( EMyIcons myIcon )
{
	getTitleBarWidget()->setTopMenuButtonIcon( myIcon );
}

//============================================================================
void ActivityBase::setBackButtonIcon( EMyIcons myIcon )
{
	getTitleBarWidget()->setBackButtonIcon( myIcon );
}

//=== bottom bar button icon ===// 
//============================================================================
void ActivityBase::setMenuBottomButtonIcon( EMyIcons myIcon )
{
	getBottomBarWidget()->setMenuBottomButtonIcon( myIcon );
}

//============================================================================
void ActivityBase::setExpandWindowButtonIcon( EMyIcons myIcon )
{
	getBottomBarWidget()->setExpandWindowButtonIcon( myIcon );
}

//============================================================================
//=== title bar button colors ====//
//============================================================================
void ActivityBase::setPowerButtonColor( QColor iconColor )
{
	getTitleBarWidget()->setPowerButtonColor( iconColor );
}

//============================================================================
void ActivityBase::setMicrophoneColor( QColor iconColor )
{
	getTitleBarWidget()->setMicrophoneColor( iconColor );
}

//============================================================================
void ActivityBase::setSpeakerColor( QColor iconColor )
{
	getTitleBarWidget()->setSpeakerColor( iconColor );
}

//============================================================================
void ActivityBase::setCameraColor( QColor iconColor )
{
	getTitleBarWidget()->setCameraColor( iconColor );
}

//============================================================================
void ActivityBase::setTopMenuButtonColor( QColor iconColor )
{
	getTitleBarWidget()->setTopMenuButtonColor( iconColor );
}

//============================================================================
void ActivityBase::setBackButtonColor( QColor iconColor )
{
	getTitleBarWidget()->setBackButtonColor( iconColor );
}

//============================================================================
//=== bottom bar icon color ===//
//============================================================================
//============================================================================
void ActivityBase::setMenuBottomButtonColor( QColor iconColor )
{
	getBottomBarWidget()->setMenuBottomButtonColor( iconColor );
}

//============================================================================
void ActivityBase::setExpandWindowButtonColor( QColor iconColor )
{
	getBottomBarWidget()->setExpandWindowButtonColor( iconColor );
}

//============================================================================
//=== title bar slots ====//
//============================================================================
void ActivityBase::slotPowerButtonClicked( void )
{
	emit signalPowerButtonClicked();
}

//============================================================================
void ActivityBase::slotCameraSnapshotButtonClicked( void )
{
	emit signalCameraSnapshotButtonClicked();
}

//============================================================================
void ActivityBase::slotCamPreviewClicked( void )
{
	emit signalCamPreviewClicked();
}

//============================================================================
void ActivityBase::slotMenuTopButtonClicked( void )
{
	emit signalMenuTopButtonClicked();
}

//============================================================================
void ActivityBase::slotBackButtonClicked( void )
{
	onBackButtonClicked();
}

// override default behavior of closing dialog when back button is clicked
//============================================================================
void ActivityBase::onBackButtonClicked( void )
{
	if( isAlwaysAvailableApplet() )
	{
		hide();
	}
	else
	{
		m_MyApp.activityStateChange( this, false );
		emit signalBackButtonClicked();
		closeApplet();
	}
}

//=== bottom bar slots ====//
//============================================================================
void ActivityBase::slotArrowLeftButtonClicked( void )
{
	emit signalArrowLeftButtonClicked();
}

//============================================================================
void ActivityBase::slot30SecBackwardButtonClicked( void )
{
	emit signal30SecBackwardButtonClicked();
}

//============================================================================
void ActivityBase::slotMediaPlayButtonClicked( void )
{
	emit signalMediaPlayButtonClicked();
}

//============================================================================
void ActivityBase::slotMediaTrashButtonClicked( void )
{
	emit signalMediaTrashButtonClicked();
}

//============================================================================
void ActivityBase::slot30SecForwardButtonClicked( void )
{
	emit signal30SecForwardButtonClicked();
}

//============================================================================
void ActivityBase::slotMediaFileShareClicked( void )
{
	emit signalMediaFileShareClicked();
}

//============================================================================
void ActivityBase::slotMediaLibraryButtonClicked( void )
{
	emit signalMediaLibraryButtonClicked();
}

//============================================================================
void ActivityBase::slotArrowRightButtonClicked( void )
{
	emit signalArrowRightButtonClicked();
}

//============================================================================
void ActivityBase::slotMediaRepeatButtonClicked( void )
{
	emit signalMediaRepeatButtonClicked();
}

//============================================================================
void ActivityBase::slotMenuBottomButtonClicked( void )
{
	emit signalMenuBottomButtonClicked();
}

//============================================================================
void ActivityBase::slotResizeWindowTimeout()
{
	if( m_IsResizing )
	{
		m_IsResizing = false;
		onResizeEnd( m_ResizingWindowSize );
	}
}

//============================================================================
void ActivityBase::fillMyNodeUrl( QLabel * myUrlLabel )
{
    if( myUrlLabel )
    {
        std::string url;
        m_MyApp.getEngine().fromGuiGetNodeUrl( url );
        if( !url.empty() )
        {
            myUrlLabel->setText( QString( url.c_str() ) );
        }
    }
}

//============================================================================
void ActivityBase::setTitleBarAppletIcon( EMyIcons appletIcon )
{
    TitleBarWidget * titleBar = getTitleBarWidget();
    if( titleBar )
    {
        VxPushButton * titleButton = titleBar->getAppIconPushButton();
        if( titleButton )
        {
            titleButton->setAppIcon( appletIcon, this );
            disconnect( titleButton, SIGNAL(signalAppIconSpecialClick() ), this, SLOT(slotAppIconSpecialClick() ) );
            connect( titleButton, SIGNAL(signalAppIconSpecialClick() ), this, SLOT(slotAppIconSpecialClick() ) );
        }
    }
}

//============================================================================
void ActivityBase::slotAppIconSpecialClick( void )
{
    onAppIconSpecialClick( this );
}

//============================================================================
void ActivityBase::onAppIconSpecialClick( ActivityBase* activityBase )
{
    LogMsg( LOG_VERBOSE, "onAppIconSpecialClick" );
}

//============================================================================
void ActivityBase::closeApplet( void )
{
	if( !m_HasBeenClosed )
	{
		LogMsg( LOG_VERBOSE, "closeApplet %s", this->objectName().isEmpty() ? "UNKNOWN APPLET" : this->objectName().toUtf8().constData());
		m_HasBeenClosed = true;
		close();
		deleteLater(); // required or never actually deletes the activity
	}
}

//============================================================================
bool ActivityBase::confirmDeleteFile( QString fileName, bool shredFile )
{
	return GuiHelpers::confirmDeleteFile( m_MyApp, getContentItemsFrame(), shredFile, fileName );
}

//============================================================================
bool ActivityBase::confirmDeleteFile( AssetBaseInfo& assetInfo, bool shredFile )
{
	return confirmDeleteFile( assetInfo.getAssetNameAndPath().c_str(), shredFile );
}

//============================================================================
void ActivityBase::setNewParentPage( QWidget* newParent )
{
	hide();
	VxAppStyle::clearFocusFrameWidget();
	setParent( newParent );
	show();
}

//============================================================================
void ActivityBase::startBusySpinner( QWidget* parent )
{
	if( m_BusySpinner )
	{
		LogMsg( LOG_ERROR, "AppletPlayerNlcBase::%s Busy Dialog already exists", __func__ );
		return;
	}

	m_BusySpinner = new WaitingSpinnerWidget( parent ? parent : this );
	m_BusySpinner->startWaiting( m_MyApp.getAppTheme().getNotifyColor( eNotifyOnline ) );
}

//============================================================================
void ActivityBase::stopBusySpinner( void )
{
	if( !m_BusySpinner )
	{
		//LogMsg( LOG_ERROR, "AppletPlayerNlcBase::%s Busy Spinner does NOT exists", __func__ );
		return;
	}

	m_BusySpinner->stopWaiting();
	m_BusySpinner->close();
	m_BusySpinner->deleteLater();
	m_BusySpinner = nullptr;
}

//============================================================================
void ActivityBase::delayedCloseApplet( void )
{
    m_DelayedCloseTimer->setSingleShot( true );
    m_DelayedCloseTimer->start(10);
}

//============================================================================
void ActivityBase::setAppletFileFilter( EApplet applet, EFileFilterType fileFilter )
{
	switch( applet )
	{
	case eAppletPeerSessionFileOffer:
		m_MyApp.getAppSettings().setLastFileOfferFilter( fileFilter );
		break;

	case eAppletFileShareClientView:
		m_MyApp.getAppSettings().setLastFileShareViewFilter( fileFilter );
		break;

	case eAppletLibrary:
		m_MyApp.getAppSettings().setLastLibraryFilter( fileFilter );
		break;

	case eActivityBrowseFiles:
	case eAppletBrowseFiles:
	default:
		m_MyApp.getAppSettings().setLastBrowseFilter( fileFilter );
	}
}

//============================================================================
EFileFilterType ActivityBase::getAppletFileFilter( EApplet applet )
{
	switch( applet )
	{
	case eAppletPeerSessionFileOffer:
		return m_MyApp.getAppSettings().getLastFileOfferFilter();

	case eAppletFileShareClientView:
		return m_MyApp.getAppSettings().getLastFileShareViewFilter();

	case eAppletLibrary:
		return m_MyApp.getAppSettings().getLastLibraryFilter();

	case eActivityBrowseFiles:
	case eAppletBrowseFiles:
	default:
		return m_MyApp.getAppSettings().getLastBrowseFilter();
	}
}

//============================================================================
void ActivityBase::setAppletFolder( EApplet applet, EFileFilterType fileFilter, std::string folder )
{
	if( folder.empty() )
	{
		LogMsg( LOG_ERROR, "%s attempted set empty folder", __func__ );
		return;
	}

	m_MyApp.getAppSettings().setLastBrowseDir( fileFilter, folder );
}

//============================================================================
std::string ActivityBase::getAppletFolder( EApplet applet, EFileFilterType fileFilter )
{
	std::string folder;
	m_MyApp.getAppSettings().getLastBrowseDir( fileFilter, folder );
	if( folder.empty() )
	{
		folder = getDefaultFolder( fileFilter );
	}

	if( folder.empty() )
	{
		LogMsg( LOG_ERROR, "%s folder could not be determined for %d", __func__, fileFilter  );
	}

	return folder;
}

//========================================================================
std::string ActivityBase::getDefaultFolder( EFileFilterType fileFilter )
{
	std::string defaultDir;

	switch( fileFilter )
	{
	case eFileFilterPhoto:
    case eFileFilterPhotoOnly:
		{

			QStringList paths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
			if( !paths.isEmpty() )
			{
				QString picturesLocation = paths[0];

				defaultDir = picturesLocation.toStdString();
				if( 0 != defaultDir.length() )
				{
					VxFileUtil::makeForwardSlashPath( defaultDir );
					defaultDir += "/";
				}
			}
		}
    	
    	break;
    	
	case eFileFilterAudio:
    case eFileFilterAudioOnly:
		{
			QStringList paths = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
			if( !paths.isEmpty() )
			{
				QString musicLocation = paths[0];
				defaultDir = musicLocation.toStdString();
				if( !defaultDir.empty() )
				{
					VxFileUtil::makeForwardSlashPath( defaultDir );
					defaultDir += "/";
				}
			}
		}
    	
    	break;
    	
	case eFileFilterVideo:
    case eFileFilterVideoOnly:
		{
			QStringList paths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
			if( !paths.isEmpty() )
			{
				QString moviesLocation = paths[0];

				defaultDir = moviesLocation.toStdString();
				if(  !defaultDir.empty() )
				{
					VxFileUtil::makeForwardSlashPath( defaultDir );
					defaultDir += "/";
				}
			}
		}

		break;

	case eFileFilterDocuments:
		{
			QStringList paths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
			if( !paths.isEmpty() )
			{
				QString docsLocation = paths[0];

				defaultDir = docsLocation.toStdString();
				if(  !defaultDir.empty() )
				{
					VxFileUtil::makeForwardSlashPath( defaultDir );
					defaultDir += "/";
				}
			}
		}
    	
    	break;
    	
    default:
		break;
	}
	
	if( defaultDir.empty() || !VxFileUtil::directoryExists( defaultDir.c_str() ) )
	{
		defaultDir = VxGetDownloadsDirectory();
	}
	
	return defaultDir;
}

//============================================================================
void ActivityBase::wantActivityCallbacks( bool enable )
{
	if( enable != m_ActivityCallbacksRequested )
	{
		m_ActivityCallbacksRequested = enable;
		m_MyApp.wantToGuiActivityCallbacks( this, enable );
	}	
}
