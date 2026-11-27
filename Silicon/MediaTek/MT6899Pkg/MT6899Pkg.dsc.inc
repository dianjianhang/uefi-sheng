##
#  MT6899Pkg.dsc.inc - MediaTek Dimensity 8400-Ultra (MT6899)
#  Structurally derived from MT6789Pkg.dsc.inc (UniXNU/uefi-sheng).
#
#  Platform constants below are sourced from the rodin vendor DTB where noted
#  (dtc of stock vendor_boot.img). Values still marked TODO(MT6899) require the
#  MT6899 TRM / bootloader data and must be validated before hardware use.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  # Physical (generic) ARM architected timer, driven by CNTFRQ (13 MHz on rodin).
  USE_PHYSICAL_TIMER                         = 1

!include MediaTekPkg/MediaTekPkg.dsc.inc

[BuildOptions]
  # MediaTekPkg's shared DXE drivers are never exercised by the upstream CI
  # matrix (no MediaTek device is Active there), so they carry latent
  # unused-but-set-variable warnings (e.g. MsdcDxe stub functions). Relax
  # that single diagnostic for the rodin build only; no code is changed.
  *_*_AARCH64_CC_FLAGS = -march=armv8.2-a+crypto+rcpc -Wno-unused-but-set-variable

[PcdsFixedAtBuild]
  #
  # ACPI - "MT6899  " OEM table id, revision placeholder
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x202039393836544D
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision|0x6899

  #
  # CPU - 8x Cortex-A725 in a single cluster (vendor DTB: MPIDR Aff1=0, Aff0=0..7)
  #
  gArmPlatformTokenSpaceGuid.PcdCoreCount|8
  gArmPlatformTokenSpaceGuid.PcdClusterCount|1

  #
  # GICv3 - live /proc/iomem and vendor DTB interrupt-controller@c400000:
  #   GICD 0x0C400000 (len 0x40000), GICR 0x0C440000 (len 0x200000).
  #   (0x0C000000/0x0C400000 are the MT6789 values and are WRONG on MT6899.)
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x0C400000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x0C440000

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosProcessorModel|"MediaTek Dimensity 8400-Ultra @ 2.60 GHz"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosProcessorPartNumber|"MT6899"
  #
  # Storage: rodin uses UFS (vendor DTB ufshci@112b0000), not eMMC.
  #
  gMediaTekPkgTokenSpaceGuid.PcdStorageIsEMMC|FALSE

[LibraryClasses]
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  #
  # SEC Libraries
  #
  PlatformSecLib|MT6899Pkg/Library/PlatformSecLib/PlatformSecLib.inf

  #
  # Custom Libraries
  #
  GpioImplLib|MT6899Pkg/Library/GpioImplLib/GpioImplLib.inf
  ClockImplLib|MT6899Pkg/Library/ClockImplLib/ClockImplLib.inf
  PmicWrapperImplLib|MT6899Pkg/Library/PmicWrapperImplLib/PmicWrapperImplLib.inf
  MsdcImplLib|MT6899Pkg/Library/MsdcImplLib/MsdcImplLib.inf

  #
  # UFS: rodin storage is UFS 3.1 (ufshci@112b0000), not eMMC.
  # The generic Samsung UfsDxe driver is reused; only the board init and the
  # (unused) Samsung calibration hooks are MediaTek-specific.
  #
  PlatformUfsLib|MT6899Pkg/Library/PlatformUfsLib/PlatformUfsLib.inf
  UfsCalAdapterLib|SamsungPkg/Library/UfsCalAdapterLib/UfsCalAdapterLib.inf

[Components]
  #
  # UFS block-I/O driver + the ChipInfo protocol it consumes.
  #
  SamsungPkg/Drivers/UfsDxe/UfsDxe.inf
  MT6899Pkg/Drivers/ChipInfoDxe/ChipInfoDxe.inf
