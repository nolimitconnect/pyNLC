#pragma once
/*
 * Minimal Android JNI wrapper for android.hardware.display.DisplayManager
 */

#include "JNIBase.h"

class CJNIDisplayManagerDisplayListener : public CJNIBase
{
public:
  CJNIDisplayManagerDisplayListener() {}
  CJNIDisplayManagerDisplayListener(const jni::jhobject& object) : CJNIBase(object) {}

  virtual ~CJNIDisplayManagerDisplayListener() = default;
  virtual void onDisplayAdded(int displayId) {}
  virtual void onDisplayChanged(int displayId) {}
  virtual void onDisplayRemoved(int displayId) {}
};

class CJNIDisplayManager : public CJNIBase
{
public:
  CJNIDisplayManager() : CJNIBase("android/hardware/display/DisplayManager") {}
  CJNIDisplayManager(const jni::jhobject& object) : CJNIBase(object) {}

  void registerDisplayListener(const CJNIBase& listener) const
  {
    jni::call_method<void>(
        m_object,
        "registerDisplayListener",
        "(Landroid/hardware/display/DisplayManager$DisplayListener;Landroid/os/Handler;)V",
        listener.get_raw(), jni::jhobject(nullptr));
  }

  void unregisterDisplayListener(const CJNIBase& listener) const
  {
    jni::call_method<void>(
        m_object,
        "unregisterDisplayListener",
        "(Landroid/hardware/display/DisplayManager$DisplayListener;)V",
        listener.get_raw());
  }
};
