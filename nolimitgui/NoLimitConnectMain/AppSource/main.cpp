//============================================================================
// Copyright (C) 2023 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include <QWidget> // must be declared first or Qt will error in qmetatype.h 2167:23: array subscript value 53 is outside the bounds

#include "GuiHelpers.h"
#include "GuiThreadMainLoader.h"
#include "GuiThreadSettingsLoader.h"
#include "NlcCommonConfig.h"

#include <src/AppCommon.h>
#include <src/AppSettings.h>
#include <src/AppTranslate.h>
#include <src/HomeWindow.h>
#include <src/GuiParams.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSettings>
#include <QStringList>
#include <QStandardPaths>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QCoreApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJniEnvironment>
#include <QtCore/private/qandroidextras_p.h>
#endif

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxGlobals.h>
#include <CoreLib/VxFileUtil.h>
#include <CoreLib/VxTime.h>

#include "AccountMgr.h"
#include "GuiHelpers.h"

#include <NetLib/VxPeerMgr.h>

#include <QLoggingCategory>
#include <ostream>
#include <string>

namespace {

#if defined(Q_OS_LINUX)
    QString getPreferredLinuxHomePath()
    {
        const QByteArray snapName = qgetenv( "SNAP_NAME" );
        if( snapName == "code" )
        {
            const QByteArray snapRealHome = qgetenv( "SNAP_REAL_HOME" );
            if( !snapRealHome.isEmpty() )
            {
                return QString::fromUtf8( snapRealHome );
            }

            const QByteArray realHome = qgetenv( "REAL_HOME" );
            if( !realHome.isEmpty() )
            {
                return QString::fromUtf8( realHome );
            }
        }

        return QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
    }
#endif // defined(Q_OS_LINUX)

    //============================================================================
    void copyBundledTranslationsIfRequired()
    {
        static const char* kTranslationFiles[] =
        {
            "nolimitconnect_ar_SA.qm",
            "nolimitconnect_de_DE.qm",
            "nolimitconnect_es_ES.qm",
            "nolimitconnect_fr_FR.qm",
            "nolimitconnect_hi_IN.qm",
            "nolimitconnect_id_ID.qm",
            "nolimitconnect_ja_JP.qm",
            "nolimitconnect_ko_KR.qm",
            "nolimitconnect_pt_PT.qm",
            "nolimitconnect_ru_RU.qm",
            "nolimitconnect_th_TH.qm",
            "nolimitconnect_zh_CN.qm",
        };

        std::string translationsDir = VxGetTranslationsDirectory();
        if( translationsDir.empty() )
        {
            return;
        }

        VxFileUtil::assurePathEndWithSlash( translationsDir );
        VxFileUtil::makeDirectory( translationsDir.c_str() );

        for( const char* translationFile : kTranslationFiles )
        {
            std::string onDiskFile = translationsDir + translationFile;
            if( false == VxFileUtil::fileExists( onDiskFile.c_str() ) )
            {
                const QString resourcePath = QString( ":/AppRes/translations/%1" ).arg( translationFile );
                GuiHelpers::copyResourceToOnDiskFile( resourcePath.toUtf8().constData(), onDiskFile.c_str() );
            }
        }
    }

#if defined(Q_OS_ANDROID)
    //============================================================================
    void setAndroidAudioMode()
    {
        QJniObject activity =
            QNativeInterface::QAndroidApplication::context();

        if (!activity.isValid())
            return;

        QJniObject audioService =
            activity.callObjectMethod(
                "getSystemService",
                "(Ljava/lang/String;)Ljava/lang/Object;",
                QJniObject::fromString("audio").object<jstring>()
                );

        if (!audioService.isValid())
            return;

        // Disable Android Audio Effects
        // Otherwise Android AEC/NS will clash with built in.
        // AudioManager.MODE_IN_COMMUNICATION = 3
        audioService.callMethod<void>(
            "setMode",
            "(I)V",
            3
            );

        // Force speakerphone on (important for AEC reference path)
        audioService.callMethod<void>(
            "setSpeakerphoneOn",
            "(Z)V",
            true
            );
    }
#endif

    //============================================================================
    void setupRootStorageDirectory()
    {
        std::string strRootStorageDataDir;

        //=== determine root path to store all application data and settings etc ===//
        QString dataPath;

    
#if defined(FLATPAKBUILD)
    // flatpak provides persistent XDG storage under ~/.var/app/<app-id>/
    dataPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
#else
# if defined(Q_OS_LINUX)
        QString preferredHome = getPreferredLinuxHomePath();
        if( !preferredHome.isEmpty() )
        {
            dataPath = preferredHome + "/.local/share/" + VxGetApplicationNameNoSpaces();
        }
        else
        {
            dataPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
        }
# else
        dataPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
# endif
#endif // defined(FLATPAKBUILD)

        strRootStorageDataDir = dataPath.toUtf8().constData();

#ifdef DEBUG
        // remove the D from the end so release and debug builds use the same storage directory
        if( !strRootStorageDataDir.empty() && ( strRootStorageDataDir.c_str()[ strRootStorageDataDir.length() - 1 ] == 'D' ) )
        {
            strRootStorageDataDir = strRootStorageDataDir.substr( 0, strRootStorageDataDir.length() - 1 );
        }
#endif // DEBUG

        VxFileUtil::makeForwardSlashPath( strRootStorageDataDir );
        strRootStorageDataDir += "/";

        // it used to be we could put data in different default locations
        // but sandboxing and other issues have made it so all data must be in one root directory 
        std::string strRootAppDataDir = strRootStorageDataDir +  "app/";

        VxSetAppDirectory( eAppData, strRootAppDataDir );

        // No need to put application in path because when call QCoreApplication::setApplicationName("AppName")
        // it made it a sub directory of DataLocation
        VxSetRootDataStorageDirectory( strRootAppDataDir.c_str() );
        VxSetRootUserDataDirectory( strRootAppDataDir.c_str() );

        std::string rootXferDir = strRootStorageDataDir + "xfer/";

        VxFileUtil::makeForwardSlashPath( rootXferDir );
        VxFileUtil::assurePathEndWithSlash( rootXferDir );

        rootXferDir += VxGetApplicationNameNoSpaces();
        VxFileUtil::assurePathEndWithSlash( rootXferDir );
        VxFileUtil::makeDirectory( rootXferDir.c_str() );

        // sets root of data transfer directories

        VxSetRootXferDirectory( rootXferDir.c_str() );

        if( !VxFileUtil::directoryExists( rootXferDir.c_str() ) )
        {
            LogMsg( LOG_ERROR, "%s Could not create xfer dir %s", __func__, rootXferDir.c_str());
        }
    }
}

//============================================================================
int runApplication( QApplication* myApp, int argc, char** argv )
{
    // Silence SSL-related warnings from Multimedia and Network
    // SSL is not needed or installed
    QLoggingCategory::setFilterRules(
        "qt.multimedia.symbolsresolver.warning=false\n"
        "qt.network.ssl.warning=false"
    );

    const int startupStartMs = GetApplicationAliveMs();
    auto logStartupStep = []( const char* stepName, int stepStartMs ) -> int
    {
        const int nowMs = GetApplicationAliveMs();
        LogModule( eLogStartup, LOG_VERBOSE, "runApplication step %s took %d ms (alive %d ms)",
                   stepName, nowMs - stepStartMs, nowMs );
        return nowMs;
    };

    // NOTE OrganizationName and ApplicationName become part of data storage location path
    QCoreApplication::setOrganizationName( "" ); // leave blank or will become part of data storage path
    QCoreApplication::setApplicationName( VxGetApplicationNameNoSpaces() );
    QCoreApplication::setApplicationVersion( VxGetAppVersionString() );
    QGuiApplication::setApplicationDisplayName( VxGetApplicationTitle() );
    QCoreApplication::setOrganizationDomain( VxGetCompanyDomain() );

 #if defined(Q_OS_ANDROID)
    setAndroidAudioMode();
#endif

    QSettings settings (VxGetCompanyDomain(), VxGetApplicationNameNoSpaces() );

    // TODO fix and apply theme to age confirm dialog.. Android Qt 6.8.3 does not send button click in ActivityMsgBoxYesNo when done before startup
    if (!settings.contains("isAdult")) {
        QString warnAdultTitle = QObject::tr( "You must be an adult to use No Limit Connect application" );
        QString warnAdultBody = QObject::tr( "Although No Limit Connect does not host any offensive media, users of No Limit Connect may host offensive material or act in an offensive manner.\n"
                                            "No Limit Connect does not monitor or log any user actions or content.\n\n"
                                            "Are you an adult and at least 18 years old?" );

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, warnAdultTitle,
                                      warnAdultBody,
                                      QMessageBox::Yes | QMessageBox::No);

        bool isAdult = (reply == QMessageBox::Yes);

        if (!isAdult) {
            QString deniedTitle = QObject::tr("Access Denied");
            QString deniedBody = QObject::tr("You must be 18 or older to use this application.");
            QMessageBox::information(nullptr, deniedTitle, deniedBody);
            return 0; // Exit application
        }

        settings.setValue("isAdult", isAdult);
    }

    int timeStart = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "runApplication startup begin at %d ms", timeStart );

    static AppSettings appSettings;
    GuiThreadSettingsLoader threadSettingsLoader(appSettings);

    // register types first so connections made in construction have registered signal/slot values
    AppCommon::registerMetaData();
    int timeRegisterMetadata = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "runApplication registerMetaData took %d ms",
               timeRegisterMetadata - timeStart );



    // must be ran after application name is set or paths with app name may be lower case instead of upper case
    int stepStartMs = GetApplicationAliveMs();
    setupRootStorageDirectory();
    stepStartMs = logStartupStep( "setupRootStorageDirectory", stepStartMs );

    copyBundledTranslationsIfRequired();
    stepStartMs = logStartupStep( "copyBundledTranslationsIfRequired", stepStartMs );

    threadSettingsLoader.start();
    stepStartMs = logStartupStep( "threadSettingsLoader.start", stepStartMs );


    // initialize display scaling etc
    // the best method I have found to scale the gui is to use the default font height as the scaling factor
    QFontMetrics fontMetrics( myApp->font() );
    GuiParams::initGuiParams(fontMetrics.height());
    stepStartMs = logStartupStep( "GuiParams::initGuiParams", stepStartMs );

    GuiThreadMainLoader mainLoaderThread;
    mainLoaderThread.start();
    stepStartMs = logStartupStep( "mainLoaderThread.start", stepStartMs );

    int timeInitFonts = GetApplicationAliveMs();

    LogModule( eLogStartup, LOG_VERBOSE, "root storage disk space path %s %s",
               VxGetRootDataStorageDirectory().c_str(), VxFileUtil::describeDiskSpace( VxGetRootDataStorageDirectory() ).c_str() );

    bool haveWaitTime{ false };
    if( !mainLoaderThread.getIsLoadComplete() )
    {
        haveWaitTime = true;
        LogModule( eLogStartup, LOG_VERBOSE, "%s waiting for main loader thread", __func__ );
        while( !mainLoaderThread.getIsLoadComplete() )
        {
            GuiHelpers::processQtEvents();
        }

        int waitMainLoaderThread = GetApplicationAliveMs();
        LogModule( eLogStartup, LOG_VERBOSE, "%s waited for main loader thread %d ms", __func__, waitMainLoaderThread - timeInitFonts );
    }

    if( !threadSettingsLoader.getIsSettingsLoaded() )
    {
        haveWaitTime = true;
        int waitStart = GetApplicationAliveMs();
        while( !threadSettingsLoader.getIsSettingsLoaded() )
        {
            GuiHelpers::processQtEvents();
        }

        // now that settings are loaded we can start using LogModule
        if( LogEnabled( eLogStartup ) )
        {
            int waitEnd = GetApplicationAliveMs();
            LogModule( eLogStartup, LOG_VERBOSE, "%s waited for settings loader thread %d ms", __func__, waitEnd - waitStart );
        }
    }

    // Ensure loader threads are fully finished before continuing startup/shutdown lifecycle.
    if( mainLoaderThread.isRunning() )
    {
        mainLoaderThread.wait();
    }
    if( threadSettingsLoader.isRunning() )
    {
        threadSettingsLoader.wait();
    }

    int timePreStartApp = GetApplicationAliveMs();
    if( LogEnabled( eLogStartup ) )
    {     
        if( haveWaitTime )
        {
            LogModule( eLogStartup, LOG_VERBOSE, "%s time waiting for loaders %d ms", __func__, timePreStartApp - timeInitFonts );
        }

        LogModule( eLogStartup, LOG_VERBOSE, "%s time register %d app fonts %d", __func__, timeRegisterMetadata - timeStart,
                   timeInitFonts - timeRegisterMetadata );
    }

    const ELanguageType selectedLanguage = appSettings.getSelectedLanguage();
    AppTranslate::applyLanguage( selectedLanguage );

    AppCommon& appCommon = CreateAppInstance( myApp, appSettings );

    int createAppCommon = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "runApplication CreateAppInstance took %d ms",
               createAppCommon - timePreStartApp );

    std::string fontDir = VxGetFontDirectory();
    std::string defaultFont = fontDir + "arial.ttf";
    if( false == VxFileUtil::fileExists( defaultFont.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/arial.ttf", defaultFont.c_str() );
    }

    std::string teletextFont = fontDir + "teletext.ttf";
    if( false == VxFileUtil::fileExists( teletextFont.c_str() ) )
    {
        GuiHelpers::copyResourceToOnDiskFile( ":/AppRes/Resources/teletext.ttf", teletextFont.c_str() );
    }

    std::string translationsDir = VxGetTranslationsDirectory();

    int copyFonts = GetApplicationAliveMs();
    LogModule( eLogStartup, LOG_VERBOSE, "runApplication font verification/copy took %d ms",
               copyFonts - createAppCommon );

    if( !appCommon.loadWithThread() )
    {
        LogMsg( LOG_ERROR, "%s user is not of legal age ", __func__ );
        return false;
    }

    if( LogEnabled( eLogStartup ) )
    {
        int timeNow = GetApplicationAliveMs();
        LogModule( eLogStartup, LOG_VERBOSE, "%s setup %d md create AppCommon %d font copy %d load %d total %d ms", __func__,
                   timePreStartApp - timeStart, createAppCommon - timePreStartApp, copyFonts - createAppCommon, timeNow - copyFonts, timeNow - timeStart );
        LogModule( eLogStartup, LOG_VERBOSE, "%s total startup since runApplication entry %d ms", __func__,
                   timeNow - startupStartMs );
    }

    int result = myApp->exec();

	return result;
}

//============================================================================
int main( int argc, char** argv )
{
#if defined(TARGET_OS_WINDOWS)
    // unfortunatly this does not fix the issue but since it only happens in debug builds the crash on shutdown can be ignored
    // QTBUG-118330 
    qputenv( "QT_FFMPEG_HWACCEL", "none" ); // to stop crash by Qt6Multimediad.dll not releasing d3d11 textures

    // Force Qt render backends to OpenGL on Windows.
    // Note: some DirectX-related system DLLs may still load due to OS/driver internals.
    qputenv( "QT_OPENGL", "desktop" );
    qputenv( "QT_DISABLE_ANGLE", "1" );
    qputenv( "QSG_RHI_BACKEND", "opengl" );
#endif // defined(TARGET_OS_WINDOWS)

    int retVal{ 0 };

    VxSetGuiThreadId();
    LogModule( eLogStartup, LOG_VERBOSE, "main Creating QApplication at %d ms", GetApplicationAliveMs() );

    QCoreApplication::addLibraryPath( "." );

    QApplication::setAttribute( Qt::AA_ShareOpenGLContexts );

#if !defined(Q_OS_ANDROID)
    QApplication::setAttribute( Qt::AA_DontCheckOpenGLContextThreadAffinity );
#endif // !defined(Q_OS_ANDROID)

    // for some reason QApplication must be newed or does not initialize
    QApplication* myApp = new QApplication( argc, argv );

    try
    {
        retVal = runApplication( myApp, argc, argv );
    }
    catch( ... )
    {
        // clean up here, e.g. save the session
        // and close all config files.
        LogMsg( LOG_ERROR, "ERROR Application threw and exception" );

        delete myApp;
        myApp = nullptr;

        return EXIT_FAILURE; // exit the application
    }

    delete myApp;
    myApp = nullptr;

    return retVal;
}
