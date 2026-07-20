//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxAndroid.h"

#include <CoreLib/VxDefs.h>
#include <CoreLib/VxDebug.h>
#include <CoreLib/VxFileUtil.h>

#include <filesystem>
#include <system_error>

#if defined(TARGET_OS_ANDROID)
#include <CoreLib/VxJni.h>
#include <jni.h>
#endif // defined(TARGET_OS_ANDROID)

namespace fs = std::filesystem;

namespace
{
bool NormalizeDirectoryPath( const char* dirPath, std::string& normalizedPath )
{
    if( nullptr == dirPath || 0 == dirPath[0] )
    {
        return false;
    }

    normalizedPath = dirPath;
    VxFileUtil::removeTrailingDirectorySlash( normalizedPath );
    return !normalizedPath.empty();
}

void FillPathInfoFromDirectoryEntry( const fs::directory_entry& entry, VxAndroidPathInfo& pathInfo )
{
    pathInfo.m_FileNameAndPath = entry.path().u8string();
    pathInfo.m_FileName = entry.path().filename().u8string();

    std::error_code ec;
    pathInfo.m_IsDirectory = entry.is_directory( ec );
    if( ec )
    {
        pathInfo.m_IsDirectory = false;
    }

    pathInfo.m_IsReadable = entry.exists( ec );
    if( ec )
    {
        pathInfo.m_IsReadable = false;
    }

    pathInfo.m_IsExecutable = false;
    pathInfo.m_FileLength = 0;
    if( !pathInfo.m_IsDirectory )
    {
        const auto fileSize = entry.file_size( ec );
        if( !ec )
        {
            pathInfo.m_FileLength = static_cast<int64_t>( fileSize );
        }
    }
}

bool IsProviderPath( const char* fileNameAndPath )
{
    return VxFileUtil::fileIsProviderFile( fileNameAndPath );
}

#if defined(TARGET_OS_ANDROID)
bool HasAndClearPendingException( JNIEnv* env, const char* funcName )
{
    if( nullptr == env || !env->ExceptionCheck() )
    {
        return false;
    }

    env->ExceptionDescribe();
    env->ExceptionClear();
    LogMsg( LOG_ERROR, "%s JNI exception", funcName );
    return true;
}

std::string JStringToUtf8( JNIEnv* env, jstring jStr )
{
    if( nullptr == env || nullptr == jStr )
    {
        return std::string();
    }

    const char* chars = env->GetStringUTFChars( jStr, nullptr );
    if( nullptr == chars )
    {
        return std::string();
    }

    std::string outStr( chars );
    env->ReleaseStringUTFChars( jStr, chars );
    return outStr;
}

jstring Utf8ToJString( JNIEnv* env, const char* str )
{
    if( nullptr == env || nullptr == str )
    {
        return nullptr;
    }

    return env->NewStringUTF( str );
}

jobject ParseUri( JNIEnv* env, const char* uriString )
{
    jclass uriClass = env->FindClass( "android/net/Uri" );
    if( nullptr == uriClass || HasAndClearPendingException( env, __func__ ) )
    {
        return nullptr;
    }

    jmethodID parseMethod = env->GetStaticMethodID( uriClass,
                                                     "parse",
                                                     "(Ljava/lang/String;)Landroid/net/Uri;" );
    if( nullptr == parseMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( uriClass );
        return nullptr;
    }

    jstring jUriString = Utf8ToJString( env, uriString );
    if( nullptr == jUriString )
    {
        env->DeleteLocalRef( uriClass );
        return nullptr;
    }

    jobject uriObj = env->CallStaticObjectMethod( uriClass, parseMethod, jUriString );
    const bool hadException = HasAndClearPendingException( env, __func__ );

    env->DeleteLocalRef( jUriString );
    env->DeleteLocalRef( uriClass );
    return hadException ? nullptr : uriObj;
}

std::string UriToString( JNIEnv* env, jobject uriObj )
{
    if( nullptr == env || nullptr == uriObj )
    {
        return std::string();
    }

    jclass uriClass = env->GetObjectClass( uriObj );
    if( nullptr == uriClass || HasAndClearPendingException( env, __func__ ) )
    {
        return std::string();
    }

    jmethodID toStringMethod = env->GetMethodID( uriClass, "toString", "()Ljava/lang/String;" );
    if( nullptr == toStringMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( uriClass );
        return std::string();
    }

    jstring jUriString = (jstring)env->CallObjectMethod( uriObj, toStringMethod );
    const bool hadException = HasAndClearPendingException( env, __func__ );

    std::string uriString;
    if( !hadException && nullptr != jUriString )
    {
        uriString = JStringToUtf8( env, jUriString );
        env->DeleteLocalRef( jUriString );
    }

    env->DeleteLocalRef( uriClass );
    return uriString;
}

jobject GetContentResolver( JNIEnv* env, jobject appContext )
{
    jclass contextClass = env->GetObjectClass( appContext );
    if( nullptr == contextClass || HasAndClearPendingException( env, __func__ ) )
    {
        return nullptr;
    }

    jmethodID getResolverMethod = env->GetMethodID( contextClass,
                                                     "getContentResolver",
                                                     "()Landroid/content/ContentResolver;" );
    if( nullptr == getResolverMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( contextClass );
        return nullptr;
    }

    jobject resolverObj = env->CallObjectMethod( appContext, getResolverMethod );
    const bool hadException = HasAndClearPendingException( env, __func__ );
    env->DeleteLocalRef( contextClass );
    return hadException ? nullptr : resolverObj;
}

jobject QueryCursor( JNIEnv* env, jobject resolverObj, jobject uriObj )
{
    jclass resolverClass = env->GetObjectClass( resolverObj );
    if( nullptr == resolverClass || HasAndClearPendingException( env, __func__ ) )
    {
        return nullptr;
    }

    jmethodID queryMethod = env->GetMethodID( resolverClass,
                                              "query",
                                              "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;" );
    if( nullptr == queryMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( resolverClass );
        return nullptr;
    }

    jobject cursorObj = env->CallObjectMethod( resolverObj, queryMethod, uriObj, nullptr, nullptr, nullptr, nullptr );
    const bool hadException = HasAndClearPendingException( env, __func__ );
    env->DeleteLocalRef( resolverClass );
    return hadException ? nullptr : cursorObj;
}

int CursorGetColumnIndex( JNIEnv* env, jobject cursorObj, const char* columnName )
{
    jclass cursorClass = env->GetObjectClass( cursorObj );
    if( nullptr == cursorClass || HasAndClearPendingException( env, __func__ ) )
    {
        return -1;
    }

    jmethodID columnIndexMethod = env->GetMethodID( cursorClass, "getColumnIndex", "(Ljava/lang/String;)I" );
    if( nullptr == columnIndexMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( cursorClass );
        return -1;
    }

    jstring jColName = Utf8ToJString( env, columnName );
    if( nullptr == jColName )
    {
        env->DeleteLocalRef( cursorClass );
        return -1;
    }

    jint idx = env->CallIntMethod( cursorObj, columnIndexMethod, jColName );
    const bool hadException = HasAndClearPendingException( env, __func__ );
    env->DeleteLocalRef( jColName );
    env->DeleteLocalRef( cursorClass );
    return hadException ? -1 : (int)idx;
}

std::string CursorGetString( JNIEnv* env, jobject cursorObj, int colIdx )
{
    if( colIdx < 0 )
    {
        return std::string();
    }

    jclass cursorClass = env->GetObjectClass( cursorObj );
    if( nullptr == cursorClass || HasAndClearPendingException( env, __func__ ) )
    {
        return std::string();
    }

    jmethodID getStringMethod = env->GetMethodID( cursorClass, "getString", "(I)Ljava/lang/String;" );
    if( nullptr == getStringMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( cursorClass );
        return std::string();
    }

    jstring jVal = (jstring)env->CallObjectMethod( cursorObj, getStringMethod, (jint)colIdx );
    const bool hadException = HasAndClearPendingException( env, __func__ );
    std::string outVal;
    if( !hadException && nullptr != jVal )
    {
        outVal = JStringToUtf8( env, jVal );
        env->DeleteLocalRef( jVal );
    }

    env->DeleteLocalRef( cursorClass );
    return outVal;
}

int64_t CursorGetLong( JNIEnv* env, jobject cursorObj, int colIdx )
{
    if( colIdx < 0 )
    {
        return 0;
    }

    jclass cursorClass = env->GetObjectClass( cursorObj );
    if( nullptr == cursorClass || HasAndClearPendingException( env, __func__ ) )
    {
        return 0;
    }

    jmethodID getLongMethod = env->GetMethodID( cursorClass, "getLong", "(I)J" );
    if( nullptr == getLongMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( cursorClass );
        return 0;
    }

    jlong val = env->CallLongMethod( cursorObj, getLongMethod, (jint)colIdx );
    const bool hadException = HasAndClearPendingException( env, __func__ );
    env->DeleteLocalRef( cursorClass );
    return hadException ? 0 : (int64_t)val;
}

bool CursorMoveToFirst( JNIEnv* env, jobject cursorObj )
{
    jclass cursorClass = env->GetObjectClass( cursorObj );
    if( nullptr == cursorClass || HasAndClearPendingException( env, __func__ ) )
    {
        return false;
    }

    jmethodID moveMethod = env->GetMethodID( cursorClass, "moveToFirst", "()Z" );
    if( nullptr == moveMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( cursorClass );
        return false;
    }

    jboolean moved = env->CallBooleanMethod( cursorObj, moveMethod );
    const bool hadException = HasAndClearPendingException( env, __func__ );
    env->DeleteLocalRef( cursorClass );
    return !hadException && moved;
}

bool CursorMoveToNext( JNIEnv* env, jobject cursorObj )
{
    jclass cursorClass = env->GetObjectClass( cursorObj );
    if( nullptr == cursorClass || HasAndClearPendingException( env, __func__ ) )
    {
        return false;
    }

    jmethodID moveMethod = env->GetMethodID( cursorClass, "moveToNext", "()Z" );
    if( nullptr == moveMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( cursorClass );
        return false;
    }

    jboolean moved = env->CallBooleanMethod( cursorObj, moveMethod );
    const bool hadException = HasAndClearPendingException( env, __func__ );
    env->DeleteLocalRef( cursorClass );
    return !hadException && moved;
}

void CursorClose( JNIEnv* env, jobject cursorObj )
{
    if( nullptr == env || nullptr == cursorObj )
    {
        return;
    }

    jclass cursorClass = env->GetObjectClass( cursorObj );
    if( nullptr == cursorClass || HasAndClearPendingException( env, __func__ ) )
    {
        return;
    }

    jmethodID closeMethod = env->GetMethodID( cursorClass, "close", "()V" );
    if( nullptr != closeMethod )
    {
        env->CallVoidMethod( cursorObj, closeMethod );
        HasAndClearPendingException( env, __func__ );
    }

    env->DeleteLocalRef( cursorClass );
}

bool GetProviderPathInfo( const char* fileNameAndPath, VxAndroidPathInfo& pathInfo )
{
    JNIEnv* env = VxJni::getJavaEnv();
    if( nullptr == env )
    {
        return false;
    }

    jobject appContext = VxJni::getApplicationContext( env );
    if( nullptr == appContext )
    {
        return false;
    }

    jobject resolverObj = GetContentResolver( env, appContext );
    if( nullptr == resolverObj )
    {
        env->DeleteLocalRef( appContext );
        return false;
    }

    jobject uriObj = ParseUri( env, fileNameAndPath );
    if( nullptr == uriObj )
    {
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return false;
    }

    jobject cursorObj = QueryCursor( env, resolverObj, uriObj );
    if( nullptr == cursorObj )
    {
        env->DeleteLocalRef( uriObj );
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return false;
    }

    bool ok = false;
    if( CursorMoveToFirst( env, cursorObj ) )
    {
        const int nameIdx = CursorGetColumnIndex( env, cursorObj, "_display_name" );
        const int sizeIdx = CursorGetColumnIndex( env, cursorObj, "_size" );
        const int mimeIdx = CursorGetColumnIndex( env, cursorObj, "mime_type" );

        std::string displayName = CursorGetString( env, cursorObj, nameIdx );
        std::string mimeType = CursorGetString( env, cursorObj, mimeIdx );
        const int64_t fileLen = CursorGetLong( env, cursorObj, sizeIdx );

        if( displayName.empty() )
        {
            displayName = fileNameAndPath;
        }

        pathInfo.m_FileName = displayName;
        pathInfo.m_FileNameAndPath = fileNameAndPath;
        pathInfo.m_FileLength = fileLen;
        pathInfo.m_IsReadable = true;
        pathInfo.m_IsExecutable = false;
        pathInfo.m_IsDirectory = mimeType == "vnd.android.document/directory";
        ok = true;
    }

    CursorClose( env, cursorObj );
    env->DeleteLocalRef( cursorObj );
    env->DeleteLocalRef( uriObj );
    env->DeleteLocalRef( resolverObj );
    env->DeleteLocalRef( appContext );
    return ok;
}

int ListProviderDirectory( const char* srcDir, std::vector<VxAndroidPathInfo>& fileList )
{
    JNIEnv* env = VxJni::getJavaEnv();
    if( nullptr == env )
    {
        return -1;
    }

    jobject appContext = VxJni::getApplicationContext( env );
    if( nullptr == appContext )
    {
        return -1;
    }

    jobject resolverObj = GetContentResolver( env, appContext );
    if( nullptr == resolverObj )
    {
        env->DeleteLocalRef( appContext );
        return -1;
    }

    jobject treeUriObj = ParseUri( env, srcDir );
    if( nullptr == treeUriObj )
    {
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return -1;
    }

    jclass docsClass = env->FindClass( "android/provider/DocumentsContract" );
    if( nullptr == docsClass || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( treeUriObj );
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return -1;
    }

    jmethodID getTreeDocIdMethod = env->GetStaticMethodID( docsClass,
                                                            "getTreeDocumentId",
                                                            "(Landroid/net/Uri;)Ljava/lang/String;" );
    jmethodID buildChildrenMethod = env->GetStaticMethodID( docsClass,
                                                             "buildChildDocumentsUriUsingTree",
                                                             "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;" );
    jmethodID buildDocUriMethod = env->GetStaticMethodID( docsClass,
                                                           "buildDocumentUriUsingTree",
                                                           "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;" );
    if( nullptr == getTreeDocIdMethod || nullptr == buildChildrenMethod || nullptr == buildDocUriMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( docsClass );
        env->DeleteLocalRef( treeUriObj );
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return -1;
    }

    jstring jTreeDocId = (jstring)env->CallStaticObjectMethod( docsClass, getTreeDocIdMethod, treeUriObj );
    if( nullptr == jTreeDocId || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( docsClass );
        env->DeleteLocalRef( treeUriObj );
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return -1;
    }

    jobject childUriObj = env->CallStaticObjectMethod( docsClass, buildChildrenMethod, treeUriObj, jTreeDocId );
    if( nullptr == childUriObj || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( jTreeDocId );
        env->DeleteLocalRef( docsClass );
        env->DeleteLocalRef( treeUriObj );
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return -1;
    }

    jobject cursorObj = QueryCursor( env, resolverObj, childUriObj );
    if( nullptr == cursorObj )
    {
        env->DeleteLocalRef( childUriObj );
        env->DeleteLocalRef( jTreeDocId );
        env->DeleteLocalRef( docsClass );
        env->DeleteLocalRef( treeUriObj );
        env->DeleteLocalRef( resolverObj );
        env->DeleteLocalRef( appContext );
        return -1;
    }

    const int nameIdx = CursorGetColumnIndex( env, cursorObj, "_display_name" );
    const int sizeIdx = CursorGetColumnIndex( env, cursorObj, "_size" );
    const int mimeIdx = CursorGetColumnIndex( env, cursorObj, "mime_type" );
    const int docIdIdx = CursorGetColumnIndex( env, cursorObj, "document_id" );

    if( CursorMoveToFirst( env, cursorObj ) )
    {
        do
        {
            VxAndroidPathInfo pathInfo;
            pathInfo.m_FileName = CursorGetString( env, cursorObj, nameIdx );
            pathInfo.m_FileLength = CursorGetLong( env, cursorObj, sizeIdx );
            const std::string mimeType = CursorGetString( env, cursorObj, mimeIdx );
            const std::string documentId = CursorGetString( env, cursorObj, docIdIdx );

            pathInfo.m_IsReadable = true;
            pathInfo.m_IsExecutable = false;
            pathInfo.m_IsDirectory = mimeType == "vnd.android.document/directory";

            if( !documentId.empty() )
            {
                jstring jDocId = Utf8ToJString( env, documentId.c_str() );
                if( nullptr != jDocId )
                {
                    jobject docUriObj = env->CallStaticObjectMethod( docsClass, buildDocUriMethod, treeUriObj, jDocId );
                    if( nullptr != docUriObj && !HasAndClearPendingException( env, __func__ ) )
                    {
                        pathInfo.m_FileNameAndPath = UriToString( env, docUriObj );
                        env->DeleteLocalRef( docUriObj );
                    }

                    env->DeleteLocalRef( jDocId );
                }
            }

            if( pathInfo.m_FileNameAndPath.empty() )
            {
                pathInfo.m_FileNameAndPath = srcDir;
            }

            if( !pathInfo.m_FileName.empty() )
            {
                fileList.emplace_back( std::move( pathInfo ) );
            }
        }
        while( CursorMoveToNext( env, cursorObj ) );
    }

    CursorClose( env, cursorObj );
    env->DeleteLocalRef( cursorObj );
    env->DeleteLocalRef( childUriObj );
    env->DeleteLocalRef( jTreeDocId );
    env->DeleteLocalRef( docsClass );
    env->DeleteLocalRef( treeUriObj );
    env->DeleteLocalRef( resolverObj );
    env->DeleteLocalRef( appContext );
    return 0;
}
#endif // defined(TARGET_OS_ANDROID)
}

namespace VxAndroid
{
//============================================================================
bool requestPermission( const char* permissionName )
{
#if defined(TARGET_OS_ANDROID)
    if( nullptr == permissionName || 0 == permissionName[0] )
    {
        return false;
    }

    if( VxJni::hasPermission( permissionName ) )
    {
        return true;
    }

    return VxJni::requestPermission( permissionName, 1001 );
#else
    (void)permissionName;
    return true;
#endif // defined(TARGET_OS_ANDROID)
}

//============================================================================
bool directoryExists( const char* dirPath )
{
    if( IsProviderPath( dirPath ) )
    {
#if defined(TARGET_OS_ANDROID)
    VxAndroidPathInfo pathInfo;
    return GetProviderPathInfo( dirPath, pathInfo ) && pathInfo.m_IsDirectory;
#else
    return false;
#endif // defined(TARGET_OS_ANDROID)
    }

    std::string normalizedPath;
    if( !NormalizeDirectoryPath( dirPath, normalizedPath ) )
    {
        return false;
    }

    std::error_code ec;
    return fs::is_directory( fs::u8path( normalizedPath ), ec ) && !ec;
}

//============================================================================
bool getPathInfo( const char* fileNameAndPath, VxAndroidPathInfo& pathInfo )
{
    pathInfo = VxAndroidPathInfo();
    if( nullptr == fileNameAndPath || 0 == fileNameAndPath[0] )
    {
        return false;
    }

    if( IsProviderPath( fileNameAndPath ) )
    {
    #if defined(TARGET_OS_ANDROID)
        return GetProviderPathInfo( fileNameAndPath, pathInfo );
    #else
        return false;
    #endif // defined(TARGET_OS_ANDROID)
    }

    std::error_code ec;
    const fs::path checkPath = fs::u8path( fileNameAndPath );
    if( !fs::exists( checkPath, ec ) || ec )
    {
        return false;
    }

    pathInfo.m_FileNameAndPath = fileNameAndPath;
    pathInfo.m_FileName = checkPath.filename().u8string();
    pathInfo.m_IsDirectory = fs::is_directory( checkPath, ec ) && !ec;
    pathInfo.m_IsReadable = true;
    pathInfo.m_IsExecutable = false;

    if( !pathInfo.m_IsDirectory )
    {
        const auto fileSize = fs::file_size( checkPath, ec );
        if( !ec )
        {
            pathInfo.m_FileLength = static_cast<int64_t>( fileSize );
        }
    }

    return true;
}

//============================================================================
int listDirectory( const char* srcDir, std::vector<VxAndroidPathInfo>& fileList )
{
    fileList.clear();
    if( nullptr == srcDir || 0 == srcDir[0] )
    {
        return -1;
    }

    if( IsProviderPath( srcDir ) )
    {
    #if defined(TARGET_OS_ANDROID)
        return ListProviderDirectory( srcDir, fileList );
    #else
        return -1;
    #endif // defined(TARGET_OS_ANDROID)
    }

    std::error_code ec;
    const fs::path browsePath = fs::u8path( srcDir );
    if( !fs::is_directory( browsePath, ec ) || ec )
    {
        return -1;
    }

    fs::directory_iterator iter( browsePath, ec );
    if( ec )
    {
        return -1;
    }

    for( const auto& entry : iter )
    {
        VxAndroidPathInfo pathInfo;
        FillPathInfoFromDirectoryEntry( entry, pathInfo );
        fileList.emplace_back( std::move( pathInfo ) );
    }

    return 0;
}

} // namespace VxAndroid
