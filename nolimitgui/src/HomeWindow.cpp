//============================================================================
// Copyright (C) 2023 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "HomeWindow.h"

#include "AppCommon.h"
#include "AppSettings.h"
#include "VxAppTheme.h"
#include "VxAppDisplay.h"

#include "AppletBase.h"
#include "AppletLaunchPage.h"
#include "AppletMultiMessenger.h"
#include "MessengerPage.h"
#include "GuiParams.h"
#include "GuiHelpers.h"
#include "TitleBarWidget.h"
#include "VxFrame.h"
#include "VxPushButton.h"

#include <CoreLib/ObjectCommonDefs.h>
#include <CoreLib/VxAppInfo.h>
#include <CoreLib/VxDebug.h>

#include <GuiInterface/ICamCapture.h>

#include <QScreen>
#include <QtWidgets>

//============================================================================
HomeWindow::HomeWindow( AppCommon&	appCommon, QString title )
: QDialog( )
, m_MyApp( appCommon )
, m_AppDisplay( m_MyApp.getAppDisplay() )
, m_AppTitle( title )
, m_WindowSettings( 0 )
, m_LastHomeLayout( eHomeLayoutPlayerPlusMessenger )
, m_MainLayout( 0 )
, m_HomeFrameUpperLeft( new VxFrame( appCommon, this ) )
, m_HomeFrameRight( new VxFrame( appCommon, this ) )
, m_HomeFrameBottom( new VxFrame( appCommon, this ) )
{
    setObjectName( "HomeWindow" );
    m_HomeFrameUpperLeft->setObjectName( OBJNAME_FRAME_LAUNCH_PAGE );
    // messenger can be in right frame or bottom frame depending on layout orientation
    m_HomeFrameRight->setObjectName( OBJNAME_FRAME_MESSAGER_PAGE );
    m_HomeFrameBottom->setObjectName( OBJNAME_FRAME_MESSAGER_PAGE );

    VxAppInfo appInfo;
    m_WindowSettings = new QSettings( appInfo.getCompanyDomain(), appInfo.getAppNameNoSpaces(), this );
    setWindowFlags( windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint );
}

//============================================================================
void HomeWindow::accept()
{
    // user has clicked the X to exit application.. prompt if really wants to exit
    if( m_MyApp.confirmAppShutdown( this ) )
    {
        QDialog::reject();
    }
}

//============================================================================
void HomeWindow::reject()
{
#if defined(TARGET_OS_ANDROID)
    // this also gets called on android when android device back button is pressed
    AppletBase * curApplet = findActiveApplet();
    if(curApplet && (curApplet->getAppletType() != eAppletHomePage))
    {
        if( curApplet->getTitleBarWidget() )
        {
            curApplet->getTitleBarWidget()->slotBackButtonClicked();
        }
    }
    else
    {
        QDialog::reject();
    }
#else
    if( m_MyApp.confirmAppShutdown( m_HomeFrameUpperLeft ) )
    {
        GetAppInstance().shutdownAppCommon();
        QDialog::reject();
    }
#endif //defined(TARGET_OS_ANDROID)
}

//============================================================================
void HomeWindow::resizeEvent( QResizeEvent* ev)
{
    // LogModule( eLogWindowPositions, LOG_VERBOSE, "HomeWindow::resizeEvent w %d h %d", ev->size().width(), ev->size().height() );
	emit signalMainWindowResized();
}

//============================================================================
void HomeWindow::moveEvent( QMoveEvent * )
{
    // LogModule( eLogWindowPositions, LOG_VERBOSE, "HomeWindow::moveEvent" );
    emit signalMainWindowMoved();
}

//============================================================================
void HomeWindow::showEvent( QShowEvent* ev )
{
    QDialog::showEvent( ev );
    static bool firstShow{ true };
    if( firstShow )
    {
        firstShow = false;
        if( !GuiParams::requestAllDangerousPermissions() )
        {
            LogMsg( LOG_ERROR, "Not all permissions were granted" );
        }

#if !defined(TARGET_OS_ANDROID)
        ICamCapture::getICamCapture().startupCamCapture();
#endif // !defined(TARGET_OS_ANDROID)
    }
}

//============================================================================
void HomeWindow::paintEvent( QPaintEvent* ev )
{
     QDialog::paintEvent( ev );
}

//============================================================================
void HomeWindow::initializeHomePage()
{
    restoreHomeWindowGeometry();

	initializeNlcDynamicLayout();
	connect( &m_AppDisplay, SIGNAL(signalDeviceOrientationChanged(int)), this, SLOT(slotDeviceOrientationChanged(int)) );
    connect( this, SIGNAL(signalMainWindowResized()), &m_MyApp, SLOT(slotMainWindowResized()) );
    connect( this, SIGNAL(signalMainWindowMoved()), &m_MyApp, SLOT(slotMainWindowMoved()) );
    m_AppDisplay.initializeAppDisplay();
	m_AppDisplay.forceOrientationUpdate();
}

//============================================================================
void HomeWindow::restoreHomeWindowGeometry()
{
#if !defined(TARGET_OS_ANDROID)
    QByteArray restoreGeom = m_WindowSettings->value( "mainWindowGeometry" ).toByteArray();
    if( restoreGeom.isEmpty() )
    {
#if QT_VERSION > QT_VERSION_CHECK(6,0,0)
        const QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
            (availableGeometry.height() - height()) / 2);
#else
        const QRect availableGeometry = QApplication::desktop()->availableGeometry( this );
        resize( availableGeometry.width() / 3, availableGeometry.height() / 2 );
        move( ( availableGeometry.width() - width() ) / 2,
            ( availableGeometry.height() - height() ) / 2 );
#endif // QT_VERSION > QT_VERSION_CHECK(6,0,0)
    }
    else
    {
        restoreGeometry( restoreGeom );
    }
#else
    updateAndroidGeomety();
#endif // !defined(TARGET_OS_ANDROID)}
}

//============================================================================
void HomeWindow::saveHomeWindowGeometry()
{
#if !defined(TARGET_OS_ANDROID)
    if( !QWidget::isMaximized() && !QWidget::isMinimized() )
    {
        QByteArray saveGeom = saveGeometry();
        m_WindowSettings->setValue( "mainWindowGeometry", saveGeom );
    }

    //settings.setValue( "mainWindowState", saveState() );
#endif // !defined(TARGET_OS_ANDROID)
}

//============================================================================
void HomeWindow::initializeNlcDynamicLayout( void )
{
    const char* NLC_FRAME = "NlcFram";
#if !defined(TARGET_OS_ANDROID)
    setMinimumSize( (int)( 400.0f * GuiParams::getGuiScale() ), (int)( 400.0f * GuiParams::getGuiScale()     ) );
#endif // !defined(TARGET_OS_ANDROID)

	m_MainLayout = new QGridLayout();
	m_MainLayout->setSizeConstraint( QLayout::SetNoConstraint );
	m_MainLayout->setSpacing( 2 );
    // some devices do not handle touch well at edge of screen .. add small margin 
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    m_MainLayout->setContentsMargins(3,3,3,3);
#else
	m_MainLayout->setMargin( 3 );
#endif // QT_VERSION >= QT_VERSION_CHECK(6,0,0)

	// make a frame for each possible layout area and we will show/hide/ re-parent based on the layout type
	m_HomeFrameUpperLeft->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	m_HomeFrameUpperLeft->setFrameShape( QFrame::Shape::Box );
	m_HomeFrameUpperLeft->setLineWidth( 1 );
	m_HomeFrameUpperLeft->setProperty( NLC_FRAME, true );
	//m_HomeFrameUpperLeft->setFocusPolicy( Qt::FocusPolicy::TabFocus );
	m_HomeFrameUpperLeft->setProperty( "FrameFocus", true );
	m_MainLayout->addWidget( m_HomeFrameUpperLeft, 0, 0 );

	m_HomeFrameRight->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	m_HomeFrameRight->setFrameShape( QFrame::Shape::Box );
	m_HomeFrameRight->setLineWidth( 1 );
	m_HomeFrameRight->setProperty( NLC_FRAME, true );
	//m_HomeFrameRight->setFocusPolicy( Qt::FocusPolicy::TabFocus );
	m_MainLayout->addWidget( m_HomeFrameRight, 0, 1 );

	m_HomeFrameBottom->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	m_HomeFrameBottom->setFrameShape( QFrame::Shape::Box );
	m_HomeFrameBottom->setLineWidth( 1 );
	m_HomeFrameBottom->setProperty( NLC_FRAME, true );
	//m_HomeFrameBottom->setFocusPolicy( Qt::FocusPolicy::TabFocus );
	m_MainLayout->addWidget( m_HomeFrameBottom, 1, 0 );

	connect( m_HomeFrameUpperLeft,	SIGNAL(signalFrameResized()), this, SLOT(slotFrameResized()) );
	connect( m_HomeFrameRight,		SIGNAL(signalFrameResized()), this, SLOT(slotFrameResized()) );
	connect( m_HomeFrameBottom,		SIGNAL(signalFrameResized()), this, SLOT(slotFrameResized()) );
	setLayout( m_MainLayout );

    createAppletLaunchPage();
    createMessengerPage();
    m_HomeFrameRight->setVisible( false );
    m_HomeFrameBottom->setVisible( false );
}

//============================================================================
void HomeWindow::switchWindowFocus(  QWidget* appIconButton )
{
	if( ( false == m_HomeFrameUpperLeft->isVisible() )
		|| ( false == getMessengerParentFrame()->isVisible() ) )
	{
		// no window to switch to
		appIconButton->nextInFocusChain();
		return;
	}
	else
	{
		if( true == appIconButton->property( "FocusNext" ) )
		{
			//LogMsg( LOG_DEBUG, "Nlc Button %d focus next true\n", appIconButton );
			appIconButton->setProperty( "FocusNext", false );
			appIconButton->nextInFocusChain();
			appIconButton->clearFocus();
			if( true == m_HomeFrameUpperLeft->property( "FrameFocus" ) )
			{
				m_HomeFrameUpperLeft->setProperty( "FrameFocus", false );
				m_HomeFrameUpperLeft->clearFocus();
				getMessengerParentFrame()->setProperty( "FrameFocus", true );
				getMessengerParentFrame()->activateWindow();
				m_MessengerPage->getAppIconPushButton()->setFocus();
			}
			else
			{
				m_HomeFrameUpperLeft->setProperty( "FrameFocus", true );
				getMessengerParentFrame()->setProperty( "FrameFocus", false );
				getMessengerParentFrame()->clearFocus();
				m_HomeFrameUpperLeft->activateWindow();
				m_AppletLaunchPage->getAppIconPushButton()->setFocus();
			}
		}
		else
		{
			appIconButton->setProperty( "FocusNext", true );
			appIconButton->nextInFocusChain();
		}
	}
}

//============================================================================
QFrame* HomeWindow::getAppletFrame( EApplet applet )
{
	if( eAppletMessengerFrame == applet || eAppletMultiMessenger == applet )
	{
		return getMessengerParentFrame();
	}
	else
	{
		return m_HomeFrameUpperLeft;
	}
}

//============================================================================
QFrame* HomeWindow::getLaunchParentFrame( ELaunchFrame launchFrame )
{
	if( eLaunchFrameHome == launchFrame )
	{
        return m_HomeFrameUpperLeft;	
	}
	else
	{
		return getMessengerParentFrame();
	}
}

//============================================================================
QFrame* HomeWindow::getMessengerParentFrame( void )
{
	if( Qt::Vertical == m_Orientation )
	{
		return m_HomeFrameBottom;
	}
	else
	{
		return m_HomeFrameRight;
	}
}

//============================================================================
QFrame* HomeWindow::getLaunchPageFrame( void )
{ 
    return m_HomeFrameUpperLeft;
}

//============================================================================
void HomeWindow::slotDeviceOrientationChanged( int qtOrientation )
{
    m_Orientation = (Qt::Orientation)qtOrientation;
	if( 0 == m_MessengerPage )
	{
        LogMsg( LOG_ERROR, "m_MessengerPage is null" );
		return;
	}

    LogMsg( LOG_VERBOSE, "HomeWindow::%s %s", __func__, GuiParams::describeOrientation(m_Orientation).toUtf8().constData() );

    m_HomeFrameRight->setVisible( false );
    m_HomeFrameBottom->setVisible( false );

	if( Qt::Vertical == m_Orientation )
	{
		m_MessengerPage->setNewParent( m_HomeFrameBottom );
	}
	else
	{
		m_MessengerPage->setNewParent( m_HomeFrameRight );
	}

    // force back to normal 2 part not maximized config .. might be annoying for android users who constantly change device orientation
    setIsMaxScreenSize( false, false );
    setIsMaxScreenSize( true, false );

    updateAndroidGeomety();

    emit signalDeviceOrientationChanged( m_Orientation );
	emit signalMainWindowResized( );
}

//============================================================================
void HomeWindow::updateAndroidGeomety()
{
#if defined(TARGET_OS_ANDROID)
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->availableGeometry();
    int height = screenGeometry.height() - 20;
    int width = screenGeometry.width() - 20;
    resize(width, height);
    move( screenGeometry.left() + 10, screenGeometry.top() + 10 );
    LogMsg( LOG_VERBOSE, "Home Screen Size %d %d", width, height );
#endif // defined(TARGET_OS_ANDROID)
}

//============================================================================
void HomeWindow::setIsMaxScreenSize( bool isMessagerFrame, bool isFullSizeWindow )
{	
	if( isMessagerFrame )
	{
        if( isFullSizeWindow )
        {
            // full screen messenger
            m_HomeFrameFullSize = false;
            m_MessengerIsFullSize = true;
            m_HomeFrameUpperLeft->setVisible( false );
            getMessengerParentFrame()->setVisible( true );
        }
        else
        {
            m_HomeFrameFullSize = false;
            m_MessengerIsFullSize = false;
            m_HomeFrameUpperLeft->setVisible( true );
            getMessengerParentFrame()->setVisible( true );
        }
	}
	else
	{
        if( isFullSizeWindow )
        {
            // full screen applet page
            m_HomeFrameFullSize = true;
            m_MessengerIsFullSize = false;
            m_HomeFrameUpperLeft->setVisible( true );
            getMessengerParentFrame()->setVisible( false );
        }
        else
        {
            m_HomeFrameFullSize = false;
            m_MessengerIsFullSize = false;
            m_HomeFrameUpperLeft->setVisible( true );
            getMessengerParentFrame()->setVisible( true );
        }
	}

    emit signalMainWindowResized( );
}

//============================================================================
bool HomeWindow::getIsMaxScreenSize( bool isMessagerFrame )
{
    return isMessagerFrame ? m_MessengerIsFullSize : m_HomeFrameFullSize;
}

//============================================================================
void HomeWindow::help( void )
{
	//QMessageBox::information( this, tr( "Dynamic Layouts Help" ),
	//	tr( "This example shows how to change layouts "
	//		"dynamically." ) );
}

//============================================================================
void HomeWindow::createAppletLaunchPage( void )
{
	m_AppletLaunchPage = new AppletLaunchPage( getMyApp(), m_HomeFrameUpperLeft, eAppletHomePage, OBJNAME_APPLET_LAUNCH_PAGE );
	connect( m_AppletLaunchPage, SIGNAL(signalBackButtonClicked()), this, SLOT( help() ) );
}

//============================================================================
void HomeWindow::createMessengerPage( void )
{
	m_MessengerPage = new MessengerPage( getMyApp(), m_HomeFrameRight );
	connect( m_MessengerPage, SIGNAL(signalBackButtonClicked()), this, SLOT( help() ) );
    m_AppletMultiMessenger = new AppletMultiMessenger( getMyApp(), m_MessengerPage );
    m_MyApp.setAppletMultiMessenger( m_AppletMultiMessenger );
}

//============================================================================
void HomeWindow::slotFrameResized( void )
{
    emit signalMainWindowResized();
}

//============================================================================
void HomeWindow::closeEvent(QCloseEvent *event)
{
    saveHomeWindowGeometry();
	QDialog::closeEvent( event );
}

//============================================================================
AppletBase * HomeWindow::findActiveApplet( void )
{
    AppletBase * curApplet = nullptr;
    // get the widget with focus then find a parent that is a base applet
    QWidget* curWidget = QApplication::focusWidget();
    while(curWidget)
    {
        curApplet = dynamic_cast<AppletBase *>(curWidget);
        if( curApplet )
        {
            break;
        }

        curWidget = dynamic_cast<QWidget*>(curWidget->parent());
    }

    return curApplet;
}
