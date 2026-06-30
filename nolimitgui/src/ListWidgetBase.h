#pragma once
//============================================================================
// Copyright (C) 2021 Brett R. Jones 
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

#include <GuiInterface/IDefs.h>

#include <CoreLib/VxElapseTimer.h>
#include <CoreLib/VxGUID.h>

#include <QListWidget>

class AppCommon;
class GuiUser;
class GuiUserSessionBase;
class GuiUserMgr;
class GuiThumb;
class GuiThumbMgr;
class MyIcons;
class P2PEngine;

class ListWidgetBase : public QListWidget
{
    Q_OBJECT

public:
    ListWidgetBase( QWidget* parent );

    AppCommon&                  getMyApp( void )                    { return m_MyApp; }
    MyIcons&                    getMyIcons( void )                  { return m_MyIcons; }
    GuiUserMgr&                 getUserMgr( void )                  { return m_UserMgr; }
    P2PEngine&                  getEngine( void )                   { return m_Engine; }
    GuiThumbMgr&                getThumbMgr( void )                 { return m_ThumbMgr; }

    void                        setIsHostView( bool isHost )        { m_IsHostView = isHost; }
    bool                        getIsHostView( void )               { return m_IsHostView; }
    void                        setHostType( EHostType hostType )   { m_HostType = hostType; }
    EHostType                   getHostType( void )                 { return m_HostType; }

    void                        launchChangeFriendship( GuiUser* ident );

    void                        setIsIgnoredHostsView( bool isIgnored ) { m_IsIgnoredHostsView = isIgnored; }
    bool                        getIsIgnoredHostsView( void )       { return m_IsIgnoredHostsView; }

protected:
    //=== vars ===//
    AppCommon&                  m_MyApp;
    MyIcons&                    m_MyIcons;
    GuiUserMgr&                 m_UserMgr;
    P2PEngine&                  m_Engine;
    GuiThumbMgr&                m_ThumbMgr;
    bool                        m_IsHostView{ false };
    bool                        m_IsIgnoredHostsView{ false };
    EHostType                   m_HostType{ eHostTypeUnknown };

    VxElapseTimer						m_ClickEventTimer; // avoid duplicate clicks
    VxElapseTimer						m_ListItemClickEventTimer; // avoid duplicate clicks
};

