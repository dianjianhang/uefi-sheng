// NOTE(MT6899): Structural derivation from MediaTek MT6789Pkg (UniXNU/uefi-sheng).
// MT6899 (Dimensity 8400-Ultra) register maps, clock tree and GPIO/PMIC/UFS/Msdc
// tuning are NOT yet validated against the MT6899 TRM. This file is a build
// scaffold only - do not run on hardware until re-derived from sourced data.
// See Platforms/Xiaomi/rodinPkg/PORTING.md.
//
// Copyright (c) 2012-2013, ARM Limited. All rights reserved.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include <AsmMacroLib.h>

ASM_FUNC (PlatformAssemblyInitialize)
  // Nothing to do Here.
  ret
