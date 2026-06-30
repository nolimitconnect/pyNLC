#pragma once
//============================================================================
// Copyright (C) 2009 Brett R. Jones 
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license 
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <QWidget> // must be declared first or linux Qt will error in qmetatype.h 2167:23: array subscript value 53 is outside the bounds

#include "AppDefs.h"
#include "AppGlobals.h"

#include "GuiAudioMgr.h"

#include "CamLogic.h"

#include "FriendList.h"
#include "GuiConnectIdListMgr.h"

#include "GuiFileXferMgr.h"
#include "GuiGroupieListMgr.h"
#include "GuiHostedListMgr.h"
#include "GuiHostedByMeJoinMgr.h"

#include "GuiOfferMgr.h"
#include "GuiUserJoinMgr.h"
#include "GuiUserMgr.h"
#include "GuiThumbMgr.h"
#include "GuiWebPageMgr.h"
#include "MyIconsDefs.h"
#include "SoundDefs.h"
#include "VxAppTheme.h"
#include "VxAppStyle.h"
#include "VxAppDisplay.h" 

#include "GuiFriendRequest/GuiFriendRequestMgr.h"

#include "ToGuiActivityInterface.h"
#include "ToGuiHardwareControlInterface.h"
#include "ToGuiUserUpdateInterface.h"
#include "ToGuiThumbUpdateInterface.h"

#include "GuiInterface/IToGui.h"
#include "GuiInterface/INlcRender.h"
#include "GuiInterface/INlcEvents.h"
#include "GuiInterface/IAudioInterface.h"

#include <BlobXferMgr/BlobInfo.h>
#include <Plugins/FileInfo.h>

#include <CoreLib/VxThread.h>

class AccountMgr;

class ActivityBase;
class ActivityShowHelp;
class ActivityOfferListDlg;

class AdminAvailMgr;

class AppModuleState;
class AppletMultiMessenger;
class AppletDownloads;
class AppletMgr;
class AppletUploads;

class AppSettings;
class AssetSendMgr;

class BlobInfo;
class FriendListEntryWidget;
class FileListReplySession;
class HomeWindow;

class GuiFileXferSession;
class GuiFavoriteMgr;
class GuiMemberActiveMgr;
class GuiOfferSession;
class GuiPlayerMgr;
class GuiPluginMgr;
class GuiPushToTalkMgr;
class GuiRandConnectMgr;
class GuiSendQueueMgr;

class KodiThread;
class MediaPlayerNlc;
class MyIcons;
class PopupMenu;
class RenderGlWidget;
class TodGameMgr;

class VxPeerMgr;
class VxTilePositioner;

// media
class CRenderBuffer;

class AppCommon : public QWidget, public IToGui, public INlcRender, public INlcEvents, public IAudioRequests, public IAudioCallbacks
{
    Q_OBJECT

public:
    AppCommon( QApplication& myQApp,
        AdminAvailMgr& adminAvailMgr,
        AppModuleState& appModuleState,
        AppSettings& appSettings,
        AccountMgr& myDataHelper,
        GuiFavoriteMgr& favoritMgr,
        GuiMemberActiveMgr& memberActiveMgr,
        GuiPlayerMgr& playerMgr,
        GuiPluginMgr& pluginMgr,
        GuiPushToTalkMgr& pushToTalkMgr,
        GuiRandConnectMgr& randConnectMgr,
        GuiSendQueueMgr& sendQueueMgr,
        AssetSendMgr& assetSendMgr,
        MyIcons& myIcons,
        TodGameMgr& todGameMgr,
        SoundFxMgr& soundFxMgr
    );

    AppCommon( const AppCommon& rhs ) = delete;
    virtual ~AppCommon() override = default;

    INlcRender& getINlcRender( void ) { return *this; }
    IToGui& getIToGui( void ) { return *this; }
    IAudioRequests& getIAudioRequests( void ) { return *this; }

    // elapsed high resolution timer
    int                         elapsedMilliseconds( void );

    int                         elapsedSeconds( void ) { return elapsedMilliseconds() / 1000; }

    bool                        loadWithThread( void );
    // cannot launch any applets until logon is completed
    void                        setLoginCompleted( bool completed ) { m_LoginComplete = completed; }
    bool                        getLoginCompleted( void ) { return m_LoginComplete; }

    // some applets cannot be launched until application is fully ready for network use
    void                        setIsAppInitialized( bool initialized ) { m_AppInitialized = initialized; }
    bool                        getIsAppInitialized( void ) { return m_AppInitialized; }
    bool                        getPtopNetworkReady( void ) { return m_PtopNetworkReady; }

    // diagnose to much cpu usage in gui thread
    void                        setGuiCpuTimeEnable( bool enable ) { m_GuiCpuTimeEnable = enable; }
    bool                        getGuiCpuTimeEnable( void ) { return m_GuiCpuTimeEnable; }

    AccountMgr&                 getAccountMgr( void ) { return m_AccountMgr; }
    AdminAvailMgr&              getAdminAvailMgr( void ) { return m_AdminAvailMgr; }
    VxAppDisplay&               getAppDisplay( void ) { return m_AppDisplay; }
    AppGlobals&                 getAppGlobals( void ) { return m_AppGlobals; }
    QFrame*                     getAppletFrame( EApplet applet );
    AppletMgr&                  getAppletMgr( void ) { return m_AppletMgr; }
    AssetSendMgr&               getAssetSendMgr( void ) { return m_AssetSendMgr; }
    AppSettings&                getAppSettings( void ) { return m_AppSettings; }
    QString&                    getAppShortName( void ) { return m_AppShortName; }
    VxAppStyle&                 getAppStyle( void ) { return m_AppStyle; }
    QString&                    getAppTitle( void ) { return m_AppTitle; }
    VxAppTheme&                 getAppTheme( void ) { return m_AppTheme; }

    GuiAudioMgr&                   getAudioMgr( void ) { return m_AudioMgr; }

    CamLogic&                   getCamLogic( void ) { return m_CamLogic; }
    P2PEngine&                  getEngine( void );
    IFromGui&                   getFromGuiInterface( void );
    TodGameMgr&                 getTodGameMgr( void ) { return m_TodGameMgr; }

    HomeWindow&                 getHomeWindow( void ) { return *m_HomeWindow; }
    bool						getIsVidCaptureEnabled( void ) { return m_VidCaptureEnabled; }
    bool						getIsMicrophoneHardwareEnabled( void ) { return m_MicrophoneHardwareEnabled; }
    bool						getIsSpeakerHardwareEnabled( void ) { return m_SpeakerHardwareEnabled; }
    bool						getIsMyPortOpen( void );
    MyIcons&                    getMyIcons( void ) { return m_MyIcons; }
    VxNetIdent*                 getMyNetIdent( void );
    VxGUID&                     getMyOnlineId( void );
    ENetworkStateType			getNetworkState( void ) { return m_LastNetworkState; }

    GuiConnectIdListMgr&        getConnectIdListMgr( void ) { return m_ConnectIdListMgr; }
    GuiFriendRequestMgr&        getFriendRequestMgr( void ) { return m_FriendRequestMgr; }
    GuiFavoriteMgr&             getFavoriteMgr( void ) { return m_FavoriteMgr; }
    GuiFileXferMgr&             getFileXferMgr( void ) { return m_FileXferMgr; }
    GuiGroupieListMgr&          getGroupieListMgr( void ) { return m_GroupieListMgr; }
    GuiHostedListMgr&           getHostedListMgr( void ) { return m_HostedListMgr; }
    GuiHostedByMeJoinMgr&       getHostJoinMgr( void ) { return m_HostJoinMgr; }
    GuiMemberActiveMgr&         getMemberActiveMgr( void ) { return m_MemberActiveMgr; }
    GuiOfferMgr&                getOfferMgr( void ) { return m_OfferMgr; }
    GuiPlayerMgr&               getPlayerMgr( void ) { return m_PlayerMgr; }
    GuiPluginMgr&               getPluginMgr( void ) { return m_PluginMgr; }
    GuiPushToTalkMgr&           getPushToTalkMgr( void ) { return m_PushToTalkMgr; };

    GuiRandConnectMgr&          getRandConnectMgr( void ) { return m_RandConnectMgr; }
    GuiSendQueueMgr&            getSendQueueMgr( void ) { return m_SendQueueMgr; }
    GuiThumbMgr&                getThumbMgr( void ) { return m_ThumbMgr; }
    GuiUserJoinMgr&             getUserJoinMgr( void ) { return m_UserJoinMgr; }
    GuiWebPageMgr&              getWebPageMgr( void ) { return m_WebPageMgr; }
    GuiUserMgr&                 getUserMgr( void ) { return m_UserMgr; }

    RenderGlWidget*             getRenderConsumer( void );

    SoundFxMgr&                 getSoundFxMgr( void ) { return m_SoundFxMgr; }

    QApplication&               getQApplication( void ) { return m_QApp; }

    void                        setGuiThreadId( unsigned int threadId ) { m_GuiThreadId = threadId; }
    unsigned int                getGuiThreadId( void ) { return m_GuiThreadId; }

    void						setCamCaptureRotation( uint32_t rot )  { m_CamCaptureRotation = rot; }
    int							getCamCaptureRotation( void ) { return m_CamCaptureRotation; }

    void 						setAccountUserName( const char* name ) { m_strAccountUserName = name; }
    std::string					getAccountUserName( void ) { return m_strAccountUserName; }

    void						setIsMaxScreenSize( bool isMessagerFrame, bool isFullSizeWindow );
    bool						getIsMaxScreenSize( bool isMessagerFrame );

    // permanent applets for lifetime of application
    void						setAppletMultiMessenger( AppletMultiMessenger* applet ) { m_AppletMultiMessenger = applet; }
    AppletMultiMessenger*       getAppletMultiMessenger( void ) { return m_AppletMultiMessenger; };
    void						setAppletDownloads( AppletDownloads* applet ) { m_AppletDownloads = applet; }
    AppletDownloads*            getAppletDownloads( void ) { return m_AppletDownloads; };
    void						setAppletUploads( AppletUploads* applet ) { m_AppletUploads = applet; }
    AppletUploads*              getAppletUploads( void ) { return m_AppletUploads; };

    bool                        hasExistingAccount( void );

    void						switchWindowFocus( QWidget* appIconButton );

    void						applySoundSettings( bool useDefaultsInsteadOfSettings = false );
    void						playSound( ESndDef sndDef );
    void						insertKeystroke( int keyNum );

    virtual void				okMessageBox( QString title, QString msg );
    virtual void				okMessageBox2( QString title, const char* msg, ... );
    virtual bool				yesNoMessageBox( QString title, QString msg );
    virtual bool				yesNoMessageBox2( QString title, const char* msg, ... );
    virtual void				errMessageBox( QString title, QString msg );
    virtual void				errMessageBox2( QString title, const char* msg, ... );

    //=== app methods ===//
    virtual void				startupAppCommon( QFrame* appletFrame, QFrame* messangerFrame );

    virtual void				doLogin( void );
    virtual void				completeLogin( void );

    // prompt user to confirm wants to shutdown app.. caller must call appCommonShutdown if answer is yes
    virtual bool				confirmAppShutdown( QWidget* parentWindow );
    virtual void				shutdownAppCommon( void );

    void						loadAccountSpecificSettings( const char* userName );
    void                        copyAssetsToFoldersIfRequired( void );

    ActivityBase*               launchApplet( EApplet applet, QWidget* parent );
    ActivityBase*               launchApplet( EApplet applet, QWidget* parent, QString launchParam, VxGUID& assetId );

    bool						launchOfferSendApplet( EPluginType pluginType, GuiUser* guiUser, QWidget* parent = nullptr );
    bool                        launchOfferSendSession( EPluginType pluginType, GuiUser* guiUser, std::shared_ptr<GuiOfferSession> existingOffer, QWidget* parent = nullptr );

    void						activityStateChange( ActivityBase* activity, bool isCreated );

    void						setIsLibraryActivityActive( bool isActive ) { m_LibraryActivityActive = isActive; }
    bool						getIsLibraryActivityActive( void ) { return m_LibraryActivityActive; }

    void						wantToGuiActivityCallbacks( ToGuiActivityInterface* callback, bool wantCallback );
    void						wantToGuiHardwareCtrlCallbacks( ToGuiHardwareControlInterface* callback, bool wantCallback );
    void						wantToGuiUserUpdateCallbacks( ToGuiUserUpdateInterface* callback, bool	wantCallback );

    //============================================================================
    //=== to player-nlc lib events ===//
    //============================================================================
    virtual void                fromGuiKeyPressEvent( EMediaModule mediaModule, int key, int mod ) override;
    virtual void                fromGuiKeyReleaseEvent( EMediaModule mediaModule, int key, int mod ) override;

    virtual void                fromGuiMousePressEvent( EMediaModule mediaModule, int mouseXPos, int mouseyPos, int mouseButton ) override;
    virtual void                fromGuiMouseReleaseEvent( EMediaModule mediaModule, int mouseXPos, int mouseyPos, int mouseButton ) override;
    virtual void                fromGuiMouseMoveEvent( EMediaModule mediaModule, int mouseXPos, int mouseyPos ) override;

    virtual void                fromGuiCloseEvent( EMediaModule mediaModule ) override;
    virtual void                fromGuiVisibleEvent( EMediaModule mediaModule, bool isVisible ) override;

    //============================================================================
    //=== from gui audio/camera callbacks ===//
    //============================================================================

    /// Mute/Unmute microphone
    virtual void				fromGuiMuteMicrophone( bool muteMic ) override;
    /// Returns true if microphone is muted
    virtual bool				fromGuiIsMicrophoneMuted( void ) override;
    /// Mute/Unmute speaker
    virtual void				fromGuiMuteSpeaker( bool muteSpeaker ) override;
    /// Returns true if speaker is muted
    virtual bool				fromGuiIsSpeakerMuted( void ) override;

    virtual void				fromGuiCameraEnable( bool enableCamera );
    virtual void				fromGuiCaptureRunning( bool camCaptureRunning );

    //============================================================================
    //=== to gui media/render ===//
    //============================================================================

    bool                        toGuiMediaAction( EMediaModule mediaModule, EMediaPlayerAction playerAction, int actionVal = 0, const char* fileName = "" ) override;
    void                        toGuiMediaError( EMediaModule mediaModule, EMediaError mediaError, const char* msg ) override;

    virtual void                verifyGlState( const char* msg = nullptr ) override; // show gl error if any

    //=== textures ===//
    void                        setActiveGlTexture( unsigned int activeTextureNum = 0 /* 0 == GL_TEXTURE0 , 1 == GL_TEXTURE1 etc*/ ) override;

    void                        createTextureObject( CTextureQt* texture ) override;
    void                        destroyTextureObject( CTextureQt* texture ) override;
    bool                        loadToGPU( CTextureQt* texture ) override;
    void                        bindToUnit( CTextureQt* texture, unsigned int unit ) override;

    void                        beginGuiTexture( CGUITextureQt* guiTexture, NlcColor color ) override;
    void                        drawGuiTexture( CGUITextureQt* guiTexture, float* x, float* y, float* z, const NlcRect& texture, const NlcRect& diffuse, int orientation ) override;
    void                        endGuiTexture( CGUITextureQt* guiTexture ) override;
    void                        drawQuad( const NlcRect& rect, NlcColor color, CTextureBase* texture, const NlcRect* texCoords ) override;

    bool                        firstBegin( CGUIFontTTFQt* font )  override;
    void                        lastEnd( CGUIFontTTFQt* font ) override;

    CVertexBuffer               createVertexBuffer( CGUIFontTTFQt* font, const std::vector<SVertex>& vertices )  override;

    void                        destroyVertexBuffer( CGUIFontTTFQt* font, CVertexBuffer& buffer )  override;

    virtual void                deleteHardwareTexture( CGUIFontTTFQt* font )  override;
    virtual void                createStaticVertexBuffers( CGUIFontTTFQt* font )  override;
    virtual void                destroyStaticVertexBuffers( CGUIFontTTFQt* font )  override;

    //=== render ===//
    void                        captureScreen( CScreenshotSurface* screenCaptrue, NlcRect& captureArea ) override;

    void                        toGuiRenderVideoFrame( int textureIdx, CRenderBuffer* videoBuffer );
    bool                        initRenderSystem() override;
    bool                        destroyRenderSystem() override;
    bool                        resetRenderSystem( int width, int height ) override;

    int                         getMaxTextureSize() override;

    bool                        beginRender() override;
    bool                        endRender() override;
    void                        presentRender( bool rendered, bool videoLayer ) override;
    bool                        clearBuffers( NlcColor color ) override;
    bool                        isExtSupported( const char* extension ) override;

    void                        setVSync( bool vsync ) override;
    void                        resetVSync() override {  }

    void                        setViewPort( const NlcRect& viewPort ) override;
    void                        getViewPort( NlcRect& viewPort ) override;

    bool                        scissorsCanEffectClipping() override;
    NlcRect                     clipRectToScissorRect( const NlcRect& rect ) override;
    void                        setScissors( const NlcRect& rect ) override;
    void                        resetScissors() override;

    void                        captureStateBlock() override;
    void                        applyStateBlock() override;

    void                        setCameraPosition( const NlcPoint& camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f ) override;

    void                        applyHardwareTransform( const TransformMatrix& matrix ) override;
    void                        restoreHardwareTransform() override;
    bool                        supportsStereo( RENDER_STEREO_MODE mode ) const override { return false; }

    bool                        testRender() override;

    void                        project( float& x, float& y, float& z ) override;

    //=== shaders ===//
    std::string                 getShaderPath( const std::string& filename ) override { return ""; }

    void                        initializeShaders() override;
    void                        releaseShaders() override;
    bool                        enableShader( ESHADERMETHOD method ) override;
    bool                        isShaderValid( ESHADERMETHOD method ) override;
    void                        disableShader( ESHADERMETHOD method ) override;
    void                        disableGUIShader() override;

    int                         shaderGetPos()  override;
    int                         shaderGetCol()  override;
    int                         shaderGetModel()  override;
    int                         shaderGetCoord0()  override;
    int                         shaderGetCoord1()  override;
    int                         shaderGetUniCol()  override;

    // yuv shader
    void                        shaderSetField( ESHADERMETHOD shader, int field )   override;
    void                        shaderSetWidth( ESHADERMETHOD shader, int w )   override;
    void                        shaderSetHeight( ESHADERMETHOD shader, int h )  override;

    void                        shaderSetBlack( ESHADERMETHOD shader, float black ) override;
    void                        shaderSetContrast( ESHADERMETHOD shader, float contrast ) override;
    void                        shaderSetConvertFullColorRange( ESHADERMETHOD shader, bool convertFullRange ) override;

    int                         shaderGetVertexLoc( ESHADERMETHOD shader ) override;
    int                         shaderGetYcoordLoc( ESHADERMETHOD shader ) override;
    int                         shaderGetUcoordLoc( ESHADERMETHOD shader ) override;
    int                         shaderGetVcoordLoc( ESHADERMETHOD shader ) override;

    void                        shaderSetMatrices( ESHADERMETHOD shader, const float* p, const float* m ) override;
    void                        shaderSetAlpha( ESHADERMETHOD shader, float alpha ) override;

    void                        shaderSetFlags( ESHADERMETHOD shader, unsigned int flags ) override;
    void                        shaderSetFormat( ESHADERMETHOD shader, EShaderFormat format ) override;
    void                        shaderSourceTexture( ESHADERMETHOD shader, int ytex ) override;
    void                        shaderSetStepX( ESHADERMETHOD shader, float stepX ) override;
    void                        shaderSetStepY( ESHADERMETHOD shader, float stepY )  override;

    // filter shader
    bool                        shaderGetTextureFilter( ESHADERMETHOD shader, int& filter ) override;
    int                         shaderGetcoordLoc( ESHADERMETHOD shader ) override;

    // renderqt
    int                         shaderVertexAttribPointer( ESHADERMETHOD shader, unsigned int index, int size, int type, bool normalized, int stride, const void* pointer ) override;
    void                        shaderEnableVertexAttribArray( ESHADERMETHOD shader, int arrayId ) override;
    void                        shaderDisableVertexAttribArray( ESHADERMETHOD shader, int arrayId ) override;

    // frame buffers
    void                        frameBufferGen( int bufCount, unsigned int* fboId ) override;
    void                        frameBufferDelete( int bufCount, unsigned int* fboId ) override;
    void                        frameBufferTexture2D( int target, unsigned int texureId )  override;
    void                        frameBufferBind( unsigned int fboId ) override;
    bool                        frameBufferStatus() override;

    // gl functions
    void                        glFuncDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices ) override;
    void                        glFuncDisable( GLenum cap ) override;
    void                        glFuncBindTexture( GLenum target, GLuint texture ) override;
    void                        glFuncViewport( GLint x, GLint y, GLsizei width, GLsizei height ) override;
    void                        glFuncScissor( GLint x, GLint y, GLsizei width, GLsizei height ) override;

    void                        glFuncGenTextures( GLsizei n, GLuint* textures ) override;
    void                        glFuncDeleteTextures( GLsizei n, const GLuint* textures ) override;
    void                        glFuncTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels ) override;
    void                        glFuncTexParameteri( GLenum target, GLenum pname, GLint param ) override;
    void                        glFuncReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels ) override;
    void                        glFuncPixelStorei( GLenum pname, GLint param ) override;
    void                        glFuncFinish() override;

    void                        glFuncEnable( GLenum cap ) override;
    void                        glFuncTexSubImage2D( GLenum target, GLint level,
                                                     GLint xoffset, GLint yoffset,
                                                     GLsizei width, GLsizei height,
                                                     GLenum format, GLenum type,
                                                     const GLvoid* pixels ) override;
    void                        glFuncBlendFunc( GLenum sfactor, GLenum dfactor ) override;
    void                        glFuncVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer ) override;
    void                        glFuncDisableVertexAttribArray( GLuint index ) override;
    void                        glFuncEnableVertexAttribArray( GLuint index ) override;
    void                        glFuncDrawArrays( GLenum mode, GLint first, GLsizei count ) override;


    //============================================================================
    //=== end to gui media/render ===//
    //============================================================================



    //============================================================================
    //=== to gui ===//
    //============================================================================

    void                        toGuiAdminAvail( GroupieId& adminGroupieId, bool adminAvail ) override;

    void                        toGuiSetIsAppModuleRunning( EMediaModule mediaModule, bool isRunning ) override;
    bool                        toGuiGetIsAppModuleRunning( EMediaModule mediaModule ) override;

    bool                        toGuiRunModule( EMediaModule mediaModule ) override;
    bool                        toGuiStopModule( EMediaModule mediaModule ) override;

    void				        toGuiPlayNlcMedia( AssetBaseInfo* assetInfo ) override;
    void				        toGuiLog( int logFlags, const char* pMsg ) override;
    void				        toGuiAppErr( EAppErr eAppErr, const char* errMsg = "" ) override;
    void				        toGuiAppPopupErr( EAppErr eAppErr, const char* errMsg ) override;
    void				        toGuiStatusMessage( const char* errMsg ) override;
    // NOTE: toGuiUserMessage should be called from in gui on gui thread only
    void				        toGuiUserMessage( const char* userMsg, ... );
    void				        toGuiPluginMsg( EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, const char* paramMsg = "" ) override;
    void				        toGuiPluginCommError( EPluginType pluginType, VxGUID& onlineId, EPluginMsgType msgType, ECommErr commErr ) override;

    /// Send Network available status to GUI for display
    void				        toGuiNetAvailableStatus( ENetAvailStatus eNetAvailStatus ) override;
    void				        toGuiNetworkState( ENetworkStateType eNetworkState, const char* stateMsg = "" ) override;

    void				        toGuiHostAnnounceStatus( EHostType hostType, VxGUID& sessionId, EHostAnnounceStatus joinStatus, const char* msg = "" ) override;
    void				        toGuiHostJoinStatus( EHostType hostType, VxGUID& sessionId, EHostJoinStatus joinStatus, const char* msg = "" ) override;

    void				        toGuiHostSearchStatus( EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr = eCommErrNone, const char* msg = "" ) override;
    void				        toGuiHostSearchResult( EHostType hostType, VxGUID& sessionId, HostedInfo& hostedInfo ) override;
    void				        toGuiHostSearchComplete( EHostType hostType, VxGUID& sessionId ) override;

    void				        toGuiGroupieSearchStatus( EHostType hostType, VxGUID& sessionId, EHostSearchStatus searchStatus, ECommErr commErr = eCommErrNone, const char* msg = "" ) override;
    void				        toGuiGroupieSearchResult( EHostType hostType, VxGUID& sessionId, GroupieInfo& hostedInfo ) override;
    void				        toGuiGroupieSearchComplete( EHostType hostType, VxGUID& sessionId ) override;

    void				        toGuiIsPortOpenStatus( EIsPortOpenStatus eIsPortOpenStatus, const char* msg = "" ) override;
    void				        toGuiRunTestStatus( const char* testName, ERunTestStatus eRunTestStatus, const char* msg = "" ) override;
    void				        toGuiRandomConnectStatus( ERandomConnectStatus eRandomConnectStatus, const char* msg = "" ) override;

    // return true if any microphone device is available to be enabled
    bool				        toGuiIsMicrophoneDeviceAvailable( void ) override;

    void				        toGuiWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput ) override;

    void				        toGuiWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput ) override;

    int				            toGuiModuleAudioFrame( EMediaModule mediaModule, int16_t* pu16PcmData, int pcmDataLenInBytes ) override;

    int				            toGuiPlayerNlcAudio( EMediaModule mediaModule, float* audioDataFload, int audioDataLenInBytes ) override;

    float                       toGuiGetAudioDelaySeconds( EMediaModule mediaModule ) override;

    float                       toGuiGetAudioCacheFreeSpaceBytes( EMediaModule mediaModule ) override;

    float                       toGuiGetAudioCacheMaxSeconds( EMediaModule mediaModule ) override;
   
    void				        toGuiUpdateWantMicrophoneCount( int wantMicCnt ) override;
    void				        toGuiUpdateWantSpeakerCount( int wantSpeakerCnt ) override;

    void				        toGuiWantVideoCapture( EMediaModule mediaModule, bool wantVidCapture ) override;
    void				        toGuiPlayJpgVideo( VxGUID& onlineId, std::shared_ptr<CamJpgVideo>& jpgVideo ) override;

    // user update interface
    void				        toGuiIndentListUpdate( EUserViewType listType, VxGUID& onlineId, uint64_t timestamp ) override;
    void				        toGuiIndentListRemove( EUserViewType listType, VxGUID& onlineId ) override;

    void				        toGuiContactAdded( VxNetIdent* netIdent ) override;
    void				        toGuiContactRemoved( VxGUID& onlineId ) override;

    void				        toGuiContactOnline( VxNetIdent* netIdent ) override;

    void				        toGuiContactAnythingChange( VxNetIdent* netIdent ) override;
    void				        toGuiContactLastSessionTimeChange( VxNetIdent* netIdent ) override;

    void				        toGuiUpdateMyIdent( VxNetIdent* netIdent ) override;
    void				        toGuiSaveMyIdent( VxNetIdent* netIdent ) override;

    void				        toGuiPluginStatus( EPluginType		pluginType,
                                                   int				statusType,
                                                   int				statusValue ) override;

    //=== to gui session ===//
    void				        toGuiRxedPluginOffer( VxGUID onlineId, OfferBaseInfo& offerInfo ) override;
    void				        toGuiRxedOfferReply( VxGUID onlineId, OfferBaseInfo& offerInfo ) override;

    void				        toGuiPluginSessionStarted( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId ) override;
    void				        toGuiPluginSessionEnded( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId ) override;

    void				        toGuiInstMsg( VxGUID& onlineId, EPluginType	pluginType, const char* pMsg ) override;

    void				        toGuiTodGameAction( EPluginType	pluginType, VxGUID& onlineId, ETodGameAction todGameAction ) override;

    //=== to gui file ===//
    void				        toGuiFileListReply( VxGUID& onlineId, EPluginType pluginType, FileInfo& fileInfo ) override;

    void				        toGuiFileUploadStart( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId, FileInfo& fileInfo ) override;

    void				        toGuiFileDownloadStart( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId, FileInfo& fileInfo ) override;

    void				        toGuiFileXferState( EPluginType pluginType, VxGUID& lclSessionId, EXferDirection xferDir, EXferState xferState, EXferError xferErr, int param1 ) override;
    void				        toGuiFileDeleted( std::string& fileName ) override;

    void				        toGuiFileDownloadComplete( EPluginType pluginType, VxGUID& lclSessionId, std::string& fileName, EXferError xferError ) override;
    void				        toGuiFileUploadComplete( EPluginType pluginType, VxGUID& lclSessionId, std::string& fileName, EXferError xferError ) override;

    void				        toGuiFileList( VxGUID& appInstId, FileInfo& fileInfo ) override;
    void				        toGuiFileListCompleted( VxGUID& appInstId ) override;

    void				        toGuiFolderScan( VxGUID& appInstId, FileInfo& fileInfo ) override;
    void				        toGuiFolderScanCompleted( VxGUID& appInstId, bool wasCanceled ) override;
    
    //=== to gui search ===//
    void				        toGuiSearchResultFileSearch( VxGUID& onlineId, EPluginType pluginType, VxGUID& lclSessionId, FileInfo& fileInfo ) override;

    //=== to gui asset ===//
    void				        toGuiAssetAdded( AssetBaseInfo* assetInfo ) override;
    void				        toGuiAssetUpdated( AssetBaseInfo* assetInfo ) override;
    void				        toGuiAssetRemoved( AssetBaseInfo* assetInfo ) override;

    void				        toGuiAssetXferState( VxGUID& assetUniqueId, EAssetSendState assetSendState, int param ) override;

    void				        toGuiAssetSessionHistory( AssetBaseInfo* assetInfo ) override;
    void				        toGuiAssetAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 ) override;
    void				        toGuiMultiSessionAction( EMSessionAction mSessionAction, VxGUID onlineId, int pos0to100000 ) override;

    //=== to gui host list ===//
    void				        toGuiBlobAdded( BlobInfo* assetInfo ) override;
    void				        toGuiBlobSessionHistory( BlobInfo* assetInfo ) override;
    void				        toGuiBlobAction( EAssetAction assetAction, VxGUID& assetId, int pos0to100000 ) override;

    /// a module has changed state
    void				        toGuiModuleState( EMediaModule moduleNum, EModuleState moduleState )  override;

    void				        toGuiNetworkIsTested( bool requiresRelay, std::string& ipAddr, uint16_t ipPort )  override;

    //============================================================================
    //=== implementation ===//
    //============================================================================

    bool						userCanceled( void );

    // returns true if showed activity
    bool 						offerToFriendPluginSession( GuiUser* guiUser, EPluginType pluginType, QWidget* parent = nullptr );
    void						offerToFriendSendFile( GuiUser* guiUser, QWidget* parent = nullptr );

    void						createAccountForUser( std::string& strUserName, VxNetIdent& userAccountIdent, const char* moodMsg, int gender,
                                                      EAgeType age, int primaryLanguage, int contentType );
    void                        setupAccountResources( VxNetIdent& userAccountIdent );
    // updates my ident in database and engine and global ident
    void                        updateMyIdent( VxNetIdent* myIdent, bool permissionAndStateOnly = false );

    std::string					getUserXferDirectoryFromAccountUserName( const char* userName );
    std::string 				getUserSpecificDataDirectoryFromAccountUserName( const char* userName );

    void						refreshFriend( VxGUID& onlineId ); // called to emit signalRefreshFriend
    bool						loadLastUserAccount( void );
    void                        onMessengerReady( bool isReady );
    bool                        isMessengerReady( void ) { return m_IsMessengerReady; }
    void                        onUserLoggedOn( void );
    bool                        checkSystemReady( void );
    bool                        isSystemReady( void ) { return m_IsGuiSystemReady; };

    std::string                 getUserName( const VxGUID& onlineId );

    std::string                 describeConnectId( ConnectId& connectionId );
    std::string                 describeGroupieId( GroupieId& groupieId );
    std::string                 describeHostedId( HostedId& hostedId );
    std::string                 describeUser( VxGUID& onlineId );
    std::string                 describeUser( GuiUser* guiUser );

    //! throw error if not gui thread
    void                        checkIsGuiThread( void );

    static void					registerMetaData( void );

    bool                        iAmHostAdmin( EPluginType pluginType, bool showErrMsg = false );

    bool                        getThumbImage( VxGUID& thumbId, QImage& image );

signals:
    void						signalMessengerReady( bool isReady );    // emitted when messenger ready state changes
    void						signalMainWindowResized( void );    // emitted if main window is resized
    void						signalMainWindowMoved( void );      // emitted if main window is moved

    void						signalFinishedLoadingGui( void );
    void						signalFinishedLoadingEngine( void );

    void						signalLog( int iPluginNum, QString strMsg );
    void						signalAppErr( EAppErr eAppErr, QString errMsg );
    void						signalStatusMsg( QString strMsg );

    void						signalHostAnnounceStatus( EHostType hostType, VxGUID sessionId, EHostAnnounceStatus hostStatus, QString strMsg );
    void						signalHostJoinStatus( EHostType hostType, VxGUID sessionId, EHostJoinStatus hostStatus, QString strMsg );

    void						signalHostSearchStatus( EHostType hostType, VxGUID sessionId, EHostSearchStatus hostStatus, QString strMsg );
    void						signalGroupieSearchStatus( EHostType hostType, VxGUID sessionId, EHostSearchStatus hostStatus, QString strMsg );

    void						signalIsPortOpenStatus( EIsPortOpenStatus eIsPortOpenStatus, QString strMsg );
    void						signalRunTestStatus( QString testName, ERunTestStatus eRunTestStatus, QString strMsg );
    void						signalRandomConnectStatus( ERandomConnectStatus eRandomConnectStatus, QString strMsg );
    void						signalNetworkStateChanged( ENetworkStateType eNetworkState );
    void						signalNetAvailStatus( ENetAvailStatus eNetAvailStatus );

    void						signalRefreshFriend( VxGUID onlineId ); // emitted if friend has changed
    void						signalAssetViewMsgAction( EAssetAction, VxGUID onlineId, int pos0to100000 );
    void						signalBlobViewMsgAction( EAssetAction, VxGUID onlineId, int pos0to100000 );

    void						signalToGuiInstMsg( VxGUID onlineId, EPluginType pluginType, QString pMsg );

    void						signalMicrophonePeak( int peekVal0to32768 );

    void						signalInternalWantMicrophoneRecording( EMediaModule mediaModule, bool enableMicInput );

    void						signalInternalWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput );

    void						signalInternalWantVideoCapture( EMediaModule mediaModule, bool enableCapture );

    void						signalSetRelayHelpButtonVisibility( bool isVisible );

    void                        signalSystemReady( bool isReady );

    void                        signalInternalNetAvailStatus( ENetAvailStatus netAvailStatus );
    void                        signalInternalPluginMessage( EPluginType pluginType, VxGUID onlineId, EPluginMsgType msgType, QString paramValue );
    void                        signalInternalPluginErrorMsg( EPluginType pluginType, VxGUID onlineId, EPluginMsgType msgType, ECommErr commError );

    void                        signalInternalToGuiFileDownloadStart( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId, FileInfo fileInfo );
    void                        signalInternalToGuiFileDownloadComplete( EPluginType pluginType, VxGUID lclSessionId, QString fileName, EXferError xferError );
    void                        signalInternalToGuiFileUploadStart( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId, FileInfo fileInfo );
    void                        signalInternalToGuiFileUploadComplete( EPluginType pluginType, VxGUID lclSessionId, QString fileName, EXferError xferError );
    void                        signalInternalToGuiFileXferState( EPluginType pluginType, VxGUID lclSessionId, EXferDirection xferDir, EXferState xferState, EXferError xferErr, int param1 );
    void                        signalInternalToGuiFileDeleted( QString fileName );

    void                        signalInternalToGuiFileList( VxGUID appInstId, FileInfo fileInfo );
    void                        signalInternalToGuiFileListCompleted( VxGUID appInstId );

    void                        signalInternalToGuiFolderScan( VxGUID appInstId, FileInfo fileInfo );
    void                        signalInternalToGuiFolderScanCompleted( VxGUID appInstId, bool wasCanceled );

    void                        signalInternalToGuiTodGameAction( EPluginType  pluginType, VxGUID onlineId, ETodGameAction todGameAction );

    void				        signalInternalToGuiAssetAdded( AssetBaseInfo assetInfo );
    void				        signalInternalToGuiAssetUpdated( AssetBaseInfo assetInfo );
    void				        signalInternalToGuiAssetRemoved( AssetBaseInfo assetInfo );

    void				        signalInternalToGuiAssetXferState( VxGUID assetUniqueId, EAssetSendState assetSendState, int param );

    void				        signalInternalToGuiAssetSessionHistory( AssetBaseInfo* assetInfo );
    void				        signalInternalToGuiAssetAction( EAssetAction assetAction, VxGUID assetId, int pos0to100000 );

    void                        signalInternalMultiSessionAction( VxGUID onlineId, EMSessionAction mSessionAction, int pos0to100000 );

    void                        signalInternalBlobAction( EAssetAction assetAction, VxGUID assetId, int pos0to100000 );
    void                        signalInternalBlobAdded( BlobInfo blobInfo );
    void                        signalInternalBlobSessionHistory( BlobInfo blobInfo );

    void                        signalInternalToGuiIndentListUpdate( EUserViewType listType, VxGUID onlineId, uint64_t timestamp );
    void                        signalInternalToGuiIndentListRemove( EUserViewType listType, VxGUID onlineId );

    void                        signalInternalToGuiContactAdded( VxNetIdent netIdent );
    void                        signalInternalToGuiContactRemoved( VxGUID onlineId );

    void                        signalInternalToGuiContactOnline( VxNetIdent netIdent );

    void                        signalInternalToGuiContactUpdated( VxNetIdent netIdent );

    void                        signalInternalToGuiContactLastSessionTimeChange( VxNetIdent netIdent );

    void                        signalInternalToGuiUpdateIdent( VxNetIdent netIdent );
    void                        signalInternalToGuiSaveMyIdent( VxNetIdent netIdent );

    void                        signalInternalNetworkIsTested( bool requiresRelay, QString ipAddr, uint16_t ipPort );

    void                        signalInternalPlayNlcMedia( AssetBaseInfo assetInfo );

    void                        signalInternalToGuiSearchResultFileSearch( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId, FileInfo fileInfo );
    void                        signalInternalToGuiFileListReply( VxGUID onlineId, EPluginType pluginType, FileInfo fileInfo );

    void                        signalInternalToGuiRxedPluginOffer( VxGUID onlineId, OfferBaseInfo offerInfo );
    void                        signalInternalToGuiRxedOfferReply( VxGUID onlineId, OfferBaseInfo offerInfo );

    void                        signalInternalToGuiPluginSessionStarted( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId );
    void                        signalInternalToGuiPluginSessionEnded( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId );

    void                        signalInternalMediaAction( EMediaModule mediaModule, EMediaPlayerAction playerAction, int actionVal, QString fileName );
    void                        signalInternalMediaError( EMediaModule mediaModule, EMediaError mediaError, QString msg );

    void						signalExpandWindowChanged( bool isMessengerFrame, bool isMaxScreenSize );

    void						signalInternalAppPopupErr( EAppErr eAppErr, QString errMsg );

    void                        signalInternalToGuiAdminAvail( GroupieId adminGroupieId, bool adminAvail );

    void                        signalShutdownApp( void );

private slots:
    void                        slotInternalNetAvailStatus( ENetAvailStatus netAvailStatus );
    void                        slotInternalPluginMessage( EPluginType pluginType, VxGUID onlineId, EPluginMsgType msgType, QString paramValue );
    void                        slotInternalPluginErrorMsg( EPluginType pluginType, VxGUID onlineId, EPluginMsgType msgType, ECommErr commError );

    void                        slotInternalToGuiFileDownloadStart( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId, FileInfo fileInfo );
    void                        slotInternalToGuiFileDownloadComplete( EPluginType pluginType, VxGUID lclSessionId, QString fileName, EXferError xferError );
    void                        slotInternalToGuiFileUploadStart( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId, FileInfo fileInfo );
    void                        slotInternalToGuiFileUploadComplete( EPluginType pluginType, VxGUID lclSessionId, QString fileName, EXferError xferError );
    void                        slotInternalToGuiFileXferState( EPluginType pluginType, VxGUID lclSessionId, EXferDirection xferDir, EXferState xferState, EXferError xferErr, int param1 );
    void                        slotInternalToGuiFileDeleted( QString fileName );

    void                        slotInternalToGuiFileList( VxGUID appInstId, FileInfo fileInfo );
    void                        slotInternalToGuiFileListCompleted( VxGUID appInstId );

    void                        slotInternalToGuiFolderScan( VxGUID appInstId, FileInfo fileInfo );
    void                        slotInternalToGuiFolderScanCompleted( VxGUID appInstId, bool wasCanceled );

    void                        slotInternalToGuiTodGameAction( EPluginType  pluginType, VxGUID onlineId, ETodGameAction todGameAction );

    void				        slotInternalToGuiAssetAdded( AssetBaseInfo assetInfo );
    void				        slotInternalToGuiAssetUpdated( AssetBaseInfo assetInfo );
    void				        slotInternalToGuiAssetRemoved( AssetBaseInfo assetInfo );

    void				        slotInternalToGuiAssetXferState( VxGUID assetUniqueId, EAssetSendState assetSendState, int param );

    void				        slotInternalToGuiAssetSessionHistory( AssetBaseInfo* assetInfo );
    void				        slotInternalToGuiAssetAction( EAssetAction assetAction, VxGUID assetId, int pos0to100000 );

    void                        slotInternalMultiSessionAction( VxGUID onlineId, EMSessionAction mSessionAction, int pos0to100000 );

    void                        slotInternalBlobAction( EAssetAction assetAction, VxGUID assetId, int pos0to100000 );
    void                        slotInternalBlobAdded( BlobInfo blobInfo );
    void                        slotInternalBlobSessionHistory( BlobInfo blobInfo );

    void                        slotInternalToGuiIndentListUpdate( EUserViewType listType, VxGUID onlineId, uint64_t timestamp );
    void                        slotInternalToGuiIndentListRemove( EUserViewType listType, VxGUID onlineId );

    void                        slotInternalToGuiContactAdded( VxNetIdent netIdent );
    void                        slotInternalToGuiContactRemoved( VxGUID onlineId );

    void                        slotInternalToGuiContactOnline( VxNetIdent netIdent );

    void                        slotInternalToGuiContactUpdated( VxNetIdent netIdent );

    void                        slotInternalToGuiContactLastSessionTimeChange( VxNetIdent netIdent );

    void                        slotInternalToGuiUpdateIdent( VxNetIdent netIdent );
    void                        slotInternalToGuiSaveMyIdent( VxNetIdent netIdent );

    void						slotInternalWantMicrophoneRecording( EMediaModule mediaModule, bool wantMicInput );

    void						slotInternalWantSpeakerOutput( EMediaModule mediaModule, bool wantSpeakerOutput );

    void						slotInternalWantVideoCapture( EMediaModule mediaModule, bool enableCapture );

    void                        slotInternalNetworkIsTested( bool requiresRelay, QString ipAddr, uint16_t ipPort );

    void                        slotInternalPlayNlcMedia( AssetBaseInfo assetInfo );

    void                        slotInternalToGuiSearchResultFileSearch( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId, FileInfo fileInfo );
    void                        slotInternalToGuiFileListReply( VxGUID onlineId, EPluginType pluginType, FileInfo fileInfo );

    void                        slotInternalToGuiRxedPluginOffer( VxGUID onlineId, OfferBaseInfo offerInfo );
    void                        slotInternalToGuiRxedOfferReply( VxGUID onlineId, OfferBaseInfo offerInfo );

    void                        slotInternalToGuiPluginSessionStarted( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId );
    void                        slotInternalToGuiPluginSessionEnded( VxGUID onlineId, EPluginType pluginType, VxGUID lclSessionId );

    void                        slotInternalMediaAction( EMediaModule mediaModule, EMediaPlayerAction playerAction, int actionVal, QString fileName );
    void                        slotInternalMediaError( EMediaModule mediaModule, EMediaError mediaError, QString msg );

    void                        slotInternalToGuiAdminAvail( GroupieId adminGroupieId, bool adminAvail );

protected slots:
    void						slotMainWindowResized( void );
    void						slotMainWindowMoved( void );

    void						slotStatusMsg( QString strMsg );
    void						slotAppErr( EAppErr eAppErr, QString errMsg );

    void						slotOnNotifyIconFlashTimeout( bool bWhite );

    void						slotToGuiInstMsg( VxGUID onlineId, EPluginType pluginType, QString pMsg );

    void						slotRelayHelpButtonClicked( void );
    void						slotSetRelayHelpButtonVisibility( bool isVisible );
    void						slotNetworkStateChanged( ENetworkStateType eNetworkState );

    void						onMenuFileSelected( int iMenuId, PopupMenu* popupMenu, ActivityBase* contentFrame );

    void						onOncePerSecond( void );

    void						onUpdateMyIdent( VxNetIdent* poMyIdent );

    void						slotGuiStartupTimer( void );

    void                        slotApplyStartupSettingsToEngine( void );

    void						slotAccountCreated( bool wasCreated );

    void						slotAfterLoginComplete( void );

    void						slotInternalAppPopupErr( EAppErr eAppErr, QString errMsg );

    void                        slotShutdownApp( void );

protected:
    void                        connectSignals( void );

    void                        scheduleHardwareCtrlStateReplay( void );
    void                        replayHardwareCtrlState( void );

    void						showUserNameInTitle();
    void						sendAppSettingsToEngine( void );

    void						updateFriendList( GuiUser* guiUser, bool sessionTimeChange = false );

    void						clearToGuiActivityInterfaceList( void );
    void						clearHardwareCtrlList( void );
    void						clearUserUpdateClientList( void );

    void						doAccountStartup( void );

    void                        checkReadyToLaunchAfterLogonApplets( void );
    bool                        isReadyToLaunchAfterLogonApplets( void );

    void                        checkReadyToConnectToLastConnectedHost( void );

    void                        showCreateAccount( void );

    //=== vars ===//
    QApplication&               m_QApp;

    AppModuleState&             m_AppModuleState;

    AppGlobals					m_AppGlobals;
    AppSettings&                m_AppSettings;
    
    QString						m_AppShortName;
    QString						m_AppTitle;
    AccountMgr&                 m_AccountMgr;

    AdminAvailMgr&              m_AdminAvailMgr;
    TodGameMgr&                 m_TodGameMgr;

    GuiConnectIdListMgr			m_ConnectIdListMgr;
    GuiFavoriteMgr&			    m_FavoriteMgr;
    GuiFriendRequestMgr			m_FriendRequestMgr;
    GuiFileXferMgr              m_FileXferMgr;
    GuiThumbMgr					m_ThumbMgr;
    GuiMemberActiveMgr&         m_MemberActiveMgr;
    GuiOfferMgr		            m_OfferMgr;
    GuiPlayerMgr&               m_PlayerMgr;
    GuiPluginMgr&               m_PluginMgr;
    GuiPushToTalkMgr&           m_PushToTalkMgr;
    GuiUserMgr					m_UserMgr;
    GuiGroupieListMgr			m_GroupieListMgr;
    GuiHostedListMgr			m_HostedListMgr;
    GuiHostedByMeJoinMgr		m_HostJoinMgr;

    GuiRandConnectMgr&          m_RandConnectMgr;
    GuiSendQueueMgr&            m_SendQueueMgr;
    AssetSendMgr&               m_AssetSendMgr;
    
    GuiUserJoinMgr				m_UserJoinMgr;
    GuiWebPageMgr               m_WebPageMgr;

    MyIcons&					m_MyIcons;
    VxAppTheme					m_AppTheme;
    VxAppStyle					m_AppStyle;
    VxAppDisplay				m_AppDisplay;

    CamLogic                    m_CamLogic;

    GuiAudioMgr                    m_AudioMgr;
    SoundFxMgr&                 m_SoundFxMgr;

    HomeWindow*					m_HomeWindow{ nullptr };

    ActivityShowHelp*           m_ActivityShowHelpDlg{ nullptr };

    AppletMultiMessenger*       m_AppletMultiMessenger{ nullptr };
    AppletDownloads*            m_AppletDownloads{ nullptr };
    AppletUploads*              m_AppletUploads{ nullptr };

    std::string					m_strAccountUserName;

    QTimer*                     m_OncePerSecondTimer;
    EFriendViewType				m_eLastSelectedWhichContactsToView; // last selection of which friends to view

    bool						m_bUserCanceledCreateProfile;
    VxMutex						m_AppMutex;
    std::vector<QString>		m_DebugLogQue;
    std::vector<QString>		m_AppErrLogQue;
    ENetworkStateType			m_LastNetworkState;

    std::string					m_CamSourceId;
    uint32_t					m_CamCaptureRotation;

    bool	                    m_ToGuiActivityInterfaceBusy{ false };
    bool	                    m_ToGuiFileXferInterfaceBusy{ false };
    bool                        m_ToGuiHardwareCtrlBusy{ false };
    bool                        m_ToGuiHardwareCtrlReplayPending{ false };
    bool                        m_ToGuiUserUpdateClientBusy{ false };

    std::vector<ToGuiActivityInterface*>	    m_ToGuiActivityInterfaceList;
    std::vector<ToGuiHardwareControlInterface*> m_ToGuiHardwareCtrlList;
    std::vector<ToGuiUserUpdateInterface*>      m_ToGuiUserUpdateClientList;

    bool						m_LibraryActivityActive = false;
    bool						m_VidCaptureEnabled = false;
    bool						m_MicrophoneHardwareEnabled = false;
    bool						m_SpeakerHardwareEnabled = false;
    AppletMgr&                  m_AppletMgr;
    bool                        m_AppCommonInitialized = false;
    bool                        m_SignalsConnected = false;
    bool                        m_LoginBegin = false;
    bool                        m_LoginComplete = false;
    bool                        m_AppInitialized = false;
    bool                        m_IsMessengerReady{ false };
    bool                        m_IsLoggedOn{ false };
    bool                        m_IsGuiSystemReady{ false };
    bool                        m_PtopNetworkReady{ false };

    bool                        m_GuiCpuTimeEnable{ false };

    QTimer*                     m_GuiStartupTimer = nullptr;

    bool                        m_LauchedAfterLogonApplets{ false };
    bool                        m_ConnectToLastConnectedHost{ false };

    VxThread                    m_AudioDevicesThread;
    VxThread                    m_EngineStartupThread;
    bool                        m_EngineStartupStarted{ false };
    int                         m_GuiStartupAudioWaitStartMs{ 0 };
    bool                        m_GuiStartupAudioWaitBypassed{ false };

    unsigned int                m_GuiThreadId{ 0 };
};

AppCommon& CreateAppInstance( QApplication* myApp, AppSettings& appSettings );

AppCommon& GetAppInstance( void );

void DestroyAppInstance( );


