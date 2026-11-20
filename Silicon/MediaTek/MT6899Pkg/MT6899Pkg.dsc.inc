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
  *_*_AARCH64_CC_FLAGS = -march=armv8.2-a+crypto+rcpc

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
  # GICv3 - distributor 0x0C000000 (gic500@c000000), CPU interface 0x0C400000 (gic-cpu@c400000)
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x0C000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x0C400000

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
