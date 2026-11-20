##
#  rodin (Xiaomi Redmi Turbo 4 / POCO X7 Pro, MediaTek Dimensity 8400-Ultra MT6899)
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = rodin
  PLATFORM_GUID                  = 9c3f7a21-6d4e-4b8a-a2c1-0e5f6d7c8b9a
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/rodinPkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = RELEASE|DEBUG
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = rodinPkg/rodin.fdf
  USE_CUSTOM_DISPLAY_DRIVER      = 0

!include MT6899Pkg/MT6899Pkg.dsc.inc

[PcdsFixedAtBuild]
  #
  # DDR Memory
  # The vendor DTB only describes a 1 GiB window at 0x40000000.
  # TODO(MT6899): full DRAM (12 GiB) bring-up is deferred.
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x40000000

  #
  # UEFI Stack
  #
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x40000000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x40000

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemManufacturer|"Xiaomi"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemModel|"Redmi Turbo 4"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"rodin"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailSku|"24129RT7CC"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemBoardModel|"Redmi Turbo 4"

  #
  # Simple Frame Buffer
  # TODO(MT6899): panel geometry to be confirmed during display bring-up.
  #
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|1220
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferHeight|2712
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferColorDepth|32

  #
  # Storage (rodin boots from UFS)
  #
  gMediaTekPkgTokenSpaceGuid.PcdStorageIsEMMC|FALSE

[LibraryClasses]
  #
  # Memory Libraries
  #
  MemoryMapLib|rodinPkg/Library/MemoryMapLib/MemoryMapLib.inf
