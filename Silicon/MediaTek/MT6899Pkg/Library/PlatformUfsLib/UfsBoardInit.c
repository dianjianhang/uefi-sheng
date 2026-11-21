/**
  MT6899 (MediaTek Dimensity 8400-Ultra) UFS board initialisation.

  rodin storage: ufshci@112b0000 (UFSHCI 4.0, IP_VER_MT6899 0x10450100)
                 ufsphy@112a0000 (MT8183-compatible MPHY)

  All host-controller access goes through the standard UFSHCI register window
  (Ufs->IoAddr). MediaTek keeps its vendor registers inside that same window at
  offsets 0x140/0x144/0x160/0x2100/..., so VsAddr aliases IoAddr.

  The MPHY power-on sequence below is a direct port of the Linux driver
  drivers/phy/mediatek/phy-mtk-ufs.c (ufs_mtk_phy_set_active); it only touches
  four MPHY registers and needs no firmware assistance for bring-up.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/UfsHostBridge.h>

//
// rodin register bases (vendor DTB ufshci@112b0000 / ufsphy@112a0000)
//
#define MT6899_UFS_HCI_BASE    0x112B0000UL
#define MT6899_UFS_PHY_BASE    0x112A0000UL

//
// MediaTek UFSHCI vendor registers (inside the UFSHCI window)
//
#define REG_UFS_XOUFS_CTRL     0x140
#define REG_UFS_REFCLK_CTRL    0x144
#define REG_UFS_MMIO_OPT_CTRL0 0x160

//
// ufscfg_pdn reset controller (vendor DTB syscon@112bb000, phandle 0x166).
// ti,reset-bits = <0x48 0x00 0x4c 0x00 ...> for unipro_rst / crypto_rst / hci_rst.
//   assert   = write bit to +0x48 (reset set)
//   deassert = write bit to +0x4c (reset clear)
//
#define MT6899_UFS_CFG_PDN_BASE  0x112BB000UL
#define UFS_RST_ASSERT_REG       0x48
#define UFS_RST_DEASSERT_REG     0x4C
#define UFS_RST_UNIPRO_BIT       BIT0
#define UFS_RST_CRYPTO_BIT       BIT1
#define UFS_RST_HCI_BIT          BIT2

//
// MediaTek MPHY registers (from phy-mtk-ufs.c)
//
#define MP_GLB_DIG_8C          0x008C
#define MP_LN_DIG_RX_9C        0xA09C
#define MP_LN_DIG_RX_AC        0xA0AC
#define MP_LN_RX_44            0xB044

#define FRC_PLL_ISO_EN         BIT8
#define PLL_ISO_EN             BIT9
#define FRC_FRC_PWR_ON         BIT10
#define PLL_PWR_ON             BIT11

#define FSM_DIFZ_FRC           BIT18

#define FRC_RX_SQ_EN           BIT0
#define RX_SQ_EN               BIT1

#define FRC_CDR_PWR_ON         BIT17
#define CDR_PWR_ON             BIT18
#define FRC_CDR_ISO_EN         BIT19
#define CDR_ISO_EN             BIT20

//
// REFCLK handshake
//
#define REFCLK_RELEASE         0x0
#define REFCLK_REQUEST         BIT0
#define REFCLK_ACK             BIT1
#define REFCLK_TIMEOUT_US      3000

#define UFS_MCLK_RATE          403000000UL   // ufs_sel max on rodin DTB
#define UFS_GEAR_MODE          4

//
// Backing store for the Exynos-only vendor register block that the generic
// UfsDxe pokes through VsAddr / DevPwrAddr / PhyIsoAddr.  The highest VS_*
// offset used is 0x114 (VS_MPHY_REFCLK_SEL), so 0x400 bytes is ample.  Static
// storage starts zeroed and stays writable, so reads return 0 and writes are
// harmlessly discarded instead of hitting real (or absent) hardware.
//
STATIC UINT32  mVsScratch[0x400 / sizeof (UINT32)];

STATIC
VOID
MphySetBits (
  IN UINTN  Offset,
  IN UINT32 Bits
  )
{
  MmioOr32 (MT6899_UFS_PHY_BASE + Offset, Bits);
}

STATIC
VOID
MphyClearBits (
  IN UINTN  Offset,
  IN UINT32 Bits
  )
{
  MmioAnd32 (MT6899_UFS_PHY_BASE + Offset, ~Bits);
}

/**
  Assert-then-deassert the UFS host resets (unipro_rst, crypto_rst, hci_rst).

  Mirrors ufs_mtk_host_reset() in drivers/ufs/host/ufs-mediatek.c, which the
  Samsung Exynos driver never performs.  Without a clean host reset the UniPro
  and HCI can come out of the bootloader hand-off in a stale state and
  DME_LINK_STARTUP later fails (observed as "UIC cmd 0x16 failed ... arg2=1").
**/
STATIC
VOID
Mt6899HostReset (
  VOID
  )
{
  UINT32 Mask = UFS_RST_UNIPRO_BIT | UFS_RST_CRYPTO_BIT | UFS_RST_HCI_BIT;

  MmioWrite32 ((UINTN)(MT6899_UFS_CFG_PDN_BASE + UFS_RST_ASSERT_REG), Mask);
  MicroSecondDelay (200);

  MmioWrite32 ((UINTN)(MT6899_UFS_CFG_PDN_BASE + UFS_RST_DEASSERT_REG), Mask);
  MicroSecondDelay (1000);

  DEBUG ((DEBUG_ERROR, "UFS: host reset done (0x48=0x%08x 0x4c=0x%08x)\n",
    MmioRead32 ((UINTN)(MT6899_UFS_CFG_PDN_BASE + UFS_RST_ASSERT_REG)),
    MmioRead32 ((UINTN)(MT6899_UFS_CFG_PDN_BASE + UFS_RST_DEASSERT_REG))));
}

/**
  Bring the MediaTek MPHY out of reset - mirrors ufs_mtk_phy_set_active().
**/
STATIC
VOID
Mt6899MphyPowerOn (
  VOID
  )
{
  //
  // Release DA_MP_PLL_PWR_ON
  //
  MphySetBits (MP_GLB_DIG_8C, PLL_PWR_ON);
  MphyClearBits (MP_GLB_DIG_8C, FRC_FRC_PWR_ON);

  //
  // Release DA_MP_PLL_ISO_EN
  //
  MphyClearBits (MP_GLB_DIG_8C, PLL_ISO_EN);
  MphyClearBits (MP_GLB_DIG_8C, FRC_PLL_ISO_EN);

  //
  // Release DA_MP_CDR_PWR_ON
  //
  MphySetBits (MP_LN_RX_44, CDR_PWR_ON);
  MphyClearBits (MP_LN_RX_44, FRC_CDR_PWR_ON);

  //
  // Release DA_MP_CDR_ISO_EN
  //
  MphyClearBits (MP_LN_RX_44, CDR_ISO_EN);
  MphyClearBits (MP_LN_RX_44, FRC_CDR_ISO_EN);

  //
  // Release DA_MP_RX0_SQ_EN
  //
  MphySetBits (MP_LN_DIG_RX_AC, RX_SQ_EN);
  MphyClearBits (MP_LN_DIG_RX_AC, FRC_RX_SQ_EN);

  //
  // Let DIFZ settle, then release it
  //
  MicroSecondDelay (1);
  MphyClearBits (MP_LN_DIG_RX_9C, FSM_DIFZ_FRC);
}

/**
  Assert the reference clock request and wait for the controller to ack -
  mirrors ufs_mtk_setup_ref_clk(TRUE).
**/
STATIC
EFI_STATUS
Mt6899RefClkEnable (
  IN struct UfsHost  *Ufs
  )
{
  UINT32 Value;
  UINT32 Waited;

  MmioWrite32 ((UINTN)Ufs->IoAddr + REG_UFS_REFCLK_CTRL, REFCLK_REQUEST);

  for (Waited = 0; Waited < REFCLK_TIMEOUT_US; Waited += 10) {
    Value = MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_REFCLK_CTRL);
    if (((Value & REFCLK_ACK) >> 1) == (Value & REFCLK_REQUEST)) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (10);
  }

  DEBUG ((DEBUG_ERROR, "UFS: refclk ack missing, reg=0x%08x\n", Value));
  return EFI_TIMEOUT;
}

/**
  Configure the MT6899 UFS host for first use.

  Fills every field UfsDxe needs (register windows, clock rate, gear) and
  performs the MPHY / reference-clock power-on that the Samsung implementation
  delegates to its PMU and clock-manager drivers.
**/
EFI_STATUS
UfsBoardInit (
  IN struct UfsHost  *Ufs
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "UFS: MT6899 board init\n"));

  //
  // Real register windows.  All standard UFSHCI access (UIC / UTP / HCE) goes
  // through IoAddr; the MPHY is reached through PhyPma.
  //
  Ufs->IoAddr      = (VOID *)(UINTN)MT6899_UFS_HCI_BASE;
  Ufs->UniProAddr  = (VOID *)(UINTN)MT6899_UFS_HCI_BASE;
  Ufs->PhyPma      = (VOID *)(UINTN)MT6899_UFS_PHY_BASE;

  //
  // MediaTek has no Exynos PMU / isolation / vendor-register block, and the
  // Samsung VS_* offsets (0x000..0x114) would alias real UFSHCI registers if
  // VsAddr pointed at the host window.  Redirect VsAddr / DevPwrAddr /
  // PhyIsoAddr at a private zeroed RAM page so the generic Exynos code runs
  // without faulting (PhyIsoAddr == NULL previously caused a FAR=0 data abort)
  // and without corrupting the controller.
  //
  Ufs->VsAddr      = (VOID *)mVsScratch;
  Ufs->DevPwrAddr  = (VOID *)mVsScratch;
  Ufs->DevPwrShift = 0;
  Ufs->PhyIsoAddr  = (VOID *)mVsScratch;

  Ufs->MclkRate    = UFS_MCLK_RATE;
  Ufs->GearMode    = UFS_GEAR_MODE;

  //
  // Bring up the MPHY analog, then the reference clock, exactly as Linux
  // ufs_mtk_init() does (mphy_power_on -> setup_clocks/ref_clk) *before* the
  // host reset carried out in ufs_mtk_hce_enable_notify().
  //
  Mt6899MphyPowerOn ();

  Status = Mt6899RefClkEnable (Ufs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UFS: refclk handshake returned %r (continuing)\n", Status));
  }

  //
  // Host reset (unipro/crypto/hci).  Done before the XOUFS/DDR_EN programming
  // because hci_rst clears the UFSHCI vendor registers.
  //
  Mt6899HostReset ();

  //
  // The reset may have dropped the refclk handshake; re-assert it and re-power
  // the MPHY (both are idempotent).
  //
  Mt6899MphyPowerOn ();

  Status = Mt6899RefClkEnable (Ufs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UFS: refclk re-handshake returned %r (continuing)\n", Status));
  }

  //
  // Clock gating early, to avoid the ERR_CHK hazard described in
  // ufs_mtk_hce_enable_notify().
  //
  MmioOr32 ((UINTN)Ufs->IoAddr + REG_UFS_XOUFS_CTRL, 0x80);

  //
  // DDR_EN for UFSHCI 4.0 (IP_VER_MT6989 and later), which covers MT6899.
  //
  {
    UINT32 Value;

    Value  = MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_MMIO_OPT_CTRL0);
    Value &= ~(0x7FFF << 8);
    Value |= (0x453000 & (0x7FFF << 8));
    MmioWrite32 ((UINTN)Ufs->IoAddr + REG_UFS_MMIO_OPT_CTRL0, Value);
  }

  //
  // Diagnostics (ERROR level so it survives any DebugLib print-level filter).
  //
  DEBUG ((DEBUG_ERROR,
    "UFS: board init: VER=0x%08x MTKIPVER=0x%08x XOUFS=0x%08x OPT0=0x%08x "
    "REFCLK=0x%08x HCS=0x%08x\n",
    MmioRead32 ((UINTN)Ufs->IoAddr + 0x00),
    MmioRead32 ((UINTN)Ufs->IoAddr + 0x2240),
    MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_XOUFS_CTRL),
    MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_MMIO_OPT_CTRL0),
    MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_REFCLK_CTRL),
    MmioRead32 ((UINTN)Ufs->IoAddr + 0x30)));

  DEBUG ((DEBUG_INFO, "UFS: MT6899 board init done (hci=0x%lx phy=0x%lx)\n",
          (UINTN)MT6899_UFS_HCI_BASE, (UINTN)MT6899_UFS_PHY_BASE));

  return EFI_SUCCESS;
}
