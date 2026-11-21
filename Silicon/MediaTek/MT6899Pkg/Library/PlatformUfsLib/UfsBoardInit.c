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
#define MT6899_UFS_CFG_AO_BASE 0x112B8000UL   // syscon@112b8000 ufscfg_ao
#define MT6899_UFS_CFG_PDN_BASE 0x112BB000UL  // syscon@112bb000 ufscfg_pdn

//
// MT6899 UFS clock gates (from Linux clk-mt6899-peri.c, mtk_clk_gate_ops_setclr):
//   ufscfg_ao  +0x8 SET / +0xC CLR / +0x4 STA.  Gate is ACTIVE-LOW:
//   writing 1 to CLR (+0xC) enables the clock; STA bit == 0 means enabled.
//   ufscfg_pdn +0x8 SET / +0xC CLR / +0x4 STA (same layout).
//
// ufsao_clks:  bit3  unipro_sys   (parent ufs_ck)
//              bit8  u_phy_sap     (parent vlp_infra_26m_ck)
//              bit9  u_phy_ahb_s_busck (parent ufs_faxi_ck)
// ufspdn_clks: bit0  ufshci_ufs
//              bit1  ufshci_aes
//              bit3  ufshci_u_ahb
//
// Without u_phy_sap (26 MHz PHY reference) and u_phy_ahb_s_busck, all MPHY
// MMIO writes at 0x112A0000 are dropped by the bus (read back as 0) and
// DME_LINK_STARTUP always fails with arg2=1. This is the v7 root cause.
//
#define UFSCFG_CG_SET_OFFSET    0x08
#define UFSCFG_CG_CLR_OFFSET    0x0C
#define UFSCFG_CG_STA_OFFSET    0x04

#define UFSAO_BIT_UNIPRO_SYS    BIT3
#define UFSAO_BIT_PHY_SAP       BIT8
#define UFSAO_BIT_PHY_AHB       BIT9
#define UFSPDN_BIT_UFSHCI_UFS   BIT0
#define UFSPDN_BIT_UFSHCI_AES   BIT1
#define UFSPDN_BIT_UFSHCI_AHB   BIT3

#define UFSAO_CLK_MASK (UFSAO_BIT_UNIPRO_SYS | UFSAO_BIT_PHY_SAP | UFSAO_BIT_PHY_AHB)
#define UFSPDN_CLK_MASK (UFSPDN_BIT_UFSHCI_UFS | UFSPDN_BIT_UFSHCI_AES | UFSPDN_BIT_UFSHCI_AHB)

//
// MediaTek UFSHCI vendor registers (inside the UFSHCI window)
//
#define REG_UFS_XOUFS_CTRL     0x140
#define REG_UFS_REFCLK_CTRL    0x144
#define REG_UFS_UFS_MMIO_OTSD_CTRL 0x14C
#define REG_UFS_MMIO_OPT_CTRL0 0x160

//
// ufscfg_pdn reset controller (vendor DTB syscon@112bb000, phandle 0x166).
// ti,reset-bits = <0x48 0x00 0x4c 0x00 ...> for unipro_rst / crypto_rst / hci_rst.
//   assert   = write bit to +0x48 (reset set)
//   deassert = write bit to +0x4c (reset clear)
//
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
   Enable every clock the UFS controller, UniPro and MPHY need.

   Ported from Linux clk-mt6899-peri.c (ufsao/ufspdn gate clks) and the
   TOP_UFS_SEL mux-gate in ClockImplLib.  MediaTek clk gates use the set/clr
   register pair: writing 1 to the SET register enables the clock, and the
   STA register reports the current state.  These gates are OFF after the
   bootloader hands off, which is why MPHY MMIO writes read back as 0 in v7
   and DME_LINK_STARTUP always failed with arg2=1.
**/
STATIC
VOID
Mt6899ClockEnable (
  VOID
  )
{
  UINT32 StaAo;
  UINT32 StaPdn;

  // NOTE: topckgen TOP_UFS_SEL mux/PLL is left untouched -- the bootloader
  // configured ufs_ck before it loaded us (the HCI registers are readable),
  // and re-selecting its parent would risk switching to a wrong clock source.

  //
  // mtk_clk_gate_ops_setclr is ACTIVE-LOW: enable = mtk_cg_clr_bit(), i.e.
  // write 1 to the CLR register (+0xC); "enabled" means STA bit == 0.
  // v8 wrote the SET register (+0x8), which GATED all UFS clocks off -- that
  // is why HCI regs read back all 0.  v9 writes CLR (+0xC) instead.
  //
  // ufscfg_ao: unipro_sys, phy_sap (26 MHz PHY ref), phy_ahb_s_busck.
  MmioWrite32 ((UINTN)(MT6899_UFS_CFG_AO_BASE + UFSCFG_CG_CLR_OFFSET), UFSAO_CLK_MASK);
  // ufscfg_pdn: ufshci_ufs, ufshci_aes, ufshci_u_ahb.
  MmioWrite32 ((UINTN)(MT6899_UFS_CFG_PDN_BASE + UFSCFG_CG_CLR_OFFSET), UFSPDN_CLK_MASK);
  MicroSecondDelay (100);

  StaAo  = MmioRead32 ((UINTN)(MT6899_UFS_CFG_AO_BASE + UFSCFG_CG_STA_OFFSET));
  StaPdn = MmioRead32 ((UINTN)(MT6899_UFS_CFG_PDN_BASE + UFSCFG_CG_STA_OFFSET));

  DEBUG ((DEBUG_ERROR, "UFS CLK AO=%08x want=0 PDN=%08x want=0\n",
    StaAo & UFSAO_CLK_MASK, StaPdn & UFSPDN_CLK_MASK));
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
  // Report the bootloader hand-off state before touching any hardware.
  // OTSD_CTRL bits[31:24] carry the UniPro link state on MT6899
  // (VS_LINK_UP == 2), per ufs_mtk_wait_link_state().
  //
  {
    UINT32 Ot = MmioRead32 ((UINTN)MT6899_UFS_HCI_BASE + REG_UFS_UFS_MMIO_OTSD_CTRL);

    DEBUG ((DEBUG_ERROR, "UFS BI entry OTSD=0x%08x LNK=%d HCS=0x%08x HCE=0x%08x\n",
      Ot, (Ot >> 24) & 0xFF,
      MmioRead32 ((UINTN)MT6899_UFS_HCI_BASE + 0x30),
      MmioRead32 ((UINTN)MT6899_UFS_HCI_BASE + 0x34)));

    if (((Ot >> 24) & 0xFF) == 2) {
      //
      // The bootloader already has the UFS link up (it read boot_a from UFS to
      // load us).  Re-running MPHY power-on / host reset / LINK_STARTUP on a
      // live link makes DME_LINK_STARTUP return FAILURE (arg2=1), which is what
      // every prior build hit.  Take over the working link untouched.
      //
      DEBUG ((DEBUG_ERROR, "UFS: link UP at entry -> warm takeover, skip re-init\n"));
      DEBUG ((DEBUG_ERROR, "UFS MPHY 8C=%08x 9C=%08x AC=%08x 44=%08x\n",
        MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C),
        MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_LN_DIG_RX_9C),
        MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_LN_DIG_RX_AC),
        MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_LN_RX_44)));
      return EFI_SUCCESS;
    }
  }

  //
  // CRITICAL (v8 root-cause fix): enable the UFS clock gates FIRST.
  // On MT6899 the ufscfg_ao/ufscfg_pdn gates are OFF after bootloader handoff.
  // Without phy_sap (26 MHz PHY reference) and phy_ahb_s_busck, every MPHY
  // MMIO write is dropped by the bus and reads back as 0 (exactly the v7
  // symptom: 8C/9C/AC/44 all 0).  Mirror Linux clk-mt6899-peri.c + ufs_mtk_init.
  //
  Mt6899ClockEnable ();

  //
  // Bring up the MPHY analog, then the reference clock, before the host reset
  // carried out in ufs_mtk_hce_enable_notify().
  //
  Mt6899MphyPowerOn ();

  {
    UINT32 C8 = MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C);
    DEBUG ((DEBUG_ERROR, "UFS MPHY after power-on 8C=%08x (was 0 in v7)\n", C8));
  }

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
  // The reset may have gated clocks and dropped the refclk handshake; re-enable
  // clocks, re-power the MPHY and re-assert refclk (all idempotent).
  //
  Mt6899ClockEnable ();
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
  // Split into short lines so nothing wraps off the phone screen.
  //
  DEBUG ((DEBUG_ERROR, "UFS BI1 VER=%08x IPVER=%08x\n",
    MmioRead32 ((UINTN)Ufs->IoAddr + 0x00),
    MmioRead32 ((UINTN)Ufs->IoAddr + 0x2240)));
  DEBUG ((DEBUG_ERROR, "UFS BI2 XOUFS=%08x OPT0=%08x\n",
    MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_XOUFS_CTRL),
    MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_MMIO_OPT_CTRL0)));
  DEBUG ((DEBUG_ERROR, "UFS BI3 REFCLK=%08x HCS=%08x HCE=%08x\n",
    MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_REFCLK_CTRL),
    MmioRead32 ((UINTN)Ufs->IoAddr + 0x30),
    MmioRead32 ((UINTN)Ufs->IoAddr + 0x34)));
  DEBUG ((DEBUG_ERROR, "UFS BI4 MPHY 8C=%08x 9C=%08x AC=%08x 44=%08x\n",
    MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C),
    MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_LN_DIG_RX_9C),
    MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_LN_DIG_RX_AC),
    MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_LN_RX_44)));
  {
    UINT32 Ot = MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_UFS_MMIO_OTSD_CTRL);
    DEBUG ((DEBUG_ERROR, "UFS BI5 post OTSD=%08x LNK=%d\n", Ot, (Ot >> 24) & 0xFF));
  }

  DEBUG ((DEBUG_INFO, "UFS: MT6899 board init done (hci=0x%lx phy=0x%lx)\n",
          (UINTN)MT6899_UFS_HCI_BASE, (UINTN)MT6899_UFS_PHY_BASE));

  return EFI_SUCCESS;
}
