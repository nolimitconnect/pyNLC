/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_OS_ANDROID)

#include "utils/MemUtils.h"

#if defined(TARGET_OS_ANDROID) && !defined(HAVE_NLC_GUI)
# include "platform/android/activity/XBMCApp.h"
#elif defined(TARGET_OS_ANDROID) && defined(HAVE_NLC_GUI)
# include "platform/nlc/KodiNlcApp.h"
#endif // defined(TARGET_OS_ANDROID) && !defined(HAVE_NLC_GUI)

#include <stdlib.h>

namespace KODI
{
namespace MEMORY
{

void* AlignedMalloc(size_t s, size_t alignTo)
{
  void* p;
  posix_memalign(&p, alignTo, s);

  return p;
}

void AlignedFree(void* alignMemPtr)
{
  if (!alignMemPtr)
      return;

#if 0
  free(alignMemPtr); // causes crash when free gl pixels
#else
  char *pFull = *(char **)(((char *)alignMemPtr) - sizeof(char *));
  free(pFull);
#endif // 0
}

/*
void GetMemoryStatus(MemoryStatus* buffer)
{
  if (!buffer)
    return;

  long availMem = 0;
  long totalMem = 0;

  if (CXBMCApp::Get().GetMemoryInfo(availMem, totalMem))
  {
    *buffer = {};
    buffer->totalPhys = static_cast<unsigned long>(totalMem);
    buffer->availPhys = static_cast<unsigned long>(availMem);
  }
}
*/

}
}

#endif // defined(TARGET_OS_ANDROID)
