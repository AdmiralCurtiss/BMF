/**
* This file is part of Batman "Fix".
*
* Batman "Fix" is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* The Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Batman "Fix" is distributed in the hope that it will be useful,
* But WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Batman "Fix". If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef __BMF__MEMORY_MONITOR_H__
#define __BMF__MEMORY_MONITOR_H__

#include <Windows.h>

#define _WIN32_DCOM
#include <Wbemidl.h>

struct WMI_refresh_instance_thread_t
{
  HANDLE                   hThread                      = 0;

  IWbemRefresher          *pRefresher                   = nullptr;
  IWbemConfigureRefresher *pConfig                      = nullptr;
  IWbemObjectAccess       *pAccess                      = nullptr;
  long                     lID                          = 0;
};

struct process_stats_t : WMI_refresh_instance_thread_t
{
  long               hVirtualBytes         = 0;
  long               hVirtualBytesPeak     = 0;
  long               hWorkingSet           = 0;
  long               hWorkingSetPeak       = 0;
  long               hPrivateBytes         = 0;
  long               hThreadCount          = 0;
  long               hPageFileBytes        = 0;
  long               hPageFileBytesPeak    = 0;

  struct {
    uint64_t virtual_bytes;
    uint64_t virtual_bytes_peak;
    uint64_t working_set;
    uint64_t working_set_peak;
    uint64_t private_bytes;
    uint64_t page_file_bytes;
    uint64_t page_file_bytes_peak;
  } memory;

  struct {
    uint32_t thread_count;
  } tasks;
};

extern process_stats_t process_stats;

DWORD
WINAPI
BMF_MonitorProcess (LPVOID user);

#endif /* __BMF__MEMORY_MONITOR_H__ */