#include "VxJni.h"

#ifdef TARGET_OS_ANDROID

#include <CoreLib/VxDebug.h>
#include <CoreLib/VxJava.h>

namespace
{
static jclass g_QtNativeClass = nullptr;
static jmethodID g_QtNativeActivityMethod = nullptr;

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

bool EnsureQtNativeBindings( JNIEnv* env )
{
    if( nullptr == env )
    {
        return false;
    }

    if( nullptr != g_QtNativeClass && nullptr != g_QtNativeActivityMethod )
    {
        return true;
    }

    jclass qtNativeLocal = env->FindClass( "org/qtproject/qt/android/QtNative" );
    if( nullptr == qtNativeLocal || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to find QtNative", __func__ );
        return false;
    }

    jmethodID activityMethod = env->GetStaticMethodID( qtNativeLocal,
                                                        "activity",
                                                        "()Landroid/app/Activity;" );
    if( nullptr == activityMethod || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to get QtNative.activity", __func__ );
        env->DeleteLocalRef( qtNativeLocal );
        return false;
    }

    jclass qtNativeGlobal = reinterpret_cast<jclass>( env->NewGlobalRef( qtNativeLocal ) );
    env->DeleteLocalRef( qtNativeLocal );
    if( nullptr == qtNativeGlobal || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to create global QtNative ref", __func__ );
        return false;
    }

    g_QtNativeClass = qtNativeGlobal;
    g_QtNativeActivityMethod = activityMethod;
    return true;
}
}

//============================================================================
bool VxJni::initJavaBindings( JNIEnv* env )
{
    return EnsureQtNativeBindings( env );
}

//============================================================================
void VxJni::shutdownJavaBindings( JNIEnv* env )
{
    if( nullptr != env && nullptr != g_QtNativeClass )
    {
        env->DeleteGlobalRef( g_QtNativeClass );
    }

    g_QtNativeClass = nullptr;
    g_QtNativeActivityMethod = nullptr;
}

//============================================================================
JNIEnv* VxJni::getJavaEnv( void )
{
    JNIEnv* env = VxJava::getJavaEnv();
    if( nullptr == env )
    {
        LogMsg( LOG_ERROR, "%s failed to get Java env", __func__ );
    }

    return env;
}

//============================================================================
jobject VxJni::getActivity( JNIEnv* env )
{
    if( nullptr == env )
    {
        env = getJavaEnv();
    }

    if( nullptr == env )
    {
        return nullptr;
    }

    if( !EnsureQtNativeBindings( env ) )
    {
        LogMsg( LOG_ERROR, "%s QtNative JNI bindings unavailable", __func__ );
        return nullptr;
    }

    jobject activity = env->CallStaticObjectMethod( g_QtNativeClass, g_QtNativeActivityMethod );
    if( nullptr == activity || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to get Android activity", __func__ );
        return nullptr;
    }

    return activity;
}

//============================================================================
jobject VxJni::getApplicationContext( JNIEnv* env )
{
    if( nullptr == env )
    {
        env = getJavaEnv();
    }

    if( nullptr == env )
    {
        return nullptr;
    }

    jobject activity = getActivity( env );
    if( nullptr == activity )
    {
        return nullptr;
    }

    jclass activityClass = env->GetObjectClass( activity );
    if( nullptr == activityClass || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to get activity class", __func__ );
        env->DeleteLocalRef( activity );
        return nullptr;
    }

    jmethodID getAppContextMethod = env->GetMethodID( activityClass,
                                                       "getApplicationContext",
                                                       "()Landroid/content/Context;" );
    if( nullptr == getAppContextMethod || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to get application context method", __func__ );
        env->DeleteLocalRef( activityClass );
        env->DeleteLocalRef( activity );
        return nullptr;
    }

    jobject appContext = env->CallObjectMethod( activity, getAppContextMethod );
    env->DeleteLocalRef( activityClass );
    env->DeleteLocalRef( activity );

    if( nullptr == appContext || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to get application context", __func__ );
        return nullptr;
    }

    return appContext;
}

//============================================================================
bool VxJni::hasPermission( const char* permissionName )
{
    if( nullptr == permissionName || 0 == permissionName[0] )
    {
        LogMsg( LOG_ERROR, "%s invalid permission name", __func__ );
        return false;
    }

    JNIEnv* env = getJavaEnv();
    if( nullptr == env )
    {
        return false;
    }

    jobject activity = getActivity( env );
    if( nullptr == activity )
    {
        return false;
    }

    jclass versionClass = env->FindClass( "android/os/Build$VERSION" );
    if( nullptr == versionClass || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activity );
        return false;
    }

    jfieldID sdkIntField = env->GetStaticFieldID( versionClass, "SDK_INT", "I" );
    if( nullptr == sdkIntField || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( versionClass );
        env->DeleteLocalRef( activity );
        return false;
    }

    const jint sdkInt = env->GetStaticIntField( versionClass, sdkIntField );
    env->DeleteLocalRef( versionClass );
    if( HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activity );
        return false;
    }

    if( sdkInt < 23 )
    {
        env->DeleteLocalRef( activity );
        return true;
    }

    jstring jPermission = env->NewStringUTF( permissionName );
    if( nullptr == jPermission || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activity );
        return false;
    }

    jclass packageManagerClass = env->FindClass( "android/content/pm/PackageManager" );
    if( nullptr == packageManagerClass || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( jPermission );
        env->DeleteLocalRef( activity );
        return false;
    }

    jfieldID grantedField = env->GetStaticFieldID( packageManagerClass, "PERMISSION_GRANTED", "I" );
    if( nullptr == grantedField || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( packageManagerClass );
        env->DeleteLocalRef( jPermission );
        env->DeleteLocalRef( activity );
        return false;
    }

    const jint permissionGranted = env->GetStaticIntField( packageManagerClass, grantedField );
    env->DeleteLocalRef( packageManagerClass );
    if( HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( jPermission );
        env->DeleteLocalRef( activity );
        return false;
    }

    jclass activityClass = env->GetObjectClass( activity );
    if( nullptr == activityClass || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( jPermission );
        env->DeleteLocalRef( activity );
        return false;
    }

    jmethodID checkPermissionMethod = env->GetMethodID( activityClass, "checkSelfPermission", "(Ljava/lang/String;)I" );
    if( nullptr == checkPermissionMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activityClass );
        env->DeleteLocalRef( jPermission );
        env->DeleteLocalRef( activity );
        return false;
    }

    const jint checkResult = env->CallIntMethod( activity, checkPermissionMethod, jPermission );
    const bool hadException = HasAndClearPendingException( env, __func__ );

    env->DeleteLocalRef( activityClass );
    env->DeleteLocalRef( jPermission );
    env->DeleteLocalRef( activity );

    return !hadException && ( checkResult == permissionGranted );
}

//============================================================================
bool VxJni::requestPermissionWithCallback( const char* permissionName,
                                           int requestCode,
                                           const char* callbackType )
{
    if( nullptr == permissionName || 0 == permissionName[0] )
    {
        LogMsg( LOG_ERROR, "%s invalid permission name", __func__ );
        return false;
    }

    const char* callbackTag = ( nullptr != callbackType && 0 != callbackType[0] )
                                ? callbackType
                                : "camera";

    JNIEnv* env = getJavaEnv();
    if( nullptr == env )
    {
        return false;
    }

    jobject activity = getActivity( env );
    if( nullptr == activity )
    {
        return false;
    }

    jclass cameraServiceClass = env->FindClass( "org/nolimitconnect/nolimitconnect/Camera2Service" );
    if( nullptr != cameraServiceClass )
    {
        jmethodID requestPermissionStaticMethod = env->GetStaticMethodID( cameraServiceClass,
                                                                           "requestPermissionStatic",
                                                                           "(Landroid/app/Activity;Ljava/lang/String;ILjava/lang/String;)V" );
        if( nullptr != requestPermissionStaticMethod )
        {
            jstring jPermission = env->NewStringUTF( permissionName );
            jstring jCallbackType = env->NewStringUTF( callbackTag );
            if( nullptr == jPermission || nullptr == jCallbackType || HasAndClearPendingException( env, __func__ ) )
            {
                if( nullptr != jPermission )
                {
                    env->DeleteLocalRef( jPermission );
                }

                if( nullptr != jCallbackType )
                {
                    env->DeleteLocalRef( jCallbackType );
                }

                env->DeleteLocalRef( cameraServiceClass );
                env->DeleteLocalRef( activity );
                return false;
            }

            env->CallStaticVoidMethod( cameraServiceClass,
                                       requestPermissionStaticMethod,
                                       activity,
                                       jPermission,
                                       (jint)requestCode,
                                       jCallbackType );
            const bool hadException = HasAndClearPendingException( env, __func__ );

            env->DeleteLocalRef( jPermission );
            env->DeleteLocalRef( jCallbackType );
            env->DeleteLocalRef( cameraServiceClass );
            env->DeleteLocalRef( activity );
            return !hadException;
        }

        // Clear NoSuchMethodError if this helper is not present and fall back.
        HasAndClearPendingException( env, __func__ );
        env->DeleteLocalRef( cameraServiceClass );
    }
    else
    {
        // Clear ClassNotFoundError if helper class isn't available and fall back.
        HasAndClearPendingException( env, __func__ );
    }

    jclass activityClass = env->GetObjectClass( activity );
    if( nullptr == activityClass || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activity );
        return false;
    }

    jmethodID requestMethod = env->GetMethodID( activityClass, "requestPermissions", "([Ljava/lang/String;I)V" );
    if( nullptr == requestMethod || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activityClass );
        env->DeleteLocalRef( activity );
        return false;
    }

    jclass stringClass = env->FindClass( "java/lang/String" );
    if( nullptr == stringClass || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activityClass );
        env->DeleteLocalRef( activity );
        return false;
    }

    jobjectArray permissionArray = env->NewObjectArray( 1, stringClass, nullptr );
    env->DeleteLocalRef( stringClass );
    if( nullptr == permissionArray || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( activityClass );
        env->DeleteLocalRef( activity );
        return false;
    }

    jstring jPermission = env->NewStringUTF( permissionName );
    if( nullptr == jPermission || HasAndClearPendingException( env, __func__ ) )
    {
        env->DeleteLocalRef( permissionArray );
        env->DeleteLocalRef( activityClass );
        env->DeleteLocalRef( activity );
        return false;
    }

    env->SetObjectArrayElement( permissionArray, 0, jPermission );
    const bool setArrayException = HasAndClearPendingException( env, __func__ );
    env->DeleteLocalRef( jPermission );
    if( setArrayException )
    {
        env->DeleteLocalRef( permissionArray );
        env->DeleteLocalRef( activityClass );
        env->DeleteLocalRef( activity );
        return false;
    }

    env->CallVoidMethod( activity, requestMethod, permissionArray, (jint)requestCode );
    const bool hadException = HasAndClearPendingException( env, __func__ );

    env->DeleteLocalRef( permissionArray );
    env->DeleteLocalRef( activityClass );
    env->DeleteLocalRef( activity );

    return !hadException;
}

//============================================================================
bool VxJni::requestPermission( const char* permissionName, int requestCode )
{
    return requestPermissionWithCallback( permissionName, requestCode, "camera" );
}

//============================================================================
bool VxJni::callStaticVoidWithAppContext( const char* className,
                                          const char* methodName,
                                          const char* methodSignature )
{
    JNIEnv* env = getJavaEnv();
    if( nullptr == env )
    {
        return false;
    }

    jobject appContext = getApplicationContext( env );
    if( nullptr == appContext )
    {
        LogMsg( LOG_ERROR, "%s invalid app context", __func__ );
        return false;
    }

    jclass targetClass = env->FindClass( className );
    if( nullptr == targetClass || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to find class %s", __func__, className );
        env->DeleteLocalRef( appContext );
        return false;
    }

    jmethodID methodId = env->GetStaticMethodID( targetClass, methodName, methodSignature );
    if( nullptr == methodId || HasAndClearPendingException( env, __func__ ) )
    {
        LogMsg( LOG_ERROR, "%s failed to find static method %s", __func__, methodName );
        env->DeleteLocalRef( targetClass );
        env->DeleteLocalRef( appContext );
        return false;
    }

    env->CallStaticVoidMethod( targetClass, methodId, appContext );
    const bool hadException = HasAndClearPendingException( env, __func__ );

    env->DeleteLocalRef( targetClass );
    env->DeleteLocalRef( appContext );

    return !hadException;
}

#endif // TARGET_OS_ANDROID
