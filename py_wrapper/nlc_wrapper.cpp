#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "IDefs.h"
#include "IFromGui.h"
#include "IFromGuiDefs.h"
#include "IToGui.h"

#include <NetLib/NetHostSetting.h>
#include <NetLib/NetSettings.h>

#include <PktLib/VxCommon.h>
#include <PktLib/SearchParams.h>

#include <Plugins/FileInfo.h>
#include <HostListMgr/HostedInfo.h>

#include <CoreLib/VxGUID.h>
#include <CoreLib/PktBlobEntry.h>
#include <CoreLib/VxXferDefs.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace py = pybind11;

class PythonHackReportCallback : public IHackReportCallbackInterface {
public:
    void setHandler(py::object handler) {
        py::gil_scoped_acquire gil;
        handler_ = std::move(handler);
    }

    void reportHackOffense(EHackerLevel hackerLevel,
                           EHackerReason hackerReason,
                           std::string ipAddr,
                           std::string hackDescription) override {
        py::gil_scoped_acquire gil;
        if (handler_.is_none()) {
            return;
        }

        try {
            handler_(hackerLevel, hackerReason, ipAddr, hackDescription);
        } catch (const py::error_already_set&) {
            PyErr_Print();
        }
    }

private:
    py::object handler_ = py::none();
};

static std::unique_ptr<PythonHackReportCallback> g_pythonHackReportCallback;

class PythonIToGuiAdapter : public IToGui {
public:
    void setDefaultHandler(py::object handler) {
        py::gil_scoped_acquire gil;
        defaultHandler_ = std::move(handler);
    }

    void clearDefaultHandler() {
        py::gil_scoped_acquire gil;
        defaultHandler_ = py::none();
    }

    void registerHandler(const std::string& methodName, py::object handler) {
        py::gil_scoped_acquire gil;
        handlers_[methodName].push_back(std::move(handler));
    }

    void clearHandlers(const std::string& methodName) {
        py::gil_scoped_acquire gil;
        handlers_.erase(methodName);
    }

    void clearAllHandlers() {
        py::gil_scoped_acquire gil;
        handlers_.clear();
    }

    bool toGuiMediaAction(EMediaModule, EMediaPlayerAction, int, const char*) override { return false; }
    void toGuiMediaError(EMediaModule, EMediaError, const char*) override {}
    void toGuiSetIsAppModuleRunning(EMediaModule, bool) override {}
    bool toGuiGetIsAppModuleRunning(EMediaModule) override { return false; }
    bool toGuiRunModule(EMediaModule) override { return false; }
    bool toGuiStopModule(EMediaModule) override { return false; }
    void toGuiPlayNlcMedia(AssetBaseInfo*) override {}
    void toGuiLog(int, const char*) override {}
    void toGuiAppErr(EAppErr, const char*) override {}
    void toGuiAppPopupErr(EAppErr, const char*) override {}

    void toGuiStatusMessage(const char* errMsg) override {
        dispatch("toGuiStatusMessage", std::string(errMsg ? errMsg : ""));
    }

    void toGuiPluginMsg(EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, const char* paramMsg) override {
        dispatch("toGuiPluginMsg", pluginType, onlineId, msgType, std::string(paramMsg ? paramMsg : ""));
    }

    void toGuiPluginCommError(EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, ECommErr commErr) override {
        dispatch("toGuiPluginCommError", pluginType, onlineId, msgType, static_cast<int>(commErr));
    }

    void toGuiModuleState(EMediaModule, EModuleState) override {}
    void toGuiWantVideoCapture(EMediaModule, bool) override {}
    void toGuiPlayJpgVideo(VxGUID&, std::shared_ptr<CamJpgVideo>&) override {}
    void toGuiHostAnnounceStatus(EHostType, VxGUID&, EHostAnnounceStatus, const char*) override {}
    void toGuiHostJoinStatus(EHostType, VxGUID&, EHostJoinStatus, const char*) override {}
    void toGuiHostSearchStatus(EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr, const char* msg) override {
        dispatch(
            "toGuiHostSearchStatus",
            static_cast<int>(hostType),
            sessionId,
            static_cast<int>(searchStatus),
            static_cast<int>(commErr),
            std::string(msg ? msg : "")
        );
    }
    void toGuiHostSearchResult(EHostType hostType, VxGUID& sessionId, HostedInfo& hostedInfo) override {
        dispatch("toGuiHostSearchResult", static_cast<int>(hostType), sessionId, hostedInfo);
    }
    void toGuiHostSearchComplete(EHostType hostType, VxGUID& sessionId) override {
        dispatch("toGuiHostSearchComplete", static_cast<int>(hostType), sessionId);
    }
    void toGuiGroupieSearchStatus(EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr, const char* msg) override {
        dispatch(
            "toGuiGroupieSearchStatus",
            static_cast<int>(hostType),
            sessionId,
            static_cast<int>(searchStatus),
            static_cast<int>(commErr),
            std::string(msg ? msg : "")
        );
    }
    void toGuiGroupieSearchResult(EHostType, VxGUID&, GroupieInfo&) override {}
    void toGuiGroupieSearchComplete(EHostType hostType, VxGUID& sessionId) override {
        dispatch("toGuiGroupieSearchComplete", static_cast<int>(hostType), sessionId);
    }
    void toGuiIsPortOpenStatus(EIsPortOpenStatus, const char*) override {}

    void toGuiNetAvailableStatus(ENetAvailStatus netAvailStatus) override {
        dispatch("toGuiNetAvailableStatus", netAvailStatus);
    }

    void toGuiNetworkState(ENetworkStateType networkState, const char* stateMsg) override {
        dispatch("toGuiNetworkState", static_cast<int>(networkState), std::string(stateMsg ? stateMsg : ""));
    }

    void toGuiRandomConnectStatus(ERandomConnectStatus, const char*) override {}
    void toGuiRunTestStatus(const char*, ERunTestStatus, const char*) override {}
    void toGuiIndentListUpdate(EUserViewType, VxGUID&, uint64_t) override {}
    void toGuiIndentListRemove(EUserViewType, VxGUID&) override {}
    void toGuiContactAdded(VxNetIdent*) override {}
    void toGuiContactRemoved(VxGUID&) override {}
    void toGuiContactOnline(VxNetIdent*) override {}
    void toGuiContactAnythingChange(VxNetIdent*) override {}
    void toGuiContactLastSessionTimeChange(VxNetIdent*) override {}
    void toGuiUpdateMyIdent(VxNetIdent*) override {}
    void toGuiSaveMyIdent(VxNetIdent*) override {}
    void toGuiRxedPluginOffer(VxGUID, OfferBaseInfo&) override {}
    void toGuiRxedOfferReply(VxGUID, OfferBaseInfo&) override {}
    void toGuiPluginSessionStarted(VxGUID&, EPluginType, VxGUID&) override {}
    void toGuiPluginSessionEnded(VxGUID&, EPluginType, VxGUID&) override {}
    void toGuiPluginStatus(EPluginType pluginType, int statusType, int statusValue) override {
        dispatch("toGuiPluginStatus", pluginType, statusType, statusValue);
    }
    void toGuiInstMsg(VxGUID&, EPluginType, const char*) override {}
    void toGuiFileListReply(VxGUID&, EPluginType, FileInfo&) override {}
    void toGuiFileList(VxGUID&, FileInfo&) override {}
    void toGuiFileListCompleted(VxGUID&) override {}
    void toGuiFolderScan(VxGUID&, FileInfo&) override {}
    void toGuiFolderScanCompleted(VxGUID&, bool) override {}
    void toGuiFileUploadStart(VxGUID&, EPluginType, VxGUID&, FileInfo&) override {}
    void toGuiFileUploadComplete(EPluginType, VxGUID&, std::string&, EXferError) override {}
    void toGuiFileDownloadStart(VxGUID&, EPluginType, VxGUID&, FileInfo&) override {}
    void toGuiFileDownloadComplete(EPluginType, VxGUID&, std::string&, EXferError) override {}

    void toGuiFileXferState(EPluginType pluginType,
                            VxGUID& lclSessionId,
                            EXferDirection xferDir,
                            EXferState xferState,
                            EXferError xferErr,
                            int param1) override {
        dispatch("toGuiFileXferState", pluginType, lclSessionId, xferDir, xferState, xferErr, param1);
    }

    void toGuiFileDeleted(std::string&) override {}
    void toGuiAssetAdded(AssetBaseInfo*) override {}
    void toGuiAssetUpdated(AssetBaseInfo*) override {}
    void toGuiAssetRemoved(AssetBaseInfo*) override {}
    void toGuiAssetXferState(VxGUID&, EAssetSendState, int) override {}
    void toGuiAssetSessionHistory(AssetBaseInfo*) override {}
    void toGuiAssetAction(EAssetAction, VxGUID&, int) override {}
    void toGuiMultiSessionAction(EMSessionAction, VxGUID, int) override {}
    void toGuiBlobAdded(BlobInfo*) override {}
    void toGuiBlobAction(EAssetAction, VxGUID&, int) override {}
    void toGuiBlobSessionHistory(BlobInfo*) override {}
    void toGuiTodGameAction(EPluginType, VxGUID&, ETodGameAction) override {}
    void toGuiSearchResultFileSearch(VxGUID&, EPluginType, VxGUID&, FileInfo&) override {}
    void toGuiNetworkIsTested(bool, std::string&, uint16_t) override {}
    void toGuiAdminAvail(GroupieId&, bool) override {}
    void toGuiUpdateWantMicrophoneCount(int) override {}
    void toGuiUpdateWantSpeakerCount(int) override {}

private:
    template <typename... Args>
    void dispatch(const char* methodName, Args&&... args) {
        py::gil_scoped_acquire gil;

        auto it = handlers_.find(methodName);
        if (it != handlers_.end()) {
            for (py::object& handler : it->second) {
                try {
                    handler(py::str(methodName), py::cast(std::forward<Args>(args))...);
                } catch (const py::error_already_set&) {
                    PyErr_Print();
                }
            }
        }

        if (!defaultHandler_.is_none()) {
            try {
                defaultHandler_(py::str(methodName), py::cast(std::forward<Args>(args))...);
            } catch (const py::error_already_set&) {
                PyErr_Print();
            }
        }
    }

    py::object defaultHandler_ = py::none();
    std::unordered_map<std::string, std::vector<py::object>> handlers_;
};

// Trampoline class for Python implementations of IFromGui.
// Only the currently wired high-use methods are overridden here.
class PyIFromGui : public IFromGui {
public:
    using IFromGui::IFromGui;

    void fromGuiAppStartup(std::string assetsDir, std::string rootDataDir, bool fromThread) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiAppStartup, assetsDir, rootDataDir, fromThread);
    }
    void fromGuiSetUserSpecificDir(std::string userSpecificDir, bool fromThread) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiSetUserSpecificDir, userSpecificDir, fromThread);
    }
    void fromGuiSetUserXferDir(std::string userDownloadDir, bool fromThread) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiSetUserXferDir, userDownloadDir, fromThread);
    }
    void fromGuiAppShutdown(void) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiAppShutdown);
    }
    bool fromGuiDeleteUser(VxGUID& onlineId) override {
        PYBIND11_OVERRIDE_PURE(bool, IFromGui, fromGuiDeleteUser, onlineId);
    }
    uint64_t fromGuiGetDiskFreeSpace(const char* dir) override {
        PYBIND11_OVERRIDE_PURE(uint64_t, IFromGui, fromGuiGetDiskFreeSpace, dir);
    }
    uint64_t fromGuiClearCache(ECacheType cacheType) override {
        PYBIND11_OVERRIDE_PURE(uint64_t, IFromGui, fromGuiClearCache, cacheType);
    }
};

PYBIND11_MODULE(nlc_engine, m) {
    m.doc() = "Python bindings for NoLimitConnect core logic libraries";

    py::enum_<EAppState>(m, "EAppState")
        .value("invalid", eAppStateInvalid)
        .value("startup", eAppStateStartup)
        .value("shutdown", eAppStateShutdown)
        .value("sleep", eAppStateSleep)
        .value("wake", eAppStateWake)
        .value("pause", eAppStatePause)
        .value("resume", eAppStateResume)
        .value("permission_error", eAppStatePermissionErr)
        .value("inactive", eAppStateInactive)
        .value("max", eMaxAppState)
        .export_values();

    py::enum_<ECacheType>(m, "ECacheType")
        .value("none", eCacheTypeNone)
        .value("thumbnail", eCacheTypeThumbnail)
        .value("max", eMaxCacheType)
        .export_values();

    py::enum_<EFirewallTestType>(m, "EFirewallTestType")
        .value("url_connection_test", eFirewallTestUrlConnectionTest)
        .value("assume_no_firewall", eFirewallTestAssumeNoFirewall)
        .value("max", eMaxFirewallTestType)
        .export_values();

    py::enum_<EFriendState>(m, "EFriendState")
        .value("ignore", eFriendStateIgnore)
        .value("anonymous", eFriendStateAnonymous)
        .value("guest", eFriendStateGuest)
        .value("friend", eFriendStateFriend)
        .value("admin", eFriendStateAdmin)
        .export_values();

    py::enum_<EPluginAccess>(m, "EPluginAccess")
        .value("not_set", ePluginAccessNotSet)
        .value("ok", ePluginAccessOk)
        .value("locked", ePluginAccessLocked)
        .value("disabled", ePluginAccessDisabled)
        .value("ignored", ePluginAccessIgnored)
        .value("inactive", ePluginAccessInactive)
        .value("busy", ePluginAccessBusy)
        .value("requires_direct_connect", ePluginAccessRequiresDirectConnect)
        .value("requires_online", ePluginAccessRequiresOnline)
        .value("max", eMaxPluginAccess)
        .export_values();

    py::enum_<EPluginServerState>(m, "EPluginServerState")
        .value("disabled", ePluginServerStateDisabled)
        .value("started", ePluginServerStateStarted)
        .value("stopped", ePluginServerStateStopped)
        .value("max", eMaxPluginServerState)
        .export_values();

    py::enum_<EHackerLevel>(m, "EHackerLevel")
        .value("unknown", eHackerLevelUnknown)
        .value("suspicious", eHackerLevelSuspicious)
        .value("medium", eHackerLevelMedium)
        .value("severe", eHackerLevelSevere)
        .value("max", eMaxHackerLevel)
        .export_values();

    py::enum_<EHackerReason>(m, "EHackerReason")
        .value("unknown", eHackerReasonUnknown)
        .value("peer_name", eHackerReasonPeerName)
        .value("host_by_name", eHackerReasonHostByName)
        .value("no_host_ip_addr", eHackerReasonNoHostIpAddr)
        .value("host_ip_options", eHackerReasonHostIpOptions)
        .value("net_cmd_length", eHackerReasonNetCmdLength)
        .value("net_cmd_list_invalid", eHackerReasonNetCmdListInvalid)
        .value("net_srv_url_invalid", eHackerReasonNetSrvUrlInvalid)
        .value("net_srv_plugin_invalid", eHackerReasonNetSrvPluginInvalid)
        .value("net_srv_query_id_permission", eHackerReasonNetSrvQueryIdPermission)
        .value("http_attack", eHackerReasonHttpAttack)
        .value("pkt_online_id_me_from_my_ip", eHackerReasonPktOnlineIdMeFromMyIp)
        .value("pkt_online_id_me_from_another_ip", eHackerReasonPktOnlineIdMeFromAnotherIp)
        .value("pkt_ann_not_first_packet", eHackerReasonPktAnnNotFirstPacket)
        .value("pkt_hdr_invalid", eHackerReasonPktHdrInvalid)
        .value("invalid_pkt", eHackerReasonInvalidPkt)
        .value("access_denied", eHackerReasonAccessDenied)
        .value("lurker_did_not_send_pkt_ann", eHackerReasonLurkerDidNotSendPktAnn)
        .value("friend_request_from_ignored_user", eHackerReasonFriendRequestFromIgnoredUser)
        .value("max", eMaxHackerReason)
        .export_values();

    py::enum_<EHostType>(m, "EHostType")
        .value("unknown", eHostTypeUnknown)
        .value("connect_test", eHostTypeConnectTest)
        .value("network", eHostTypeNetwork)
        .value("peer_user", eHostTypePeerUser)
        .value("group", eHostTypeGroup)
        .value("chat_room", eHostTypeChatRoom)
        .value("random_connect", eHostTypeRandomConnect)
        .value("max", eMaxHostType)
        .export_values();

    py::enum_<EHostSearchStatus>(m, "EHostSearchStatus")
        .value("unknown", eHostSearchUnknown)
        .value("invalid_url", eHostSearchInvalidUrl)
        .value("query_id_in_progress", eHostSearchQueryIdInProgress)
        .value("query_id_success", eHostSearchQueryIdSuccess)
        .value("query_id_failed", eHostSearchQueryIdFailed)
        .value("connecting", eHostSearchConnecting)
        .value("handshaking", eHostSearchHandshaking)
        .value("handshake_timeout", eHostSearchHandshakeTimeout)
        .value("connect_success", eHostSearchConnectSuccess)
        .value("connect_failed", eHostSearchConnectFailed)
        .value("sending_search_request", eHostSearchSendingSearchRequest)
        .value("send_search_request_failed", eHostSearchSendSearchRequestFailed)
        .value("success", eHostSearchSuccess)
        .value("fail", eHostSearchFail)
        .value("fail_permission", eHostSearchFailPermission)
        .value("fail_connect_dropped", eHostSearchFailConnectDropped)
        .value("invalid_param", eHostSearchInvalidParam)
        .value("plugin_disabled", eHostSearchPluginDisabled)
        .value("no_matches", eHostSearchNoMatches)
        .value("completed", eHostSearchCompleted)
        .value("done", eHostSearchDone)
        .value("max", eMaxHostSearchStatus)
        .export_values();

    py::enum_<ESearchType>(m, "ESearchType")
        .value("none", eSearchNone)
        .value("chat_room_host", eSearchChatRoomHost)
        .value("group_host", eSearchGroupHost)
        .value("random_connect_host", eSearchRandomConnectHost)
        .value("max", eMaxSearchType)
        .export_values();

    py::enum_<EInternetStatus>(m, "EInternetStatus")
        .value("no_internet", eInternetNoInternet)
        .value("internet_available", eInternetInternetAvailable)
        .value("test_host_unavailable", eInternetTestHostUnavailable)
        .value("test_host_available", eInternetTestHostAvailable)
        .value("assume_direct_connect", eInternetAssumeDirectConnect)
        .value("can_direct_connect", eInternetCanDirectConnect)
        .value("requires_relay", eInternetRequiresRelay)
        .value("max", eMaxInternetStatus)
        .export_values();

    py::enum_<ENetAvailStatus>(m, "ENetAvailStatus")
        .value("no_internet", eNetAvailNoInternet)
        .value("host_avail", eNetAvailHostAvail)
        .value("p2p_avail", eNetAvailP2PAvail)
        .value("online_no_relay", eNetAvailOnlineButNoRelay)
        .value("full_online_with_relay", eNetAvailFullOnlineWithRelay)
        .value("full_online_direct", eNetAvailFullOnlineDirectConnect)
        .value("relay_group_host", eNetAvailRelayGroupHost)
        .value("direct_group_host", eNetAvailDirectGroupHost)
        .value("max", eMaxNetAvailStatus)
        .export_values();

    py::enum_<EJoinState>(m, "EJoinState")
        .value("none", eJoinStateNone)
        .value("sending", eJoinStateSending)
        .value("send_fail", eJoinStateSendFail)
        .value("send_acked", eJoinStateSendAcked)
        .value("join_requested", eJoinStateJoinRequested)
        .value("join_was_granted", eJoinStateJoinWasGranted)
        .value("join_is_granted", eJoinStateJoinIsGranted)
        .value("join_denied", eJoinStateJoinDenied)
        .value("join_leave_host", eJoinStateJoinLeaveHost)
        .value("max", eMaxJoinState)
        .export_values();

    py::enum_<ENetworkStateType>(m, "ENetworkStateType")
        .value("unknown", eNetworkStateTypeUnknown)
        .value("lost", eNetworkStateTypeLost)
        .value("avail", eNetworkStateTypeAvail)
        .value("test_connection", eNetworkStateTypeTestConnection)
        .value("online_direct", eNetworkStateTypeOnlineDirect)
        .value("wait_for_relay", eNetworkStateTypeWaitForRelay)
        .value("online_through_relay", eNetworkStateTypeOnlineThroughRelay)
        .value("no_internet_connection", eNetworkStateTypeNoInternetConnection)
        .value("failed_resolve_host_network", eNetworkStateTypeFailedResolveHostNetwork)
        .value("failed_resolve_host_group_list", eNetworkStateTypeFailedResolveHostGroupList)
        .value("failed_resolve_host_group", eNetworkStateTypeFailedResolveHostGroup)
        .value("ip_change", eNetworkStateTypeIpChange)
        .value("max", eMaxNetworkStateType)
        .export_values();

    py::enum_<EMSessionAction>(m, "EMSessionAction")
        .value("none", eMSessionActionNone)
        .value("chat_session_req", eMSessionActionChatSessionReq)
        .value("chat_session_accept", eMSessionActionChatSessionAccept)
        .value("chat_session_reject", eMSessionActionChatSessionReject)
        .value("offer", eMSessionActionOffer)
        .value("accept", eMSessionActionAccept)
        .value("reject", eMSessionActionReject)
        .value("hangup", eMSessionActionHangup)
        .value("max", eMaxMSessionAction)
        .export_values();

    py::enum_<EPluginMsgType>(m, "EPluginMsgType")
        .value("none", ePluginMsgNone)
        .value("connecting", ePluginMsgConnecting)
        .value("connect_failed", ePluginMsgConnectFailed)
        .value("retrieve_info", ePluginMsgRetrieveInfo)
        .value("retrieve_info_complete", ePluginMsgRetrieveInfoComplete)
        .value("retrieve_info_failed", ePluginMsgRetrieveInfoFailed)
        .value("downloading", ePluginMsgDownloading)
        .value("download_failed", ePluginMsgDownloadFailed)
        .value("download_complete", ePluginMsgDownloadComplete)
        .value("canceled", ePluginMsgCanceled)
        .value("permission_error", ePluginMsgPermissionError)
        .value("low_disk_space", ePluginMsgLowDiskSpace)
        .value("invalid_param", ePluginMsgInvalidParam)
        .value("max", eMaxPluginMsgType)
        .export_values();

    py::enum_<ECommErr>(m, "ECommErr")
        .value("none", eCommErrNone)
        .value("invalid_pkt", eCommErrInvalidPkt)
        .value("user_offline", eCommErrUserOffline)
        .value("search_text_too_short", eCommErrSearchTextToShort)
        .value("search_text_too_long", eCommErrSearchTextToLong)
        .value("search_no_match", eCommErrSearchNoMatch)
        .value("invalid_host_type", eCommErrInvalidHostType)
        .value("plugin_not_enabled", eCommErrPluginNotEnabled)
        .value("plugin_permission", eCommErrPluginPermission)
        .value("not_found", eCommErrNotFound)
        .value("invalid_param", eCommErrInvalidParam)
        .value("max", eMaxCommErr)
        .export_values();

    py::enum_<EPluginType>(m, "EPluginType")
        .value("invalid", ePluginTypeInvalid)
        .value("host_connect_test", ePluginTypeHostConnectTest)
        .value("host_network", ePluginTypeHostNetwork)
        .value("host_chat_room", ePluginTypeHostChatRoom)
        .value("host_group", ePluginTypeHostGroup)
        .value("host_random_connect", ePluginTypeHostRandomConnect)
        .value("host_peer_user", ePluginTypeHostPeerUser)
        .value("about_me_page_server", ePluginTypeAboutMePageServer)
        .value("messenger", ePluginTypeMessenger)
        .value("push_to_talk", ePluginTypePushToTalk)
        .value("person_file_xfer", ePluginTypePersonFileXfer)
        .value("cam_server", ePluginTypeCamServer)
        .value("file_share_server", ePluginTypeFileShareServer)
        .value("storyboard_server", ePluginTypeStoryboardServer)
        .value("truth_or_dare", ePluginTypeTruthOrDare)
        .value("video_chat", ePluginTypeVideoChat)
        .value("voice_phone", ePluginTypeVoicePhone)
        .value("friend_request", ePluginTypeFriendRequest)
        .value("client_connect_test", ePluginTypeClientConnectTest)
        .value("client_network", ePluginTypeClientNetwork)
        .value("client_chat_room", ePluginTypeClientChatRoom)
        .value("client_group", ePluginTypeClientGroup)
        .value("client_random_connect", ePluginTypeClientRandomConnect)
        .value("client_peer_user", ePluginTypeClientPeerUser)
        .value("about_me_page_client", ePluginTypeAboutMePageClient)
        .value("cam_client", ePluginTypeCamClient)
        .value("file_share_client", ePluginTypeFileShareClient)
        .value("storyboard_client", ePluginTypeStoryboardClient)
        .value("thumbnail", ePluginTypeThumbnail)
        .value("net_services", ePluginTypeNetServices)
        .value("library_server", ePluginTypeLibraryServer)
        .value("personal_recorder", ePluginTypePersonalRecorder)
        .value("max", eMaxPluginType)
        .export_values();

    py::enum_<EXferDirection>(m, "EXferDirection")
        .value("none", eXferDirectionNone)
        .value("rx", eXferDirectionRx)
        .value("tx", eXferDirectionTx)
        .value("max", eMaxXferDirection)
        .export_values();

    py::enum_<EXferError>(m, "EXferError")
        .value("none", eXferErrorNone)
        .value("disconnected", eXferErrorDisconnected)
        .value("permission", eXferErrorPermission)
        .value("file_not_found", eXferErrorFileNotFound)
        .value("canceled", eXferErrorCanceled)
        .value("bad_param", eXferErrorBadParam)
        .value("at_src", eXferErrorAtSrc)
        .value("busy", eXferErrorBusy)
        .value("already_downloading", eXferErrorAlreadyDownloading)
        .value("already_downloaded", eXferErrorAlreadyDownloaded)
        .value("already_uploading", eXferErrorAlreadyUploading)
        .value("file_create_error", eXferErrorFileCreateError)
        .value("file_open_append_error", eXferErrorFileOpenAppendError)
        .value("file_open_error", eXferErrorFileOpenError)
        .value("file_seek_error", eXferErrorFileSeekError)
        .value("file_read_error", eXferErrorFileReadError)
        .value("file_write_error", eXferErrorFileWriteError)
        .value("file_move_error", eXferErrorFileMoveError)
        .value("max", eMaxXferError)
        .export_values();

    py::enum_<EXferState>(m, "EXferState")
        .value("unknown", eXferStateUnknown)
        .value("upload_not_started", eXferStateUploadNotStarted)
        .value("waiting_offer_response", eXferStateWaitingOfferResponse)
        .value("in_upload_que", eXferStateInUploadQue)
        .value("begin_upload", eXferStateBeginUpload)
        .value("in_upload_xfer", eXferStateInUploadXfer)
        .value("completed_upload", eXferStateCompletedUpload)
        .value("user_canceled_upload", eXferStateUserCanceledUpload)
        .value("upload_offer_rejected", eXferStateUploadOfferRejected)
        .value("upload_error", eXferStateUploadError)
        .value("download_not_started", eXferStateDownloadNotStarted)
        .value("in_download_que", eXferStateInDownloadQue)
        .value("begin_download", eXferStateBeginDownload)
        .value("in_download_xfer", eXferStateInDownloadXfer)
        .value("completed_download", eXferStateCompletedDownload)
        .value("user_canceled_download", eXferStateUserCanceledDownload)
        .value("download_error", eXferStateDownloadError)
        .value("streaming", eXferStateStreaming)
        .value("stream_stopped", eXferStateStreamStopped)
        .value("max", eMaxXferState)
        .export_values();

    py::enum_<EXferAction>(m, "EXferAction")
        .value("none", eXferActionNone)
        .value("download", eXferActionDownload)
        .value("upload", eXferActionUpload)
        .value("cancel_xfer", eXferActionCancelXfer)
        .value("max", eMaxXferAction)
        .export_values();

    py::class_<VxGUID>(m, "VxGUID")
        .def(py::init<>())
        .def(py::init<const char*>(), py::arg("hex_or_online_id"))
        .def("is_valid", &VxGUID::isValid)
        .def("clear", &VxGUID::clear)
        .def("to_hex", [](const VxGUID& guid) { return guid.toHexString(); })
        .def("to_online_id", [](const VxGUID& guid) { return guid.toOnlineIdString(); })
        .def("from_hex", &VxGUID::fromVxGUIDHexString, py::arg("hex_value"))
        .def("from_online_id", &VxGUID::fromOnlineIdString, py::arg("online_id"))
        .def("__repr__", [](const VxGUID& guid) {
            return "VxGUID('" + guid.toHexString() + "')";
        });

    py::class_<VxNetIdent>(m, "VxNetIdent")
        .def(py::init<>())
        .def("__eq__", [](const VxNetIdent& lhs, const VxNetIdent& rhs) { return lhs == rhs; })
        .def("__ne__", [](const VxNetIdent& lhs, const VxNetIdent& rhs) { return lhs != rhs; })
        .def("is_valid", &VxNetIdent::isValidNetIdent)
        .def("is_valid_net_ident", &VxNetIdent::isValidNetIdent)
        .def("is_myself", &VxNetIdent::isMyself)
        .def("is_online", &VxNetIdent::isOnline)
        .def("is_direct_connected", &VxNetIdent::isDirectConnected)
        .def("is_relayed", &VxNetIdent::isRelayed)
        .def("can_direct_connect_to_user", &VxNetIdent::canDirectConnectToUser)
        .def("clear_is_admin_avail", &VxNetIdent::clearIsAdminAvail)
        .def("set_online_name", [](VxNetIdent& ident, const std::string& name) {
            ident.setOnlineName(name.c_str());
        })
        .def("get_online_name", [](VxNetIdent& ident) {
            return std::string(ident.getOnlineName());
        })
        .def("set_online_description", [](VxNetIdent& ident, const std::string& desc) {
            ident.setOnlineDescription(desc.c_str());
        })
        .def("get_online_description", [](VxNetIdent& ident) {
            return std::string(ident.getOnlineDescription());
        })
        .def("set_my_friendship_to_him", &VxNetIdent::setMyFriendshipToHim, py::arg("friend_state"))
        .def("get_my_friendship_to_him", &VxNetIdent::getMyFriendshipToHim)
        .def("set_his_friendship_to_me", &VxNetIdent::setHisFriendshipToMe, py::arg("friend_state"))
        .def("get_his_friendship_to_me", &VxNetIdent::getHisFriendshipToMe)
        .def("is_ignored", &VxNetIdent::isIgnored)
        .def("is_anonymous", &VxNetIdent::isAnonymous)
        .def("is_guest", &VxNetIdent::isGuest)
        .def("is_friend", &VxNetIdent::isFriend)
        .def("is_administrator", &VxNetIdent::isAdministrator)
        .def("make_ignored", &VxNetIdent::makeIgnored)
        .def("make_anonymous", &VxNetIdent::makeAnonymous)
        .def("make_guest", &VxNetIdent::makeGuest)
        .def("make_friend", &VxNetIdent::makeFriend)
        .def("make_administrator", &VxNetIdent::makeAdministrator)
        .def("wants_to_be_friend", &VxNetIdent::wantsToBeFriend)
        .def("wants_to_be_administrator", &VxNetIdent::wantsToBeAdministrator)
        .def("upgrade_to_guest_friendship", &VxNetIdent::upgradeToGuestFriendship)
        .def("reverse_permissions", &VxNetIdent::reversePermissions)
        .def("set_is_admin_avail", &VxNetIdent::setIsAdminAvail, py::arg("host_type"), py::arg("is_admin_avail"))
        .def("get_is_admin_avail", &VxNetIdent::getIsAdminAvail, py::arg("host_type"))
        .def("get_admin_avail_flags", &VxNetIdent::getAdminAvailFlags)
        .def("clear_is_joined", &VxNetIdent::clearIsJoined)
        .def("set_is_joined", &VxNetIdent::setIsJoined, py::arg("host_type"), py::arg("is_joined"))
        .def("get_is_joined", &VxNetIdent::getIsJoined, py::arg("host_type"))
        .def("is_joined_any", &VxNetIdent::isJoinedAny)
        .def("set_plugin_permissions_to_default_values", &VxNetIdent::setPluginPermissionsToDefaultValues)
        .def("get_plugin_permissions_bytes", [](VxNetIdent& ident) {
            const uint8_t* permissions = ident.getPluginPermissions();
            return py::bytes(reinterpret_cast<const char*>(permissions), PERMISSION_ARRAY_SIZE);
        })
        .def("set_plugin_permissions_bytes", [](VxNetIdent& ident, py::bytes permissions_blob) {
            std::string permissions = permissions_blob;
            if (permissions.size() != PERMISSION_ARRAY_SIZE) {
                throw py::value_error("permissions_blob must be exactly 24 bytes (PERMISSION_ARRAY_SIZE)");
            }
            ident.setPluginPermissions(reinterpret_cast<uint8_t*>(permissions.data()));
        }, py::arg("permissions_blob"))
        .def("set_plugin_permission", &VxNetIdent::setPluginPermission, py::arg("plugin_type"), py::arg("friend_state"))
        .def("is_plugin_enabled", &VxNetIdent::isPluginEnabled, py::arg("plugin_type"))
        .def("get_plugin_permission", &VxNetIdent::getPluginPermission, py::arg("plugin_type"))
        .def("get_his_access_permission_from_me", &VxNetIdent::getHisAccessPermissionFromMe, py::arg("plugin_type"))
        .def("is_his_access_allowed_from_me", &VxNetIdent::isHisAccessAllowedFromMe, py::arg("plugin_type"))
        .def("get_my_access_permission_from_him", &VxNetIdent::getMyAccessPermissionFromHim, py::arg("plugin_type"))
        .def("is_my_access_allowed_from_him", &VxNetIdent::isMyAccessAllowedFromHim, py::arg("plugin_type"))
        .def("get_plugin_access_state", &VxNetIdent::getPluginAccessState, py::arg("plugin_type"), py::arg("friend_state"))
        .def("set_ping_time_ms", &VxNetIdent::setPingTimeMs, py::arg("ping_ms"))
        .def("get_ping_time_ms", &VxNetIdent::getPingTimeMs)
        .def("set_last_session_time_ms", &VxNetIdent::setLastSessionTimeMs, py::arg("session_time_ms"))
        .def("get_last_session_time_ms", &VxNetIdent::getLastSessionTimeMs)
        .def("set_last_groupie_info_modified_time_ms", &VxNetIdent::setLastGroupieInfoModifiedTimeMs, py::arg("modified_time_ms"))
        .def("get_last_groupie_info_modified_time_ms", &VxNetIdent::getLastGroupieInfoModifiedTimeMs)
        .def("set_truth_accept_count", &VxNetIdent::setTruthAcceptCount, py::arg("count"))
        .def("get_truth_accept_count", &VxNetIdent::getTruthAcceptCount)
        .def("set_truth_reject_count", &VxNetIdent::setTruthRejectCount, py::arg("count"))
        .def("get_truth_reject_count", &VxNetIdent::getTruthRejectCount)
        .def("set_dare_accept_count", &VxNetIdent::setDareAcceptCount, py::arg("count"))
        .def("get_dare_accept_count", &VxNetIdent::getDareAcceptCount)
        .def("set_dare_reject_count", &VxNetIdent::setDareRejectCount, py::arg("count"))
        .def("get_dare_reject_count", &VxNetIdent::getDareRejectCount)
        .def("is_vx_net_ident_match", &VxNetIdent::isVxNetIdentMatch, py::arg("other_ident"))
        .def("describe_his_friendship_to_me", [](const VxNetIdent& ident) { return std::string(ident.describeHisFriendshipToMe()); })
        .def("describe_my_friendship_to_him", [](const VxNetIdent& ident) { return std::string(ident.describeMyFriendshipToHim()); })
        .def("is_online_name_valid", &VxNetIdent::isOnlineNameValid)
        .def("can_request_join", &VxNetIdent::canRequestJoin, py::arg("host_type"))
        .def("can_join_immediate", &VxNetIdent::canJoinImmediate, py::arg("host_type"))
        .def("describe_user", &VxNetIdent::describeUser)
        .def("user_is_hosting", &VxNetIdent::userIsHosting, py::arg("host_type"))
        .def("requires_an_open_port", &VxNetIdent::requiresAnOpenPort)
        .def("debug_dump_ident", &VxNetIdent::debugDumpIdent)
        .def("dump_permissions", &VxNetIdent::dumpPermissions, py::arg("just_hosts") = false)
        .def("get_my_online_id", [](VxNetIdent& ident) {
            return ident.getMyOnlineId();
        })
        .def("set_my_online_id", [](VxNetIdent& ident, VxGUID& online_id) {
            ident.setMyOnlineId(online_id);
        })
        .def("add_to_blob_bytes", [](VxNetIdent& ident) {
            PktBlobEntry blob;
            if (!ident.addToBlob(blob)) {
                throw py::value_error("VxNetIdent.addToBlob failed");
            }
            const int blob_len = blob.getBlobLen();
            return py::bytes(reinterpret_cast<const char*>(blob.getBlobData()), blob_len);
        })
        .def("extract_from_blob_bytes", [](VxNetIdent& ident, py::bytes blob_bytes) {
            std::string raw = blob_bytes;
            PktBlobEntry blob;
            if (!blob.setBlobData(reinterpret_cast<uint8_t*>(raw.data()), static_cast<int>(raw.size()))) {
                throw py::value_error("invalid blob bytes for PktBlobEntry");
            }
            if (!ident.extractFromBlob(blob)) {
                throw py::value_error("VxNetIdent.extractFromBlob failed");
            }
        }, py::arg("blob_bytes"));

    py::class_<FileInfo>(m, "FileInfo")
        .def(py::init<>())
        .def("is_valid", &FileInfo::isValid, py::arg("include_hash_valid") = true)
        .def("set_file_name_and_path", &FileInfo::setFileNameAndPath, py::arg("file_path"))
        .def("get_local_full_file_name", [](FileInfo& f) { return f.getLocalFullFileName(); })
        .def("get_remote_file_name", &FileInfo::getRemoteFileName)
        .def("set_is_in_library", &FileInfo::setIsInLibrary, py::arg("in_library"))
        .def("get_is_in_library", &FileInfo::getIsInLibrary)
        .def("set_is_shared_file", &FileInfo::setIsSharedFile, py::arg("is_shared"))
        .def("get_is_shared_file", &FileInfo::getIsSharedFile)
        .def("set_is_stream", &FileInfo::setIsStream, py::arg("is_stream"))
        .def("is_stream", &FileInfo::isStream)
        .def("set_online_id", &FileInfo::setOnlineId, py::arg("online_id"))
        .def("get_online_id", &FileInfo::getOnlineId, py::return_value_policy::reference_internal)
        .def("set_asset_id", &FileInfo::setAssetId, py::arg("asset_id"))
        .def("get_asset_id", &FileInfo::getAssetId, py::return_value_policy::reference_internal)
        .def("set_thumb_id", &FileInfo::setThumbId, py::arg("thumb_id"))
        .def("get_thumb_id", &FileInfo::getThumbId, py::return_value_policy::reference_internal)
        .def("set_file_time", &FileInfo::setFileTime, py::arg("file_time"))
        .def("get_file_time", &FileInfo::getFileTime)
        .def("set_xfer_session_id", &FileInfo::setXferSessionId, py::arg("session_id"))
        .def("get_xfer_session_id", &FileInfo::getXferSessionId, py::return_value_policy::reference_internal)
        .def("initialize_new_xfer_session_id", &FileInfo::initializeNewXferSessionId, py::return_value_policy::reference_internal);

    py::class_<HostedInfo>(m, "HostedInfo")
        .def(py::init<>())
        .def("is_host_invite_valid", &HostedInfo::isHostInviteValid)
        .def("set_host_type", &HostedInfo::setHostType, py::arg("host_type"))
        .def("get_host_type", &HostedInfo::getHostType)
        .def("set_host_invite_url", &HostedInfo::setHostInviteUrl, py::arg("host_url"))
        .def("get_host_invite_url", [](HostedInfo& info) { return info.getHostInviteUrl(); })
        .def("set_host_title", &HostedInfo::setHostTitle, py::arg("host_title"))
        .def("get_host_title", [](HostedInfo& info) { return info.getHostTitle(); })
        .def("set_host_description", &HostedInfo::setHostDescription, py::arg("host_description"))
        .def("get_host_description", [](HostedInfo& info) { return info.getHostDescription(); })
        .def("set_is_favorite", &HostedInfo::setIsFavorite, py::arg("is_favorite"))
        .def("get_is_favorite", &HostedInfo::getIsFavorite)
        .def("set_connected_timestamp", &HostedInfo::setConnectedTimestamp, py::arg("timestamp_ms"))
        .def("get_connected_timestamp", &HostedInfo::getConnectedTimestamp)
        .def("set_joined_timestamp", &HostedInfo::setJoinedTimestamp, py::arg("timestamp_ms"))
        .def("get_joined_timestamp", &HostedInfo::getJoinedTimestamp)
        .def("set_admin_online_id", &HostedInfo::setAdminOnlineId, py::arg("online_id"))
        .def("get_admin_online_id", &HostedInfo::getAdminOnlineId, py::return_value_policy::reference_internal)
        .def("set_thumb_id", &HostedInfo::setThumbId, py::arg("thumb_id"))
        .def("get_thumb_id", &HostedInfo::getThumbId, py::return_value_policy::reference_internal)
        .def("is_valid_for_gui", &HostedInfo::isValidForGui);

    py::class_<SearchParams>(m, "SearchParams")
        .def(py::init<>())
        .def("set_host_type", &SearchParams::setHostType, py::arg("host_type"))
        .def("get_host_type", &SearchParams::getHostType)
        .def("set_search_type", &SearchParams::setSearchType, py::arg("search_type"))
        .def("get_search_type", &SearchParams::getSearchType)
        .def("set_search_session_id", &SearchParams::setSearchSessionId, py::arg("session_id"))
        .def("get_search_session_id", &SearchParams::getSearchSessionId, py::return_value_policy::reference_internal)
        .def("create_new_session_id", &SearchParams::createNewSessionId)
        .def("update_search_start_time", &SearchParams::updateSearchStartTime)
        .def("set_search_ident_guid", &SearchParams::setSearchIdentGuid, py::arg("ident_guid"))
        .def("get_search_ident_guid", &SearchParams::getSearchIndentGuid, py::return_value_policy::reference_internal)
        .def("set_search_url", &SearchParams::setSearchUrl, py::arg("url"))
        .def("get_search_url", [](SearchParams& p) { return p.getSearchUrl(); })
        .def("set_search_text", &SearchParams::setSearchText, py::arg("text"))
        .def("get_search_text", [](SearchParams& p) { return p.getSearchText(); })
        .def("set_search_list_all", &SearchParams::setSearchListAll, py::arg("list_all"))
        .def("get_search_list_all", &SearchParams::getSearchListAll);

    py::class_<NetHostSetting>(m, "NetHostSetting")
        .def(py::init<>())
        .def("set_net_host_setting_name", &NetHostSetting::setNetHostSettingName, py::arg("name"))
        .def("get_net_host_setting_name", [](NetHostSetting& s) { return s.getNetHostSettingName(); })
        .def("set_network_key", &NetHostSetting::setNetworkKey, py::arg("network_key"))
        .def("get_network_key", [](NetHostSetting& s) { return s.getNetworkKey(); })
        .def("set_network_host_url", &NetHostSetting::setNetworkHostUrl, py::arg("url"))
        .def("get_network_host_url", [](NetHostSetting& s) { return s.getNetworkHostUrl(); })
        .def("set_connect_test_url", &NetHostSetting::setConnectTestUrl, py::arg("url"))
        .def("get_connect_test_url", [](NetHostSetting& s) { return s.getConnectTestUrl(); })
        .def("set_random_connect_url", &NetHostSetting::setRandomConnectUrl, py::arg("url"))
        .def("get_random_connect_url", [](NetHostSetting& s) { return s.getRandomConnectUrl(); })
        .def("set_group_host_url", &NetHostSetting::setGroupHostUrl, py::arg("url"))
        .def("get_group_host_url", [](NetHostSetting& s) { return s.getGroupHostUrl(); })
        .def("set_chat_room_host_url", &NetHostSetting::setChatRoomHostUrl, py::arg("url"))
        .def("get_chat_room_host_url", [](NetHostSetting& s) { return s.getChatRoomHostUrl(); })
        .def("set_user_specified_extern_ip_addr", &NetHostSetting::setUserSpecifiedExternIpAddr, py::arg("ip_addr"))
        .def("get_user_specified_extern_ip_addr", [](NetHostSetting& s) { return s.getUserSpecifiedExternIpAddr(); })
        .def("set_use_upnp_port_forward", &NetHostSetting::setUseUpnpPortForward, py::arg("use_upnp"))
        .def("get_use_upnp_port_forward", &NetHostSetting::getUseUpnpPortForward)
        .def("set_tcp_port", &NetHostSetting::setTcpPort, py::arg("tcp_port"))
        .def("get_tcp_port", &NetHostSetting::getTcpPort)
        .def("set_firewall_test_type", &NetHostSetting::setFirewallTestType, py::arg("firewall_test_type"))
        .def("get_firewall_test_type", &NetHostSetting::getFirewallTestType)
        .def("set_use_ipv6", &NetHostSetting::setUseIpv6, py::arg("use_ipv6"))
        .def("get_use_ipv6", &NetHostSetting::getUseIpv6)
        .def("reset_to_default_settings", &NetHostSetting::resetToDefaultSettings, py::arg("ipv6"));

    py::class_<NetSettings, NetHostSetting>(m, "NetSettings")
        .def(py::init<>())
        .def("set_my_multicast_port", &NetSettings::setMyMulticastPort, py::arg("port"))
        .def("get_my_multicast_port", &NetSettings::getMyMulticastPort)
        .def("set_user_relay_permission_count", &NetSettings::setUserRelayPermissionCount, py::arg("count"))
        .def("get_user_relay_permission_count", &NetSettings::getUserRelayPermissionCount)
        .def("set_system_relay_permission_count", &NetSettings::setSystemRelayPermissionCount, py::arg("count"))
        .def("get_system_relay_permission_count", &NetSettings::getSystemRelayPermissionCount)
        .def("set_allow_user_location", &NetSettings::setAllowUserLocation, py::arg("enable"))
        .def("get_allow_user_location", &NetSettings::getAllowUserLocation)
        .def("set_multicast_enable", &NetSettings::setMulticastEnable, py::arg("enable"))
        .def("get_multicast_enable", &NetSettings::getMulticastEnable)
        .def("set_allow_multicast_broadcast", &NetSettings::setAllowMulticastBroadcast, py::arg("enable"))
        .def("get_allow_multicast_broadcast", &NetSettings::getAllowMulticastBroadcast);

        py::class_<PythonIToGuiAdapter, std::shared_ptr<PythonIToGuiAdapter>>(m, "IToGuiAdapter")
           .def(py::init<>())
           .def("set_default_handler", &PythonIToGuiAdapter::setDefaultHandler, py::arg("handler"))
           .def("clear_default_handler", &PythonIToGuiAdapter::clearDefaultHandler)
           .def("register_handler", &PythonIToGuiAdapter::registerHandler, py::arg("method_name"), py::arg("handler"))
           .def("clear_handlers", &PythonIToGuiAdapter::clearHandlers, py::arg("method_name"))
           .def("clear_all_handlers", &PythonIToGuiAdapter::clearAllHandlers)
           // Helper methods allow testing callback behavior from Python before full native wiring.
           .def("emit_status_message", &PythonIToGuiAdapter::toGuiStatusMessage, py::arg("message"))
           .def("emit_plugin_msg",
               &PythonIToGuiAdapter::toGuiPluginMsg,
               py::arg("plugin_type"),
               py::arg("online_id"),
               py::arg("msg_type"),
               py::arg("param_msg") = "")
           .def("emit_plugin_comm_error",
               &PythonIToGuiAdapter::toGuiPluginCommError,
               py::arg("plugin_type"),
               py::arg("online_id"),
               py::arg("msg_type"),
               py::arg("comm_error"))
           .def("emit_plugin_status",
               &PythonIToGuiAdapter::toGuiPluginStatus,
               py::arg("plugin_type"),
               py::arg("status_type"),
               py::arg("status_value"))
           .def("emit_host_search_status",
               &PythonIToGuiAdapter::toGuiHostSearchStatus,
               py::arg("host_type"),
               py::arg("session_id"),
               py::arg("search_status"),
               py::arg("comm_error") = eCommErrNone,
               py::arg("msg") = "")
           .def("emit_host_search_result",
               &PythonIToGuiAdapter::toGuiHostSearchResult,
               py::arg("host_type"),
               py::arg("session_id"),
               py::arg("hosted_info"))
           .def("emit_host_search_complete",
               &PythonIToGuiAdapter::toGuiHostSearchComplete,
               py::arg("host_type"),
               py::arg("session_id"))
           .def("emit_groupie_search_status",
               &PythonIToGuiAdapter::toGuiGroupieSearchStatus,
               py::arg("host_type"),
               py::arg("session_id"),
               py::arg("search_status"),
               py::arg("comm_error") = eCommErrNone,
               py::arg("msg") = "")
           .def("emit_groupie_search_complete",
               &PythonIToGuiAdapter::toGuiGroupieSearchComplete,
               py::arg("host_type"),
               py::arg("session_id"))
           .def("emit_file_xfer_state",
               &PythonIToGuiAdapter::toGuiFileXferState,
               py::arg("plugin_type"),
               py::arg("session_id"),
               py::arg("xfer_direction"),
               py::arg("xfer_state"),
               py::arg("xfer_error"),
               py::arg("param1"))
           .def("emit_net_available_status", &PythonIToGuiAdapter::toGuiNetAvailableStatus, py::arg("status"));

    py::class_<IFromGui, PyIFromGui>(m, "IFromGui")
        .def("from_gui_app_startup", &IFromGui::fromGuiAppStartup,
             py::arg("assets_dir"), py::arg("root_data_dir"), py::arg("from_thread") = false,
             py::call_guard<py::gil_scoped_release>())
        .def("from_gui_set_user_specific_dir", &IFromGui::fromGuiSetUserSpecificDir,
             py::arg("user_dir"), py::arg("from_thread") = false)
        .def("from_gui_set_user_xfer_dir", &IFromGui::fromGuiSetUserXferDir,
             py::arg("user_download_dir"), py::arg("from_thread") = false)
           .def("from_gui_user_logged_on", [](IFromGui& self, VxNetIdent& net_ident, bool from_thread) {
                 self.fromGuiUserLoggedOn(&net_ident, from_thread);
              },
              py::arg("net_ident"), py::arg("from_thread") = false)
        .def("from_gui_app_shutdown", &IFromGui::fromGuiAppShutdown)
        .def("from_gui_delete_user", &IFromGui::fromGuiDeleteUser, py::arg("online_id"))
        .def("from_gui_get_disk_free_space", &IFromGui::fromGuiGetDiskFreeSpace, py::arg("dir") = nullptr)
        .def("from_gui_clear_cache", &IFromGui::fromGuiClearCache, py::arg("cache_type"))
        .def("from_gui_set_relay_settings", &IFromGui::fromGuiSetRelaySettings,
             py::arg("user_relay_max_count"), py::arg("system_relay_max_count"))
        .def("from_gui_run_is_port_open_test", &IFromGui::fromGuiRunIsPortOpenTest, py::arg("port"))
        .def("from_gui_set_ident_has_text_offers", [](IFromGui& self, VxGUID& online_id, bool has_text_offers) {
                self.fromGuiSetIdentHasTextOffers(online_id, has_text_offers);
            },
            py::arg("online_id"), py::arg("has_text_offers"))
        .def("from_gui_change_my_friendship_to_him", [](IFromGui& self, VxGUID& online_id, EFriendState my_friendship_to_him, EFriendState his_friendship_to_me) {
                return self.fromGuiChangeMyFriendshipToHim(online_id, my_friendship_to_him, his_friendship_to_me);
            },
            py::arg("online_id"), py::arg("my_friendship_to_him"), py::arg("his_friendship_to_me"))
           .def("from_gui_online_name_changed", &IFromGui::fromGuiOnlineNameChanged,
               py::arg("new_online_name"))
           .def("from_gui_mood_message_changed", &IFromGui::fromGuiMoodMessageChanged,
               py::arg("new_mood_message"))
           .def("from_gui_set_user_has_profile_picture", &IFromGui::fromGuiSetUserHasProfilePicture,
               py::arg("have_profile_picture"))
        .def("from_gui_set_net_settings", &IFromGui::fromGuiSetNetSettings, py::arg("net_settings"))
        .def("from_gui_get_net_settings", &IFromGui::fromGuiGetNetSettings, py::arg("net_settings"))
        .def("from_gui_apply_net_host_settings", &IFromGui::fromGuiApplyNetHostSettings, py::arg("net_host_settings"))
        .def("from_gui_update_my_ident", [](IFromGui& self, VxNetIdent& net_ident, bool permission_and_stats_only) {
                self.fromGuiUpdateMyIdent(&net_ident, permission_and_stats_only);
            },
            py::arg("net_ident"), py::arg("permission_and_stats_only") = false)
        .def("from_gui_query_my_ident", [](IFromGui& self, VxNetIdent& out_ident) {
                self.fromGuiQueryMyIdent(&out_ident);
            },
            py::arg("out_ident"))
        .def("query_my_ident", [](IFromGui& self) {
                VxNetIdent ident;
                self.fromGuiQueryMyIdent(&ident);
                return ident;
            })
        .def("from_gui_get_random_tcp_port", &IFromGui::fromGuiGetRandomTcpPort)
        .def("from_gui_get_internet_status", &IFromGui::fromGuiGetInternetStatus)
        .def("from_gui_get_net_avail_status", &IFromGui::fromGuiGetNetAvailStatus)
           .def("from_gui_set_plugin_permission", &IFromGui::fromGuiSetPluginPermission,
               py::arg("plugin_type"), py::arg("friend_state"))
           .def("from_gui_get_plugin_permission", &IFromGui::fromGuiGetPluginPermission,
               py::arg("plugin_type"))
           .def("from_gui_get_plugin_server_state", &IFromGui::fromGuiGetPluginServerState,
               py::arg("plugin_type"))
           .def("from_gui_start_plugin_session", &IFromGui::fromGuiStartPluginSession,
               py::arg("plugin_type"), py::arg("online_id"), py::arg("local_session_id") = VxGUID::nullVxGUID())
           .def("from_gui_stop_plugin_session", &IFromGui::fromGuiStopPluginSession,
               py::arg("plugin_type"), py::arg("online_id"), py::arg("local_session_id") = VxGUID::nullVxGUID())
           .def("from_gui_is_plugin_in_session", [](IFromGui& self, EPluginType plugin_type, VxGUID& online_id, VxGUID local_session_id) {
                 return self.fromGuiIsPluginInSession(plugin_type, online_id, local_session_id);
              },
              py::arg("plugin_type"), py::arg("online_id"), py::arg("local_session_id") = VxGUID::nullVxGUID())
           .def("from_gui_is_plugin_in_session_simple", [](IFromGui& self, EPluginType plugin_type) {
                 VxGUID online_id = VxGUID::nullVxGUID();
                 return self.fromGuiIsPluginInSession(plugin_type, online_id, VxGUID::nullVxGUID());
              },
              py::arg("plugin_type"))
           .def("from_gui_push_to_talk", [](IFromGui& self, VxGUID& online_id, bool enable_talk) {
                 return self.fromGuiPushToTalk(online_id, enable_talk);
              },
              py::arg("online_id"), py::arg("enable_talk"))
           .def("from_gui_get_joined_list_count", &IFromGui::fromGuiGetJoinedListCount,
               py::arg("plugin_type"))
           .def("from_gui_get_announced_host_count", &IFromGui::fromGuiGetAnnouncedHostCount,
               py::arg("host_type"))
           .def("from_gui_query_default_url", &IFromGui::fromGuiQueryDefaultUrl,
               py::arg("host_type"), py::arg("ignore_myself") = false)
           .def("from_gui_set_default_url", [](IFromGui& self, EHostType host_type, const std::string& host_url) {
                 std::string mutable_host_url = host_url;
                 return self.fromGuiSetDefaultUrl(host_type, mutable_host_url);
              },
              py::arg("host_type"), py::arg("host_url"))
           .def("from_gui_get_node_url", [](IFromGui& self) {
                 std::string node_url;
                 self.fromGuiGetNodeUrl(node_url);
                 return node_url;
              })
              .def("from_gui_query_identity_by_url", [](IFromGui& self, const std::string& url, bool request_identity_if_unknown) {
                      std::string mutable_url = url;
                      VxNetIdent ident;
                      bool found = self.fromGuiQueryIdentity(mutable_url, ident, request_identity_if_unknown);
                      return py::make_tuple(found, mutable_url, ident);
                  },
                  py::arg("url"), py::arg("request_identity_if_unknown") = false)
              .def("from_gui_query_identity_by_online_id", [](IFromGui& self, const VxGUID& online_id) {
                      VxNetIdent ident;
                      VxGUID mutable_online_id = online_id;
                      bool found = self.fromGuiQueryIdentity(mutable_online_id, ident);
                      return py::make_tuple(found, ident);
                  },
                  py::arg("online_id"))
              .def("from_gui_query_join_state", [](IFromGui& self, EHostType host_type, VxNetIdent& net_ident) {
                      return self.fromGuiQueryJoinState(host_type, net_ident);
                  },
                  py::arg("host_type"), py::arg("net_ident"))
           .def("from_gui_update_plugin_permission", &IFromGui::fromGuiUpdatePluginPermission,
               py::arg("plugin_type"), py::arg("plugin_permission"))

        // Backward-compatible aliases retained for existing Python code.
        .def("app_startup", &IFromGui::fromGuiAppStartup,
             py::arg("assets_dir"), py::arg("root_data_dir"), py::arg("from_thread") = false,
             py::call_guard<py::gil_scoped_release>())
        .def("set_user_dir", &IFromGui::fromGuiSetUserSpecificDir,
             py::arg("user_dir"), py::arg("from_thread") = false)
        .def("set_xfer_dir", &IFromGui::fromGuiSetUserXferDir,
             py::arg("user_download_dir"), py::arg("from_thread") = false)
        .def("shutdown", &IFromGui::fromGuiAppShutdown)
        .def("delete_user", &IFromGui::fromGuiDeleteUser, py::arg("online_id"))
        .def("get_free_space", &IFromGui::fromGuiGetDiskFreeSpace, py::arg("dir") = nullptr);

    m.def("set_hack_report_handler", [](py::object handler) {
        if (handler.is_none()) {
            g_pythonHackReportCallback.reset();
            VxSetHackReportCallback(nullptr);
            return;
        }

        if (!PyCallable_Check(handler.ptr())) {
            throw py::type_error("handler must be callable or None");
        }

        if (!g_pythonHackReportCallback) {
            g_pythonHackReportCallback = std::make_unique<PythonHackReportCallback>();
        }

        g_pythonHackReportCallback->setHandler(std::move(handler));
        VxSetHackReportCallback(g_pythonHackReportCallback.get());
    }, py::arg("handler"), "Register a Python callback for hack offense reports.");

    m.def("clear_hack_report_handler", []() {
        g_pythonHackReportCallback.reset();
        VxSetHackReportCallback(nullptr);
    }, "Clear the active Python hack report callback.");

    m.def("describe_hacker_level", [](int hacker_level) {
        return std::string(DescribeHackerLevel(static_cast<EHackerLevel>(hacker_level)));
    });
    m.def("describe_hacker_reason", [](int hacker_reason) {
        return std::string(DescribeHackerReason(static_cast<EHackerReason>(hacker_reason)));
    });
    m.def("describe_net_avail_status", [](int net_avail_status) {
        return std::string(DescribeNetAvailStatus(static_cast<ENetAvailStatus>(net_avail_status)));
    });
    m.def("describe_network_state", [](int network_state) {
        return std::string(DescribeNetworkState(static_cast<ENetworkStateType>(network_state)));
    });
    m.def("describe_xfer_direction", [](int xfer_direction) {
        return std::string(DescribeXferDirection(static_cast<EXferDirection>(xfer_direction)));
    });
    m.def("describe_xfer_state", [](int xfer_state) {
        return std::string(DescribeXferState(static_cast<EXferState>(xfer_state)));
    });
    m.def("describe_xfer_error", [](int xfer_error) {
        return std::string(DescribeXferError(static_cast<EXferError>(xfer_error)));
    });
    m.def("describe_xfer_action", [](int xfer_action) {
        return std::string(DescribeXferAction(static_cast<EXferAction>(xfer_action)));
    });
    m.def("describe_comm_error", [](int comm_error) {
        return std::string(DescribeCommError(static_cast<ECommErr>(comm_error)));
    });
    m.def("describe_host_search_status", [](int search_status) {
        return std::string(DescribeHostSearchStatus(static_cast<EHostSearchStatus>(search_status)));
    });
    m.def("describe_host_type", [](int host_type) {
        return std::string(DescribeHostType(static_cast<EHostType>(host_type)));
    });
    m.def("describe_join_state", [](int join_state) {
        return std::string(DescribeJoinState(static_cast<EJoinState>(join_state)));
    });
    m.def("describe_plugin_type", [](int plugin_type) {
        return std::string(DescribePluginType(static_cast<EPluginType>(plugin_type)));
    });
}
