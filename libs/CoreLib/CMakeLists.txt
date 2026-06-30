
cmake_minimum_required (VERSION 3.22.1)

project(CoreLib, LANGUAGES CXX)

set(CoreLib_HEADERS)
list(APPEND CoreLib_HEADERS

    AppErr.h
    AppVersion.h.in
    AssetDefs.h
    AudioCallbackSpaceAvailable.h
    BinaryBlob.h
    Blowfish.h
    ConnectId.h
    DbBase.h
    DbDataset.h
    DbQueryData.h
    GroupieId.h
    HostedId.h
    InetAddress.h
    InetAddressParse.h
    Logger.h
    ObjectCommon.h
    OsDetect.h
    PktBlobEntry.h
    PluginId.h
    SHA1.h
    Sha1ClientInfo.h
    Sha1GeneratorMgr.h
    Sha1Info.h
    sqlite3.h
    sqlitedataset.h
    StdOutRedirect.h
    StringListBinary.h

    VxAppInfo.h
    Invite.h
    VFile.h
    VirtFileMgr.h
    VxAudioFormat.h
    VxBuffer.h
    VxChop.h
    VxChopMap.h
    VxColorUtil.h
    VxCrypto.h
    VxDebug.h
    VxDefs.h
    VxFileCopier.h
    VxFileCrc32.h
    VxFileInfo.h
    VxFileIsTypeFunctions.h
    VxFileLists.h
    VxFileShredder.h
    VxFileShredderDb.h
    VxFileUtil.h
    VxGUID.h
    VxGUIDList.h
    VxGuidPairList.h
    VxGuidPairTimeList.h
    VxGlobals.h
    VxLinuxOnly.h
    VxMathDef.h
    VxMathVec3f.h
    VxMemoryUsage.h
    VxMemUtils.h
    VxMutex.h
    VxParse.h
    VxProfile.h
    VxPtopUrl.h

    VxResolveHost.h
    VxSemaphore.h
    VxSettings.h
    VxSha1Hash.h
    VxShortcut.h
    VxSktUtil.h
    VxStringUtils.h
    VxTextStreamReader.h
    VxThread.h
    VxTimedCallback.h
    VxTimeUtil.h
    VxTime.h
    VxElapseTimer.h
    VxValuePair.h
    VxXferDefs.h
    VxXml.h
    VxUrl.h
    VxJava.h
    VxMd5.h
)

set(CoreLib_SOURCES)

list(APPEND CoreLib_SOURCES 
    AppErr_corelib.cpp
    AssetDefs.cpp
	BinaryBlob.cpp
    Blowfish_corelib.cpp
    ConnectId.cpp
    DbBase_corelib.cpp
    DbDataset_corelib.cpp
    DbQueryData_corelib.cpp
    GroupieId.cpp
    HostedId.cpp
    InetAddress.cpp
    InetAddressParse.cpp
    ObjectCommon_corelib.cpp
    OsDetect_corelib.cpp
    PktBlobEntry.cpp
    PluginId.cpp
    SHA1_corelib.cpp
    Sha1ClientInfo.cpp
    Sha1GeneratorMgr.cpp
    Sha1Info.cpp
    sqlite3_corelib.c
    sqlitedataset_corelib.cpp
    StdOutRedirect_corelib.cpp
	StringListBinary.cpp

    VxAppInfo.cpp
    Invite.cpp
    VFile.cpp
    VFileMgr.cpp

    VxAudioFormat.cpp
    VxBuffer.cpp
    VxChop.cpp
    VxChopMap.cpp
    VxColorUtil.cpp
    VxCrypto.cpp
    VxDebug.cpp
    VxElapseTimer.cpp

    VxFileCopier.cpp
    VxFileCrc32.cpp
    VxFileInfo.cpp
    VxFileIsTypeFunctions.cpp
    VxFileLists.cpp

    VxFileShredder.cpp
    VxFileShredderDb.cpp
    VxFileTypeMasks.cpp
    VxFileUtil.cpp
    VxFunctionsMissingInPosix.cpp
    VxFunctionsMissingInWindows.cpp
    VxGUID.cpp
    VxGUIDList.cpp
    VxGuidPairList.cpp
    VxGuidPairTimeList.cpp
    VxGlobals.cpp
    VxLinuxOnly.cpp
    VxMathDef.cpp
    VxMathVec3f.cpp
    VxMemoryUsage.cpp
    VxMemUtils.cpp
    VxMutex.cpp
    VxParse.cpp
    VxProfile.cpp
    VxPtopUrl.cpp

    VxResolveHost.cpp
    VxSemaphore.cpp
    VxSettings.cpp
    VxSha1Hash.cpp
    VxShortcut.cpp
    VxSktUtil.cpp
    VxStringUtils.cpp
    VxTextStreamReader.cpp
    VxThread.cpp
    VxTimeUtil.cpp
    VxTime.cpp
    VxTimedCallback.cpp

    VxValuePair.cpp
    VxXferDefs.cpp
    VxXml.cpp
    VxUrl.cpp
    VxJava.cpp
    VxFunctionsMissingInAndroid.cpp
    VxMd5.cpp
)

if(DEFINED TARGET_OS_WINDOWS)
    list(APPEND CoreLib_SOURCES
        dirent_sim.cpp
    )
endif()

# Generate AppVersion.h from the template with the CMake-configured build timestamp
configure_file(AppVersion.h.in "${CMAKE_CURRENT_BINARY_DIR}/AppVersion.h" @ONLY)

add_library(CoreLib
    STATIC
    ${CoreLib_SOURCES}
    ${CoreLib_HEADERS}
    "${CMAKE_CURRENT_BINARY_DIR}/AppVersion.h"
)

nlc_target_add_supported_warning_options(CoreLib CXX
    -Wno-deprecated-declarations
    -Wno-varargs
)

target_include_directories(CoreLib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/..")

target_compile_definitions(CoreLib
    PRIVATE
    ENABLE_VSTREAM
)

