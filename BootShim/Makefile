ARCH              = arm64
TARGET            = aarch64-linux-gnu
CC                = clang
CFLAGS            = --target=$(TARGET)
OBJCOPY           = llvm-objcopy
SRC              ?= BootShim.S
EXTRA_DEFS       ?=

all: BootShim.elf BootShim.bin

BootShim.bin: BootShim.elf
	$(OBJCOPY) -O binary $< $@

BootShim.elf: $(SRC)
	$(CC) ${CFLAGS} -c $(SRC) -o $@ -DREQUIRES_KERNEL_HEADER=$(REQUIRES_KERNEL_HEADER) -DFD_BASE=$(FD_BASE) -DFD_SIZE=$(FD_SIZE) $(EXTRA_DEFS)

BootShim.S:
