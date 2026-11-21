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

/* MT6899 vendor UIC attributes (Linux ufs/unipro.h). */
#define VS_SAVEPOWERCONTROL_ATTR  0xD0A6
#define VS_DEBUGOMC_ATTR          0xD09E

UfsCalError
UfsCalPreLink (struct UfsCalParam *p)
{
  UINT32 Value = 0;

  if (p != NULL && p->Host != NULL) {
    /* MT6899: clear VS_SAVEPOWERCONTROL bit6 before link start-up. */
    ufs_lld_dme_get (p->Host, UIC_ARG_MIB (VS_SAVEPOWERCONTROL_ATTR), &Value);
    Value &= ~(1u << 6);
    ufs_lld_dme_set (p->Host, UIC_ARG_MIB (VS_SAVEPOWERCONTROL_ATTR), Value);
  }
  return UFS_CAL_NO_ERROR;
}

UfsCalError
UfsCalPostLink (struct UfsCalParam *p)
{
  UINT32 Value = 0;

  if (p != NULL && p->Host != NULL) {
    /* MT6899: fix device PA_INIT no-adapt (VS_DEBUGOMC bit8). */
    ufs_lld_dme_get (p->Host, UIC_ARG_MIB (VS_DEBUGOMC_ATTR), &Value);
    Value |= 0x100;
    ufs_lld_dme_set (p->Host, UIC_ARG_MIB (VS_DEBUGOMC_ATTR), Value);
  }
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
