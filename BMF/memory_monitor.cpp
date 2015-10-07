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

#include "config.h"
#include "io_monitor.h"
#include "memory_monitor.h"

#include "log.h"

#pragma comment (lib, "wbemuuid.lib")

extern IWbemServices *pNameSpace;

extern bool BMF_InitCOM     (void);
extern void BMF_ShutdownCOM (void);

extern CRITICAL_SECTION com_cs;

process_stats_t process_stats;

DWORD
WINAPI
BMF_MonitorProcess (LPVOID user)
{
  BMF_InitCOM ();

  EnterCriticalSection (&com_cs);

  process_stats_t& proc   = process_stats;
  const double     update = config.mem.interval;

  HRESULT hr;

  if (FAILED (hr = CoCreateInstance (
    CLSID_WbemRefresher,
    NULL,
    CLSCTX_INPROC_SERVER,
    IID_IWbemRefresher, 
    (void**) &proc.pRefresher)))
  {
    dll_log.Log(L" [WMI]: Failed to create Refresher Instance (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pRefresher->QueryInterface (
    IID_IWbemConfigureRefresher,
    (void **)&proc.pConfig)))
  {
    dll_log.Log(L" [WMI]: Failed to Query Refresher Interface (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto PROC_CLEANUP;
  }

  IWbemClassObject *pClassObj = nullptr;

  HANDLE hProc = GetCurrentProcess ();

  DWORD   dwProcessSize = MAX_PATH;
  wchar_t wszProcessName [MAX_PATH];

  QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

  wchar_t* pwszShortName = wcsrchr (wszProcessName, L'\\') + 1;
  wchar_t* pwszTruncName = wcsrchr (pwszShortName, L'.');

  if (pwszTruncName != nullptr)
    *pwszTruncName = L'\0';

  wchar_t wszInstance [512];
  wsprintf ( wszInstance,
               L"Win32_PerfFormattedData_PerfProc_Process.Name='%ws'",
                 pwszShortName );

  if (FAILED (hr = proc.pConfig->AddObjectByPath (pNameSpace, wszInstance, 0, 0, &pClassObj, 0)))
  {
    dll_log.Log(L" [WMI]: Failed to AddObjectByPath (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = pClassObj->QueryInterface (IID_IWbemObjectAccess, (void **)(&proc.pAccess))))
  {
    dll_log.Log(L" [WMI]: Failed to Query WbemObjectAccess Interface (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    pClassObj->Release ();
    pClassObj = nullptr;

    goto PROC_CLEANUP;
  }

  pClassObj->Release ();
  pClassObj = nullptr;

  CIMTYPE variant;
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"PageFileBytes", &variant, &proc.hPageFileBytes)))
  {
    goto PROC_CLEANUP;
  }
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"PageFileBytesPeak", &variant, &proc.hPageFileBytesPeak)))
  {
    goto PROC_CLEANUP;
  }
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"ThreadCount", &variant, &proc.hThreadCount)))
  {
    goto PROC_CLEANUP;
  }
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"PrivateBytes", &variant, &proc.hPrivateBytes)))
  {
    goto PROC_CLEANUP;
  }
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"WorkingSetPeak", &variant, &proc.hWorkingSetPeak)))
  {
    goto PROC_CLEANUP;
  }
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"WorkingSet", &variant, &proc.hWorkingSet)))
  {
    goto PROC_CLEANUP;
  }
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"VirtualBytesPeak", &variant, &proc.hVirtualBytesPeak)))
  {
    goto PROC_CLEANUP;
  }
  if (FAILED (hr = proc.pAccess->GetPropertyHandle (L"VirtualBytes", &variant, &proc.hVirtualBytes)))
  {
    goto PROC_CLEANUP;
  }

  proc.pConfig->Release ();
  proc.pConfig = nullptr;

  int iter = 0;

  proc.lID = 1;

  LeaveCriticalSection (&com_cs);

  while (proc.lID != 0)
  {
    // Sleep until ready
    Sleep (DWORD (update * 1000.0));

    // Only poll WMI while the data view is visible
    if (! config.mem.show)
      continue;

    EnterCriticalSection (&com_cs);

    if (FAILED (hr = proc.pRefresher->Refresh (0L)))
    {
      goto PROC_CLEANUP;
    }

    proc.pAccess->ReadQWORD ( proc.hVirtualBytes,
                                &proc.memory.virtual_bytes );
    proc.pAccess->ReadQWORD ( proc.hVirtualBytesPeak,
                                &proc.memory.virtual_bytes_peak );

    proc.pAccess->ReadQWORD ( proc.hWorkingSet,
                                &proc.memory.working_set );
    proc.pAccess->ReadQWORD ( proc.hWorkingSetPeak,
                                &proc.memory.working_set_peak );

    proc.pAccess->ReadQWORD ( proc.hPrivateBytes,
                                &proc.memory.private_bytes );

    proc.pAccess->ReadDWORD ( proc.hThreadCount,
                                (DWORD *)&proc.tasks.thread_count );

    proc.pAccess->ReadQWORD ( proc.hPageFileBytes,
                                &proc.memory.page_file_bytes );
    proc.pAccess->ReadQWORD ( proc.hPageFileBytesPeak,
                                &proc.memory.page_file_bytes_peak );

    ++iter;

    LeaveCriticalSection (&com_cs);
  }

PROC_CLEANUP:
  dll_log.Log (L" >> PROC_CLEANUP");

  if (proc.pAccess != nullptr)
  {
    proc.pAccess->Release ();
    proc.pAccess = nullptr;
  }

  if (proc.pConfig != nullptr)
  {
    proc.pConfig->Release ();
    proc.pConfig = nullptr;
  }

  if (proc.pRefresher != nullptr)
  {
    proc.pRefresher->Release ();
    proc.pRefresher = nullptr;
  }

  LeaveCriticalSection (&com_cs);

  return 0;
}