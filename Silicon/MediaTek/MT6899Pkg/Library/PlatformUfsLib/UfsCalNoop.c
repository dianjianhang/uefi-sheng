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
#define VS_SAVEPOWERCONTROL_ATTR      0xD0A6
#define VS_UNIPROPOWERDOWNCONTROL_ATTR 0xD0A8
#define PA_LOCAL_TX_LCC_ENABLE_ATTR   0x155E
#define VS_DEBUGOMC_ATTR              0xD09E

UfsCalError
UfsCalPreLink (struct UfsCalParam *p)
{
  UINT32 Value = 0;
  UINT32 Rd;

  if (p != NULL && p->Host != NULL) {
    /*
     * MT6899: take UniPro out of low-power mode before link start-up.
     * Mirrors ufs_mtk_unipro_set_lpm(hba, false) in ufs-mediatek.c.
     * (VS_UNIPROPOWERDOWNCONTROL, genSelectorIndex 0)
     */
    ufs_lld_dme_set (p->Host, UIC_ARG_MIB_SEL (VS_UNIPROPOWERDOWNCONTROL_ATTR, 0), 0);
    Rd = 0xFFFFFFFF;
    ufs_lld_dme_get (p->Host, UIC_ARG_MIB_SEL (VS_UNIPROPOWERDOWNCONTROL_ATTR, 0), &Rd);
    DEBUG ((EFI_D_ERROR, "UFS prelink: unipowerdown rd=0x%08x\n", Rd));

    /*
     * MT6899: disable the host TX local-link-change feature before link
     * start-up.  Mirrors ufshcd_disable_host_tx_lcc() (PA_Local_TX_LCC_Enable=0).
     */
    ufs_lld_dme_set (p->Host, UIC_ARG_MIB (PA_LOCAL_TX_LCC_ENABLE_ATTR), 0);
    Rd = 0xFFFFFFFF;
    ufs_lld_dme_get (p->Host, UIC_ARG_MIB (PA_LOCAL_TX_LCC_ENABLE_ATTR), &Rd);
    DEBUG ((EFI_D_ERROR, "UFS prelink: tx_lcc rd=0x%08x\n", Rd));

    /* MT6899: clear VS_SAVEPOWERCONTROL bit6 (disable deep stall). */
    ufs_lld_dme_get (p->Host, UIC_ARG_MIB (VS_SAVEPOWERCONTROL_ATTR), &Value);
    Value &= ~(1u << 6);
    ufs_lld_dme_set (p->Host, UIC_ARG_MIB (VS_SAVEPOWERCONTROL_ATTR), Value);
    Rd = 0xFFFFFFFF;
    ufs_lld_dme_get (p->Host, UIC_ARG_MIB (VS_SAVEPOWERCONTROL_ATTR), &Rd);
    DEBUG ((EFI_D_ERROR, "UFS prelink: savepower rd=0x%08x\n", Rd));
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
