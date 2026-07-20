#pragma once
/*
 * Minimal Android JNI wrappers for android.net.nsd.NsdManager listeners
 */

#include "JNIBase.h"
#include "NsdServiceInfo.h"

class CJNINsdManager : public CJNIBase
{
public:
  CJNINsdManager() : CJNIBase("android/net/nsd/NsdManager") {}
  CJNINsdManager(const jni::jhobject& object) : CJNIBase(object) {}
};

class CJNINsdManagerDiscoveryListener : public CJNIBase
{
public:
  CJNINsdManagerDiscoveryListener() {}
  CJNINsdManagerDiscoveryListener(const jni::jhobject& object) : CJNIBase(object) {}
  virtual ~CJNINsdManagerDiscoveryListener() = default;

  virtual void onDiscoveryStarted(const std::string& serviceType) = 0;
  virtual void onDiscoveryStopped(const std::string& serviceType) = 0;
  virtual void onServiceFound(const CJNINsdServiceInfo& serviceInfo) = 0;
  virtual void onServiceLost(const CJNINsdServiceInfo& serviceInfo) = 0;
  virtual void onStartDiscoveryFailed(const std::string& serviceType, int errorCode) = 0;
  virtual void onStopDiscoveryFailed(const std::string& serviceType, int errorCode) = 0;
};

class CJNINsdManagerRegistrationListener : public CJNIBase
{
public:
  CJNINsdManagerRegistrationListener() {}
  CJNINsdManagerRegistrationListener(const jni::jhobject& object) : CJNIBase(object) {}
  virtual ~CJNINsdManagerRegistrationListener() = default;

  virtual void onRegistrationFailed(const CJNINsdServiceInfo& serviceInfo, int errorCode) = 0;
  virtual void onServiceRegistered(const CJNINsdServiceInfo& serviceInfo) = 0;
  virtual void onServiceUnregistered(const CJNINsdServiceInfo& serviceInfo) = 0;
  virtual void onUnregistrationFailed(const CJNINsdServiceInfo& serviceInfo, int errorCode) = 0;
};

class CJNINsdManagerResolveListener : public CJNIBase
{
public:
  CJNINsdManagerResolveListener() {}
  CJNINsdManagerResolveListener(const jni::jhobject& object) : CJNIBase(object) {}
  virtual ~CJNINsdManagerResolveListener() = default;

  virtual void onResolveFailed(const CJNINsdServiceInfo& serviceInfo, int errorCode) = 0;
  virtual void onServiceResolved(const CJNINsdServiceInfo& serviceInfo) = 0;
};
