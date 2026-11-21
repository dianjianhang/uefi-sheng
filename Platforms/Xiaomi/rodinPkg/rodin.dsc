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
  # Live /proc/device-tree/memory@40000000: 0x40000000 + 0x300000000 (12 GiB, single bank).
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x300000000

  #
  # UEFI Stack (must match the "UEFI Stack" BsData descriptor at 0xA760D000)
  #
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0xA760D000
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
  # Hardware stride is 4992 bytes (AB24, width padded to 1248). FrameBufferSerialPortLib
  # derives pitch = Width*4, so set Width=1248; the extra 28px fall in the non-visible pad.
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|1248
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
