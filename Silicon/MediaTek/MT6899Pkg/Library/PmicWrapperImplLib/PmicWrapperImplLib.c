// NOTE(MT6899): Structural derivation from MediaTek MT6789Pkg (UniXNU/uefi-sheng).
// MT6899 (Dimensity 8400-Ultra) register maps, clock tree and GPIO/PMIC/UFS/Msdc
// tuning are NOT yet validated against the MT6899 TRM. This file is a build
// scaffold only - do not run on hardware until re-derived from sourced data.
// See Platforms/Xiaomi/rodinPkg/PORTING.md.
#include <Library/PmicWrapperImplLib.h>

MTK_PMIC_WRAPPER_PLATFORM_INFO gPlatformInfo = {
  .RegMap = {
    [PmicWrapperInitDone2]     = 0x0,
    [PmicWrapperWacs2Cmd]      = 0x880,
    [PmicWrapperWacs2VldClr]   = 0x8A4,
    [PmicWrapperWacs2RData]    = 0x8A8,
    [PmicWrapperSwinf2WData31] = 0x884,
    [PmicWrapperSwinf2RData31] = 0x894,
  },
  .ArbCapabilities = TRUE,
};
