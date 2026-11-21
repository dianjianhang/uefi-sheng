/**
  rodin (Redmi Turbo 4 / POCO X7 Pro, MT6899) - Memory Map Library.

  GENERATED from a live, root-verified dump of a booted rodin (model 24129RT7CC,
  HyperOS 4.0 / Android 17, kernel 6.6.118, KernelSU root) on 2025-11-21:

      /proc/device-tree/memory@40000000        -> 0x40000000 + 0x300000000 (12 GiB, single bank)
      /proc/device-tree/reserved-memory (each mblock reg)  -> 53 LK "mblock" carveouts (base+size)
      /proc/iomem                              -> System RAM holes + MMIO register ranges

  The live device tree carries the addresses LK actually programmed (the vendor
  boot/lk images only carry sizes, with address fields zeroed), so this is the
  authoritative absolute layout - no mblock value below is invented.

  Tiling: the whole 12 GiB bank is partitioned with NO overlaps into
    * BsData : the Silicium UEFI runtime cluster in the free 0xA7000000..0xA8000000
               window (FD 0xA7100000, stack 0xA760D000, info block, page tables,
               log buffer, SEC/sched heaps, FV region, UEFI reserve) -- laid out to
               match the proven marble/peridot/muyu port and consumed via the
               BootShim boot-header-v4 handoff,
    * Reserv : every LK mblock -- BL31/ATF, TEE secmem/reserved/shm, GZ, SCP, SSPM,
               MCUPM, VCP, APU/apusys, ADSP, modem (md_mem/ap_md_*), framebuffer,
               display, connsys wifi/bt, wifi dma-pool, security, debug/log buffers,
    * Conv   : remaining free DRAM (~10.5 GiB total).

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

  // ---- DDR bank 0x40000000..0x340000000 (12 GiB): LK mblocks (Reserv) + Silicium runtime cluster (BsData) + free (Conv) ----
  {"System RAM", 0x040000000, 0x08080000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"aee-debug", 0x048080000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"minirdump", 0x048090000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0480A0000, 0x00370000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"crypto-hw", 0x048410000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x048420000, 0x003E0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"BL31-ATF", 0x048800000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x048A00000, 0x24200000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"display_platform_debug", 0x06CC00000, 0x02000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"pstore", 0x06EC00000, 0x00410000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x06F010000, 0x08FF0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"adsp-image", 0x078000000, 0x01930000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x079930000, 0x014D0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"mtksmmu-protpgd", 0x07AE00000, 0x02200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"emi-isu", 0x07D000000, 0x01000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"aee_lk", 0x07E000000, 0x00800000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x07E800000, 0x01080000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"gpueb-shared", 0x07F880000, 0x00180000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"tee-shm", 0x07FA00000, 0x00400000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x07FE00000, 0x000F0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"MCUPM-reserved", 0x07FEF0000, 0x00100000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"emi-mbist", 0x07FFF0000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"mm-ccu", 0x080000000, 0x00100000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x080100000, 0x07D00000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"consys-emi", 0x087E00000, 0x00D00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"wifi-dma-pool", 0x088B00000, 0x01400000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"consys-scp-emi", 0x089F00000, 0x00020000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x089F20000, 0x000E0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"ap-md-c", 0x08A000000, 0x02560000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x08C560000, 0x01AA0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"ap-md-nc", 0x08E000000, 0x00140000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x08E140000, 0x00C20000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"apu-rv-coredump", 0x08ED60000, 0x002A0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"scp-share", 0x08F000000, 0x00840000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x08F840000, 0x000C0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"apu-sapu-gzshm", 0x08F900000, 0x00100000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"apu-rv-secure", 0x08FA00000, 0x00600000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x090000000, 0x0F060000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"sspm-ap-shared", 0x09F060000, 0x00110000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"met-res-ram", 0x09F170000, 0x00C00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"SSPM-reserved", 0x09FD70000, 0x00280000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x09FFF0000, 0x07010000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"FD Reserved", 0x0A7000000, 0x00100000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"UEFI FD", 0x0A7100000, 0x00300000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"UEFI FD Reserved", 0x0A7400000, 0x00200000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"System RAM", 0x0A7600000, 0x00001000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"Info Blk", 0x0A7601000, 0x00001000, AddMem, SYS_MEM, SYS_MEM_CAP, RtData, WRITE_BACK_XN},
  {"MMU PageTables", 0x0A7602000, 0x00003000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN},
  {"Log Buffer", 0x0A7605000, 0x00008000, AddMem, SYS_MEM, SYS_MEM_CAP, RtData, WRITE_BACK_XN},
  {"UEFI Stack", 0x0A760D000, 0x00040000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN},
  {"SEC Heap", 0x0A764D000, 0x0008C000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN},
  {"Sched Heap", 0x0A76D9000, 0x00400000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN},
  {"FV Region", 0x0A7AD9000, 0x00400000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN},
  {"UEFI RESV", 0x0A7ED9000, 0x00127000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK_XN},
  {"System RAM", 0x0A8000000, 0x10000000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"SCP-reserved", 0x0B8000000, 0x00A00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0B8A00000, 0x06600000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"vcp-reserved", 0x0BF000000, 0x00800000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"tee-secmem", 0x0BF800000, 0x00500000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0BFD00000, 0x000F0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"ccci_tag_mem", 0x0BFDF0000, 0x00010000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"atf-log", 0x0BFE00000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0C0000000, 0x0C000000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0x0CC000000, 0x04110000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0D0110000, 0x000F0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0x0D0200000, 0x00250000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0D0450000, 0x00BB0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0x0D1000000, 0x074B0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0D84B0000, 0x112D0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"md-mem", 0x0E9780000, 0x00880000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0EA000000, 0x02000000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"adsp-shared", 0x0EC000000, 0x00CB0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0ECCB0000, 0x00350000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"dfd-dfdmcu_dump", 0x0ED000000, 0x00280000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0ED280000, 0x00D80000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"dfd-dfdsoc_dump", 0x0EE000000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"tee-reserved", 0x0EE200000, 0x0E300000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0FC500000, 0x000C0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"dfd-slbc_shared", 0x0FC5C0000, 0x00040000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"gpu-mputab", 0x0FC600000, 0x00600000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0FCC00000, 0x00020000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  {"Display Reserved", 0x0FCC20000, 0x033D0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"System RAM", 0x0FFFF0000, 0x00010000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
  // DXE Heap: carved out of the free 3GiB System RAM block; DxeCore locates it by name.
  {"DXE Heap",   0x100000000, 0x07C00000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK_XN},
  {"System RAM", 0x107C00000, 0x0B83F0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv, WRITE_BACK},
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
  {"UFS HCI", 0x112B0000, 0x00003000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // ufshci@112b0000 (live)
  {"Display Config", 0x14000000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // dispsys-config@14000000 (live)
  {"WatchDog Timer", 0x1C00B000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}, // watchdog@1c00b000 mt6899-wdt (live)
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
