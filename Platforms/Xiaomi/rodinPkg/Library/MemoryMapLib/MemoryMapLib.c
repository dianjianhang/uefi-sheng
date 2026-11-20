/**
  rodin (Redmi Turbo 4 / POCO X7 Pro, MT6899) - Memory Map Library.

  Data sources (all verified against the rodin vendor dumps under
  turbo4分区/ and the vendor DTB unless explicitly marked TODO(MT6899)):
    * DDR low window: vendor DTB /memory@40000000 = 0x40000000 + 0x40000000 (1 GiB).
    * Framework regions (UEFI Stack / FD / DXE heap) follow the standard MediaTek
      Mu-Silicium low-DRAM convention; the DXE heap is split around the Xiaomi
      bootmonitor_pmsg carveout.
    * BootMonitor PMSG: vendor DTB /soc/bootmonitor_pmsg@6ec00000 reg = 0x6ec00000
      + 0x410000. Reserved so the firmware never hands the pmsg/console ring to
      the OS / DXE heap.
    * Register regions: every base below was transcribed from the rodin vendor
      DTB (gic500@c000000, gic-cpu@c400000, syscon@10000000 topckgen,
      infracfg_ao 0x10001000, apmixedsys 0x1000c000, pwrap 0x1000d000,
      emicen@10219000, infracfg@1020e000, dramc 0x10230000, serial@11002000,
      xhci0@11200000, mmc@11240000, ufsphy@112a0000, ufshci@112b0000,
      dispsys-config@14000000, systimer@1cc10000, watchdog@1c00b000).

  AP watchdog: the vendor DTB exposes watchdog@1c00b000 reg 0x1c00b000 + 0x100
  compatible "mediatek,mt6899-wdt". This is the TOPRGU register block (WDT is a
  sub-node of the toprgu syscon); 0x1C00B000 is therefore confirmed, not a guess.

  TODO(MT6899) before hardware use:
    * High DRAM: board.toml declares dram_size = 0x300000000 (12 GiB) at base
      0x40000000, i.e. a contiguous 0x40000000..0x340000000 span. The vendor DTB
      only describes the low 1 GiB and carries an EMPTY /memreserve/ table, and
      gz.img (GenieZone) computes DRAM size at runtime - so the usable split and
      every TEE/ATF/SPM/SSPM/SCP/display carveout must come from the bootloader
      (LK "reserved mblock") table. Until that table is sourced, the whole region
      above the verified low window is mapped as Reserv (NOT handed to the OS):
      feeding unsourced DRAM to the payload would risk corrupting TEE/ATF.
    * Replace the HIGH_DRAM_RESERVE placeholder below with the real per-region
      carveouts once the LK DRAM table (or /proc/iomem on a booted device) is
      available, then re-classify the remaining holes as Conv.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryMapLib.h>

STATIC EFI_MEMORY_REGION_DESCRIPTOR gMemoryDescriptor[] = {
  // Name, Address, Length, HobOption, ResourceType, ResourceAttribute, MemoryType, ArmAttribute

  // ---- DDR low window: vendor DTB /memory@40000000 (0x40000000 + 1 GiB) ----
  {"UEFI Stack",         0x40000000, 0x00040000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"UEFI FD",            0x40040000, 0x00200000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  // DXE heap, split around the bootmonitor_pmsg carveout at 0x6EC00000.
  {"DXE Heap Low",       0x40240000, 0x2E9C0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK},
  // Vendor DTB /soc/bootmonitor_pmsg@6ec00000 reg 0x6ec00000 + 0x410000 (Xiaomi pmsg/log ring).
  {"BootMonitor PMSG",   0x6EC00000, 0x00410000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"DXE Heap High",      0x6F010000, 0x10FF0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK},

  // ---- High DRAM placeholder (12 GiB total - 1 GiB low window = 11 GiB) ----
  // TODO(MT6899): carve TEE/ATF/SPM/SSPM/display out of the bootloader DRAM table
  // and promote the leftover to Conv. Kept Reserv until that table is sourced.
  {"High DRAM Reserve",  0x80000000, 0x2C0000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},

  // ---- Register / MMIO regions (bases verified against vendor DTB) ----
  {"GIC Distributor",    0x0C000000, 0x00400000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // gic500@c000000
  {"GIC CPU Interface",  0x0C400000, 0x00040000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // gic-cpu@c400000
  {"Top Ck Gen",         0x10000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // syscon@10000000 topckgen
  {"Infracfg AO",        0x10001000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // infra_infracfg_ao_reg
  {"Pinctrl/GPIO",       0x10005000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // gpio/pinctrl
  {"AP Mixed",           0x1000C000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // apmixedsys 0x1000c000
  {"PMIC Wrapper",       0x1000D000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // pwrap
  {"EMI",                0x10219000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // emicen@10219000
  {"Infracfg",           0x1020E000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // infracfg@1020e000
  {"Dramc Ch0",          0x10230000, 0x00002000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // dramc 0x10230000
  {"UART0",              0x11002000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // serial@11002000
  {"MSDC-0",             0x11240000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // mmc@11240000
  {"UFS PHY",            0x112A0000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // ufsphy@112a0000
  {"UFS HCI",            0x112B0000, 0x00002A00, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // ufshci@112b0000
  {"USB XHCI",           0x11200000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // xhci0@11200000
  {"Display Config",     0x14000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // dispsys-config@14000000
  {"Systimer",           0x1CC10000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}, // systimer@1cc10000
  {"WatchDog Timer",     0x1C00B000, 0x00000100, AddDev, MMAP_IO, UNCACHEABLE, MmIO,   NS_DEVICE}  // watchdog@1c00b000 (mt6899-wdt, confirmed)
};

VOID
GetMemoryMap (
  OUT EFI_MEMORY_REGION_DESCRIPTOR **MemoryDescriptor,
  OUT UINT8                         *MemoryDescriptorCount)
{
  *MemoryDescriptor      = gMemoryDescriptor;
  *MemoryDescriptorCount = ARRAY_SIZE (gMemoryDescriptor);
}
