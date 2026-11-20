/**
  rodin (Redmi Turbo 4 / POCO X7 Pro, MT6899) - Memory Map Library.

  Data sources (verified against the rodin vendor dumps under turbo4分区/ — the
  vendor DTB, lk.img, preloader_raw.img, gz.img — unless marked TODO(MT6899)):
    * DDR low window: vendor DTB /memory@40000000 = 0x40000000 + 0x40000000 (1 GiB).
    * Framework regions (UEFI Stack / FD / DXE heap) follow the standard MediaTek
      Mu-Silicium low-DRAM convention; the DXE heap is split around the Xiaomi
      bootmonitor_pmsg carveout.
    * BootMonitor PMSG: vendor DTB /soc/bootmonitor_pmsg@6ec00000 reg = 0x6ec00000
      + 0x410000. Reserved so the firmware never hands the pmsg/console ring to
      the OS / DXE heap.
    * Register regions: every base below was transcribed from the rodin vendor
      DTB (gic500@c000000, gic-cpu@c400000, syscon@10000000 topckgen,
      infra_infracfg_ao_reg 0x10001000, apmixedsys 0x1000c000, pwrap 0x1000d000,
      emicen@10219000, infracfg@1020e000, dramc 0x10230000, serial@11002000,
      xhci0@11200000, mmc@11240000, ufsphy@112a0000, ufshci@112b0000,
      dispsys-config@14000000, systimer@1cc10000, watchdog@1c00b000).

  AP watchdog: the vendor DTB exposes watchdog@1c00b000 reg 0x1c00b000 + 0x100
  compatible "mediatek,mt6899-wdt" (TOPRGU syscon child) -> 0x1C00B000 is
  source-confirmed, not a guess.

  ----------------------------------------------------------------------------
  High DRAM and firmware carveouts - why they are NOT hardcoded here
  ----------------------------------------------------------------------------
  Total DRAM is 12 GiB (board.toml dram_size = 0x300000000) at base 0x40000000,
  i.e. a contiguous 0x40000000..0x340000000 span. lk.img / preloader_raw.img do
  NOT contain a static absolute memory-layout table: LK computes DRAM size from
  the preloader "dram_para"/calibration at boot (lk strings: "Total DRAM = %llx",
  "dramc: read size from dram_para"), and allocates every reserved mblock at
  runtime from DRAM (the DTB carrier properties carry sizes, but their address
  fields are literally 0, e.g. scp-remap-addr=0, sspm-res-ram-start=0). All three
  vendor DTBs (vendor_boot/dtbo/tee) also have an EMPTY /memreserve/ table.

  Real carveout SIZES that ARE present in the vendor DTB (base = runtime, so not
  placed here; recorded for the bootloader/OEM table lookup):
    * SCP        : scp-dram-size 0x00200000 (2 MiB), sram-size 0x240000
    * SSPM       : sspm-res-ram-size 0x00110000 (1.0625 MiB), share-region 0x8000 @ +0x38000
    * VCP        : vcp-dram-size 0x00800000 (8 MiB), secure-dump 0x200000 @ +0x600000
    * MCUPM/SSPM/GPUEB met-res-ram : 0x00400000 each
    * SVP 0x06000000 / TUI 0x04000000 / Prot 0x02000000 (memory-ssmr-features)
    * SSMR reserved CMA 0x10000000 ; CCCI dpmaif cache 0x190000 / nocache 0x50000 ;
      MKP kernel-code-protection 0x10000 ; connected-SCP shm 0x20000 @ remap-offset 0x2100000
    * TEE / ATF(BL31) / GZ(GenieZone) / modem : sizes/sites not exposed in these images.

  Therefore the region above the verified low window is mapped as a single
  Reserv placeholder (deliberately NOT handed to the OS: feeding unsourced DRAM
  to the payload would risk corrupting TEE/ATF). Replace it with the real
  per-region carveouts once the bootloader LK "reserved mblock" table - or
  /proc/iomem / an mrdump from a booted rodin - is available, then promote the
  leftover holes to Conv.

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
  // TODO(MT6899): carve TEE/ATF/SPM/SSPM/VCP/SCP/display out of the bootloader DRAM
  // table and promote the leftover to Conv. Kept Reserv until that table is sourced.
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
