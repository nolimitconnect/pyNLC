
project(nolimitconnect,
        VERSION ${NLC_VERSION}
        LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_STATIC_LIB_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui LinguistTools Multimedia OpenGL Svg Widgets)

if(COMMAND qt_policy)
    qt_policy(SET QTP0002 NEW)
endif()

if(NOT DEFINED TARGET_OS_ANDROID)
    find_package(OpenGL REQUIRED)
endif()

set (_allTargets "")

macro(add_executable _target)
    _add_executable (${_target} ${ARGN})
    list (APPEND _allTargets ${_target})
endmacro()

if(MSVC)
    # 1. The Preprocessor approach (Most effective for CRT/Standard Library)
    # This stops the "This function or variable may be unsafe" warnings (e.g. strcpy)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_SCL_SECURE_NO_WARNINGS)

    # 2. The Compiler Flag approach (Broadest hammer)
    # /wd4996 tells the compiler to literally 'Warning Disable' 4996
    add_compile_options(/wd4996)
endif()

#=== SETUP EXECUTABLE ===#
message(STATUS "nolimitconnect gui project prefix ${CMAKE_PREFIX_PATH}")
# Add the forms directory to the AUOUIC search paths
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/resources/Forms
    )

# Add the executable

set(nlc_HEADERS)
list(APPEND nlc_HEADERS
    src/AccountMgr.h
    AppCompilerConfig.h
    AppCpuArchDefines.h
    AppDependLibrariesConfig.h
    AppTargetOsConfig.h

    src/AccountDb.h
    src/AccountMgr.h
    src/AcceptCancelWidget.h
    src/ActionWidget.h

    src/ActivityBrowseFiles.h
    src/ActivityDownloadItemMenu.h
    src/ActivityGenerateHash.h
    src/ActivityMessageBox.h
    src/ActivityMsgBoxOk.h
    src/ActivityMsgBoxYesNo.h
    src/ActivitySessionOptions.h
    src/ActivityShowHelp.h
    src/ActivityTimedMessage.h
    src/ActivityBase.h

    src/AdminAvailMgr.h
    src/AppCommon.h
    src/AppDefs.h
    src/AppGlobals.h
    src/AppSettings.h
    src/AppletAboutApp.h
    src/AppletAboutFile.h
    src/AppletAboutMeClient.h
    src/AppletAboutUser.h

    src/AppletAboutMeServerViewMine.h
    src/AppletApplicationInfo.h
    src/AppletAssetPlayerBase.h
    src/AppletBase.h
    src/AppletBrowseFiles.h
    src/AppletPlayerCamClip.h
    src/AppletCamClient.h
    src/AppletCamSettings.h
    src/AppletCamServerViewMine.h
    src/AppletChatRoomJoin.h
    src/AppletChatRoomClient.h
    src/AppletChatRoomHostAdmin.h
    src/AppletChatRoomJoinSearch.h
    src/AppletChatRoomListLocalView.h
    src/AppletChooseThumbnail.h
    src/AppletClientAvatarImage.h
    src/AppletClientBase.h
    src/AppletCreateAccount.h

    src/AppletRandomConnectClient.h
    src/AppletDownloads.h
    src/AppletEditAboutMe.h
    src/AppletEditAvatarImage.h
    src/AppletEditStoryboard.h
    src/AppletFileOfferSelect.h
    src/AppletFileShareClientView.h
    src/AppletFileShareServerViewMine.h
    src/AppletFriendList.h
    src/AppletFriendRequest.h
    src/AppletFriendRequestList.h

    src/AppletGalleryImage.h
    src/AppletGalleryEmoticon.h
    src/AppletGalleryThumb.h
    src/AppletGetStarted.h
    src/AppletGroupClient.h
    src/AppletGroupHostAdmin.h
    src/AppletGroupJoinSearch.h
    src/AppletGroupJoin.h
    src/AppletGroupListClient.h
    src/AppletGroupListLocalView.h
    src/AppletHackerList.h
    src/AppletHelpNetSignalBars.h

    src/AppletHostAdminBase.h
    src/AppletHostBase.h
    src/AppletHostChatRoomStatus.h
    src/AppletHostClientBase.h
    src/AppletHostGroupStatus.h
    src/AppletHostRandomConnectStatus.h
    src/AppletHostJoinChoose.h
    src/AppletHostJoinConnect.h
    src/AppletHostJoinRequestList.h
    src/AppletHostLeave.h
    src/AppletHostNetworkStatus.h
    src/AppletIgnoredHosts.h

    src/AppletInformation.h
    src/AppletInviteAccept.h
    src/AppletInviteCreate.h
    src/AppletIsPortOpenTest.h
    src/AppletJoinBase.h
    src/AppletLanguageSelect.h   
    src/AppletLaunchPage.h
    src/AppletLaunchWidget.h
    src/AppletLibrary.h
    src/AppletLog.h
    src/AppletLogSettings.h

    src/AppletMultiMessenger.h
    src/AppletMgr.h
    src/AppletNetHostingPage.h
    src/AppletNetworkSettings.h
    src/AppletOfferInfo.h
    src/AppletOfferList.h
    src/AppletOfferResponse.h
    src/AppletOfferResponseAccept.h
    src/AppletOfferSend.h
    src/AppletOfferView.h
    src/AppletPersonOfferList.h
    src/AppletPeerBase.h
    src/AppletPeerChangeFriendship.h
    src/AppletPeerReplyFileOffer.h
    src/AppletPeerSessionFileOffer.h
    src/AppletPeerTodGame.h
    src/AppletPeerVideoPhone.h
    src/AppletPeerVoicePhone.h
    src/AppletPermissionList.h
    src/AppletPersonalRecorder.h
    src/AppletPlayerBase.h
    src/AppletPlayerPhoto.h
    src/AppletPlayerVideo.h
    src/AppletPlayerNlc.h
    src/AppletPlayerNlcBase.h

    src/AppletOfferRandSession.h
    src/AppletPopupMenu.h
    src/AppletRandomConnectHostAdmin.h
    src/AppletRandomConnectJoin.h
    src/AppletRandomConnectJoinSearch.h
    src/AppletRandomConnectListLocalView.h
    src/AppletSearchPage.h
    src/AppletServiceAboutMe.h

    src/AppletServiceBase.h
    src/AppletServiceBaseSettings.h
    src/AppletServiceConnectionTest.h
    src/AppletServiceHostNetwork.h
    src/AppletServiceShareFiles.h
    src/AppletServiceShareWebCam.h
    src/AppletServiceStoryboard.h
    src/AppletSettingsAboutMe.h
    src/AppletSettingsAvatarImage.h
    src/AppletSettingsBase.h
    src/AppletSettingsConnectionTest.h
    src/AppletSettingsFileXfer.h
    src/AppletSettingsFriendRequest.h
    src/AppletSettingsHostBase.h
    src/AppletSettingsHostChatRoom.h
    src/AppletSettingsHostGroup.h
    src/AppletSettingsHostNetwork.h
    src/AppletSettingsHostRandomConnect.h
    src/AppletSettingsMessenger.h
    src/AppletSettingsPage.h
    src/AppletSettingsPushToTalk.h
    src/AppletSettingsRandomConnect.h
    src/AppletSettingsShareFiles.h
    src/AppletSettingsShareWebCam.h
    src/AppletSettingsStoryboard.h
    src/AppletSettingsTruthOrDare.h
    src/AppletSettingsVideoPhone.h
    src/AppletSettingsVoicePhone.h
    src/AppletShareServicesPage.h
    src/AppletSnapshot.h
    src/AppletSocketList.h
    src/AppletSoundSettings.h
    src/AppletStoryboardClient.h
    src/AppletStoryboardServerViewMine.h

    src/AppletTestAndDebug.h
    src/AppletTestUpnp.h

    src/AppletTheme.h
    src/AppletUploads.h
    src/AppletUserConnections.h
    src/AppletUserIdentity.h
    src/AppletUserPreferences.h
    src/AppTranslate.h
    src/AssetFaceWidget.h
    src/AssetPhotoWidget.h
    src/AssetSendMgr.h
    src/AssetTextWidget.h
    src/AssetVideoWidget.h
    src/AssetVoiceWidget.h
    src/AssetBaseWidget.h
    src/AvatarBar.h

    src/AudioInWaveformWidget.h
    src/AudioOutWaveformWidget.h

    src/BottomBarWidget.h
    src/CamTitleBarPreview.h

    src/ChatEntryWidget.h
    src/ClipboardCopyWidget.h
    src/ClipboardPasteWidget.h

    src/DialogAddComment.h
    src/DialogConfirmRemoveMessage.h
    src/EventsQtToNlc.h
    src/FileItemInfo.h
    src/FriendList.h
    src/FriendListDelegate.h
    src/FileActionBarWidget.h
    src/FileActionMenu.h
    src/FileFilterSelectWidget.h
    src/FileListReplySession.h
    src/FileMediaSelectWidget.h
    src/FileShareItemWidget.h
    src/FileXferWidget.h

    src/FriendListEntryWidget.h
    src/FriendListWidget.h

    src/GuiAudioMgr.h   
    src/GuiConnectIdListMgr.h
    src/GuiConnectIdListCallback.h
    src/GuiFileXferMgr.h
    src/GuiFileXferSession.h
    src/GuiFavoriteMgr.h

    src/GuiFriendRequest/GuiFriendRequest.h
    src/GuiFriendRequest/GuiFriendRequestCallback.h
    src/GuiFriendRequest/GuiFriendRequestListItem.h
    src/GuiFriendRequest/GuiFriendRequestListWidget.h
    src/GuiFriendRequest/GuiFriendRequestMgr.h

    src/GuiGroupie.h
    src/GuiGroupieListCallback.h
    src/GuiGroupieListItem.h
    src/GuiGroupieListMgr.h
    src/GuiGroupieListSession.h
    src/GuiGroupieListWidget.h
    src/GuiHelpers.h
    src/GuiHosted.h
    src/GuiHostedListCallback.h
    src/GuiHostedListItem.h
    src/GuiHostedListMgr.h
    src/GuiHostedListSession.h
    src/GuiHostedListWidget.h
    src/GuiHostJoin.h
    src/GuiHostJoinCallback.h
    src/GuiHostJoinList.h
    src/GuiHostedByMeJoinMgr.h
    src/GuiHostJoinSession.h
    src/GuiHostSession.h

    src/GuiMemberActiveCallback.h
    src/GuiMemberActiveMgr.h
    src/GuiOfferCallback.h
    src/GuiOfferInfo.h
    src/GuiOfferListItem.h
    src/GuiOfferListWidget.h
    src/GuiOfferMgr.h
    src/GuiOfferMgrBase.h
    src/GuiOfferSession.h
    src/GuiPushToTalkMgr.h
    src/GuiParams.h
    src/GuiPlayerCallback.h
    src/GuiPlayerMgr.h
    src/GuiPluginMgr.h
    src/GuiRandConnectCallback.h
    src/GuiRandConnectMgr.h
    src/GuiSendQueueCallback.h
    src/GuiSendQueueMgr.h

    src/GuiThumb.h
    src/GuiThumbList.h
    src/GuiThumbMgr.h

    src/GuiThreadAppLoader.h
    src/GuiThreadMainLoader.h
    src/GuiThreadSettingsLoader.h

    src/GuiUser.h
    src/GuiUserBase.h
    src/GuiUserJoin.h
    src/GuiUserJoinCallback.h
    src/GuiUserJoinList.h
    src/GuiUserJoinListItem.h
    src/GuiUserJoinListWidget.h
    src/GuiUserJoinMgr.h
    src/GuiUserJoinSession.h
    src/GuiUserListItem.h
    src/GuiUserListWidget.h
    src/GuiUserMgr.h
    src/GuiUserMultiListWidget.h
    src/GuiUserSessionBase.h
    src/GuiUserUpdateCallback.h
    src/GuiWebPageCallback.h
    src/GuiWebPageMgr.h
    src/HistoryEntryWidget.h
    src/HistoryListWidget.h
    src/HostedPluginWidget.h
    src/HostJoinRequestListItem.h
    src/HostJoinRequestListWidget.h
    src/HomeWindow.h
    src/IdentLogicInterface.h
    src/IdentOfferWidget.h
    src/IdentWidget.h
    src/InviteUrlWidget.h

    src/ImageListEntry.h
    src/ImagePickBar.h
    src/ImageListRow.h
    src/ImageListWidget.h
    src/InformationWidget.h
    src/InputAllWidget.h
    src/InputBaseWidget.h
    src/InputClientBaseCallback.h
    src/InputClientCallback.h
    src/InputFaceWidget.h
    src/InputPhotoWidget.h
    src/InputTextWidget.h
    src/InputVideoWidget.h
    src/InputVoiceWidget.h
    src/InstMsgWidget.h
    src/ListWidgetBase.h
    src/LogMgr.h
    src/LogoRenderer.h
    src/LogWidget.h
    src/MessengerPage.h
    src/MyIcons.h
    src/MyIconsDefs.h
    src/MultiSessionState.h
    src/NetAvailStatusWidget.h
    src/OfferBarWidget.h
    src/OfferSendWidget.h
    src/OfferSessionLogic.h
    src/PermissionWidget.h
    src/PermissionListItem.h
    src/PersonOfferListItem.h
    src/PersonOfferListWidget.h
    src/PhoneRinger.h
    src/PlayControlWidget.h
    src/PluginSettingsWidget.h
    src/RenderGlOffScreenSurface.h
    src/RenderGlLogic.h
    src/RenderGlShaders.h
    src/RenderGlWidget.h
    src/RenderLogoShaders.h
    src/RenderPlayerNlcThread.h
    src/RenderShaderQt.h
    src/RichTextWidget.h
    src/SearchBarWidget.h
    src/SearchParamsWidget.h
    src/ServiceSettingsWidget.h
    src/SessionWidget.h
    src/StoryWidget.h

    src/SoundDefs.h
    src/SoundFxMgr.h

    src/TestWidget.h
    src/TextChatLogic.h
    src/ThumbnailEditWidget.h
    src/ThumbnailChooseWidget.h
    src/ThumbnailViewWidget.h
    src/TodGameDefs.h
    src/TodGameLogic.h
    src/TodGameMgr.h
    src/TodGameWidget.h
    src/TodStatsWidget.h
    src/ToGuiActivityInterface.h
    src/ToGuiFileXferInterface.h
    src/ToGuiHardwareControlInterface.h
    src/ToGuiUserUpdateInterface.h
    src/ToGuiThumbUpdateInterface.h
    src/TitleBarWidget.h

    src/VxAspectFrame.h
    src/VxContextMenu.h
    src/VxComboBox.h
    src/VxAvatarImage.h
    src/VxPushButton.h

    src/VxLabel.h
    src/VxSndInstance.h
    src/VxFrame.h
    src/VxMenu.h
    src/VxMenuButton.h
    src/VxMenuButtonBottom.h
    src/VxMenuButtonTop.h
    src/VxProgressBar.h
    src/VxResourceToRealFile.h
    src/VxScrollArea.h
    src/VxSpinProgress.h
    src/VxTagLabel.h
    src/VxTilePositioner.h
    src/VidWidget.h
    src/VxWidgetBase.h
    src/VxAppStyle.h
    src/VxAppTheme.h
    src/VxAppDisplay.h
    src/VxPlainTextEdit.h
    src/WaitingSpinnerWidget.h
    src/WebsiteWidget.h

   )

set(nlc_SOURCES)
list(APPEND nlc_SOURCES
    NoLimitConnectMain/AppSource/main.cpp
    src/AccountMgr.cpp
    NoLimitConnectMain/AppSource/main.cpp
    src/AcceptCancelWidget.cpp
    src/AccountDb.cpp
    src/AccountMgr.cpp
    
    src/ActionWidget.cpp
    src/ActivityBase.cpp
    src/ActivityBrowseFiles.cpp
    src/ActivityDownloadItemMenu.cpp
    src/ActivityGenerateHash.cpp
    src/ActivityMessageBox.cpp
    src/ActivityMsgBoxOk.cpp
    src/ActivityMsgBoxYesNo.cpp
    src/ActivitySessionOptions.cpp
    src/ActivityShowHelp.cpp
    src/ActivityTimedMessage.cpp

    src/AdminAvailMgr.cpp
    src/AppCommon.cpp
    src/AppCommonAudio.cpp
    src/AppCommonCamCapture.cpp
    src/AppCommonEvents.cpp
    src/AppCommonFileMenu.cpp
    src/AppCommonFileShare.cpp
    src/AppCommonFriend.cpp
    src/AppCommonLogin.cpp
    src/AppCommonMedia.cpp
    src/AppCommonNotify.cpp
    src/AppCommonOfferToFriend.cpp
    src/AppCommonRender.cpp
    src/AppCommonSignalsAndSlots.cpp
    src/AppDefs.cpp
    src/AppGlobals.cpp
    src/AppImplementation.cpp
    src/AppImplementation.h
    src/AppModuleState.cpp
    src/AppSettings.cpp
    src/AppletAboutApp.cpp
    src/AppletAboutFile.cpp
    src/AppletAboutMeClient.cpp
    src/AppletAboutUser.cpp
    src/AppletAboutMeServerViewMine.cpp

    src/AppletApplicationInfo.cpp
    src/AppletAssetPlayerBase.cpp
    src/AppletBase.cpp
    src/AppletBrowseFiles.cpp
    src/AppletPlayerCamClip.cpp
    src/AppletCamClient.cpp
    src/AppletCamSettings.cpp
    src/AppletCamServerViewMine.cpp
    src/AppletChatRoomClient.cpp
    src/AppletChatRoomHostAdmin.cpp
    src/AppletChatRoomJoin.cpp
    src/AppletChatRoomJoinSearch.cpp
    src/AppletChatRoomListLocalView.cpp
    src/AppletChooseThumbnail.cpp
    src/AppletClientAvatarImage.cpp
    src/AppletClientBase.cpp
    src/AppletCreateAccount.cpp
    src/AppletRandomConnectClient.cpp
    src/AppletDownloads.cpp
    src/AppletEditAboutMe.cpp
    src/AppletEditAvatarImage.cpp
    src/AppletEditStoryboard.cpp

    src/AppletFileOfferSelect.cpp
    src/AppletFileShareClientView.cpp
    src/AppletFileShareServerViewMine.cpp
    src/AppletFriendList.cpp
    src/AppletFriendRequest.cpp
    src/AppletFriendRequestList.cpp

    src/AppletGalleryImage.cpp
    src/AppletGalleryEmoticon.cpp
    src/AppletGalleryThumb.cpp
    src/AppletGetStarted.cpp
    src/AppletGroupClient.cpp
    src/AppletGroupHostAdmin.cpp
    src/AppletGroupJoin.cpp
    src/AppletGroupJoinSearch.cpp
    src/AppletGroupListClient.cpp
    src/AppletGroupListLocalView.cpp
    src/AppletHackerList.cpp
    src/AppletHelpNetSignalBars.cpp

    src/AppletHostAdminBase.cpp
    src/AppletHostBase.cpp
    src/AppletHostChatRoomStatus.cpp
    src/AppletHostClientBase.cpp
    src/AppletHostGroupStatus.cpp
    src/AppletHostJoinChoose.cpp
    src/AppletHostJoinConnect.cpp
    src/AppletHostJoinRequestList.cpp
    src/AppletHostLeave.cpp

    src/AppletHostNetworkStatus.cpp
    src/AppletHostRandomConnectStatus.cpp
    src/AppletIgnoredHosts.cpp

    src/AppletInformation.cpp
    src/AppletInviteAccept.cpp
    src/AppletInviteCreate.cpp
    src/AppletIsPortOpenTest.cpp
    src/AppletJoinBase.cpp
    src/AppletLanguageSelect.cpp
    src/AppletLaunchPage.cpp
    src/AppletLaunchWidget.cpp
    src/AppletLibrary.cpp
    src/AppletLog.cpp
    src/AppletLogSettings.cpp

    src/AppletMgr.cpp
    src/AppletMultiMessenger.cpp
    src/AppletNetHostingPage.cpp
    src/AppletNetworkSettings.cpp

    src/AppletOfferInfo.cpp
    src/AppletOfferList.cpp
    src/AppletOfferRandSession.cpp
    src/AppletOfferResponse.cpp
    src/AppletOfferResponseAccept.cpp
    src/AppletOfferSend.cpp
    src/AppletOfferView.cpp
    src/AppletPersonOfferList.cpp
    src/AppletPeerBase.cpp
    src/AppletPeerChangeFriendship.cpp
    src/AppletPeerReplyFileOffer.cpp
    src/AppletPeerSessionFileOffer.cpp
    src/AppletPeerTodGame.cpp
    src/AppletPeerVideoPhone.cpp
    src/AppletPeerVoicePhone.cpp
    src/AppletPermissionList.cpp
    src/AppletPersonalRecorder.cpp
    src/AppletPlayerBase.cpp
    src/AppletPlayerPhoto.cpp
    src/AppletPlayerVideo.cpp
    src/AppletPlayerNlc.cpp
    src/AppletPlayerNlcBase.cpp

    src/AppletPopupMenu.cpp
    src/AppletRandomConnectHostAdmin.cpp
    src/AppletRandomConnectJoin.cpp
    src/AppletRandomConnectJoinSearch.cpp
    src/AppletRandomConnectListLocalView.cpp
    src/AppletSearchPage.cpp
    src/AppletServiceAboutMe.cpp

    src/AppletServiceBase.cpp
    src/AppletServiceBaseSettings.cpp
    src/AppletServiceConnectionTest.cpp
    src/AppletSettingsHostBase.cpp
    src/AppletServiceHostNetwork.cpp
    src/AppletServiceShareFiles.cpp
    src/AppletServiceShareWebCam.cpp
    src/AppletServiceStoryboard.cpp
    src/AppletSettingsAboutMe.cpp
    src/AppletSettingsAvatarImage.cpp
    src/AppletSettingsBase.cpp
    src/AppletSettingsConnectionTest.cpp
    src/AppletSettingsFileXfer.cpp
    src/AppletSettingsFriendRequest.cpp
    src/AppletSettingsHostChatRoom.cpp
    src/AppletSettingsHostGroup.cpp
    src/AppletSettingsHostNetwork.cpp
    src/AppletSettingsHostRandomConnect.cpp
    src/AppletSettingsMessenger.cpp
    src/AppletSettingsPage.cpp
    src/AppletSettingsPushToTalk.cpp
    src/AppletSettingsRandomConnect.cpp
    src/AppletSettingsShareFiles.cpp
    src/AppletSettingsShareWebCam.cpp
    src/AppletSettingsStoryboard.cpp
    src/AppletSettingsTruthOrDare.cpp
    src/AppletSettingsVideoPhone.cpp
    src/AppletSettingsVoicePhone.cpp
    src/AppletShareServicesPage.cpp
    src/AppletSnapshot.cpp
    src/AppletSocketList.cpp
    src/AppletSoundSettings.cpp
    src/AppletStoryboardClient.cpp
    src/AppletStoryboardServerViewMine.cpp

    src/AppletTestAndDebug.cpp
    src/AppletTestUpnp.cpp

    src/AppletTheme.cpp
    src/AppletUploads.cpp
    src/AppletUserConnections.cpp
    src/AppletUserIdentity.cpp
    src/AppletUserPreferences.cpp
    src/AppTranslate.cpp
    src/BottomBarWidget.cpp
    src/AssetBaseWidget.cpp
    src/AssetFaceWidget.cpp
    src/AssetPhotoWidget.cpp
    src/AssetSendMgr.cpp
    src/AssetTextWidget.cpp
    src/AssetVideoWidget.cpp
    src/AssetVoiceWidget.cpp
    src/AvatarBar.cpp

    src/AudioInWaveformWidget.cpp
    src/AudioOutWaveformWidget.cpp

    src/CamTitleBarPreview.cpp
    src/ChatEntryWidget.cpp
    src/ClipboardCopyWidget.cpp
    src/ClipboardPasteWidget.cpp
    src/DialogAddComment.cpp
    src/DialogConfirmRemoveMessage.cpp
    src/EventsQtToNlc.cpp
    src/FileActionBarWidget.cpp
    src/FileActionMenu.cpp
    src/FileFilterSelectWidget.cpp
    src/FileItemInfo.cpp
    src/FileListReplySession.cpp
    src/FileMediaSelectWidget.cpp
    src/FileShareItemWidget.cpp
    #src/FileViewSharedActionBar.cpp

    src/FileXferWidget.cpp
    src/FriendList.cpp
    src/FriendListDelegate.cpp
    src/FriendListWidget.cpp

    src/GuiAudioMgr.cpp
    src/GuiConnectIdListMgr.cpp
    src/GuiFileXferMgr.cpp
    src/GuiFileXferSession.cpp
    src/GuiFavoriteMgr.cpp

    src/GuiFriendRequest/GuiFriendRequest.cpp
    src/GuiFriendRequest/GuiFriendRequestListItem.cpp
    src/GuiFriendRequest/GuiFriendRequestListWidget.cpp
    src/GuiFriendRequest/GuiFriendRequestMgr.cpp

    src/GuiGroupie.cpp
    src/GuiGroupieListItem.cpp
    src/GuiGroupieListMgr.cpp
    src/GuiGroupieListSession.cpp
    src/GuiGroupieListWidget.cpp
    src/GuiHelpers.cpp
    src/GuiHelpersAndroidPermissions.cpp
    src/GuiHosted.cpp
    src/GuiHostedListItem.cpp
    src/GuiHostedListMgr.cpp
    src/GuiHostedListSession.cpp
    src/GuiHostedListWidget.cpp
    src/GuiHostJoin.cpp
    src/GuiHostJoinList.cpp
    src/GuiHostedByMeJoinMgr.cpp
    src/GuiHostJoinSession.cpp
    src/GuiHostSession.cpp

    src/GuiMemberActiveMgr.cpp
    src/GuiOfferInfo.cpp
    src/GuiOfferListItem.cpp
    src/GuiOfferListWidget.cpp
    src/GuiOfferMgr.cpp
    src/GuiOfferMgrBase.cpp
    src/GuiOfferMgrBaseCallbacks.cpp
    src/GuiOfferSession.cpp
    src/GuiParams.cpp
    src/GuiPlayerMgr.cpp
    src/GuiPluginMgr.cpp
    src/GuiPushToTalkMgr.cpp
    src/GuiRandConnectMgr.cpp
    src/GuiSendQueueMgr.cpp

    src/GuiThreadAppLoader.cpp
    src/GuiThreadMainLoader.cpp
    src/GuiThreadSettingsLoader.cpp

    src/GuiThumb.cpp
    src/GuiThumbList.cpp
    src/GuiThumbMgr.cpp
    src/GuiUser.cpp
    src/GuiUserBase.cpp
    src/GuiUserJoin.cpp
    src/GuiUserJoinList.cpp
    src/GuiUserJoinListItem.cpp
    src/GuiUserJoinListWidget.cpp
    src/GuiUserJoinMgr.cpp
    src/GuiUserJoinSession.cpp
    src/GuiUserListItem.cpp
    src/GuiUserListWidget.cpp
    src/GuiUserMgr.cpp
    src/GuiUserMultiListWidget.cpp
    src/GuiUserSessionBase.cpp
    src/GuiWebPageMgr.cpp
    src/HostedPluginWidget.cpp
    src/HostJoinRequestListItem.cpp
    src/HostJoinRequestListWidget.cpp
    src/HomeWindow.cpp
    src/HistoryEntryWidget.cpp
    src/HistoryListWidget.cpp
    src/ImageListEntry.cpp
    src/ImagePickBar.cpp
    src/ImageListRow.cpp
    src/ImageListWidget.cpp
    src/InformationWidget.cpp
    src/InputAllWidget.cpp
    src/InputBaseWidget.cpp
    src/InputClientBaseCallback.cpp
    src/InputFaceWidget.cpp
    src/InputPhotoWidget.cpp
    src/InputTextWidget.cpp
    src/InputVideoWidget.cpp
    src/InputVoiceWidget.cpp
    src/IdentLogicInterface.cpp
    src/IdentOfferWidget.cpp
    src/IdentWidget.cpp
    src/InstMsgWidget.cpp
    src/InviteUrlWidget.cpp

    src/FriendListEntryWidget.cpp
    src/ListWidgetBase.cpp
    src/LogMgr.cpp
    src/LogWidget.cpp
    src/LogoRenderer.cpp

    src/MessengerPage.cpp
    src/MultiSessionState.cpp
    src/MyIcons.cpp
    src/NetAvailStatusWidget.cpp
    src/OfferBarWidget.cpp
    src/OfferSendWidget.cpp
    src/OfferSessionLogic.cpp
    src/PermissionWidget.cpp
    src/PermissionListItem.cpp
    src/PersonOfferListItem.cpp
    src/PersonOfferListWidget.cpp
    src/PhoneRinger.cpp
    src/PlayControlWidget.cpp
    src/PluginSettingsWidget.cpp
    src/RenderGlLogic.cpp
    src/RenderGlMatrix.cpp
    src/RenderGlOffScreenSurface.cpp
    src/RenderGlWidget.cpp
    src/RenderGlWidgetFonts.cpp
    src/RenderGlWidgetRender.cpp
    src/RenderGlWidgetShaderInterface.cpp
    src/RenderGlWidgetTextures.cpp
    src/RenderGlShaders.cpp
    src/RenderLogoShaders.cpp
    src/RenderPlayerNlcThread.cpp
    src/RenderShaderDefsGl.cpp
    src/RenderShaderQt.cpp
    src/RichTextWidget.cpp
    src/SearchBarWidget.cpp
    src/SearchParamsWidget.cpp
    src/SessionWidget.cpp
    src/ServiceSettingsWidget.cpp
    src/SoundFxMgr.cpp
    src/StoryWidget.cpp

    src/TestWidget.cpp
    src/TextChatLogic.cpp
    src/TitleBarWidget.cpp
    src/TodGameDefs.cpp
    src/TodGameLogic.cpp
    src/TodGameMgr.cpp
    src/TodGameWidget.cpp
    src/TodStatsWidget.cpp
    src/ThumbnailEditWidget.cpp
    src/ThumbnailChooseWidget.cpp
    src/ThumbnailViewWidget.cpp

    src/VxAspectFrame.cpp
    src/VxComboBox.cpp
    src/VxContextMenu.cpp
    src/VxAvatarImage.cpp
    src/VxFrame.cpp
    src/VxMenuButton.cpp
    src/VxMenuButtonBottom.cpp
    src/VxMenuButtonTop.cpp
    src/VxProgressBar.cpp
    src/VxPushButton.cpp
    src/VxLabel.cpp
    src/VxSndInstance.cpp
    src/VxResourceToRealFile.cpp
    src/VxScrollArea.cpp
    src/VxSpinProgress.cpp
    src/VxTagLabel.cpp
    src/VxTilePositioner.cpp
    src/VidWidget.cpp
    src/VxWidgetBase.cpp
    src/VxMenu.cpp
    src/VxAppStyle.cpp
    src/VxAppTheme.cpp
    src/VxAppDisplay.cpp
    src/VxPlainTextEdit.cpp
    src/WaitingSpinnerWidget.cpp
    src/WebsiteWidget.cpp

   )

set(nlc_FORMS)
list(APPEND nlc_FORMS
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActionWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityBrowseFiles.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityDownloadItemMenu.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityGenerateHash.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityMessageBox.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityNetworkStartupTest.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivitySessionOptions.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityShowHelp.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityTimedMessage.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ActivityYesNo.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AcceptCancelWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletAboutApp.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletAboutFile.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletAboutMeClient.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletAboutUser.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletApplicationInfo.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletBrowseFiles.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletCamClient.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletCamSettings.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletChatRoomJoinSearch.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletChatRoomListLocalView.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletChooseThumbnail.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletCreateAccount.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletDownloads.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletEditAboutMe.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletEditAvatarImage.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletEditStoryBoard.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletFileOfferSelect.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletFileShareClientView.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletFriendList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletFriendRequest.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletFriendRequestList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletGalleryImage.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletGalleryEmoticon.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletGalleryThumb.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletGetStarted.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletGroupJoinSearch.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletGroupListClient.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletGroupListLocalView.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHackerList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHelpNetSignalBars.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHostClient.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHostJoinChoose.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHostJoinConnect.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHostJoinRequestList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHostLeave.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletHostNetworkStatus.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletIgnoredHosts.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletInformation.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletInviteAccept.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletInviteCreate.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletIsPortOpenTest.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletJoinHostList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletLanguageSelect.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletLaunchWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletLibrary.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletLog.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletLogSettings.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletMultiMessenger.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletNetworkKey.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletNetworkSettings.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletOfferInfo.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletOfferList.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletOfferRandSession.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletOfferResponse.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletOfferResponseAccept.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletOfferSend.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletOfferView.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPersonOfferList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPeerChangeFriendship.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPeerReplyOfferFile.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPeerSessionFileOffer.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPeerTodGame.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPeerVideoPhone.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPeerVoicePhone.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPermissionList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPersonalRecorder.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPlayerCamClip.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPlayerPhoto.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPlayerNlc.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPlayerVideo.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletPopupMenu.ui

    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletRandomConnectJoinSearch.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletRandomConnectListLocalView.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletServiceBase.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletServiceBaseSettings.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletSettingsHostBase.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletSettingsNoUserSettings.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletSnapshot.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletSocketList.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletSoundSettings.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletStoryBoardClient.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletTestAndDebug.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletTestUpnp.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletTheme.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletUploads.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletUserConnections.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletUserIdentity.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AppletUserPreferences.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AssetFaceWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AssetPhotoWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AssetTextWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AssetVideoWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AssetVoiceWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/AvatarBarWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/BottomBarWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/CentralFrame.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/CentralWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ChatEntryWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/DialogAddComment.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/DialogConfirmRemoveMessage.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FileActionBarWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FileActionMenu.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FileDisplayWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FileFilterSelectWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FileMediaSelectWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FileShareItemWidget.ui
    #${CMAKE_SOURCE_DIR}/resources/Forms/FileViewSharedActionBar.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FileXferWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/FriendListDlg.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/GuiFriendRequestWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/GuiGroupieListItem.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/GuiHostedListItem.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/GuiUserMultiListWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/HostJoinRequestListItem.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/HistoryEntryWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/IdentWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ImageListEntry.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ImageListRow.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ImageListWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ImagePickBar.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InformationWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InputAllWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InputFaceWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InputPhotoWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InputTextWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InputVideoWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InputVoiceWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InstMsgWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/InviteUrlWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/LogWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/OfferBarWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/OfferSendWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/PermissionWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/PermissionListItemWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/PersonOfferListItem.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/PlayControlWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/PluginSettingsWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/SearchBarWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/SearchParamsWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ServiceSettingsWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/SessionWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/StoryWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/TestWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ThumbnailEditWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/ThumbnailChooseWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/TitleBarWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/TodGameWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/TodStatsWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/VidWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/Forms/WebsiteWidget.ui
    ${CMAKE_SOURCE_DIR}/resources/AppResources.qrc
    )

if(DEFINED TARGET_OS_WINDOWS)
    # Define the path to your resource file
    set(EXE_ICON_RESOURCE "${CMAKE_SOURCE_DIR}/resources/Resources/resources.rc")
    list(APPEND nlc_SOURCES ${EXE_ICON_RESOURCE})
endif()

qt_add_executable(nolimitconnect
    MANUAL_FINALIZATION
    ${nlc_FORMS}
    ${nlc_SOURCES}
    ${nlc_HEADERS}
    )

set(nlc_TRANSLATIONS_DIR "${CMAKE_SOURCE_DIR}/translations")

file(GLOB nlc_TS_FILES CONFIGURE_DEPENDS
    "${nlc_TRANSLATIONS_DIR}/*.ts"
)
list(REMOVE_DUPLICATES nlc_TS_FILES)

set(nlc_QM_FILES)
set(nlc_LUPDATE_EXECUTABLE "")
set(nlc_LRELEASE_EXECUTABLE "")
set(nlc_BASE_TS_FILE "${nlc_TRANSLATIONS_DIR}/nolimitconnect_en_EN.ts")

set(nlc_QT_BIN_HINTS)
if(DEFINED Qt6_DIR)
    get_filename_component(nlc_qt_cmake_dir "${Qt6_DIR}" ABSOLUTE)
    get_filename_component(nlc_qt_root_dir "${nlc_qt_cmake_dir}/../../.." ABSOLUTE)
    list(APPEND nlc_QT_BIN_HINTS "${nlc_qt_root_dir}/bin")
endif()
if(DEFINED QT_QMAKE_EXECUTABLE)
    get_filename_component(nlc_qmake_bin_dir "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    list(APPEND nlc_QT_BIN_HINTS "${nlc_qmake_bin_dir}")
endif()
if(CMAKE_PREFIX_PATH)
    list(GET CMAKE_PREFIX_PATH 0 nlc_qt_prefix_path)
    list(APPEND nlc_QT_BIN_HINTS "${nlc_qt_prefix_path}/bin")
endif()
list(REMOVE_DUPLICATES nlc_QT_BIN_HINTS)

foreach(nlc_qt_bin_dir IN LISTS nlc_QT_BIN_HINTS)
    if(NOT nlc_LUPDATE_EXECUTABLE AND EXISTS "${nlc_qt_bin_dir}/lupdate.exe")
        set(nlc_LUPDATE_EXECUTABLE "${nlc_qt_bin_dir}/lupdate.exe")
    endif()

    if(NOT nlc_LRELEASE_EXECUTABLE AND EXISTS "${nlc_qt_bin_dir}/lrelease.exe")
        set(nlc_LRELEASE_EXECUTABLE "${nlc_qt_bin_dir}/lrelease.exe")
    endif()
endforeach()

if(nlc_TS_FILES AND nlc_LUPDATE_EXECUTABLE AND nlc_LRELEASE_EXECUTABLE)
    foreach(tsFile IN LISTS nlc_TS_FILES)
        get_filename_component(tsFileName "${tsFile}" NAME_WE)
        set(qmFile "${nlc_TRANSLATIONS_DIR}/${tsFileName}.qm")
        add_custom_command(
            OUTPUT "${qmFile}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${nlc_TRANSLATIONS_DIR}"
            COMMAND "${nlc_LRELEASE_EXECUTABLE}" "${tsFile}" -qm "${qmFile}"
            DEPENDS "${tsFile}"
            VERBATIM
        )
        list(APPEND nlc_QM_FILES "${qmFile}")
    endforeach()

    add_custom_target(nolimitconnect_lupdate
        COMMAND "${nlc_LUPDATE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/resources/Forms"
            -ts ${nlc_TS_FILES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM
    )

    add_custom_target(nolimitconnect_translations ALL
        DEPENDS ${nlc_QM_FILES}
    )

    add_dependencies(nolimitconnect nolimitconnect_translations)
elseif(nlc_LUPDATE_EXECUTABLE)
    add_custom_target(nolimitconnect_lupdate
        COMMAND ${CMAKE_COMMAND} -E make_directory "${nlc_TRANSLATIONS_DIR}"
        COMMAND "${nlc_LUPDATE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/resources/Forms"
            -ts "${nlc_BASE_TS_FILE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM
    )
else()
    add_custom_target(nolimitconnect_lupdate
        COMMAND ${CMAKE_COMMAND} -E echo "Qt lupdate executable not found; skipping translation update."
        VERBATIM
    )
endif()

target_include_directories(nolimitconnect PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/miniaudio        
)

if(DEFINED BUILD_SYSTEM_FLATPAK)
    # glu is no longer available in flatpak-sdk, so we need to use our own library
    target_include_directories(nolimitconnect PRIVATE
        ${PROJECT_SRC_ROOT_DIR}/libs/libGLU/include       
    )
endif()

if(DEFINED TARGET_OS_LINUX)
    target_compile_definitions(nolimitconnect PRIVATE
        # use v4l2 camera capture on linux instead of qt camera which scrables images when linux is running in a vm
        TARGET_OS_LINUX
    )

endif()

if(DEFINED TARGET_OS_WINDOWS)
    target_compile_definitions(nolimitconnect PRIVATE
        NOMINMAX
        WIN64
        WIN32_LEAN_AND_MEAN
        _MBCS
        __STDC_CONSTANT_MACROS
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
        _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
    )

endif()

if(DEFINED TARGET_OS_ANDROID)
    message(STATUS "nolimitconnect cpu ${NLC_CPU}")
    if(${NLC_CPU} STREQUAL "CPU_ARM32")
        message(STATUS "nolimitconnect api armeabi-v7a")
        set_target_properties(nolimitconnect PROPERTIES
            ANDROID_TARGET_ARCH armeabi-v7a
            QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SRC_ROOT_DIR}/nolimitgui/android"
            QT_ANDROID_ABIS armeabi-v7a
            CMAKE_ANDROID_ASSETS_DIRECTORIES  "${PROJECT_SRC_ROOT_DIR}/bin-Android/assets"
            )
    else()
        message(STATUS "nolimitconnect api arm64-v8a")
        set_target_properties(nolimitconnect PROPERTIES
            ANDROID_TARGET_ARCH arm64-v8a
            QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SRC_ROOT_DIR}/nolimitgui/android"
            QT_ANDROID_ABIS arm64-v8a
            CMAKE_ANDROID_ASSETS_DIRECTORIES  "${PROJECT_SRC_ROOT_DIR}/bin-Android/assets"
            )
    endif()
else()
    set_target_properties(nolimitconnect PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
    )
endif()

target_compile_definitions(nolimitconnect
    PRIVATE
    ENABLE_NLC_PLAYER
)

target_include_directories(nolimitconnect PUBLIC
    src
    ../nolimitlib
    ../libs
    ../libs/libaudio-nlc
    ../libs/libplayer-nlc/xbmc
    ../libs/ffmpeg
    ../libs/libechocancel
    ../libs/GuiInterface
    ../libs/VirtStream
    ../libs/libptopengine
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_STATIC_LIB_DIR}
)

# common to all builds
target_link_libraries(nolimitconnect
    PUBLIC
    # Qt libraries
    Qt::Core
    Qt::CorePrivate
    Qt::Gui
    Qt::Multimedia
    Qt::OpenGL
    Qt::Svg
    Qt::Widgets

    # static libraries
    GuiInterface
    audio-nlc
    player-nlc
    VirtStream
    avdevice
    avfilter
    avformat
    avcodec
    postproc
    swresample
    swscale
    avutil
    ffmpegopus

    miniupnpc

    ptopengine
    netlib

    pktlib

    wav-decoder
    rnnoise   
    CamCapture
)

if(USE_AEC2)
    target_link_libraries(nolimitconnect
        PUBLIC
        webrtc-aec2 # webrtc audio echo cancel
        #absl::strings # absl support for webrtc audio echo cancel
        #absl::base # absl support for webrtc audio echo cancel
    )
else()
    target_link_libraries(nolimitconnect
        PUBLIC
        webrtc-aec # webrtc audio echo cancel
        absl::strings # absl support for webrtc audio echo cancel
        absl::base # absl support for webrtc audio echo cancel
    )
endif()

if (DEFINED TARGET_OS_WINDOWS)
    target_link_libraries(nolimitconnect
        PUBLIC

        x264
        x265
        dav1d
        vpx

        pcre
        opencore-amr

        mediatools
        speex
        lame
        vorbis2

        gnu
        ass
        fribidi
        freetype

        iconv
        vxvideolib

        tag
        theora
        twolame

        tinyxml
        xml2

        lzma
        lzo
        zip
        CoreLib
        zlib
        crossguid
        pthread
        ogg

        opengl32
        glu32
        ole32
        winmm
        Rpcrt4
        ws2_32
        iphlpapi    # Fixes the unresolved symbols from miniupnpc
        dwmapi      # Fixes DwmIsCompositionEnabled from player-nlc
        cfgmgr32    # Fixes CM_Get_DevNode_Status from player-nlc
        Setupapi    # Fixes SetupDiGetClassDevsA and related functions from player-nlc
        glew        # Fixes glew32s static linking on Windows used by OverlayRendererNlc.cpp from player-nlc
    )

endif()

if (DEFINED TARGET_OS_LINUX)
    if (DEFINED TARGET_CPU_ARM64)

        target_link_libraries(nolimitconnect
            PUBLIC
            x264
            x265
            dav1d
            vpx
            pcre

            opencore-amr
            mediatools
            speex
            lame
            vorbis2
            ogg

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            tag
            theora
            twolame


            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            pthread
            dl
            GLU
            GL
            m
            uuid
            rt
        )
    else()
        target_link_libraries(nolimitconnect
            PUBLIC
            x264
            x265
            dav1d
            vpx
            pcre

            opencore-amr
            mediatools
            speex
            lame
            vorbis2
            ogg

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            tag
            theora
            twolame

            freetype

            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            pthread
            dl
            GLU
            GL
            m
            uuid
            rt
        )
    endif()

elseif (DEFINED TARGET_OS_ANDROID)
    find_package(Qt6 COMPONENTS Gui REQUIRED)
    if (DEFINED TARGET_CPU_ARM64)
        target_link_libraries(nolimitconnect
            PUBLIC
            # static nlc libraries

            x264
            x265
            dav1d
            vpx
            pcre

            opencore-amr
            mediatools
            speex
            lame
            vorbis2

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            speex
            tag
            theora
            twolame

            ogg
            netlib
            pktlib

            fribidi
            freetype

            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            dl
            m
            EGL
            GLESv2
            c
            stdc++
            jnigraphics
            android
        )
    elseif(DEFINED TARGET_CPU_ARM32)
        target_link_libraries(nolimitconnect
            PUBLIC
            # static nlc libraries

            x264
            #x265 #cannot compile x265 on arm7a
            dav1d
            vpx
            pcre
            opencore-amr

            mediatools
            speex
            lame
            vorbis2

            gnu
            ass
            fribidi
            freetype

            iconv

            vxvideolib
            speex
            tag
            theora
            twolame


            freetype

            tinyxml
            xml2

            lzma
            lzo
            zip
            CoreLib
            zlib
            crossguid
            miniupnpc

            dl
            m
            EGL
            GLESv2
            c
            stdc++
            jnigraphics
            android
        )
    endif()

elseif (DEFINED TARGET_OS_WINDOWS)
    # do nothing .. was already linked
else()
    message(FATAL_ERROR "Unknown Target OS Defined for target_link_libraries")
endif()

if(USE_LIBJPEG_TURBO)
    target_link_libraries(nolimitconnect
        PUBLIC
        jpegturbo
    )
else()
    target_link_libraries(nolimitconnect
        PUBLIC
        jpg
    )
endif()

qt_finalize_target(nolimitconnect)

if(DEFINED FLATPAKBUILD)


    #set(INSTALL_DIR "") # empty to not complicate run command

    #install(TARGETS nolimitconnect
    #    RUNTIME DESTINATION "${INSTALL_DIR}"
    #    BUNDLE DESTINATION "${INSTALL_DIR}"
    #    LIBRARY DESTINATION "${INSTALL_DIR}"
    #)

    # Install executeable
    install(
        TARGETS nolimitconnect
        DESTINATION bin/
    )

    # Install desktop file
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/org.nolimitconnect.NoLimitConnect.desktop
        DESTINATION share/applications
    )

    # And install all the icons
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/icons/16x16/org.nolimitconnect.NoLimitConnect.png
        DESTINATION share/icons/hicolor/16x16/apps/
    )
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/icons/32x32/org.nolimitconnect.NoLimitConnect.png
        DESTINATION share/icons/hicolor/32x32/apps/
    )
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/icons/48x48/org.nolimitconnect.NoLimitConnect.png
        DESTINATION share/icons/hicolor/48x48/apps/
    )
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/icons/64x64/org.nolimitconnect.NoLimitConnect.png
        DESTINATION share/icons/hicolor/64x64/apps/
    )
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/icons/128x128/org.nolimitconnect.NoLimitConnect.png
        DESTINATION share/icons/hicolor/128x128/apps/
    )
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/icons/256x256/org.nolimitconnect.NoLimitConnect.png
        DESTINATION share/icons/hicolor/256x256/apps/
    )
    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/icons/512x512/org.nolimitconnect.NoLimitConnect.png
        DESTINATION share/icons/hicolor/512x512/apps/
    )
    # the max allowed icon size for flathub is 512x512

    install(
        FILES ${PROJECT_SRC_ROOT_DIR}/org.nolimitconnect.NoLimitConnect.metainfo.xml
        DESTINATION share/metainfo/
    )

else()
    message(STATUS "Post build CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "Post build CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")

    add_custom_command(TARGET nolimitconnect POST_BUILD
        COMMAND echo "echo CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")

    if(DEFINED TARGET_OS_NONE) # CPACK is no longer used. flatpak is used for linux insted
        #=== after compile add qt libraries to bin-TargetOs directory ===#
        file(MAKE_DIRECTORY "${PROJECT_BIN_DIR}/usr/lib")

        #move requred qt package into bin/usr/lib for packaging Core Gui Multimedia OpenGL Svg Widgets)
        message(STATUS "POST_BUID_TARGET copy shared qt libraries from ${CMAKE_PREFIX_PATH}/lib")
        add_custom_command(POST_BUILD TARGET nolimitconnect
            WORKING_DIRECTORY "${PROJECT_BIN_DIR}"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_PREFIX_PATH}/lib/libQt6Core.so*" "${PROJECT_BIN_DIR}/usr/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_PREFIX_PATH}/lib/libQt6Gui.so*" "${PROJECT_BIN_DIR}/usr/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_PREFIX_PATH}/lib/libQt6Multimedia.so*" "${PROJECT_BIN_DIR}/usr/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_PREFIX_PATH}/lib/libQt6OpenGL.so*" "${PROJECT_BIN_DIR}/usr/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_PREFIX_PATH}/lib/libQt6Svg.so*" "${PROJECT_BIN_DIR}/usr/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_PREFIX_PATH}/lib/libQt6Widgets.so*" "${PROJECT_BIN_DIR}/usr/lib"
            COMMAND_EXPAND_LISTS
            )


        #=== after qt libraries are copied then make installer with cpack ===#
        message(STATUS "Setting up CPACK")
        # common to all packages
        set(CPACK_PACKAGE_NAME "${NLC_NAME}")
        set(CPACK_PACKAGE_VENDOR "${PROJECT_VENDOR}")
        set(CMAKE_PROJECT_HOMEPAGE_URL  "${PROJECT_WEBSITE_URL}")
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
            "No Limit Connect - A Open Source Social Network")
        set(CPACK_PACKAGE_VERSION "${NLC_VERSION}")
        set(CPACK_PACKAGE_VERSION_MAJOR "${NLC_VERSION_MAJOR}")
        set(CPACK_PACKAGE_VERSION_MINOR "${NLC_VERSION_MINOR}")
        set(CPACK_PACKAGE_VERSION_PATCH "${NLC_VERSION_PATCH}")

        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_RESOURCE_FILE_README "${PROJECT_SRC_ROOT_DIR}/README.md")
        set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SRC_ROOT_DIR}/LICENSE")

        set(CPACK_PACKAGE_DIRECTORY "${PROJECT_PACKAGE_DIR}")


        # generate postinst file in ${CMAKE_BINARY_DIR} from template                        #
        # CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/contrib/postinst.in" "postinst" @ONLY IMMEDIATE)


        if(DEFINED TARGET_OS_LINUX)
            message(STATUS "Creating debian package")
            set(CPACK_GENERATOR "DEB")
            include(GNUInstallDirs)
            set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PROJECT_MAINTAINER}")
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libgl1, libglu1-mesa, libuuid1")
            INSTALL( DIRECTORY "${PROJECT_ASSETS_DIR}" DESTINATION usr )


        elseif(DEFINED TARGET_OS_WINDOWS)
            set(CPACK_GENERATOR "NSIS")
            set(CPACK_NSIS_MUI_ICON "${PROJECT_SRC_ROOT_DIR}/resources/Resources/NlcP2P.ico")

        endif()

        # This must always be after all CPACK\_\* variables are defined
        include(CPack)
        message(STATUS "CPACK complete")
    endif()

    # if(DEFINED TARGET_OS_ANDROID)
    #     if(DEFINED DO_MANUAL)
    #         if(MANUAL_PACKAGE)
    #             file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/nolimitgui/android-build")
    #             set(APK_ARGS "--input ${CMAKE_BINARY_DIR}/nolimitgui/android-nolimitconnect-deployment-settings.json --output ${CMAKE_BINARY_DIR}/nolimitgui/android-build --android-platform android-34 --jdk $ENV{JAVA_HOME} --verbose --gradle")
    #             message(STATUS "Post build MANUAL_PACKAGE is defined ${APK_ARGS}")
    #             if(CI_PIPELINE)
    #                 # androiddeployqt is in path
    #                 message(STATUS "Create Apk  androiddeployqt ${APK_ARGS}")
    #                 #add_custom_command(POST_BUILD TARGET nolimitconnect

    #                 add_custom_target(nolimitappsEchoDeploy ALL
    #                     COMMAND echo "echo create apk androiddeployqt ${APK_ARGS}")
    #                 add_custom_target(nolimitappsEchoDeploy ALL
    #                     COMMAND androiddeployqt "${APK_ARGS}")
    #             else()
    #                 # need full path to androiddeployqt
    #                 if(FLATPAKBUILD)
    #                     message("** Enabling Flatpak workarounds")
    #                     add_definitions(-DPQMFLATPAKBUILD)
    #                     include_directories(${GLIB_INCLUDE_DIRS})
    #                     target_link_libraries(nolimitconnect PRIVATE ${GLIB_LIBRARIES})
    #                     target_link_libraries(nolimitconnect PRIVATE "gobject-2.0")
    #                     target_link_libraries(nolimitconnect PRIVATE "gio-2.0")
    #                 endif()
    #                 message(STATUS "Create Apk ${USER_HOME_DIR}/Qt/${NLC_QT_VER}/gcc_64/bin/androiddeployqt ${APK_ARGS}")
    #                 add_custom_target(nolimitappsEchoDeploy ALL
    #                     COMMAND echo "echo create apk ${USER_HOME_DIR}/Qt/${NLC_QT_VER}/gcc_64/bin/androiddeployqt ${APK_ARGS}")
    #                 add_custom_target(nolimitappsEchoDeploy ALL
    #                     COMMAND "${USER_HOME_DIR}/Qt/${NLC_QT_VER}/gcc_64/bin/androiddeployqt" "${APK_ARGS}")
    #             endif()
    #         else()
    #             message(STATUS "Post build MANUAL_PACKAGE is NOT defined")
    #             add_custom_target(nolimitappsEchoDeploy ALL
    #                 COMMAND echo "create apk not required")
    #         endif() # if(MANUAL_PACKAGE)
    #     endif()

    #     # copy package to package directory
    #     set(NLC_APK_ROOT_DIR "${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/apk/")
    #     if (CMAKE_BUILD_TYPE MATCHES Debug)
    #         # for debugging only use apk
    #         message(STATUS "POST_BUID_TARGET copy debug android packages from ${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/apk/debug to package directory ${PROJECT_PACKAGE_DIR}")
    #         add_custom_target(nolimitappsCopyDebugApk
    #             COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/apk/debug" "${PROJECT_PACKAGE_DIR}"
    #             COMMAND_EXPAND_LISTS
    #             )
    #         add_dependencies(nolimitappsCopyDebugApk nolimitconnect)

    #     else()
    #         # for release android play store requires aab bundle
    #         message(STATUS "POST_BUID_TARGET copy release android packages from ${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/bundle/release to package directory ${PROJECT_PACKAGE_DIR}")
    #         if(EXISTS "${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/bundle/release")
    #             add_custom_target(nolimitappsCopyReleaseBundle
    #                 COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/apk/release" "${PROJECT_PACKAGE_DIR}"
    #                 COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/bundle/release" "${PROJECT_PACKAGE_DIR}"
    #                 COMMAND_EXPAND_LISTS
    #                 )
    #             add_dependencies(nolimitappsCopyReleaseBundlenolimitconnect)
    #         else()
    #             add_custom_target(nolimitappsCopyReleaseApk
    #                 COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/apk/release" "${PROJECT_PACKAGE_DIR}"
    #                 DEPENDS "${CMAKE_BINARY_DIR}/nolimitgui/android-build/build/outputs/apk/release/android-build-release.apk"
    #                 COMMAND_EXPAND_LISTS
    #             )
    #         add_dependencies(nolimitappsCopyReleaseApk nolimitconnect)
    #         endif() # if EXISTS
    #     endif() # if (CMAKE_BUILD_TYPE MATCHES Debug)
    # endif() #if(DEFINED TARGET_OS_ANDROID)


    add_custom_target(nolimitappsApk
        COMMAND echo "echo CMAKE_BINARY_DIR ***** ${CMAKE_BINARY_DIR}")
    add_dependencies(nolimitappsApk nolimitconnect)

    #=== INSTALL FILES ===#
    if(DEFINED TARGET_OS_LINUX)


    endif()
endif() # NOT DEFINED FLATPAKBUILD

# if android release build then copy apk to /deploy/android
if(ANDROID) # AND CMAKE_BUILD_TYPE STREQUAL "Release")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(APK_OUTPUT_DIR
            ${CMAKE_BINARY_DIR}/nolimitgui/android-build-nolimitconnect/build/outputs/apk/debug
        )
    else()
        set(APK_OUTPUT_DIR
            ${CMAKE_BINARY_DIR}/nolimitgui/android-build-nolimitconnect/build/outputs/apk/release
        )
    endif()

    set(DEPLOY_DIR ${CMAKE_SOURCE_DIR}/deploy/android)
    set(APP_VERSION ${NLC_VERSION})  # your version

    message(STATUS "Copy apk version ${APP_VERSION} to  ${DEPLOY_DIR}")

    if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
        message(STATUS "Windows APK Copy")
        # Find all APK files
        file(GLOB APK_FILES "${APK_OUTPUT_DIR}/*.apk")

        if(APK_FILES)
            # Take the first APK (adjust if multiple ABIs)
            list(GET APK_FILES 0 APK_TO_COPY)

            add_custom_command(
                TARGET nolimitconnect
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}
                COMMAND ${CMAKE_COMMAND} -E echo "Copying Release APK: ${APK_TO_COPY}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${APK_TO_COPY}"
                        "${DEPLOY_DIR}/nolimitconnect_v${APP_VERSION}.apk"
                COMMENT "Copy Release APK to deploy/android with version"
            )
        else()
            message(STATUS "No APK found in ${APK_OUTPUT_DIR}. Skipping copy.")
        endif()

    else()
        message(STATUS "LINUX APK Copy")
        set(_NLC_COPY_APK_SCRIPT "${CMAKE_BINARY_DIR}/copy_release_apk_if_present.cmake")
        file(WRITE "${_NLC_COPY_APK_SCRIPT}"
"file(GLOB _apk_files \"${CMAKE_BINARY_DIR}/android-build/build/outputs/apk/release/*.apk\")
if(_apk_files)
    file(MAKE_DIRECTORY \"${CMAKE_SOURCE_DIR}/deploy/android\")
    foreach(_apk \${_apk_files})
        message(STATUS \"Copying APK: \${_apk}\")
        file(COPY \"\${_apk}\" DESTINATION \"${CMAKE_SOURCE_DIR}/deploy/android\")
    endforeach()
else()
    message(STATUS \"Release APK not found yet (Gradle not run)\")
endif()
")
        add_custom_command(
            TARGET nolimitconnect
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${CMAKE_SOURCE_DIR}/deploy/android
            COMMAND ${CMAKE_COMMAND} -P "${_NLC_COPY_APK_SCRIPT}"
            COMMENT "Copy Release APK to deploy/android (if present)"
        )

    endif()
endif()


