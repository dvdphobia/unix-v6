# Userspace build (flat binaries for exec)

CC            = i686-elf-gcc
AS            = i686-elf-as
LD            = i686-elf-ld
OBJCOPY       = i686-elf-objcopy

CFLAGS        = -ffreestanding -O2 -nostdlib -Wall -Wextra
CFLAGS       += -I. -fno-builtin -fno-stack-protector
ASFLAGS       = --32
LDFLAGS       = -T linker.ld -nostdlib

BUILD_DIR     = ../build/userland

COMMON_OBJS   = $(BUILD_DIR)/start.o $(BUILD_DIR)/syscalls.o

# Programs
PROGS         = init sh ls cat echo pwd hello netdemo ps uname clear help pwd_test

BIN_SRCS      = $(addprefix bin/,ls.c cat.c echo.c pwd.c hello.c netdemo.c ps.c uname.c clear.c help.c pwd_test.c)

OBJS          = $(COMMON_OBJS) \
                $(BUILD_DIR)/init.o \
                $(BUILD_DIR)/sh.o \
                $(BUILD_DIR)/ls.o \
                $(BUILD_DIR)/cat.o \
                $(BUILD_DIR)/echo.o \
                $(BUILD_DIR)/pwd.o \
                $(BUILD_DIR)/hello.o \
                $(BUILD_DIR)/netdemo.o \
                $(BUILD_DIR)/ps.o \
                $(BUILD_DIR)/uname.o \
                $(BUILD_DIR)/clear.o \
                $(BUILD_DIR)/help.o \
                $(BUILD_DIR)/pwd_test.o

.PHONY: all clean

all: $(addprefix $(BUILD_DIR)/,$(addsuffix .bin,$(PROGS)))

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/start.o: start.S | $(BUILD_DIR)
	@echo "AS  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/syscalls.o: syscalls.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/init.o: init.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/sh.o: sh.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ls.o: bin/ls.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/cat.o: bin/cat.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/echo.o: bin/echo.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/pwd.o: bin/pwd.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/hello.o: bin/hello.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/netdemo.o: bin/netdemo.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ps.o: bin/ps.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/uname.o: bin/uname.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/clear.o: bin/clear.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/help.o: bin/help.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/pwd_test.o: bin/pwd_test.c syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Link each program as a flat binary
$(BUILD_DIR)/%.elf: $(COMMON_OBJS) $(BUILD_DIR)/%.o linker.ld
	@echo "LD  $@"
	@$(LD) $(LDFLAGS) -o $@ $(COMMON_OBJS) $(BUILD_DIR)/$*.o

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "BIN $@"
	@$(OBJCOPY) -O binary $< $@

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Userspace clean complete."
