/**
  rodin (Redmi Turbo 4 / POCO X7 Pro, MT6899) - Memory Map Library.

  GENERATED from a live, root-verified dump of a booted rodin (model 24129RT7CC,
  HyperOS 4.0 / Android 17, kernel 6.6.118, KernelSU root) on 2025-11-21:

      /proc/device-tree/memory@40000000        -> 0x40000000 + 0x300000000 (12 GiB, single bank)
      /proc/device-tree/reserved-memory/*/reg  -> 53 LK "mblock" carveouts (base + size)
      /proc/iomem                              -> System RAM holes + MMIO register ranges

  The live device tree carries the addresses LK actually programmed (the vendor
  boot/lk images only carry sizes, with address fields zeroed), so this is the
  authoritative absolute layout - no value below is invented.

  Tiling: the whole 12 GiB bank is partitioned with NO overlaps into
    * BsData : UEFI stack + firmware volume (low DRAM, 0x40000000..0x40240000),
    * Reserv : every LK mblock -- BL31/ATF, TEE secmem/reserved/shm, GZ, SCP, SSPM,
               MCUPM, VCP, APU/apusys, ADSP, modem (md_mem/ap_md_*), framebuffer,
               display, connsys wifi/bt, wifi dma-pool, security, debug/log buffers,
    * Conv   : remaining free DRAM (~10.85 GiB total, matches MemTotal ~10.6 GiB).

  Reserve-all-mblocks is deliberate and safe: launched after LK, these regions are
  physically owned by ATF/TEE/coprocessors and must never reach the payload; if UEFI
  ever replaced LK they are simply unused (small RAM loss, no corruption).

  MMIO bases verified against live /proc/iomem. Note: GICD is at 0x0C400000 (NOT
  0x0C000000), GICR at 0x0C440000, and the PMIC interface is SPMI at 0x1CC04000.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryMapLib.h>

STATIC EFI_MEMORY_REGION_DESCRIPTOR gMemoryDescriptor[] = {
  // Name, Address, Length, HobOption, ResourceType, ResourceAttribute, MemoryType, ArmAttribute

  // ---- DDR bank 0x40000000..0x340000000 (12 GiB): UEFI (BsData) + LK mblocks (Reserv) + free (Conv) ----
  {"UEFI Stack", 0x40000000, 0x00040000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"UEFI FD", 0x40040000, 0x00200000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"System RAM", 0x40240000, 0x07E40000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"aee-debug", 0x48080000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"minirdump", 0x48090000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x480A0000, 0x00370000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"crypto-hw", 0x48410000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x48420000, 0x003E0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"BL31-ATF", 0x48800000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x48A00000, 0x24200000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"display_platform_debug", 0x6CC00000, 0x02000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"pstore", 0x6EC00000, 0x00410000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x6F010000, 0x08FF0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"adsp-image", 0x78000000, 0x01930000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x79930000, 0x014D0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"mtksmmu-protpgd", 0x7AE00000, 0x02200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"emi-isu", 0x7D000000, 0x01000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"aee_lk", 0x7E000000, 0x00800000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x7E800000, 0x01080000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"gpueb-shared", 0x7F880000, 0x00180000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"tee-shm", 0x7FA00000, 0x00400000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x7FE00000, 0x000F0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"MCUPM-reserved", 0x7FEF0000, 0x00100000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"emi-mbist", 0x7FFF0000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"mm-ccu", 0x80000000, 0x00100000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x80100000, 0x07D00000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"consys-emi", 0x87E00000, 0x00D00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"wifi-dma-pool", 0x88B00000, 0x01400000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"consys-scp-emi", 0x89F00000, 0x00020000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x89F20000, 0x000E0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"ap-md-c", 0x8A000000, 0x02560000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x8C560000, 0x01AA0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"ap-md-nc", 0x8E000000, 0x00140000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x8E140000, 0x00C20000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"apu-rv-coredump", 0x8ED60000, 0x002A0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"scp-share", 0x8F000000, 0x00840000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x8F840000, 0x000C0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"apu-sapu-gzshm", 0x8F900000, 0x00100000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"apu-rv-secure", 0x8FA00000, 0x00600000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x90000000, 0x0F060000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"sspm-ap-shared", 0x9F060000, 0x00110000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"met-res-ram", 0x9F170000, 0x00C00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"SSPM-reserved", 0x9FD70000, 0x00280000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x9FFF0000, 0x18010000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"SCP-reserved", 0xB8000000, 0x00A00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xB8A00000, 0x06600000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"vcp-reserved", 0xBF000000, 0x00800000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"tee-secmem", 0xBF800000, 0x00500000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xBFD00000, 0x000F0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"ccci_tag_mem", 0xBFDF0000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"atf-log", 0xBFE00000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xC0000000, 0x0C000000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0xCC000000, 0x04110000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xD0110000, 0x000F0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0xD0200000, 0x00250000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xD0450000, 0x00BB0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0xD1000000, 0x074B0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xD84B0000, 0x112D0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0xE9780000, 0x00880000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xEA000000, 0x02000000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"adsp-shared", 0xEC000000, 0x00CB0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xECCB0000, 0x00350000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"dfd-dfdmcu_dump", 0xED000000, 0x00280000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xED280000, 0x00D80000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"dfd-dfdsoc_dump", 0xEE000000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"tee-reserved", 0xEE200000, 0x0E300000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xFC500000, 0x000C0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"dfd-slbc_shared", 0xFC5C0000, 0x00040000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"gpu-mputab", 0xFC600000, 0x00600000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xFCC00000, 0x00020000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"framebuffer", 0xFCC20000, 0x033D0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0xFFFF0000, 0xC0000000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"dramc-rk0", 0x1BFFF0000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x1C0000000, 0x165800000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"mkp", 0x325800000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"gpu-reserved", 0x325A00000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"gz", 0x325C00000, 0x02000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"cmdq-reserved", 0x327C00000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"gz-log", 0x327E00000, 0x00050000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x327E50000, 0x00110000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"security-dice", 0x327F60000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"security-booker", 0x327F70000, 0x18000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"log_store", 0x33FF70000, 0x00080000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"dramc-rk1", 0x33FFF0000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},

  // ---- Register / MMIO regions (bases verified against live /proc/iomem) ----
  {"GIC Distributor", 0x0C400000, 0x00040000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // GICD -- live /proc/iomem (NOT 0x0C000000)
  {"GIC Redistributor", 0x0C440000, 0x00200000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // GICR -- live /proc/iomem
  {"Top Ck Gen", 0x10000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // syscon@10000000 topckgen (live)
  {"Infracfg AO", 0x10001000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // infracfg-ao@10001000 (live)
  {"Pinctrl/GPIO", 0x10005000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // pinctrl -- vendor DTB
  {"AP Mixed", 0x1000C000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // syscon@1000c000 apmixedsys (live)
  {"Dramc Ch0", 0x10230000, 0x00002000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // dramc@10230000 (live)
  {"UART0", 0x11002000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // serial (live)
  {"USB0 XHCI", 0x11200000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // 11201000.usb0 (live)
  {"MSDC-0", 0x11240000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // mmc@11240000 -- vendor DTB
  {"UFS PHY", 0x112A0000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // ufsphy@112a0000 (live)
  {"UFS HCI", 0x112B0000, 0x00002A00, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // ufshci@112b0000 (live)
  {"Display Config", 0x14000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // dispsys-config@14000000 (live)
  {"WatchDog Timer", 0x1C00B000, 0x00000100, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // watchdog@1c00b000 mt6899-wdt (live)
  {"SPMI/PMIC", 0x1CC04000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // 1cc04000.spmi pmif -- live (PMIC via SPMI)
};

VOID
GetMemoryMap (
  OUT EFI_MEMORY_REGION_DESCRIPTOR **MemoryDescriptor,
  OUT UINT8                         *MemoryDescriptorCount)
{
  *MemoryDescriptor      = gMemoryDescriptor;
  *MemoryDescriptorCount = ARRAY_SIZE (gMemoryDescriptor);
}
