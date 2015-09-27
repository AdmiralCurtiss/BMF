/**
* This file is part of Batman "Fix".
*
* Batman Tweak is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* The Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Batman Tweak is distributed in the hope that it will be useful,
* But WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Batman "Fix". If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef __BMF__GPU_MONITOR_H__
#define __BMF__GPU_MONITOR_H__

#include <Windows.h>
#include <stdint.h>

// 64?! Isn't that a bit ridiculous memory wise? But NvAPI wants it that big
#define MAX_GPUS 64

struct gpu_sensors_t {
  struct {
    // NV's driver only updates load % once per-second.
    struct {
      uint32_t gpu; // GPU
      uint32_t fb;  // Memory
      uint32_t vid; // Video Encoder
      uint32_t bus; // Bus
    } loads_percent;

    struct {
      uint32_t gpu;
      uint32_t ram;
      uint32_t shader;
    } clocks_kHz;

    struct {
      uint32_t core;
    } volts_mV;

    struct {
      int32_t gpu;
      int32_t ram;
      int32_t psu;
      int32_t pcb;
    } temps_c;

    //
    // NOTE: This isn't particularly accurate; it is a sum total rather than
    //         the ammount an application is using.
    //
    //         We can do better in Windows 10!  (*See DXGI 1.4 Memory Info)
    //
    struct {
      uint64_t local;
      uint64_t nonlocal; // COMPUTED: (total - local)
      uint64_t total;
    } memory_B;
  } gpus [MAX_GPUS];

  int num_gpus = 0;

  ULARGE_INTEGER last_update = { 0ULL };
} extern gpu_stats;

void BMF_PollGPU (void);

#if 0
NV_GPU_THERMAL_SETTINGS thermal;
thermal.version = NV_GPU_THERMAL_SETTINGS_VER;

NvAPI_GPU_GetThermalSettings(hGPU, NVAPI_THERMAL_TARGET_ALL, &thermal);
#endif

#endif /* __BMF__GPU_MONITOR_H__ */