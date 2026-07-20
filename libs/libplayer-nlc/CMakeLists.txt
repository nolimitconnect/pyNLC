
cmake_minimum_required (VERSION 3.22.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


project(player-nlc, LANGUAGES C CXX)

if(DEFINED TARGET_OS_ANDROID)
    message(STATUS "player-nlc prefix directory ${CMAKE_PREFIX_PATH}")
    message(STATUS "player-nlc find root path ${CMAKE_FIND_ROOT_PATH}")
    message(STATUS "player-nlc find module path ${CMAKE_MODULE_PATH}")
    message(STATUS "player-nlc platform ${ANDROID_PLATFORM}")
    message(STATUS "player-nlc abi ${ANDROID_ABI}")
    message(STATUS "player-nlc ndk ${ANDROID_NDK}")
    message(STATUS "player-nlc sdk root ${ANDROID_SDK_ROOT}")
    message(STATUS "player-nlc Qt6Core_DIR ${Qt6Core_DIR}")
    
    find_package(Qt6 REQUIRED COMPONENTS Core)
endif()


set(player-nlc_SOURCES)

# ALL TARGET OSes
list(APPEND player-nlc_SOURCES 
    xbmc/contrib/kissfft/kiss_fft.c
    xbmc/contrib/kissfft/kiss_fftr.c
    xbmc/commons/Exception.cpp
    xbmc/commons/ilog.cpp

    xbmc/cores/AudioEngine/Sinks/AESinkNlc.cpp

    xbmc/cores/playercorefactory/PlayerCoreConfig.cpp
    xbmc/cores/playercorefactory/PlayerCoreFactory.cpp
    xbmc/cores/playercorefactory/PlayerSelectionRule.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DllDvdNav.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/InputStreamPVRBase.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/InputStreamPVRChannel.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/InputStreamPVRRecording.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/FrameBufferObject.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/OverlayRendererNlc.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/RendererNlc.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/VideoShaders/VideoFilterShaderNlc.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/VideoShaders/YUV2RGBShaderNlc.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/VideoShaders/YUVMatrix.cpp
    xbmc/cores/AudioEngine/AEResampleFactory.cpp
    xbmc/cores/AudioEngine/AESinkFactory.cpp
    xbmc/cores/AudioEngine/Encoders/AEEncoderFFmpeg.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAE.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEFilter.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEResampleFFMPEG.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAESettings.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAESink.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAESound.cpp
    xbmc/cores/AudioEngine/Engines/ActiveAE/ActiveAEStream.cpp

    # xbmc/cores/AudioEngine/Sinks/AESinkDirectSound.cpp
    # xbmc/cores/AudioEngine/Sinks/AESinkWASAPI.cpp
    # xbmc/cores/AudioEngine/Sinks/windows/AESinkFactoryWin.cpp
    # xbmc/cores/AudioEngine/Sinks/windows/AESinkFactoryWin32.cpp

    xbmc/cores/AudioEngine/Utils/AEBitstreamPacker.cpp
    xbmc/cores/AudioEngine/Utils/AEChannelInfo.cpp
    xbmc/cores/AudioEngine/Utils/AEDeviceInfo.cpp
    xbmc/cores/AudioEngine/Utils/AELimiter.cpp
    xbmc/cores/AudioEngine/Utils/AEPackIEC61937.cpp
    xbmc/cores/AudioEngine/Utils/AEStreamInfo.cpp
    xbmc/cores/AudioEngine/Utils/AEUtil.cpp
    xbmc/cores/ExternalPlayer/ExternalPlayer.cpp
    xbmc/cores/DataCacheCore.cpp
    xbmc/cores/FFmpeg.cpp
    xbmc/cores/VideoPlayer/AudioSinkAE.cpp
    xbmc/cores/VideoPlayer/Buffers/VideoBuffer.cpp
    xbmc/cores/VideoPlayer/DVDClock.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodecFFmpeg.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodecPassthrough.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/DVDCodecUtils.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayCodec.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayCodecCCText.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayCodecFFmpeg.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayCodecSSA.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayCodecText.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayCodecTX3G.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/contrib/cc_decoder.c
    xbmc/cores/VideoPlayer/DVDCodecs/Overlay/contrib/cc_decoder708.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Video/AddonVideoCodec.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecFFmpeg.cpp
    xbmc/cores/VideoPlayer/DVDCodecs/Video/DVDVideoPPFFmpeg.cpp

    # xbmc/cores/VideoPlayer/DVDCodecs/Video/DXVA.cpp

    xbmc/cores/VideoPlayer/DVDDemuxSPU.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemux.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxBXA.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxCC.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxCDDA.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxClient.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxFFmpeg.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxVobsub.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DVDFactoryDemuxer.cpp
    xbmc/cores/VideoPlayer/DVDDemuxers/DemuxMultiSource.cpp
    xbmc/cores/VideoPlayer/DVDFileInfo.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDFactoryInputStream.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDInputStream.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDInputStreamBluray.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDInputStreamFFmpeg.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDInputStreamFile.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDInputStreamMemory.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDInputStreamNavigator.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDInputStreamStack.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/DVDStateSerializer.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/InputStreamAddon.cpp
    xbmc/cores/VideoPlayer/DVDInputStreams/InputStreamMultiSource.cpp
    xbmc/cores/VideoPlayer/DVDMessage.cpp
    xbmc/cores/VideoPlayer/DVDMessageQueue.cpp
    xbmc/cores/VideoPlayer/DVDOverlayContainer.cpp
    xbmc/cores/VideoPlayer/DVDStreamInfo.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDFactorySubtitle.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleLineCollection.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleParserMPL2.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleParserMicroDVD.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleParserSSA.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleParserSami.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleParserSubrip.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleParserVplayer.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleStream.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleTagMicroDVD.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitleTagSami.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/DVDSubtitlesLibass.cpp
    xbmc/cores/VideoPlayer/DVDSubtitles/SubtitlesAdapter.cpp
    xbmc/cores/VideoPlayer/Edl.cpp
    xbmc/cores/VideoPlayer/PTSTracker.cpp
    xbmc/cores/VideoPlayer/Process/ProcessInfo.cpp
    xbmc/cores/VideoPlayer/Process/windows/ProcessInfoWin.cpp
    xbmc/cores/VideoPlayer/VideoPlayer.cpp
    xbmc/cores/VideoPlayer/VideoPlayerAudio.cpp
    xbmc/cores/VideoPlayer/VideoPlayerAudioID3.cpp
    xbmc/cores/VideoPlayer/VideoPlayerRadioRDS.cpp
    xbmc/cores/VideoPlayer/VideoPlayerSubtitle.cpp
    xbmc/cores/VideoPlayer/VideoPlayerTeletext.cpp
    xbmc/cores/VideoPlayer/VideoPlayerVideo.cpp
    xbmc/cores/VideoPlayer/VideoReferenceClock.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/BaseRenderer.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/ColorManager.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/DebugRenderer.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/HwDecRender/DXVAHD.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/OverlayRenderer.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/OverlayRendererUtil.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/RenderCapture.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/RenderFactory.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/RenderFlags.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/RenderManager.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/VideoShaders/ConversionMatrix.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/VideoShaders/ConvolutionKernels.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/VideoShaders/WinVideoFilter.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/WinRenderBuffer.cpp
    xbmc/cores/VideoPlayer/VideoRenderers/WinRenderer.cpp
    xbmc/cores/VideoSettings.cpp
    xbmc/cores/paplayer/AudioDecoder.cpp
    xbmc/cores/paplayer/CodecFactory.cpp
    xbmc/cores/paplayer/PAPlayer.cpp
    xbmc/cores/paplayer/VideoPlayerCodec.cpp
    xbmc/cores/playercorefactory/PlayerCoreFactory.cpp
    xbmc/cores/playercorefactory/PlayerSelectionRule.cpp
    xbmc/events/AddonEvent.cpp
    xbmc/events/AddonManagementEvent.cpp
    xbmc/events/BaseEvent.cpp
    xbmc/events/EventLog.cpp
    xbmc/events/EventLogManager.cpp
    xbmc/ffmpeg_util/ffmpeg_util.cpp
    xbmc/filesystem/AddonsDirectory.cpp
    xbmc/filesystem/AudioBookFileDirectory.cpp
    xbmc/filesystem/CacheStrategy.cpp
    xbmc/filesystem/CircularCache.cpp
    xbmc/filesystem/DAVCommon.cpp
    xbmc/filesystem/DAVDirectory.cpp
    xbmc/filesystem/DAVFile.cpp
    xbmc/filesystem/Directory.cpp
    xbmc/filesystem/DirectoryCache.cpp
    xbmc/filesystem/DirectoryFactory.cpp
    xbmc/filesystem/DirectoryHistory.cpp
    xbmc/filesystem/EventsDirectory.cpp
    xbmc/filesystem/FileCache.cpp
    xbmc/filesystem/FileDirectoryFactory.cpp
    xbmc/filesystem/FileFactory.cpp
    xbmc/filesystem/IDirectory.cpp
    xbmc/filesystem/IFile.cpp
    xbmc/filesystem/ImageFile.h
    xbmc/filesystem/ImageFile.cpp
    xbmc/filesystem/MultiPathDirectory.h
    xbmc/filesystem/MultiPathDirectory.cpp
    xbmc/filesystem/MultiPathFile.h
    xbmc/filesystem/MultiPathFile.cpp
    xbmc/filesystem/OverrideDirectory.cpp
    xbmc/filesystem/OverrideFile.cpp
    xbmc/filesystem/PluginFile.cpp
    xbmc/filesystem/ResourceDirectory.cpp
    xbmc/filesystem/ResourceFile.cpp
    xbmc/filesystem/SourcesDirectory.cpp
    xbmc/filesystem/SpecialProtocol.cpp
    xbmc/filesystem/SpecialProtocolDirectory.cpp
    xbmc/filesystem/SpecialProtocolFile.cpp
    xbmc/filesystem/StackDirectory.cpp
    xbmc/filesystem/VideoDatabaseDirectory.cpp
    xbmc/filesystem/VirtualDirectory.cpp
    xbmc/filesystem/VirtualStreamFile.cpp
    xbmc/filesystem/XbtFile.cpp
    xbmc/filesystem/XbtManager.cpp
    xbmc/filesystem/ZipDirectory.cpp
    xbmc/filesystem/ZipFile.cpp
    xbmc/filesystem/ZipManager.cpp
    xbmc/filesystem/File2.cpp
    xbmc/fmt/format_fmt.cpp
    xbmc/fmt/ostream.cpp

    # xbmc/fmt/posix.cpp

    xbmc/guilib/guiinfo/GUIInfoBool.cpp
    xbmc/guilib/DDSImage.cpp
    xbmc/guilib/FFmpegImage.cpp
    xbmc/guilib/GUIComponent.cpp
    xbmc/guilib/GUIFont.cpp
    xbmc/guilib/GUIFontCache.cpp
    xbmc/guilib/GUIFontManager.cpp
    xbmc/guilib/GUIFontTTF.cpp
    xbmc/guilib/GUIFontTTFNlc.cpp
    xbmc/guilib/GUIListItem.cpp
    xbmc/guilib/GUIMessage.cpp
    xbmc/guilib/GUIRangesControl.cpp
    xbmc/guilib/GUITexture.cpp
    xbmc/guilib/GUITextureNlc.cpp
    xbmc/guilib/IWindowManagerCallback.cpp
    xbmc/guilib/LocalizeStrings.h
    xbmc/guilib/LocalizeStrings.cpp
    xbmc/guilib/StereoscopicsManager.h
    xbmc/guilib/StereoscopicsManager.cpp
    xbmc/guilib/TextureNlc.cpp
    xbmc/guilib/Texture.cpp
    xbmc/guilib/TextureBundle.cpp
    xbmc/guilib/TextureBundleXBT.cpp
    xbmc/guilib/TextureManager.cpp
    xbmc/guilib/XBTF.cpp
    xbmc/guilib/XBTFReader.cpp
    xbmc/guilib/imagefactory.cpp
    xbmc/imagefiles/SpecialImageLoaderFactory.cpp
    xbmc/input/actions/Action.cpp
    xbmc/input/actions/ActionTranslator.cpp
    xbmc/input/mouse/MouseStat.cpp
	xbmc/input/mouse/MouseStat.h
    xbmc/input/Key.cpp
    xbmc/input/XBMC_keytable.cpp
    xbmc/interfaces/AnnouncementManager3.cpp
    xbmc/media/MediaType.cpp
    xbmc/messaging/ApplicationMessenger.cpp

    xbmc/music/tags/MusicInfoTag.cpp
    xbmc/music/tags/MusicInfoTagLoaderFFmpeg.cpp
    xbmc/music/tags/MusicInfoTagLoaderFactory.cpp
    xbmc/music/tags/MusicInfoTagLoaderShn.cpp
    xbmc/music/tags/ReplayGain.cpp
    xbmc/music/tags/TagLibVFSStream.cpp
    xbmc/music/tags/TagLoaderTagLib.cpp
    xbmc/music/Album.cpp
    xbmc/music/Artist.cpp
    xbmc/music/MusicEmbeddedImageFileLoader.h
    xbmc/music/MusicEmbeddedImageFileLoader.cpp
    xbmc/music/Song.cpp

    xbmc/pictures/ExifParse.cpp
    xbmc/pictures/IptcParse.cpp
    xbmc/pictures/JpegParse.cpp
    xbmc/pictures/Picture.cpp
    xbmc/pictures/PictureInfoTag.cpp
    xbmc/pictures/PictureScalingAlgorithm.cpp
    xbmc/pictures/libexif.cpp

    # xbmc/platform/posix/FilesystemPosix.cpp
    # xbmc/platform/posix/MessagePrinterPosix.cpp
    # xbmc/platform/posix/XTimeUtils.cpp
    # xbmc/platform/win32/XTimeUtils.cpp

    xbmc/platform/nlc/KodiNlcApp.cpp
    xbmc/platform/Environment.cpp
    xbmc/platform/Platform.cpp

    # xbmc/platform/xbmc.cpp

    xbmc/playlists/PlayList.cpp
    xbmc/playlists/PlayListB4S.cpp
    xbmc/playlists/PlayListFactory.cpp
    xbmc/playlists/PlayListM3U.cpp
    xbmc/playlists/PlayListPLS.cpp
    xbmc/playlists/PlayListURL.cpp
    xbmc/playlists/PlayListWPL.cpp
    xbmc/playlists/PlayListXML.cpp
    xbmc/playlists/PlayListXSPF.cpp
    xbmc/playlists/SmartPlayList.cpp
    xbmc/playlists/SmartPlaylistFileItemListModifier.cpp
    xbmc/profiles/Profile.cpp
    xbmc/profiles/ProfileManager.cpp
    xbmc/rendering/RenderSystem.cpp
    xbmc/rendering/dx/DeviceResources.cpp
    xbmc/rendering/dx/RenderSystemDX.cpp
    xbmc/rendering/gles/GLESShader.cpp
    xbmc/rendering/gles/RenderSystemGLES.cpp
    xbmc/rendering/qt/RenderSystemNlc.cpp

    xbmc/settings/lib/ISetting.cpp
    xbmc/settings/lib/ISettingControl.cpp
    xbmc/settings/lib/Setting.h
    xbmc/settings/lib/Setting.cpp 
    xbmc/settings/lib/SettingConditions2.cpp  
    xbmc/settings/lib/SettingCategoryAccess.h
    xbmc/settings/lib/SettingCategoryAccess.cpp

    xbmc/settings/lib/SettingDependency.cpp  
    xbmc/settings/lib/SettingRequirement.cpp
    xbmc/settings/lib/SettingSection.cpp
    xbmc/settings/lib/SettingsManager.cpp
    xbmc/settings/lib/SettingUpdate.cpp

    xbmc/settings/AdvancedSettings.cpp
    xbmc/settings/DisplaySettings.cpp
    xbmc/settings/MediaSettings.cpp
    xbmc/settings/MediaSourceSettings.cpp
    xbmc/settings/SettingConditions.cpp
    xbmc/settings/SettingControl.cpp
    xbmc/settings/SettingCreator.cpp
    xbmc/settings/SettingDateTime.cpp
    
    xbmc/settings/SettingPath.cpp
    xbmc/settings/Settings.cpp
    xbmc/settings/SettingsBase.cpp
    xbmc/settings/SettingsComponent.cpp
    xbmc/settings/SettingsValueFlatJsonSerializer.cpp
    xbmc/settings/SettingsValueXmlSerializer.cpp
    
    xbmc/settings/SettingUtils.cpp
    xbmc/settings/SubtitlesSettings.cpp

    xbmc/storage/MediaManager.cpp

    # xbmc/threads/platform/win/ThreadImplWin32.cpp
    # xbmc/threads/platform/win/Win32Exception.cpp

    xbmc/threads/Atomics.cpp
    xbmc/threads/Event.cpp
    xbmc/threads/SystemClock.cpp
    xbmc/threads/Thread.cpp
    xbmc/threads/Timer.cpp

    xbmc/utils/AMLUtils.cpp
    xbmc/utils/ActorProtocol.cpp
    xbmc/utils/AlarmClock.cpp
    xbmc/utils/AliasShortcutUtils.cpp
    xbmc/utils/Archive.cpp
    xbmc/utils/Base64.cpp
    xbmc/utils/BitstreamConverter.cpp
    xbmc/utils/BitstreamReader.cpp
    xbmc/utils/BitstreamStats.cpp
    xbmc/utils/BitstreamWriter.cpp
    xbmc/utils/BooleanLogic.cpp
    xbmc/utils/CPUInfo.cpp
    xbmc/utils/CharArrayParser.cpp
    xbmc/utils/CharsetConverter.cpp
    xbmc/utils/CharsetDetection.cpp
    xbmc/utils/ColorUtils.cpp
    xbmc/utils/ContentUtils.cpp
    xbmc/utils/CPUInfo.cpp
    xbmc/utils/Crc32.cpp

    xbmc/utils/EndianSwap.h
    xbmc/utils/EndianSwap.cpp
    xbmc/utils/ExecString.cpp
    xbmc/utils/EmbeddedArt.cpp

    xbmc/utils/Fanart.h
    xbmc/utils/Fanart.cpp
    xbmc/utils/FileExtensionProvider.h
    xbmc/utils/FileExtensionProvider.cpp
    xbmc/utils/FileOperationJob.cpp
    xbmc/utils/FileUtils.cpp
    xbmc/utils/FontUtils.cpp
    xbmc/utils/HTMLUtil.h
    xbmc/utils/HTMLUtil.cpp

    xbmc/utils/InfoLoader.h
    xbmc/utils/InfoLoader.cpp
    xbmc/utils/GLUtils.cpp
    xbmc/utils/JobManager.cpp
    xbmc/utils/Locale.cpp
    xbmc/utils/LangCodeExpander.cpp
    xbmc/utils/Mime.cpp
    xbmc/utils/MovingSpeed.cpp

    xbmc/utils/Observer.h
    xbmc/utils/Observer.cpp
    xbmc/utils/POUtils.h
    xbmc/utils/POUtils.cpp
    xbmc/utils/PlayerUtils.cpp
    xbmc/utils/ProgressJob.cpp
    xbmc/utils/RegExp.cpp
    xbmc/utils/RingBuffer.cpp

    xbmc/utils/Speed.cpp
    xbmc/utils/SortUtils.cpp
    xbmc/utils/Stopwatch.h
    xbmc/utils/Stopwatch.cpp
    xbmc/utils/StreamDetails.cpp
    xbmc/utils/StreamUtils.cpp
    xbmc/utils/StringUtils2.cpp
    xbmc/utils/SysfsUtils.cpp
    xbmc/utils/SystemInfo.cpp

    xbmc/utils/TextureUtils.h
    xbmc/utils/TextureUtils.cpp
    xbmc/utils/TimeUtils2.cpp

    xbmc/utils/URIUtils.cpp

    xbmc/utils/UrlOptions.cpp
    xbmc/utils/Utf8Utils.cpp
    xbmc/utils/VC1BitstreamParser.cpp
    xbmc/utils/Variant.cpp
    xbmc/utils/Vector.cpp
    xbmc/utils/XBMCTinyXML.cpp
    xbmc/utils/XMLUtils.cpp
    xbmc/utils/auto_buffer.cpp
    xbmc/utils/fstrcmp.c
    xbmc/utils/log.cpp
    xbmc/utils/rfft.cpp
    xbmc/video/Bookmark.cpp
    xbmc/video/GUIViewStateVideo.cpp
    xbmc/video/Teletext.cpp
    xbmc/video/VideoDbUrl.cpp
    xbmc/video/VideoEmbeddedImageFileLoader.cpp
    xbmc/video/VideoGeneratedImageFileLoader.cpp
    xbmc/video/VideoInfoDownloader.cpp
    xbmc/video/VideoInfoTag.cpp
    xbmc/view/GUIViewControl.cpp
    xbmc/view/GUIViewState.cpp
    xbmc/view/ViewDatabase.cpp
    xbmc/view/ViewStateSettings.cpp
    xbmc/windowing/OSScreenSaver.cpp
    xbmc/windowing/WinSystem.cpp
 
    xbmc/windowing/windows/WinEventsWin32.cpp

    xbmc/windowing/nlc/OSScreenSaverNlc.cpp
    xbmc/windowing/nlc/NlcUtils.cpp
    xbmc/windowing/nlc/VideoSyncNlc.cpp
    xbmc/windowing/nlc/WinEventsNlc.cpp
    xbmc/windowing/nlc/WinSystemNlc.cpp
    xbmc/windowing/nlc/WinSystemNlcContext.cpp
    xbmc/windowing/GraphicContext.cpp
    xbmc/windowing/Resolution.cpp
    xbmc/windowing/WindowSystemFactory.cpp
    xbmc/windowing/WinSystem.cpp
    xbmc/Application.cpp
    xbmc/ApplicationActionListeners.cpp
    xbmc/ApplicationEvents.cpp
    xbmc/ApplicationPlayer.cpp
    xbmc/ApplicationPlayerCallback.cpp
    xbmc/AppInboundProtocol.cpp

    xbmc/CueDocument.cpp
    xbmc/CompileInfo.cpp
    xbmc/DbUrl.cpp
    xbmc/FileItem.cpp
    xbmc/FileItemListModification.cpp
    xbmc/GUILargeTextureManager.cpp
    xbmc/InfoScanner.cpp
    xbmc/LangInfoKodi.cpp
    xbmc/MediaPlayerNlc.cpp
    xbmc/MediaSource.cpp
    xbmc/NlcCoreUtil.cpp
    xbmc/NlcUrl.cpp
    xbmc/PasswordManager.cpp
    xbmc/PlayListPlayer.cpp
    xbmc/SeekHandler.cpp
    xbmc/ServiceBroker.cpp
    xbmc/ServiceManager.cpp
    xbmc/SystemGlobals.cpp
    xbmc/TextureCache.cpp
    xbmc/TextureCacheJob.cpp
    xbmc/XBDateTime.cpp

    xbmc/platform/xbmc.cpp
)

# YES Windows
if(DEFINED TARGET_OS_WINDOWS)
    list(APPEND player-nlc_SOURCES

        xbmc/platform/nlc/KodiNlcApp.h
        xbmc/platform/nlc/KodiNlcApp.cpp

        xbmc/platform/win32/CharsetConverterWin32.cpp
        xbmc/platform/win32/EnvironmentWin32.cpp
        xbmc/platform/win32/FilesystemWin32.cpp
        xbmc/platform/win32/MemUtils_w32.cpp
        xbmc/platform/win32/MessagePrinterWin32.cpp

        xbmc/platform/win32/WIN32Util.cpp

        #xbmc/platform/win32/WMIInterface.cpp
        xbmc/platform/win32/WindowHelper.cpp
        xbmc/platform/win32/crts_caller.cpp
        # xbmc/platform/win32/dxerr.cpp
        xbmc/platform/win32/filesystem/Win32Directory.cpp
        xbmc/platform/win32/filesystem/Win32File.cpp
        xbmc/platform/win32/stat_utf8.cpp
        xbmc/platform/win32/stdio_utf8.cpp
        xbmc/platform/win32/strverscmp.cpp

        xbmc/platform/win32/XTimeUtils.cpp

        xbmc/platform/win32/utils/Win32InterfaceForCLog.cpp

        xbmc/platform/win32/storage/Win32StorageProvider.cpp

        xbmc/threads/platform/win/Win32Exception.cpp
        xbmc/threads/platform/win/ThreadImplWin32.cpp

    )
else()
    # NOT Windows
    list(APPEND player-nlc_SOURCES
        xbmc/fmt/posix.cpp
        xbmc/platform/posix/FilesystemPosix.cpp
        xbmc/platform/posix/MessagePrinterPosix.cpp
        xbmc/platform/posix/XTimeUtils.cpp
        xbmc/platform/win32/XTimeUtils.cpp
        xbmc/utils/URIUtils.cpp
    )

endif()

if(DEFINED TARGET_OS_LINUX OR DEFINED TARGET_OS_ANDROID)
    list(APPEND player-nlc_SOURCES
        xbmc/platform/linux/ConvUtils.cpp
        xbmc/platform/linux/DBusMessage.cpp
        xbmc/platform/linux/DBusReserve.cpp
        xbmc/platform/linux/DBusUtil.cpp
        xbmc/platform/linux/FDEventMonitor.cpp
        xbmc/platform/linux/LinuxResourceCounter.cpp
        xbmc/platform/linux/OMXClock.cpp
        xbmc/platform/linux/OMXCore.cpp
        xbmc/platform/linux/OptionalsReg.cpp
        xbmc/platform/linux/PosixMountProvider.cpp
        xbmc/platform/linux/RBP.cpp
        xbmc/platform/linux/TimeUtils.cpp
        xbmc/platform/linux/XFileUtils.cpp
        xbmc/platform/linux/XHandle.cpp
        xbmc/platform/linux/XMemUtils.cpp
        xbmc/platform/linux/XTimeUtils.cpp
        xbmc/platform/linux/storage/LinuxStorageProvider.cpp
        xbmc/platform/posix/filesystem/PosixDirectory.cpp
        xbmc/platform/posix/filesystem/PosixFile.cpp
        xbmc/platform/posix/utils/Mmap.cpp
        xbmc/platform/posix/utils/PosixInterfaceForCLog.cpp
        xbmc/platform/posix/utils/SharedMemory.cpp
        xbmc/platform/posix/PosixResourceCounter.cpp
        xbmc/platform/posix/PosixTimezone.cpp
    )
endif()

if(DEFINED TARGET_OS_LINUX)
    list(APPEND player-nlc_SOURCES
        xbmc/platform/linux/MemUtils_linux.cpp
        xbmc/platform/posix/PosixResourceCounter.cpp
    )
endif()

if (DEFINED TARGET_OS_ANDROID)
    list(APPEND player-nlc_SOURCES
        #../androidNdk/android/cpufeatures/cpu-features_ndk.c # for android_getCpuFeatures etc
        xbmc/filesystem/AndroidAppDirectory.cpp
        xbmc/filesystem/AndroidAppFile.cpp
        xbmc/filesystem/APKFile.cpp
        xbmc/filesystem/APKDirectory.cpp
        #xbmc/interfaces/builtins/AndroidBuiltins.cpp
        xbmc/platform/nlc/android/jni/BitmapDrawable.cpp
        xbmc/platform/nlc/android/jni/Build.cpp
        xbmc/platform/nlc/android/jni/Context.cpp
        xbmc/platform/nlc/android/jni/DisplayMetrics.cpp
        xbmc/platform/nlc/android/jni/PackageManager.cpp
        xbmc/platform/nlc/android/jni/Resources.cpp
        xbmc/platform/nlc/android/jni/JNIBase.cpp
        xbmc/platform/nlc/android/jni/JNIEnvironment.cpp
        xbmc/platform/nlc/android/jni/JNIFile.cpp
        xbmc/platform/nlc/android/jni/JNIThreading.cpp
        xbmc/platform/nlc/android/jni/jutils.cpp
        xbmc/platform/android/MemUtils_android.cpp
        xbmc/platform/android/peripherals/AndroidJoystickState.cpp
        xbmc/platform/android/peripherals/PeripheralBusAndroid.cpp
        xbmc/platform/nlc/android/activity/AndroidFeatures.cpp
        xbmc/platform/nlc/android/jni/ApplicationInfo.cpp
        xbmc/platform/nlc/android/jni/Environment.cpp
        xbmc/platform/nlc/android/jni/File.cpp
        xbmc/platform/nlc/android/jni/PackageItemInfo.cpp
        xbmc/platform/nlc/android/jni/StatFs.cpp
        xbmc/platform/nlc/android/jni/System.cpp
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia OpenGL OpenGLWidgets Svg Widgets)

add_library(player-nlc 
    STATIC
    ${player-nlc_SOURCES}
)

target_compile_definitions(player-nlc
  PRIVATE
    ENABLE_NLC_PLAYER
    HAVE_NLC_GUI
)

if(DEFINED BUILD_SYSTEM_FLATPAK)
    # glu is no longer available in flatpak-sdk, so we need to use our own library
    target_include_directories(player-nlc PRIVATE
        ${PROJECT_SRC_ROOT_DIR}/libs/libGLU/include       
    )
endif()

if(DEFINED TARGET_OS_WINDOWS)

    target_include_directories(player-nlc
          PRIVATE
            xbmc/platform/win32
            xbmc/cores/AudioEngine
            ../sysheaders
        )
endif()

if(DEFINED TARGET_OS_LINUX)
    target_include_directories(player-nlc
      PRIVATE
        xbmc/platform/linux
        xbmc/platform/posix
    )
endif()

if(DEFINED TARGET_OS_ANDROID)
    target_include_directories(player-nlc
      PRIVATE
        xbmc/platform/nlc/android
        xbmc/platform/linux
        xbmc/platform/posix
        ${CMAKE_PREFIX_PATH}/../
    )
endif()

target_include_directories(player-nlc
  PRIVATE
    xbmc
    xbmc/platform/nlc
    ../libptopengine
    ../ffmpeg
    ../libpcre
    ../libfreetype/include
    ../libpthread 
)

target_link_libraries(player-nlc Qt::Core Qt::CorePrivate)


if(MSVC)
    # 1. This clears the inherited Unicode definitions from the target
    set_target_properties(player-nlc PROPERTIES COMPILE_DEFINITIONS "")

    # 2. Re-apply ONLY the ones you actually want for this target
    target_compile_definitions(player-nlc
        PRIVATE
        NOMINMAX
        _LIB
        LIB_STATIC
        WIN64
        _WINDOWS
        MS_WINDOWS
        TARGET_WINDOWS
        TARGET_WINDOWS_DESKTOP
        _MBCS
        __SSE__
        HAVE_SSE
        __SSE2__
        HAVE_SSE2

        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
        _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )

    # 3. Use the 'Force Undefine' flag to undefine UNICODE that was set by target_link_libraries(player-nlc Qt::Core Qt::CorePrivate)
    # This tells the compiler: "No matter what you saw earlier, UNSET Unicode"
    target_compile_options(player-nlc PRIVATE /UUNICODE /U_UNICODE /permissive /Zc:strictStrings-)
endif()


if(NOT DEFINED TARGET_OS_WINDOWS)
    nlc_target_add_supported_warning_options(player-nlc CXX
        -Wno-deprecated-declarations
    )

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        nlc_target_add_supported_warning_options(player-nlc CXX
            -Wno-user-defined-warnings
            -Wno-undefined-var-template
        )
    endif()
endif()

if(MSVC)
    # Fix UNICODE definitions that may have been inherited from linked libraries (like Qt) or global settings
    string(REPLACE "/DUNICODE" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/D_UNICODE" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    # This forcefully UNDEFINES the macros at the compiler level
    # and ensures they are overridden even if they appear earlier in the command line.
    add_compile_options(/UUNICODE /U_UNICODE)

    add_compile_options(
        "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:/W0>"  # This turns off all warnings for windows builds
        "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:/Zc:strictStrings->"  # Disables specific check for string literal const-qualifiers
        "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:/permissive>" # Allows non-const char* conversion (Legacy behavior).. Usually set by /std:c++17 or later
    )
endif()


