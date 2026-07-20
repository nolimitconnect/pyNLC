#pragma once
/*
 * Minimal Android JNI wrapper for android.net.nsd.NsdServiceInfo
 */

#include "JNIBase.h"

class CJNINsdServiceInfo : public CJNIBase
{
public:
  CJNINsdServiceInfo() : CJNIBase("android/net/nsd/NsdServiceInfo") {}
  CJNINsdServiceInfo(const jni::jhobject& object) : CJNIBase(object) {}

  std::string getServiceName() const
  {
    return jni::jcast<std::string>(
        jni::call_method<jni::jhstring>(m_object, "getServiceName", "()Ljava/lang/String;"));
  }

  std::string getServiceType() const
  {
    return jni::jcast<std::string>(
        jni::call_method<jni::jhstring>(m_object, "getServiceType", "()Ljava/lang/String;"));
  }
};
