#pragma once
//============================================================================
// Copyright (C) 2018 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "RenderGlLogic.h"
#include "RenderShaderQt.h"
#include "EventsQtToNlc.h"
#include "GuiInterface/INlcRender.h"
#include "GuiInterface/INlcEvents.h"

class NlcRenderFrame;
class CRenderBuffer;
class QKeyEvent;
class QMouseEvent;

class RenderGlWidget : public RenderGlLogic, public INlcRender
{
	Q_OBJECT
public:
    static const int MAX_RENDER_PLANES = 3;
    static const int MAX_VISIBLE_OVERLAYS = 4;

    typedef struct
    {
        GLenum                  m_Target;
        GLint                   m_InternalFormat;
        GLenum                  m_Format;
        GLenum                  m_Type;
    } GlTextureDescriptor;

    typedef struct
    {
        GLsizei                 m_Width;
        GLsizei                 m_Height;
    } GlTextureSize;

    RenderGlWidget( QWidget* parent=nullptr );
    virtual ~RenderGlWidget( ) override;

    AppCommon&				    getMyApp() { return  m_MyApp; }

    QSize                       getRenderWindowSize( void ) { return m_ScreenSize; }

    //! take a snapshot of current render
    void                        takeSnapshot( void );

    void                        showAppIcon( void );

    //! ignore from kodi
    void                        initializeShaders() override;
    //! ignore from kodi
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
    virtual void                shaderSetField( ESHADERMETHOD shader, int field )   override;
    virtual void                shaderSetWidth( ESHADERMETHOD shader, int w )   override;
    virtual void                shaderSetHeight( ESHADERMETHOD shader, int h )  override;

    virtual void                shaderSetBlack( ESHADERMETHOD shader, float black ) override;
    virtual void                shaderSetContrast( ESHADERMETHOD shader, float contrast ) override;
    virtual void                shaderSetConvertFullColorRange( ESHADERMETHOD shader, bool convertFullRange ) override;

    virtual int                 shaderGetVertexLoc( ESHADERMETHOD shader ) override;
    virtual int                 shaderGetYcoordLoc( ESHADERMETHOD shader ) override;
    virtual int                 shaderGetUcoordLoc( ESHADERMETHOD shader ) override;
    virtual int                 shaderGetVcoordLoc( ESHADERMETHOD shader ) override;

    virtual void                shaderSetMatrices( ESHADERMETHOD shader, const float *p, const float *m ) override;
    virtual void                shaderSetAlpha( ESHADERMETHOD shader, float alpha ) override;

    virtual void                shaderSetFlags( ESHADERMETHOD shader, unsigned int flags ) override;
    virtual void                shaderSetFormat( ESHADERMETHOD shader, EShaderFormat format ) override;
    virtual void                shaderSourceTexture( ESHADERMETHOD shader, int ytex ) override;
    virtual void                shaderSetStepX( ESHADERMETHOD shader, float stepX ) override;
    virtual void                shaderSetStepY( ESHADERMETHOD shader, float stepY )  override;

    // filter shader
    virtual bool                shaderGetTextureFilter( ESHADERMETHOD shader, int& filter ) override;
    virtual int                 shaderGetcoordLoc( ESHADERMETHOD shader ) override;

    // renderqt
    virtual int                 shaderVertexAttribPointer( ESHADERMETHOD shader, unsigned int index, int size, int type, bool normalized, int stride, const void * pointer ) override;
    virtual void                shaderEnableVertexAttribArray( ESHADERMETHOD shader, int arrayId ) override;
    virtual void                shaderDisableVertexAttribArray( ESHADERMETHOD shader, int arrayId ) override;

	void						onModuleState( EMediaModule moduleNum, EModuleState moduleState );


    //============================================================================
    //=== to gui media/render ===//
    //============================================================================

    virtual void                verifyGlState( const char* msg = nullptr ) override { RenderGlLogic::verifyGlState( msg ); } // show gl error if any

    //=== textures ===//
    void                        setActiveGlTexture( unsigned int activeTextureNum = 0 /* 0 == GL_TEXTURE0 , 1 == GL_TEXTURE1 etc*/ ) override;

    void                        createTextureObject( CTextureNlc * texture ) override;
    void                        destroyTextureObject( CTextureNlc * texture ) override;
    bool                        loadToGPU( CTextureNlc * texture ) override;
    void                        bindToUnit( CTextureNlc * texture, unsigned int unit ) override;

    void                        beginGuiTexture( CGUITextureNlc * guiTexture, NlcColor color ) override;
    void                        drawGuiTexture( CGUITextureNlc * textrue, float * x, float * y, float * z, const NlcRect& texture, const NlcRect& diffuse, int orientation ) override;
    void                        endGuiTexture( CGUITextureNlc * guiTexture ) override;
    void                        drawQuad( const NlcRect &rect, NlcColor color, CTextureBase * texture, const NlcRect * texCoords ) override;

    bool                        firstBegin( CGUIFontTTFNlc * font )  override;
    void                        lastEnd( CGUIFontTTFNlc * font ) override;

    CVertexBuffer               createVertexBuffer( CGUIFontTTFNlc * font, const std::vector<SVertex>& vertices )  override;

    void                        destroyVertexBuffer( CGUIFontTTFNlc * font, CVertexBuffer& vertBuffer )  override;

    virtual void                deleteHardwareTexture( CGUIFontTTFNlc * font )  override;
    virtual void                createStaticVertexBuffers( CGUIFontTTFNlc * font )   override;
    virtual void                destroyStaticVertexBuffers( CGUIFontTTFNlc * font )  override;

    //=== remder ===//
    void                        captureScreen( CScreenshotSurface * screenCaptrue, NlcRect& captureArea ) override;

    void                        toGuiRenderVideoFrame( int textureIdx, CRenderBuffer* videoBuffer );
    bool                        initRenderSystem() override;
    bool                        destroyRenderSystem() override;
    bool                        resetRenderSystem( int width, int height ) override;

    int                         getMaxTextureSize( void ) override { return m_MaxTextureSize; }

    bool                        beginRender() override{ return RenderGlLogic::beginRenderGl(); }
    bool                        endRender() override{ return RenderGlLogic::endRenderGl(); }
    void                        presentRender( bool rendered, bool videoLayer ) override{ RenderGlLogic::presentRenderGl( rendered, videoLayer ); }

    bool                        clearBuffers( NlcColor color ) override;
    bool                        isExtSupported( const char* extension ) override;

    void                        setVSync( bool vsync ) override;
    void                        resetVSync() override {  }

    void                        setViewPort( const NlcRect& viewPort ) override;
    void                        getViewPort( NlcRect& viewPort ) override;

    bool                        scissorsCanEffectClipping() override;
    NlcRect                     clipRectToScissorRect( const NlcRect &rect ) override;
    void                        setScissors( const NlcRect& rect ) override;
    void                        resetScissors() override;

    void                        captureStateBlock() override;
    void                        applyStateBlock() override;

    void                        setCameraPosition( const NlcPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f ) override;

    void                        applyHardwareTransform( const TransformMatrix &matrix ) override;
    void                        restoreHardwareTransform() override;
    bool                        supportsStereo( RENDER_STEREO_MODE /*mode*/ ) const override { return false; }

    bool                        testRender() override;

    void                        project( float &x, float &y, float &z ) override;

    // frame buffers
    virtual void                frameBufferGen( int bufCount, unsigned int* fboId ) override;
    virtual void                frameBufferDelete( int bufCount, unsigned int* fboId ) override;
    virtual void                frameBufferTexture2D( int target, unsigned int texureId )  override;
    virtual void                frameBufferBind( unsigned int fboId ) override;
    virtual bool                frameBufferStatus() override;

    // gl functions
    void                        glFuncDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) override;
    void                        glFuncDisable( GLenum cap ) override;
    void                        glFuncBindTexture( GLenum target, GLuint texture ) override;
    void                        glFuncViewport( GLint x, GLint y, GLsizei width, GLsizei height ) override;
    void                        glFuncScissor( GLint x, GLint y, GLsizei width, GLsizei height ) override;

    virtual void                glFuncGenTextures( GLsizei n, GLuint * textures ) override;
    virtual void                glFuncDeleteTextures( GLsizei n, const GLuint *textures ) override;
    virtual void                glFuncTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels ) override;
    virtual void                glFuncTexParameteri( GLenum target, GLenum pname, GLint param ) override;
    virtual void                glFuncReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) override;
    virtual void                glFuncPixelStorei( GLenum pname, GLint param ) override;
    virtual void                glFuncFinish() override;
    
    virtual void                glFuncEnable( GLenum cap ) override;
    virtual void                glFuncTexSubImage2D( GLenum target, GLint level,
                                                       GLint xoffset, GLint yoffset,
                                                       GLsizei width, GLsizei height,
                                                       GLenum format, GLenum type,
                                                       const GLvoid *pixels ) override;
    virtual void                glFuncBlendFunc( GLenum sfactor, GLenum dfactor ) override;
    virtual void                glFuncVertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer ) override;
    virtual void                glFuncDisableVertexAttribArray( GLuint index ) override;
    virtual void                glFuncEnableVertexAttribArray( GLuint index ) override;
    virtual void                glFuncDrawArrays( GLenum mode, GLint first, GLsizei count ) override;
    
    void                        onThreadFrameRendered() { emit signalFrameRendered(); }

public slots:
    void                        slotOnFrameRendered();

protected:
    void                        paintEvent( QPaintEvent * ) override;

    virtual void				showEvent( QShowEvent* ev ) override;
    virtual void				hideEvent( QHideEvent* ev ) override;
    virtual void				closeEvent( QCloseEvent * ev ) override;
    virtual void                resizeEvent( QResizeEvent* ev ) override;

    virtual void				mousePressEvent( QMouseEvent * ev ) override;
    virtual void				mouseReleaseEvent( QMouseEvent * ev ) override;
    virtual void                mouseMoveEvent( QMouseEvent * ev ) override;

    virtual void                keyPressEvent( QKeyEvent * ev ) override;
    virtual void                keyReleaseEvent( QKeyEvent * ev ) override;

    void                        initTextures();
    void                        handleGlResize( int width, int height );
 
    //=== vars ===//
    AppCommon&				    m_MyApp;
    EventsQtToNlc               m_QtToPlayerNlc;

    GLuint                      m_TextureIds[ MAX_RENDER_PLANES ];
    GlTextureSize               m_TexSize[ MAX_RENDER_PLANES ];
    bool                        m_TexturesInited{ false };
    GlTextureDescriptor         m_TexDescriptor;
    int                         m_MaxTextureSize = 2048;

    NlcRenderFrame *            m_Frame = nullptr;

    // render
    int                         m_SrcWidth = 320;
    int                         m_SrcHeight = 240;
    GLint                       m_viewPort[ 4 ];

	// sizing window
    QSize                       m_ScreenSize;
 
    QMatrix4x4                  m_ColorMatrix;

    bool                        m_RenderLogicInitialized{ false };
};
