// NOTE(MT6899): Structural derivation from MediaTek MT6789Pkg (UniXNU/uefi-sheng).
// MT6899 (Dimensity 8400-Ultra) register maps, clock tree and GPIO/PMIC/UFS/Msdc
// tuning are NOT yet validated against the MT6899 TRM. This file is a build
// scaffold only - do not run on hardware until re-derived from sourced data.
// See Platforms/Xiaomi/rodinPkg/PORTING.md.
#include <Library/GpioImplLib.h>

MTK_GPIO_PLATFORM_INFO gPlatformInfo = {
  .SetOffset     = 0x4,
  .ResetOffset   = 0x8,
  .DirOffset     = 0x0,
  .DataOutOffset = 0x100,
  .DataInOffset  = 0x200,
  .ModeOffset    = 0x300,
  .MaxPin        = 189,
};
