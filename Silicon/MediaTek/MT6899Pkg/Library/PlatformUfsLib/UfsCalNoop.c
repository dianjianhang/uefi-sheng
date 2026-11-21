/**
  MT6899 (MediaTek Dimensity 8400-Ultra) UFS calibration hooks.

  The generic Samsung UfsDxe calls the UfsCal* hooks for Samsung-specific
  PMA/lane calibration. MediaTek brings the MPHY up in UfsBoardInit() and needs
  no per-gear calibration table for initial bring-up, so these hooks are
  deliberate no-ops that report success so UfsInitCal()/link start-up proceeds.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UfsHostBridge.h>
#include <Library/UfsCalAdapterLib.h>

UfsCalError
UfsCalInit (struct UfsCalParam *p)
{
  (VOID)p;
  return UFS_CAL_NO_ERROR;
}

UfsCalError
UfsCalPreLink (struct UfsCalParam *p)
{
  (VOID)p;
  return UFS_CAL_NO_ERROR;
}

UfsCalError
UfsCalPostLink (struct UfsCalParam *p)
{
  (VOID)p;
  return UFS_CAL_NO_ERROR;
}

UfsCalError
UfsCalPrePmc (struct UfsCalParam *p)
{
  (VOID)p;
  return UFS_CAL_NO_ERROR;
}

UfsCalError
UfsCalPostPmc (struct UfsCalParam *p)
{
  (VOID)p;
  return UFS_CAL_NO_ERROR;
}

UINT8
UfsCalGetTargetBoard (VOID)
{
  return 0;
}
