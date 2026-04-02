OUTPUT := doppio
ARCH   ?= x86_64

export BASE_DIR   := $(shell pwd)
export BUILD_DIR  := $(BASE_DIR)/build
export OBJ_DIR    := $(BUILD_DIR)/obj
export BIN_DIR    := $(BASE_DIR)/bin

ISO_IMAGE := $(BIN_DIR)/doppio.iso
ISO_ROOT  := $(BUILD_DIR)/iso_root

LIMINE_DIR := $(BASE_DIR)/limine

SUBDIRS := lib libc util kernel base modules boot

ifeq ($(ARCH), x86_64)
    ARCH_CFLAGS  := -m64 -march=x86-64 -mno-red-zone
    ARCH_LDFLAGS := -m elf_x86_64
    LIMINE_EFI   := $(LIMINE_DIR)/BOOTX64.EFI
else ifeq ($(ARCH), aarch64)
    ARCH_CFLAGS  := -march=armv8-a -mcpu=cortex-a72
    ARCH_LDFLAGS := -m aarch64elf
    LIMINE_EFI   := $(LIMINE_DIR)/BOOTAA64.EFI
endif

export CC      := clang -target $(ARCH)-unknown-none-elf
export LD      := ld.lld
export AS      := nasm 

export CFLAGS  := -g -O2 -ffreestanding -fno-stack-protector \
				  -fno-PIC -fno-PIE \
                  $(ARCH_CFLAGS) \
                  -ffunction-sections -fdata-sections -Wall -Wextra

export CPPFLAGS := -I$(BASE_DIR) \
                   -I$(BASE_DIR)/libc/include \
				   -I$(BASE_DIR)/include \
                   -D__$(ARCH)__

export LDFLAGS  := $(ARCH_LDFLAGS) -nostdlib -static --gc-sections \
				   -no-pie -z max-page-size=0x1000 \
                   -T $(BASE_DIR)/kernel/arch/$(ARCH)/linker.ld

.PHONY: all clean $(SUBDIRS) run limine_setup

all: $(BIN_DIR)/$(OUTPUT)

$(SUBDIRS):
	@mkdir -p $(OBJ_DIR)/$@
	@$(MAKE) -C $@ ARCH=$(ARCH)

$(BIN_DIR)/$(OUTPUT): $(SUBDIRS)
	@mkdir -p $(BIN_DIR)
	@echo "[LD] Linking $@"
	$(LD) $(LDFLAGS) $(shell find $(OBJ_DIR) -name "*.o" | sort) -o $@

limine_setup:
	@if [ ! -d "$(LIMINE_DIR)" ]; then \
		echo "Downloading Limine binaries..."; \
		git clone https://codeberg.org/Limine/Limine.git limine --branch=v10.x-binary --depth=1; \
	fi

iso: all
	@rm -rf $(ISO_ROOT)
	@mkdir -p $(ISO_ROOT)/boot/limine
	@mkdir -p $(ISO_ROOT)/EFI/BOOT

	@cp -v $(BIN_DIR)/$(OUTPUT) $(ISO_ROOT)/boot/
	@cp -v $(BASE_DIR)/limine.conf $(ISO_ROOT)/boot/limine/

	@cp -v $(LIMINE_DIR)/limine-bios.sys \
	       $(LIMINE_DIR)/limine-bios-cd.bin \
	       $(LIMINE_DIR)/limine-uefi-cd.bin \
	       $(ISO_ROOT)/boot/limine/
	@cp -v $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/
	@cp -v $(LIMINE_DIR)/BOOTIA32.EFI $(ISO_ROOT)/EFI/BOOT/

	@xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
	        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
	        -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
	        -efi-boot-part --efi-boot-image --protective-msdos-label \
	        $(ISO_ROOT) -o $(ISO_IMAGE)

	# @$(LIMINE_DIR)/limine bios-install $(ISO_IMAGE)

run: iso
	qemu-system-x86_64 \
		-cdrom $(ISO_IMAGE) \
		-m 8G \
		-serial stdio -d int -no-reboot -cpu host -accel kvm

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)