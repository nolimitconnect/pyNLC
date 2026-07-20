/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceManager.h"

#include "ContextMenuManager.h"
#include "DatabaseManager.h"
#include "PlayListPlayer.h"

#include "cores/DataCacheCore.h"

#include "cores/playercorefactory/PlayerCoreFactory.h"

#include "utils/FileExtensionProvider.h"

#include "profiles/ProfileManager.h"

#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/log.h"


#include "windowing/nlc/WinSystemNlcContext.h"

using namespace KODI;

CServiceManager::CServiceManager()
{
}

CServiceManager::~CServiceManager()
{
    if( init_level > 2 )
        DeinitStageThree();
    if( init_level > 1 )
        DeinitStageTwo();
    if( init_level > 0 )
        DeinitStageOne();
}

bool CServiceManager::InitForTesting()
{
    //m_databaseManager.reset( new CDatabaseManager );

    m_fileExtensionProvider.reset(new CFileExtensionProvider());

    init_level = 1;
    return true;
}

void CServiceManager::DeinitTesting()
{
    init_level = 0;
    m_fileExtensionProvider.reset();

    //m_databaseManager.reset();

}

bool CServiceManager::InitStageOne()
{
    CWinSystemNlcContext::Register();
    //m_playlistPlayer.reset( new PLAYLIST::CPlayListPlayer() );

    init_level = 1;
    return true;
}

bool CServiceManager::InitStageTwo()
{
    // Initialize the addon database (must be before the addon manager is init'd)
    //m_databaseManager.reset( new CDatabaseManager );

    m_Platform.reset( CPlatform::CreateInstance() );
    m_Platform->Init();

    m_dataCacheCore.reset( new CDataCacheCore() );

    //m_inputManager.reset(new CInputManager());
    //m_inputManager->InitializeInputs();


    m_fileExtensionProvider.reset(new CFileExtensionProvider());

    m_mediaManager.reset(new CMediaManager());
    m_mediaManager->Initialize();


    init_level = 2;
    return true;
}

// stage 3 is called after successful initialization of WindowManager
bool CServiceManager::InitStageThree( const std::shared_ptr<CProfileManager>& profileManager )
{
    m_playerCoreFactory.reset( new CPlayerCoreFactory( *profileManager ) );

    init_level = 3;
    return true;
}

void CServiceManager::DeinitStageThree()
{
    init_level = 2;

    m_playerCoreFactory.reset();

}

void CServiceManager::DeinitStageTwo()
{
    init_level = 1;

    m_fileExtensionProvider.reset();

    //m_inputManager.reset();

    m_contextMenuManager.reset();

    m_dataCacheCore.reset();

    m_mediaManager->Stop();
    m_mediaManager.reset();

    m_Platform.reset();
}

void CServiceManager::DeinitStageOne()
{
    init_level = 0;

    m_playlistPlayer.reset();
}

CContextMenuManager& CServiceManager::GetContextMenuManager()
{
    return *m_contextMenuManager;
}

CDataCacheCore& CServiceManager::GetDataCacheCore()
{
    return *m_dataCacheCore;
}

CPlatform& CServiceManager::GetPlatform()
{
    return *m_Platform;
}

PLAYLIST::CPlayListPlayer& CServiceManager::GetPlaylistPlayer()
{
    return *m_playlistPlayer;
}

//CInputManager& CServiceManager::GetInputManager()
//{
//    return *m_inputManager;
//}

CFileExtensionProvider& CServiceManager::GetFileExtensionProvider()
{
    return *m_fileExtensionProvider;
}


// deleters for unique_ptr
void CServiceManager::delete_dataCacheCore::operator()( CDataCacheCore *p ) const
{
    delete p;
}

void CServiceManager::delete_contextMenuManager::operator()( CContextMenuManager *p ) const
{
#if HAVE_ADDONS
    delete p;
#endif // HAVE_ADDONS
}


CPlayerCoreFactory &CServiceManager::GetPlayerCoreFactory()
{
    return *m_playerCoreFactory;
}


CMediaManager& CServiceManager::GetMediaManager()
{
  return *m_mediaManager;
}
