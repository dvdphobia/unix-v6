# Userspace build (flat binaries for exec)

CC            = i686-elf-gcc
AS            = i686-elf-as
LD            = i686-elf-ld
AR            = i686-elf-ar
OBJCOPY       = i686-elf-objcopy

CFLAGS        = -ffreestanding -O2 -nostdlib -Wall -Wextra
CFLAGS       += -I. -ILibraries/libc/include -fno-builtin -fno-stack-protector
ASFLAGS       = --32
LDFLAGS       = -T linker.ld -nostdlib

BUILD_DIR     = ../build/userland
LIBC_DIR      = Libraries/libc
LIBC          = $(BUILD_DIR)/libc.a

# C Runtime objects from libc/src/crt/
CRT_START     = $(BUILD_DIR)/crt0.o $(BUILD_DIR)/crti.o $(BUILD_DIR)/crtbegin.o
CRT_END       = $(BUILD_DIR)/crtend.o $(BUILD_DIR)/crtn.o
LIBC_LIBS     = $(LIBC)

# Programs
PROGS         = init sh ls cat echo pwd hello netdemo ps uname clear help pwd_test tcc mkdir rm cp winserver gui

BIN_SRCS      = $(addprefix Utilities/,ls.c cat.c echo.c pwd.c hello.c netdemo.c ps.c uname.c clear.c help.c pwd_test.c tcc.c mkdir.c rm.c cp.c)

OBJS          = \
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
                $(BUILD_DIR)/pwd_test.o \
                $(BUILD_DIR)/tcc.o \
                $(BUILD_DIR)/mkdir.o \
                $(BUILD_DIR)/rm.o \
                $(BUILD_DIR)/cp.o \
                $(BUILD_DIR)/winserver.o \
                $(BUILD_DIR)/gui.o

# ... (omitted)



.PHONY: all clean libc crt applications

all: crt libc applications $(addprefix $(BUILD_DIR)/,$(addsuffix .bin,$(PROGS)))

# Build applications
applications: crt libc
	@$(MAKE) -C Applications

# Build C runtime objects from libc/src/crt/
crt: $(CRT_START) $(CRT_END)

$(BUILD_DIR)/crt0.o: $(LIBC_DIR)/src/crt/crt0.o | $(BUILD_DIR)
	@cp $< $@

$(BUILD_DIR)/crt1.o: $(LIBC_DIR)/src/crt/crt1.o | $(BUILD_DIR)
	@cp $< $@

$(BUILD_DIR)/crti.o: $(LIBC_DIR)/src/crt/crti.o | $(BUILD_DIR)
	@cp $< $@

$(BUILD_DIR)/crtn.o: $(LIBC_DIR)/src/crt/crtn.o | $(BUILD_DIR)
	@cp $< $@

$(BUILD_DIR)/crtbegin.o: $(LIBC_DIR)/src/crt/crtbegin.o | $(BUILD_DIR)
	@cp $< $@

$(BUILD_DIR)/crtend.o: $(LIBC_DIR)/src/crt/crtend.o | $(BUILD_DIR)
	@cp $< $@

# Ensure CRT objects are built in libc
$(LIBC_DIR)/src/crt/%.o: $(LIBC_DIR)/src/crt/%.c
	@$(MAKE) -C $(LIBC_DIR) crt

# Build libc
libc: $(LIBC)

$(LIBC): $(wildcard $(LIBC_DIR)/src/*.c) | $(BUILD_DIR)
	@echo "Building libc..."
	@$(MAKE) -C $(LIBC_DIR) CC=$(CC) AR=$(AR)
	@cp $(LIBC_DIR)/libc.a $(LIBC)
	@echo "libc built successfully"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)



# Build program object files
$(BUILD_DIR)/init.o: Init/init.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/sh.o: Shell/sh.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ls.o: Utilities/ls.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/cat.o: Utilities/cat.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/echo.o: Utilities/echo.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/pwd.o: Utilities/pwd.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/hello.o: Utilities/hello.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/netdemo.o: Utilities/netdemo.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ps.o: Utilities/ps.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/uname.o: Utilities/uname.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD_DIR)/clear.o: Utilities/clear.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/help.o: Utilities/help.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/pwd_test.o: Utilities/pwd_test.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/tcc.o: Utilities/tcc.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/mkdir.o: Utilities/mkdir.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/rm.o: Utilities/rm.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/cp.o: Utilities/cp.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Link each program with full CRT and libc
$(BUILD_DIR)/%.elf: $(BUILD_DIR)/%.o $(CRT_START) $(LIBC) $(CRT_END) linker.ld
	@echo "LD  $@"
	@$(LD) -T linker.ld -nostdlib -o $@ \
		$(CRT_START) \
		$(BUILD_DIR)/$*.o \
		$(LIBC) \
		$(CRT_END)

# Convert ELF to flat binary
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "BIN $@"
	@$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/winserver.o: WindowServer/main.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/gui.o: Utilities/gui.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@$(MAKE) -C Applications clean
	@$(MAKE) -C $(LIBC_DIR) clean
	@rm -rf $(BUILD_DIR)
	@echo "Userspace clean complete."
