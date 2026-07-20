/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "System.h"
#include "jutils/jutils-details.hpp"

#include "platform/nlc/android/jni/Environment.h"
#include "platform/nlc/android/jni/File.h"
#include "platform/nlc/android/jni/StatFs.h"

#include "utils/StringUtils.h"

#include <sstream>

#define GIGABYTES       1073741824

using namespace jni;

std::string CJNISystem::getProperty(const std::string &property)
{
  return jcast<std::string>(call_static_method<jhstring>("java/lang/System",
    "getProperty", "(Ljava/lang/String;)Ljava/lang/String;",
    jcast<jhstring>(property)));
}

std::string CJNISystem::getProperty(const std::string &property, const std::string &defaultValue)
{
  return jcast<std::string>(call_static_method<jhstring>("java/lang/System",
    "getProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
    jcast<jhstring>(property), jcast<jhstring>(defaultValue)));
}

std::string CJNISystem::setProperty(const std::string &property, const std::string &defaultValue)
{
  return jcast<std::string>(call_static_method<jhstring>("java/lang/System",
    "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
    jcast<jhstring>(property), jcast<jhstring>(defaultValue)));
}

std::string CJNISystem::clearProperty(const std::string &property)
{
  return jcast<std::string>(call_static_method<jhstring>("java/lang/System",
    "clearProperty", "(Ljava/lang/String;)Ljava/lang/String;",
    jcast<jhstring>(property)));
}

int64_t CJNISystem::nanoTime()
{
  return call_static_method<jlong>("java/lang/System",
                           "nanoTime",
                           "()J");
}


bool CJNISystem::GetExternalStorage(bool& retIsWritable, std::string &path, const std::string &type /* = "" */)
{
  retIsWritable = false;

  std::string sType;
  std::string mountedState;
  bool mounted = false;

  if(type == "files" || type.empty())
  {
      CJNIFile external = CJNIEnvironment::getExternalStorageDirectory();
      if (external)
          path = external.getAbsolutePath();
  }
  else
  {
      if (type == "music")
          sType = "Music"; // Environment.DIRECTORY_MUSIC
      else if (type == "videos")
          sType = "Movies"; // Environment.DIRECTORY_MOVIES
      else if (type == "pictures")
          sType = "Pictures"; // Environment.DIRECTORY_PICTURES
      else if (type == "photos")
          sType = "DCIM"; // Environment.DIRECTORY_DCIM
      else if (type == "downloads")
          sType = "Download"; // Environment.DIRECTORY_DOWNLOADS
      if (!sType.empty())
      {
          CJNIFile external = CJNIEnvironment::getExternalStoragePublicDirectory(sType);
          if (external)
              path = external.getAbsolutePath();
      }
  }
  mountedState = CJNIEnvironment::getExternalStorageState();
  mounted = (mountedState == "mounted" || mountedState == "mounted_ro");
  if(mountedState == "mounted")
  {
      retIsWritable = true;
  }

  return mounted && !path.empty();
}

bool CJNISystem::GetStorageUsage(const std::string &path, std::string &usage)
{
#define PATH_MAXLEN 38

  if (path.empty())
  {
      std::ostringstream fmt;

      fmt.width(PATH_MAXLEN);
      fmt << std::left << "Filesystem";

      fmt.width(12);
      fmt << std::right << "Size";

      fmt.width(12);
      fmt << "Used";

      fmt.width(12);
      fmt << "Avail";

      fmt.width(12);
      fmt << "Use %";

      usage = fmt.str();
      return false;
  }

  CJNIStatFs fileStat(path);
  int blockSize = fileStat.getBlockSize();
  int blockCount = fileStat.getBlockCount();
  int freeBlocks = fileStat.getFreeBlocks();

  if (blockSize <= 0 || blockCount <= 0 || freeBlocks < 0)
      return false;

  float totalSize = (float)blockSize * blockCount / GIGABYTES;
  float freeSize = (float)blockSize * freeBlocks / GIGABYTES;
  float usedSize = totalSize - freeSize;
  float usedPercentage = usedSize / totalSize * 100;

  std::ostringstream fmt;

  fmt << std::fixed;
  fmt.precision(1);

  fmt.width(PATH_MAXLEN);
  fmt << std::left
      << (path.size() < PATH_MAXLEN - 1 ? path : StringUtils::Left(path, PATH_MAXLEN - 4) + "...");

  fmt.width(11);
  fmt << std::right << totalSize << "G";

  fmt.width(11);
  fmt << usedSize << "G";

  fmt.width(11);
  fmt << freeSize << "G";

  fmt.precision(0);
  fmt.width(11);
  fmt << usedPercentage << "%";

  usage = fmt.str();
  return true;
}



