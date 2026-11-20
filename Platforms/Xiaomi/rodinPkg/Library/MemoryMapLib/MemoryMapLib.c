/**
  rodin (Redmi Turbo 4 / POCO X7 Pro, MT6899) - Memory Map Library.

  Phase 1 scaffold. Data sources:
    * DDR low window: vendor DTB /memory@40000000 = 0x40000000 + 0x40000000 (1 GiB).
    * Framework regions (UEFI Stack / FD / DXE Heap) follow the standard MediaTek
      Mu-Silicium low-DRAM convention; exact addresses must be confirmed against the
      bootloader (LK) DRAM table before hardware use.
    * Register regions: base addresses transcribed from the rodin vendor DTB
      (gic500@c000000, gic-cpu@c400000, watchdog@1c00b000, ufshci@112b0000,
      systimer@1cc10000, serial@11002000, dispsys-config@14000000, ...).

  TODO(MT6899) before device use:
    * Add the remaining ~11 GiB high DRAM bank (board.toml dram_size = 0x300000000)
      and all TEE/ATF/SPM/display carveouts from the bootloader DRAM table.
    * Confirm the AP watchdog control register (DTB exposes watchdog@1c00b000 as a
      syscon; the MTK AP WDT key register may differ from older SoCs).
    * Validate every register base/offset against the MT6899 TRM.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryMapLib.h>

STATIC EFI_MEMORY_REGION_DESCRIPTOR gMemoryDescriptor[] = {
  // Name, Address, Length, HobOption, ResourceType, ResourceAttribute, MemoryType, ArmAttribute

  // ---- DDR low window (vendor DTB /memory@40000000: 1 GiB) ----
  {"UEFI Stack",         0x40000000, 0x00040000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"UEFI FD",            0x40040000, 0x00200000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  // Remaining low window (up to 0x80000000) is conventional memory.
  // TODO(MT6899): insert TEE/ATF/display carveouts before handing this to the OS.
  {"DXE Heap",           0x40240000, 0x3FDC0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK},

  // ---- Register / MMIO regions (bases from vendor DTB) ----
  {"GIC Distributor",    0x0C000000, 0x00400000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // gic500@c000000
  {"GIC CPU Interface",  0x0C400000, 0x00040000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // gic-cpu@c400000
  {"Top Ck Gen",         0x10000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // topckgen
  {"Infracfg AO",        0x10001000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"Pinctrl/GPIO",       0x10005000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // gpio/pinctrl
  {"AP Mixed",           0x1000C000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // apmixed
  {"PMIC Wrapper",       0x1000D000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // pwrap
  {"EMI",                0x10219000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // emicen
  {"Infracfg",           0x1020E000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE},
  {"Dramc Ch0",          0x10230000, 0x00002000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // dramc
  {"UART0",              0x11002000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // serial@11002000
  {"MSDC-0",             0x11240000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // mmc@11240000
  {"UFS PHY",            0x112A0000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // ufsphy
  {"UFS HCI",            0x112B0000, 0x00002A00, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // ufshci
  {"USB XHCI",           0x11200000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // xhci0
  {"Display Config",     0x14000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // dispsys-config
  {"Systimer",           0x1CC10000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // systimer
  {"WatchDog Timer",     0x1C00B000, 0x00000100, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}  // watchdog (TODO: confirm AP WDT reg)
};

VOID
GetMemoryMap (
  OUT EFI_MEMORY_REGION_DESCRIPTOR **MemoryDescriptor,
  OUT UINT8                         *MemoryDescriptorCount)
{
  *MemoryDescriptor      = gMemoryDescriptor;
  *MemoryDescriptorCount = ARRAY_SIZE (gMemoryDescriptor);
}
