// NOTE(MT6899): Structural derivation from MediaTek MT6789Pkg (UniXNU/uefi-sheng).
// MT6899 (Dimensity 8400-Ultra) register maps, clock tree and GPIO/PMIC/UFS/Msdc
// tuning are NOT yet validated against the MT6899 TRM. This file is a build
// scaffold only - do not run on hardware until re-derived from sourced data.
// See Platforms/Xiaomi/rodinPkg/PORTING.md.
/**
  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/IoLib.h>
#include <Library/MemoryMapHelperLib.h>
#include <Library/PlatformSecLib.h>

#include "PlatformRegisters.h"

VOID
DisableWatchDogTimer ()
{
  EFI_STATUS                   Status;
  EFI_MEMORY_REGION_DESCRIPTOR WDogRegion;

  // Locate "WatchDog Timer" Memory Region
  Status = LocateMemoryRegionByName ("WatchDog Timer", &WDogRegion);
  if (EFI_ERROR (Status)) {
    return;
  }

  // Disable WatchDog Timer
  MmioWrite32 (WDogRegion.Address, WDT_MODE_KEY);
}

VOID
PlatformInitialize ()
{
  // Disable WatchDog Timer
  DisableWatchDogTimer ();
}
