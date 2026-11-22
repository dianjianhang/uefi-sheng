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
// ufscfg_ao reset-controller (vendor DTB syscon@112b8000/reset-controller,
// ti,reset-bits = <0x48 0x9 0x4c 0x9 ...>): single reset at bit 9, which is
// the UFS PHY reset.  If it stays asserted every MPHY register reads 0.
#define UFS_AO_MPHY_RST_BIT      BIT9
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

//
// Compact diagnostics that UfsDxe prints again right before it halts, so the
// key values stay on the last screen instead of scrolling away.
//   [0] entry OTSD        [1] ufscfg_ao STA     [2] ufscfg_pdn STA
//   [3] MPHY 8C after power-on                  [4] SMCCC version
//   [5] MTCMOS-on SMC ret [6] refclk SMC ret    [7] device-reset SMC ret
//   [8] HCS after init    [9] OPT_CTRL0
//   [10] MPHY 8C after MTCMOS (before clocks)   [11] MPHY 9C after power-on
//   [12] HCE at entry                           [13] OTSD after board init
//
UINT32  gMt6899UfsDiag[14];
UINT32  gMt6899SpmDiag[6];

//
// MediaTek SiP SMC for UFS (from ufs-mediatek-sip.h).
//   MTK_SIP_UFS_CONTROL = ARM_SMCCC_CALL_VAL(FAST_CALL, SMC_64,
//                          OWNER_SIP(2), 0x276)
//                       = 0xC2000276
//   cmd: MTCMOS_CTRL = BIT9 (0x200), MPHY_CTRL = BIT8 (0x100)
//   arg1 (v1): MTCMOS on/off (1=on), MPHY op (0=BACKUP,1=RESTORE)
//
#define MTK_SIP_UFS_CONTROL     0xC2000276ULL   // KERNEL table (NS_EL1) - NOT for BL33
#define MTK_SIP_BL_UFS_CONTROL  0xC200040DULL   // BL33 table (UEFI/LK) - only DEVICE_RESET
#define MT6899_SPM_BASE         0x1C001000UL    // power-controller@1c001000 (SPM/SCPSYS)
#define SPM_POWERON_CONFIG_EN   (MT6899_SPM_BASE + 0x000)
#define SPM_PWR_STATUS          (MT6899_SPM_BASE + 0x180)
#define SPM_PROJECT_CODE        0xB16U
// MT6899 SPM register offsets (drivers/misc/mediatek/lpm/.../mt6899/spm_reg.h)
//   UFS0_PWR_CON      = SPM_BASE + 0xE18
//   UFS0_PHY_PWR_CON  = SPM_BASE + 0xE1C
#define MT6899_UFS0_PWR_CON_OFF      0xE18U
#define MT6899_UFS0_PHY_PWR_CON_OFF  0xE1CU
#define PWR_RST_B               BIT0
#define PWR_ISO                 BIT1
#define PWR_ON                  BIT2
#define PWR_ON_2ND              BIT3
#define PWR_CLK_DIS             BIT4
#define PWR_ACK                 BIT30
#define PWR_ACK_2ND             BIT31
#define SRAM_PDN                BIT8
#define SRAM_PDN_ACK            BIT12
#define UFS_SIP_MTCMOS_CTRL     0x200ULL   // BIT(9)
#define UFS_SIP_MPHY_CTRL       0x100ULL   // BIT(8)
#define UFS_SIP_REF_CLK_NOTIFY  0x8ULL     // BIT(3)
#define UFS_SIP_DEVICE_RESET    0x2ULL     // BIT(1)
#define UFS_MTCMOS_ON           1ULL
#define UFS_MTCMOS_OFF          0ULL
#define UFS_MPHY_RESTORE        1ULL
#define UFS_REFCLK_ON           1ULL
#define UFS_REFCLK_OFF          0ULL
#define UFS_SMC_PRE_CHANGE      0ULL
#define UFS_SMC_POST_CHANGE     1ULL
#define UFS_DEV_RESET_ASSERT    0ULL   // active-low
#define UFS_DEV_RESET_DEASSERT  1ULL

STATIC
UINT64
Mt6899UfsSipSmc (
  IN UINT64 Cmd,
  IN UINT64 Arg1,
  IN UINT64 Arg2
  )
{
  //
  // CRITICAL: the SMCCC ABI requires the function ID in x0 and arguments in
  // x1..x3.  Without explicit register binding the compiler is free to pick
  // any register, so the SMC ran with garbage and returned the unmodified FID
  // (v10/v11 photos showed ret=0xC2000276 = the function ID itself).
  //
  register UINT64 X0 __asm__("x0") = MTK_SIP_UFS_CONTROL;
  register UINT64 X1 __asm__("x1") = Cmd;
  register UINT64 X2 __asm__("x2") = Arg1;
  register UINT64 X3 __asm__("x3") = Arg2;

  __asm__ volatile ("smc #0"
    : "+r" (X0), "+r" (X1), "+r" (X2), "+r" (X3)
    :
    : "memory", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
      "x12", "x13", "x14", "x15", "x16", "x17");
  return X0;
}

/**
  Probe the SMCCC version (function ID 0x80000000).  A working SMC conduit
  returns 0x10000 | minor (e.g. 0x10001 for SMCCC 1.1).  Anything else means
  SMC is not usable from this exception level and every MTK SiP call below
  is a no-op.
**/
STATIC
UINT64
Mt6899SmcccVersion (
  VOID
  )
{
  register UINT64 X0 __asm__("x0") = 0x80000000ULL;
  register UINT64 X1 __asm__("x1") = 0;
  register UINT64 X2 __asm__("x2") = 0;
  register UINT64 X3 __asm__("x3") = 0;

  __asm__ volatile ("smc #0"
    : "+r" (X0), "+r" (X1), "+r" (X2), "+r" (X3)
    :
    : "memory", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
      "x12", "x13", "x14", "x15", "x16", "x17");
  return X0;
}

STATIC
BOOLEAN
SpmWaitAck (
  IN UINTN   Reg,
  IN UINT32  Mask,
  IN BOOLEAN Set
  )
{
  UINT32 Retry;
  for (Retry = 0; Retry < 500; Retry++) {
    if (((MmioRead32 (Reg) & Mask) != 0) == Set) {
      return TRUE;
    }
    MicroSecondDelay (1);
  }
  return FALSE;
}

/**
  Power on one SPM MTCMOS domain (TF-A plat/mediatek/drivers/mtcmos/mtcmos.c
  sequence).  MT6899 SPM lives at 0x1C001000 (power-controller@1c001000).

  The UFS PHY sits on its own domain (UFS0_PHY_PWR_CON); while it is off every
  MPHY register reads back 0.  We probe the known candidate offsets first and
  only touch one that looks powered-down.
**/
STATIC
VOID
SpmPowerOnDomain (
  IN UINTN   Off,
  IN BOOLEAN HasSram
  )
{
  UINT32 V = MmioRead32 ((UINTN)(MT6899_SPM_BASE + Off));

  DEBUG ((DEBUG_ERROR, "UFS SPM dom 0x%lx before=%08x\n", (UINTN)Off, V));

  if ((V & PWR_ACK) != 0) {
    DEBUG ((DEBUG_ERROR, "UFS SPM dom 0x%lx already ON\n", (UINTN)Off));
    return;
  }

  // Unlock SPM register writes (project code 0xB16).
  MmioWrite32 ((UINTN)SPM_POWERON_CONFIG_EN, ((UINT32)SPM_PROJECT_CODE << 16) | 1);

  MmioOr32 ((UINTN)(MT6899_SPM_BASE + Off), PWR_ON);
  if (!SpmWaitAck (MT6899_SPM_BASE + Off, PWR_ACK, TRUE)) {
    DEBUG ((DEBUG_ERROR, "UFS SPM 0x%lx PWR_ACK timeout\n", (UINTN)Off));
  }
  MicroSecondDelay (50);

  MmioOr32 ((UINTN)(MT6899_SPM_BASE + Off), PWR_ON_2ND);
  if (!SpmWaitAck (MT6899_SPM_BASE + Off, PWR_ACK_2ND, TRUE)) {
    DEBUG ((DEBUG_ERROR, "UFS SPM 0x%lx PWR_ACK_2ND timeout\n", (UINTN)Off));
  }

  MmioAnd32 ((UINTN)(MT6899_SPM_BASE + Off), ~(PWR_CLK_DIS | PWR_ISO));
  MicroSecondDelay (10);
  MmioOr32 ((UINTN)(MT6899_SPM_BASE + Off), PWR_RST_B);

  if (HasSram) {
    MmioAnd32 ((UINTN)(MT6899_SPM_BASE + Off), ~SRAM_PDN);
    SpmWaitAck (MT6899_SPM_BASE + Off, SRAM_PDN_ACK, FALSE);
  }

  DEBUG ((DEBUG_ERROR, "UFS SPM dom 0x%lx after=%08x\n",
    (UINTN)Off, MmioRead32 ((UINTN)(MT6899_SPM_BASE + Off))));
}

/**
  Power on the MT6899 UFS power domains directly through the SPM.

  TF-A exposes this only through MTK_SIP_KERNEL_UFS_CONTROL (0x276) which is
  in the NS_EL1 table and therefore NOT callable from BL33 (UEFI).  The SPM is
  ordinary MMIO, so we replay exactly what spm_mtcmos_ctrl() does.

  Offsets are authoritative from the MT6899 BSP header
  drivers/misc/mediatek/lpm/modules/include/mt6899/spm_reg.h:
    UFS0_PWR_CON     = 0x1C001000 + 0xE18
    UFS0_PHY_PWR_CON = 0x1C001000 + 0xE1C
  The PHY domain is the one whose power-off makes every MPHY register read 0.
**/
STATIC
VOID
Mt6899SpmProbeAndPowerPhy (
  VOID
  )
{
  UINT32 Cfg;

  //
  // Unlock SPM register writes first (TF-A writes project code 0xB16 | BIT0
  // before every MTCMOS operation), then read it back to validate the base.
  //
  MmioWrite32 ((UINTN)SPM_POWERON_CONFIG_EN, ((UINT32)SPM_PROJECT_CODE << 16) | 1);
  Cfg = MmioRead32 ((UINTN)SPM_POWERON_CONFIG_EN);

  gMt6899SpmDiag[0] = Cfg;
  gMt6899SpmDiag[1] = MmioRead32 ((UINTN)SPM_PWR_STATUS);
  gMt6899SpmDiag[2] = MmioRead32 ((UINTN)(MT6899_SPM_BASE + 0xE10));
  gMt6899SpmDiag[3] = MmioRead32 ((UINTN)(MT6899_SPM_BASE + 0xE14));
  gMt6899SpmDiag[4] = MmioRead32 ((UINTN)(MT6899_SPM_BASE + MT6899_UFS0_PWR_CON_OFF));
  gMt6899SpmDiag[5] = MmioRead32 ((UINTN)(MT6899_SPM_BASE + MT6899_UFS0_PHY_PWR_CON_OFF));

  DEBUG ((DEBUG_ERROR,
    "UFS SPM cfg=%08x sta=%08x usb0=%08x audio=%08x ufs0=%08x ufsphy=%08x\n",
    gMt6899SpmDiag[0], gMt6899SpmDiag[1], gMt6899SpmDiag[2],
    gMt6899SpmDiag[3], gMt6899SpmDiag[4], gMt6899SpmDiag[5]));

  //
  // NOTE: MT6899's POWERON_CONFIG_EN does not read back the 0xB16 project
  // code -- it only returns bit0 (v16 photo: cfg=0x00000001).  The write is
  // the unlock, so do NOT gate the power sequence on a read-back signature.
  //
  // UFS controller domain first (usually already on), then the PHY domain.
  SpmPowerOnDomain (MT6899_UFS0_PWR_CON_OFF, TRUE);
  SpmPowerOnDomain (MT6899_UFS0_PHY_PWR_CON_OFF, FALSE);

  gMt6899SpmDiag[4] = MmioRead32 ((UINTN)(MT6899_SPM_BASE + MT6899_UFS0_PWR_CON_OFF));
  gMt6899SpmDiag[5] = MmioRead32 ((UINTN)(MT6899_SPM_BASE + MT6899_UFS0_PHY_PWR_CON_OFF));
  DEBUG ((DEBUG_ERROR, "UFS SPM final ufs0=%08x ufsphy=%08x\n",
    gMt6899SpmDiag[4], gMt6899SpmDiag[5]));
}

/**
   Power on the UFS MTCMOS power domain and restore the MPHY calibration.

   Linux ufs_mtk_init() (drivers/ufs/host/ufs-mediatek.c) does exactly this
   when the DT has "mediatek,ufs-rtff-mtcmos" -- which rodin.dtb does.  Without
   it the MPHY analog block stays unpowered and every write to 0x112A0000
   reads back 0 (the v9 symptom), even with all clock gates enabled.
**/
STATIC
VOID
Mt6899MtcmosOn (
  VOID
  )
{
  UINT64 RmtOff;
  UINT64 RmtOn;
  UINT64 Rmp;

  // MTCMOS off first to reset the power domain (Linux: "First Restore
  // here, to avoid backup unexpected value"), then on to init.
  RmtOff = Mt6899UfsSipSmc (UFS_SIP_MTCMOS_CTRL, UFS_MTCMOS_OFF, 0);
  RmtOn  = Mt6899UfsSipSmc (UFS_SIP_MTCMOS_CTRL, UFS_MTCMOS_ON, 0);
  // Restore MPHY settings after power-on.
  Rmp = Mt6899UfsSipSmc (UFS_SIP_MPHY_CTRL, UFS_MPHY_RESTORE, 0);

  gMt6899UfsDiag[5] = (UINT32)RmtOn;
  DEBUG ((DEBUG_ERROR, "UFS SMC MTCMOS off=%lx on=%lx MPHYrst=%lx\n",
    (UINTN)RmtOff, (UINTN)RmtOn, (UINTN)Rmp));
}

/**
  Notify the SCP/PMIC to enable or disable the 26 MHz reference clock to the
  UFS MPHY.  Linux calls ufs_mtk_ref_clk_notify(on, stage, res) around the
  register-level REFCLK_REQUEST/ACK handshake; without the SMC the external
  clock source stays gated and the MPHY PLL cannot lock (PA LINERESET).
**/
STATIC
VOID
Mt6899RefClkNotify (
  IN UINT64  On,
  IN UINT64  Stage
  )
{
  UINT64 Ret = Mt6899UfsSipSmc (UFS_SIP_REF_CLK_NOTIFY, On, Stage);
  if (Stage == UFS_SMC_POST_CHANGE) {
    gMt6899UfsDiag[6] = (UINT32)Ret;
  }
  DEBUG ((DEBUG_ERROR, "UFS SMC refclk on=%lx stage=%lx ret=%lx\n",
    (UINTN)On, (UINTN)Stage, (UINTN)Ret));
}

/**
  Pulse the UFS device reset (active-low RST_n).  Linux ufs_mtk_device_reset()
  asserts low >= 10us, deasserts high, waits 10ms for the device to boot.
**/
STATIC
VOID
Mt6899DeviceReset (
  VOID
  )
{
  UINT64 Rlow;
  UINT64 Rhigh;

  // The BL33 table FID 0xC200040D returned -1 (not supported) on this TF-A;
  // the kernel FID 0xC2000276 is reachable from UEFI (refclk call returned 0),
  // so use it for DEVICE_RESET.
  Rlow = Mt6899UfsSipSmc (UFS_SIP_DEVICE_RESET, UFS_DEV_RESET_ASSERT, 0);
  MicroSecondDelay (20);       // >= 10us per spec

  Rhigh = Mt6899UfsSipSmc (UFS_SIP_DEVICE_RESET, UFS_DEV_RESET_DEASSERT, 0);

  gMt6899UfsDiag[7] = (UINT32)Rhigh;
  DEBUG ((DEBUG_ERROR, "UFS SMC devrst lo=%lx hi=%lx\n",
    (UINTN)Rlow, (UINTN)Rhigh));
}

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

   Ported from Linux clk-mt6899-peri.c (ufsao/ufspdn gate clks).  These
   mtk_clk_gate_ops_setclr gates are ACTIVE-LOW: writing 1 to CLR (+0xC)
   enables the clock and STA bit==0 means enabled.  Gates are OFF after
   bootloader handoff, so MPHY/HCI MMIO would read back 0 if left gated.
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

  gMt6899UfsDiag[1] = StaAo;
  gMt6899UfsDiag[2] = StaPdn;
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
  Release the UFS PHY reset on the ufscfg_ao reset controller (bit 9).
  MediaTek ti,syscon-reset blocks use +0x48 to assert and +0x4C to deassert.
  While this reset is asserted every MPHY register reads back 0.
**/
STATIC
VOID
Mt6899MphyResetRelease (
  VOID
  )
{
  MmioWrite32 ((UINTN)(MT6899_UFS_CFG_AO_BASE + UFS_RST_ASSERT_REG), UFS_AO_MPHY_RST_BIT);
  MicroSecondDelay (200);
  MmioWrite32 ((UINTN)(MT6899_UFS_CFG_AO_BASE + UFS_RST_DEASSERT_REG), UFS_AO_MPHY_RST_BIT);
  MicroSecondDelay (1000);

  DEBUG ((DEBUG_ERROR, "UFS MPHY reset released (ao 0x48=%08x 0x4c=%08x)\n",
    MmioRead32 ((UINTN)(MT6899_UFS_CFG_AO_BASE + UFS_RST_ASSERT_REG)),
    MmioRead32 ((UINTN)(MT6899_UFS_CFG_AO_BASE + UFS_RST_DEASSERT_REG))));
}

/**
  Full MT6899 MPHY tuning sequence, transcribed from the stock preloader
  (preloader_raw.img, function at 0x8e1fc; helper mphy_update(off,clr,set) at
  0x9007c writes 0x112A0000+off).  Linux's phy-mtk-ufs.c only restores the
  four power registers, because on a normal boot the preloader's calibration
  survives; if the PHY domain was power-cycled we have to replay all of it.
**/
STATIC
VOID
Mt6899MphyUpdate (
  IN UINT32 Off,
  IN UINT32 Clr,
  IN UINT32 Set
  )
{
  UINT32 V = MmioRead32 ((UINTN)(MT6899_UFS_PHY_BASE + Off));
  V = (V & ~Clr) | Set;
  MmioWrite32 ((UINTN)(MT6899_UFS_PHY_BASE + Off), V);
}

STATIC
VOID
Mt6899MphyTuning (
  VOID
  )
{
  UINT32 i;

  // global registers
  Mt6899MphyUpdate (0x1000, 0x0000003C, 0x0000002C);
  Mt6899MphyUpdate (0x1064, 0x00000FFF, 0x00000ACC);
  Mt6899MphyUpdate (0x9000, 0x00000040, 0);
  Mt6899MphyUpdate (0x9100, 0x00000040, 0);
  Mt6899MphyUpdate (0x105C, 0, 0x00000009);
  Mt6899MphyUpdate (0x00F0, 0x0003C07F, 0x00030058);
  Mt6899MphyUpdate (0x1010, 0x00000010, 0);
  Mt6899MphyUpdate (0x0010, 0x00200000, 0);
  Mt6899MphyUpdate (0x8110, 0x00000300, 0);
  Mt6899MphyUpdate (0x00F4, 0x00000F00, 0);
  Mt6899MphyUpdate (0x001C, 0xFF000000, 0x40000000);
  Mt6899MphyUpdate (0x0200, 0, 0x0000FFFF);

  for (i = 0; i < 2; i++) {
    UINT32 L = i * 0x100;

    Mt6899MphyUpdate (0xA6D8 + L, 0x03FFE000, 0x03FE0000);
    Mt6899MphyUpdate (0xC0E8 + L, 0x0000FFFF, 0);
    Mt6899MphyUpdate (0xC0F0 + L, 0xFFFFFFFF, 0);
    Mt6899MphyUpdate (0xA6D0 + L, 0x0000F000, 0xC0007000);
    Mt6899MphyUpdate (0xA6D4 + L, 0x00003FFF, 0x00001DDD);
    Mt6899MphyUpdate (0xA480 + L, 0x00000222, 0);
    Mt6899MphyUpdate (0xA208 + L, 0x0C000000, 0x08000000);
    Mt6899MphyUpdate (0xA20C + L, 0x000F3000, 0x000A2000);
    Mt6899MphyUpdate (0xA210 + L, 0x000000C3, 0x00000082);
    Mt6899MphyUpdate (0xA214 + L, 0x60000000, 0x40000000);
    Mt6899MphyUpdate (0xB030 + L, 0, 0x20000000);
    Mt6899MphyUpdate (0xA6C8 + L, 0x0C000000, 0x0C000000);
    Mt6899MphyUpdate (0xA020 + L, 0x0E000000, 0x06000000);
    Mt6899MphyUpdate (0xA4F0 + L, 0xFFFFFFFF, 0x80000000);
    Mt6899MphyUpdate (0xB034 + L, 0x000000FF, 0x000000AC);
    Mt6899MphyUpdate (0xB030 + L, 0x000000FF, 0x00000024);
  }

  DEBUG ((DEBUG_ERROR, "UFS MPHY tuning applied (preloader sequence)\n"));
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

  // SMC PRE_CHANGE: tell SCP/PMIC to start the 26MHz XO.
  Mt6899RefClkNotify (UFS_REFCLK_ON, UFS_SMC_PRE_CHANGE);

  MmioWrite32 ((UINTN)Ufs->IoAddr + REG_UFS_REFCLK_CTRL, REFCLK_REQUEST);

  for (Waited = 0; Waited < REFCLK_TIMEOUT_US; Waited += 10) {
    Value = MmioRead32 ((UINTN)Ufs->IoAddr + REG_UFS_REFCLK_CTRL);
    if (((Value & REFCLK_ACK) >> 1) == (Value & REFCLK_REQUEST)) {
      // SMC POST_CHANGE: confirm refclk is up.
      Mt6899RefClkNotify (UFS_REFCLK_ON, UFS_SMC_POST_CHANGE);
      MicroSecondDelay (32);   // ungating wait (Linux REFCLK_DEFAULT_WAIT_US)
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

    gMt6899UfsDiag[0] = Ot;
    gMt6899UfsDiag[12] = MmioRead32 ((UINTN)MT6899_UFS_HCI_BASE + 0x34);
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
  // v10: power on the UFS MTCMOS domain + restore MPHY calibration via SiP SMC.
  // rodin.dtb has "mediatek,ufs-rtff-mtcmos"; Linux ufs_mtk_init() does this
  // and without it the MPHY analog block stays off (MPHY regs read 0 in v9).
  //
  gMt6899UfsDiag[4] = (UINT32)Mt6899SmcccVersion ();
  DEBUG ((DEBUG_ERROR, "UFS SMCCC version=0x%x\n", gMt6899UfsDiag[4]));

  Mt6899SpmProbeAndPowerPhy ();
  Mt6899MtcmosOn ();
  gMt6899UfsDiag[10] = MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C);
  DEBUG ((DEBUG_ERROR, "UFS MPHY 8C after MTCMOS = %08x\n", gMt6899UfsDiag[10]));

  // Reset the UFS device (active-low RST_n pulse via SMC) so it enters a
  // clean state before we try to talk to it.  Linux calls this via
  // vops->device_reset in ufshcd_link_startup error recovery.
  Mt6899DeviceReset ();

  //
  // CRITICAL (v8 root-cause fix): enable the UFS clock gates.
  // On MT6899 the ufscfg_ao/ufscfg_pdn gates are OFF after bootloader handoff.
  // Without phy_sap (26 MHz PHY reference) and phy_ahb_s_busck, every MPHY
  // MMIO write is dropped by the bus and reads back as 0.  Mirror Linux
  // clk-mt6899-peri.c + ufs_mtk_init.
  //
  Mt6899ClockEnable ();

  //
  // Release the UFS PHY reset (ufscfg_ao bit 9), then bring up the MPHY
  // analog and the reference clock, before the host reset carried out in
  // ufs_mtk_hce_enable_notify().
  //
  Mt6899MphyResetRelease ();
  Mt6899MphyPowerOn ();
  Mt6899MphyTuning ();

  {
    UINT32 C8 = MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C);
    UINT32 T;
    gMt6899UfsDiag[3] = C8;
    gMt6899UfsDiag[11] = MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_LN_DIG_RX_9C);
    DEBUG ((DEBUG_ERROR, "UFS MPHY after power-on 8C=%08x (was 0 in v7)\n", C8));

    //
    // Write-readback probe: proves whether the MPHY register block is clocked
    // and powered at all.  A value that does not stick means the block is
    // isolated (still unpowered / held in reset).
    //
    MmioWrite32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C, 0x00000001);
    T = MmioRead32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C);
    MmioWrite32 ((UINTN)MT6899_UFS_PHY_BASE + MP_GLB_DIG_8C, C8);
    DEBUG ((DEBUG_ERROR, "UFS MPHY write-test 8C<-1 read=%08x\n", T));

    DEBUG ((DEBUG_ERROR, "UFS HCI MPHYWIN 2200=%08x 2204=%08x 2240=%08x\n",
      MmioRead32 ((UINTN)Ufs->IoAddr + 0x2200),
      MmioRead32 ((UINTN)Ufs->IoAddr + 0x2204),
      MmioRead32 ((UINTN)Ufs->IoAddr + 0x2240)));
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
  Mt6899MphyResetRelease ();
  Mt6899MphyPowerOn ();
  Mt6899MphyTuning ();

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

  gMt6899UfsDiag[13] = MmioRead32 ((UINTN)MT6899_UFS_HCI_BASE + REG_UFS_UFS_MMIO_OTSD_CTRL);
  gMt6899UfsDiag[8] = MmioRead32 ((UINTN)MT6899_UFS_HCI_BASE + 0x30);
  gMt6899UfsDiag[9] = MmioRead32 ((UINTN)MT6899_UFS_HCI_BASE + REG_UFS_MMIO_OPT_CTRL0);

  DEBUG ((DEBUG_INFO, "UFS: MT6899 board init done (hci=0x%lx phy=0x%lx)\n",
          (UINTN)MT6899_UFS_HCI_BASE, (UINTN)MT6899_UFS_PHY_BASE));

  return EFI_SUCCESS;
}
