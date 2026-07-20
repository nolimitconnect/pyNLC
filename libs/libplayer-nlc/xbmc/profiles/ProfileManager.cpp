/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProfileManager.h"
#include "NlcCoreUtil.h"

#include "DatabaseManager.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "config_components_kodi.h"

#include "ServiceBroker.h"

#include "Application.h" 
#include "ApplicationComponents.h"

#include "events/EventLog.h"
#include "events/EventLogManager.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"

#include "music/MusicLibraryQueue.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "threads/SingleLock.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>
#if !defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
#include "storage/DetectDVDType.h"
#endif
#include "ContextMenuManager.h" //! @todo Remove me
#include "PlayListPlayer.h" //! @todo Remove me
 //! @todo Remove me

#include "guilib/StereoscopicsManager.h" //! @todo Remove me

 //! @todo Remove me
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoLibraryQueue.h"//! @todo Remove me
 //! @todo Remove me

//! @todo
//! eventually the profile should dictate where special://masterprofile/ is
//! but for now it makes sense to leave all the profile settings in a user
//! writeable location like special://masterprofile/
#define PROFILES_FILE     "special://masterprofile/profiles.xml"

#define XML_PROFILES      "profiles"
#define XML_AUTO_LOGIN    "autologin"
#define XML_LAST_LOADED   "lastloaded"
#define XML_LOGIN_SCREEN  "useloginscreen"
#define XML_NEXTID        "nextIdProfile"
#define XML_PROFILE       "profile"

using namespace XFILE;

static CProfile EmptyProfile;

CProfileManager::CProfileManager() : m_eventLogs(new CEventLogManager)
{
}

CProfileManager::~CProfileManager()
{
}

void CProfileManager::Initialize(const std::shared_ptr<CSettings>& settings)
{
  m_settings = settings;

  if (m_settings->IsLoaded())
    OnSettingsLoaded();

  m_settings->GetSettingsManager()->RegisterSettingsHandler(this);

  std::set<std::string> settingSet = {
    CSettings::SETTING_EVENTLOG_SHOW
  };

  m_settings->GetSettingsManager()->RegisterCallback(this, settingSet);
}

void CProfileManager::Uninitialize()
{
  m_settings->GetSettingsManager()->UnregisterCallback(this);
  m_settings->GetSettingsManager()->UnregisterSettingsHandler(this);
}

void CProfileManager::OnSettingsLoaded()
{
  // check them all
  std::string strDir = m_settings->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
  if (strDir == "set default" || strDir.empty())
  {
    strDir = "special://profile/playlists/";
    m_settings->SetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH, strDir.c_str());
  }

  CDirectory::Create(strDir);
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"music"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"video"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"mixed"));
}

void CProfileManager::OnSettingsSaved() const
{
  // save mastercode
  Save();
}

void CProfileManager::OnSettingsCleared()
{
  Clear();
}

bool CProfileManager::Load()
{
  bool ret = true;
  const std::string file = PROFILES_FILE;

  std::unique_lock<CCriticalSection> lock(m_critical);

  // clear out our profiles
  m_profiles.clear();

  if (CFile::Exists(file))
  {
    CXBMCTinyXML profilesDoc;
    if (profilesDoc.LoadFile(file))
    {
      const TiXmlElement *rootElement = profilesDoc.RootElement();
      if (rootElement && StringUtils::EqualsNoCase(rootElement->Value(), XML_PROFILES))
      {
        XMLUtils::GetUInt(rootElement, XML_LAST_LOADED, m_lastUsedProfile);
        XMLUtils::GetBoolean(rootElement, XML_LOGIN_SCREEN, m_usingLoginScreen);
        XMLUtils::GetInt(rootElement, XML_AUTO_LOGIN, m_autoLoginProfile);
        XMLUtils::GetInt(rootElement, XML_NEXTID, m_nextProfileId);
        
        std::string defaultDir("special://home/userdata");
        if (!CDirectory::Exists(defaultDir))
          defaultDir = "special://xbmc/userdata";
        
        const TiXmlElement* pProfile = rootElement->FirstChildElement(XML_PROFILE);
        while (pProfile)
        {
          CProfile profile(defaultDir);
          profile.Load(pProfile, GetNextProfileId());
          AddProfile(profile);

          pProfile = pProfile->NextSiblingElement(XML_PROFILE);
        }
      }
      else
      {
        CLog::Log(LOGERROR, "CProfileManager: error loading %s, no <profiles> node", file.c_str());
        ret = false;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CProfileManager: error loading %s, Line %d\n%s", file.c_str(), profilesDoc.ErrorRow(), profilesDoc.ErrorDesc());
      ret = false;
    }
  }
  if (!ret)
  {
    CLog::Log(LOGERROR,
              "Failed to load profile - might be corrupted - falling back to master profile");
    m_profiles.clear();
    CFile::Delete(file);

    ret = true;
  }

  if (m_profiles.empty())
  { // add the master user
    CProfile profile("special://masterprofile/", "Master user", 0);
    AddProfile(profile);
  }

  // check the validity of the previous profile index
  if (m_lastUsedProfile >= m_profiles.size())
    m_lastUsedProfile = 0;

  SetCurrentProfileId(m_lastUsedProfile);

  // check the validity of the auto login profile index
  if (m_autoLoginProfile < -1 || m_autoLoginProfile >= (int)m_profiles.size())
    m_autoLoginProfile = -1;
  else if (m_autoLoginProfile >= 0)
    SetCurrentProfileId(m_autoLoginProfile);

  // the login screen runs as the master profile, so if we're using this, we need to ensure
  // we switch to the master profile
  if (m_usingLoginScreen)
    SetCurrentProfileId(0);

  return ret;
}

bool CProfileManager::Save() const
{
  const std::string file = PROFILES_FILE;

  std::unique_lock<CCriticalSection> lock(m_critical);

  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement(XML_PROFILES);
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (pRoot == nullptr)
    return false;

  XMLUtils::SetInt(pRoot, XML_LAST_LOADED, m_currentProfile);
  XMLUtils::SetBoolean(pRoot, XML_LOGIN_SCREEN, m_usingLoginScreen);
  XMLUtils::SetInt(pRoot, XML_AUTO_LOGIN, m_autoLoginProfile);
  XMLUtils::SetInt(pRoot, XML_NEXTID, m_nextProfileId);      

  for (const auto& profile : m_profiles)
    profile.Save(pRoot);

  // save the file
  return xmlDoc.SaveFile(file);
}

void CProfileManager::Clear()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  m_usingLoginScreen = false;
  m_profileLoadedForLogin = false;
  m_previousProfileLoadedForLogin = false;
  m_lastUsedProfile = 0;
  m_nextProfileId = 0;
  SetCurrentProfileId(0);
  m_profiles.clear();
}

void CProfileManager::PrepareLoadProfile(unsigned int profileIndex)
{

}

bool CProfileManager::LoadProfile(unsigned int index)
{
  PrepareLoadProfile(index);

  if (index == 0 && IsMasterProfile())
  {
    //CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_HOME);
    //if (pWindow)
    //  pWindow->ResetControlStates();

    UpdateCurrentProfileDate();
    FinalizeLoadProfile();

    return true;
  }

  std::unique_lock<CCriticalSection> lock(m_critical);
  // check if the index is valid or not
  if (index >= m_profiles.size())
    return false;

  // check if the profile is already active
  if (m_currentProfile == index)
    return true;

  // @todo: why is m_settings not used here?
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  // unload any old settings
  settings->Unload();

  SetCurrentProfileId(index);
  m_previousProfileLoadedForLogin = false;

  // load the new settings
  if (!settings->Load())
  {
    CLog::Log(LOGFATAL, "CProfileManager: unable to load settings for profile \"%s\"", m_profiles.at(index).getName().c_str());
    return false;
  }
  settings->SetLoaded();

  CreateProfileFolders();

  //CServiceBroker::GetDatabaseManager().Initialize();
  //CServiceBroker::GetInputManager().LoadKeymaps();

  //CServiceBroker::GetInputManager().SetMouseEnabled(settings->GetBool(CSettings::SETTING_INPUT_ENABLEMOUSE));

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
  {
    //CGUIInfoManager& infoMgr = gui->GetInfoManager();
    //infoMgr.ResetCache();
    //infoMgr.GetInfoProviders().GetGUIControlsInfoProvider().ResetContainerMovingCache();
    //infoMgr.GetInfoProviders().GetLibraryInfoProvider().ResetLibraryBools();
  }

  //if (m_currentProfile != 0)
  //{
  //  CXBMCTinyXML doc;
  //  if (doc.LoadFile(URIUtils::AddFileToFolder(GetUserDataFolder(), "guisettings.xml")))
  //  {
  //    settings->LoadSetting(doc.RootElement(), CSettings::SETTING_MASTERLOCK_MAXRETRIES);
  //    settings->LoadSetting(doc.RootElement(), CSettings::SETTING_MASTERLOCK_STARTUPLOCK);
  //  }
  //}

  // init windows
  //CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESET);
  //CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  CUtil::DeleteDirectoryCache();
  g_directoryCache.Clear();

  lock.unlock();

  UpdateCurrentProfileDate();
  FinalizeLoadProfile();

  m_profileLoadedForLogin = false;

  return true;
}

void CProfileManager::FinalizeLoadProfile()
{

  PLAYLIST::CPlayListPlayer &playlistManager = CServiceBroker::GetPlaylistPlayer();
  CStereoscopicsManager &stereoscopicsManager = CServiceBroker::GetGUI()->GetStereoscopicsManager();

  if (m_lastUsedProfile != m_currentProfile)
  {
    playlistManager.ClearPlaylist(PLAYLIST::TYPE_VIDEO);
    playlistManager.ClearPlaylist(PLAYLIST::TYPE_MUSIC);
    playlistManager.SetCurrentPlaylist(PLAYLIST::TYPE_NONE);
  }



  // let CApplication know that we are logging into a new profile
  g_application.SetLoggingIn(true);

  if (!g_application.LoadLanguage(true))
  {
    CLog::Log(LOGFATAL, "Unable to load language for profile \"%s\"", GetCurrentProfile().getName().c_str());
    return;
  }


  stereoscopicsManager.Initialize();



  //the user interfaces has been fully initialized, let everyone know
  //CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_SETTINGS_PROFILES, 0, GUI_MSG_UI_READY);
  //  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CProfileManager::LogOff()
{
  g_application.StopPlaying();

  LoadMasterProfileForLogin();

}

bool CProfileManager::DeleteProfile(unsigned int index)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  const CProfile *profile = GetProfile(index);
  if (profile == NULL)
    return false;



  // fall back to master profile if necessary
  if ((int)index == m_autoLoginProfile)
    m_autoLoginProfile = 0;

  // delete profile
  std::string strDirectory = profile->getDirectory();
  m_profiles.erase(m_profiles.begin() + index);

  // fall back to master profile if necessary
  if (index == m_currentProfile)
  {
    LoadProfile(0);
    m_settings->Save();
  }

  CFileItemPtr item = CFileItemPtr(new CFileItem(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory)));
  item->SetPath(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory + "/"));
  item->m_bIsFolder = true;
  item->Select(true);

  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui && gui->ConfirmDelete(item->GetPath()))
  CFileUtils::DeleteItem(item);

  return Save();
}

void CProfileManager::CreateProfileFolders()
{
    // no setting files needed
    return;
  CDirectory::Create(GetDatabaseFolder());
  CDirectory::Create(GetCDDBFolder());
  CDirectory::Create(GetLibraryFolder());

  // create Thumbnails/*
  CDirectory::Create(GetThumbnailsFolder());
  CDirectory::Create(GetVideoThumbFolder());
  CDirectory::Create(GetBookmarksThumbFolder());
  CDirectory::Create(GetSavestatesFolder());
  for (size_t hex = 0; hex < 16; hex++)
    CDirectory::Create(URIUtils::AddFileToFolder(GetThumbnailsFolder(), StringUtils::Format("%lx", hex)));

  // no addons or keyboard
  //CDirectory::Create("special://profile/addon_data");
  //CDirectory::Create("special://profile/keymaps");
}

const CProfile& CProfileManager::GetMasterProfile() const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (!m_profiles.empty())
    return m_profiles[0];

  CLog::Log(LOGERROR, "%s: master profile doesn't exist", __FUNCTION__);
  return EmptyProfile;
}

const CProfile& CProfileManager::GetCurrentProfile() const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (m_currentProfile < m_profiles.size())
    return m_profiles[m_currentProfile];

  CLog::Log(LOGERROR, "CProfileManager: current profile index ({0}) is outside of the valid range ({1})", m_currentProfile, m_profiles.size());
  return EmptyProfile;
}

const CProfile* CProfileManager::GetProfile(unsigned int index) const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (index < m_profiles.size())
    return &m_profiles[index];

  return NULL;
}

CProfile* CProfileManager::GetProfile(unsigned int index)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (index < m_profiles.size())
    return &m_profiles[index];

  return NULL;
}

int CProfileManager::GetProfileIndex(const std::string &name) const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  for (int i = 0; i < static_cast<int>(m_profiles.size()); i++)
  {
    if (StringUtils::EqualsNoCase(m_profiles[i].getName(), name))
      return i;
  }

  return -1;
}

void CProfileManager::AddProfile(const CProfile &profile)
{
  {
    std::unique_lock<CCriticalSection> lock(m_critical);
  // data integrity check - covers off migration from old profiles.xml,
  // incrementing of the m_nextIdProfile,and bad data coming in
  m_nextProfileId = std::max(m_nextProfileId, profile.getId() + 1);

  m_profiles.push_back(profile);
  }
  Save();
}

void CProfileManager::UpdateCurrentProfileDate()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (m_currentProfile < m_profiles.size())
  {
    m_profiles[m_currentProfile].setDate();
    CSingleExit exit(m_critical);
    Save();
  }
}

void CProfileManager::LoadMasterProfileForLogin()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  // save the previous user
  m_lastUsedProfile = m_currentProfile;
  if (m_currentProfile != 0)
  {
    // determines that the (master) profile has only been loaded for login
    m_profileLoadedForLogin = true;

    LoadProfile(0);

    // remember that the (master) profile has only been loaded for login
    m_previousProfileLoadedForLogin = true;
  }
}

bool CProfileManager::GetProfileName(const unsigned int profileId, std::string& name) const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  const CProfile *profile = GetProfile(profileId);
  if (!profile)
    return false;

  name = profile->getName();
  return true;
}

std::string CProfileManager::GetUserDataFolder() const
{
  return GetMasterProfile().getDirectory();
}

std::string CProfileManager::GetProfileUserDataFolder() const
{
  if (m_currentProfile == 0)
    return GetUserDataFolder();

  return URIUtils::AddFileToFolder(GetUserDataFolder(), GetCurrentProfile().getDirectory());
}

std::string CProfileManager::GetDatabaseFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Database");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Database");
}

std::string CProfileManager::GetCDDBFolder() const
{
  return URIUtils::AddFileToFolder(GetDatabaseFolder(), "CDDB");
}

std::string CProfileManager::GetThumbnailsFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails");
}

std::string CProfileManager::GetVideoThumbFolder() const
{
  return URIUtils::AddFileToFolder(GetThumbnailsFolder(), "Video");
}

std::string CProfileManager::GetBookmarksThumbFolder() const
{
  return URIUtils::AddFileToFolder(GetVideoThumbFolder(), "Bookmarks");
}

std::string CProfileManager::GetLibraryFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "library");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "library");
}

std::string CProfileManager::GetSavestatesFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Savestates");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Savestates");
}

//std::string CProfileManager::GetSettingsFile() const
//{
//  if (m_currentProfile == 0)
//  {
//#if defined(HAVE_NLC_GUI)
//    // user cannot change the settings so use the read only settings in assets
//    return "special://xbmcbin/userdata/guisettings.xml";
//#else
//    return "special://masterprofile/guisettings.xml";
//#endif // defined(HAVE_NLC_GUI)
//  }
//
//  return "special://profile/guisettings.xml";
//}

std::string CProfileManager::GetUserDataItem(const std::string& strFile) const
{
  std::string path;
  path = "special://profile/" + strFile;

  // check if item exists in the profile (either for folder or
  // for a file (depending on slashAtEnd of strFile) otherwise
  // return path to masterprofile
  if ((URIUtils::HasSlashAtEnd(path) && !CDirectory::Exists(path)) || !CFile::Exists(path))
    path = "special://masterprofile/" + strFile;

  return path;
}

CEventLog& CProfileManager::GetEventLog()
{
  return m_eventLogs->GetEventLog(GetCurrentProfileId());
}

void CProfileManager::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_EVENTLOG_SHOW)
    GetEventLog().ShowFullEventLog();
}

void CProfileManager::SetCurrentProfileId(unsigned int profileId)
{
  {
    std::unique_lock<CCriticalSection> lock(m_critical);
  m_currentProfile = profileId;
  CSpecialProtocol::SetProfilePath(GetProfileUserDataFolder());
  }
  //Save();
}
