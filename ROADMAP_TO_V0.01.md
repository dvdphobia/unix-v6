# Roadmap to Unix V6 x86 OS V0.01
## A Usable Operating System

**Current Status**: You have a bootable kernel with basic process management, a simple shell, and fundamental I/O. You've successfully implemented init, fork/exec, and a ramdisk filesystem.

**Goal**: Transform this into a **usable OS V0.01** that can run real programs, support development tools, and provide a stable POSIX-like environment.

---

## Critical Path to V0.01 (Priority Order)

### Phase 1: Core System Stability (CRITICAL - Week 1-2)

#### 1.1 Fix Existing System Calls
**Status**: Many syscalls are stubbed or incomplete
**Impact**: HIGH - Nothing works reliably without these

- [x] **Complete `stat`/`fstat`/`lstat` implementation**
  - Files: `kernel/core/sys.c`, `kernel/fs/iget.c`
  - Currently stubbed, needed for: ls -l, file utilities, build tools
  - Must return: file size, mode, timestamps, inode number
  
- [ ] **Implement `brk`/`sbrk` properly**
  - Files: `kernel/core/sys.c`, `kernel/core/alloc.c`
  - Currently exists but may need heap management fixes
  - Critical for: malloc/free, dynamic memory allocation
  - Test with memory-intensive programs

- [ ] **Fix `pipe` system call**
  - Files: `kernel/core/pipe.c` (exists but needs testing)
  - Status: Implementation exists, needs verification
  - Critical for: shell redirection, inter-process communication
  - Test: `ls | cat`, `echo hello | cat`

- [ ] **Implement `dup`/`dup2`**
  - Files: `kernel/core/sys.c`
  - Needed for: I/O redirection in shell
  - Test: `ls > file.txt`, `cat < input.txt`

#### 1.2 File System Operations
**Status**: Basic read/write works, but missing critical operations
**Impact**: HIGH - Can't manage files properly

- [ ] **Implement `link`/`unlink`**
  - Files: `kernel/fs/nami.c`, `kernel/fs/alloc.c`
  - Needed for: file deletion, hard links
  - Test: `rm file`, `ln source dest`

- [ ] **Implement `mkdir`/`rmdir`**
  - Files: `kernel/fs/nami.c`, `kernel/fs/alloc.c`
  - Needed for: directory management
  - Test: `mkdir test`, `rmdir test`

- [ ] **Implement `chmod`/`umask`**
  - Files: `kernel/core/sys.c`
  - Needed for: file permissions
  - Test: `chmod 755 file`

- [ ] **Implement `rename`**
  - Files: `kernel/fs/nami.c`
  - Needed for: moving/renaming files
  - Test: `mv old new`

#### 1.3 Process Management Completeness
**Status**: Basic fork/exec works, but missing critical features
**Impact**: HIGH - Can't run complex programs

- [ ] **Fix `wait`/`waitpid` exit status handling**
  - Files: `kernel/core/sys.c`
  - Currently returns PID but exit status handling is incomplete
  - Needed for: shell to detect command failures
  - Test: `false; echo $?` should print non-zero

- [ ] **Implement proper `execve` with argv/envp**
  - Files: `kernel/core/sys.c`
  - Currently argv works, but environment variables don't
  - Needed for: passing environment to programs
  - Test: `env`, `export VAR=value`

- [ ] **Implement `getppid`**
  - Files: `kernel/core/sys.c`
  - Needed for: process tree utilities
  - Test: `ps` showing parent PIDs

---

### Phase 2: Usability Features (HIGH - Week 3-4)

#### 2.1 Essential Utilities
**Status**: You have basic utilities (ls, cat, echo, pwd)
**Impact**: HIGH - Need these for daily use

- [x] **Implement `rm` utility**
  - Depends on: `unlink` syscall
  - File: `userland/Utilities/rm.c`
  
- [ ] **Implement `mv` utility**
  - Depends on: `rename` syscall
  - File: `userland/Utilities/mv.c`

- [ ] **Implement `cp` utility**
  - Depends on: `open`, `read`, `write`, `stat`
  - File: `userland/Utilities/cp.c`

- [x] **Implement `mkdir` utility**
  - Depends on: `mkdir` syscall
  - File: `userland/Utilities/mkdir.c`

- [x] **Enhance `ls` with `-l` flag**
  - Depends on: `stat` syscall
  - File: `userland/Utilities/ls.c`
  - Show: permissions, size, timestamps

- [ ] **Implement `touch` utility**
  - Depends on: `creat`, `utime`
  - File: `userland/Utilities/touch.c`

#### 2.2 Shell Improvements
**Status**: Basic shell works, but missing critical features
**Impact**: HIGH - Shell is the primary interface

- [ ] **Add I/O redirection to shell**
  - Depends on: `dup2`, `open`, `creat`
  - File: `userland/sh.c`
  - Support: `>`, `<`, `>>`
  - Test: `ls > files.txt`, `cat < input.txt`

- [ ] **Add pipe support to shell**
  - Depends on: `pipe`, `dup2`, `fork`
  - File: `userland/sh.c`
  - Support: `|`
  - Test: `ls | cat`, `cat file | grep pattern`

- [ ] **Add background jobs**
  - Depends on: `fork`, `wait`
  - File: `userland/sh.c`
  - Support: `&`
  - Test: `sleep 10 &`

- [ ] **Add basic job control**
  - Depends on: `setpgid`, `tcsetpgrp`
  - File: `userland/sh.c`
  - Support: Ctrl+C, Ctrl+Z

#### 2.3 Text Processing
**Status**: Only basic `cat` exists
**Impact**: MEDIUM - Needed for file manipulation

- [ ] **Implement `grep` utility**
  - File: `userland/Utilities/grep.c`
  - Basic pattern matching
  - Test: `grep "pattern" file.txt`

- [ ] **Implement `wc` utility**
  - File: `userland/Utilities/wc.c`
  - Count lines, words, characters
  - Test: `wc file.txt`

- [ ] **Implement `head`/`tail` utilities**
  - Files: `userland/Utilities/head.c`, `userland/Utilities/tail.c`
  - Test: `head -n 10 file.txt`

---

### Phase 3: Developer Tools (MEDIUM - Week 5-6)

#### 3.1 Basic Text Editor
**Status**: None
**Impact**: HIGH - Can't edit files without it

- [ ] **Implement `ed` or simple line editor**
  - File: `userland/Applications/ed.c`
  - Basic line-based editing
  - Alternative: Port `nano` or implement simple `vi`

#### 3.2 Build System Support
**Status**: No native compilation yet
**Impact**: MEDIUM - Needed for self-hosting

- [ ] **Implement `make` utility (basic)**
  - File: `userland/Utilities/make.c`
  - Parse Makefiles, run commands
  - Dependency tracking

- [ ] **Port or implement `ar` (archive utility)**
  - Needed for: creating `.a` libraries
  - File: `userland/Utilities/ar.c`

#### 3.3 Scripting Support
**Status**: Shell is basic
**Impact**: MEDIUM - Needed for automation

- [ ] **Add shell scripting features**
  - File: `userland/sh.c`
  - Support: variables, conditionals, loops
  - Test: Run `.sh` scripts

---

### Phase 4: System Robustness (MEDIUM - Week 7-8)

#### 4.1 Signal Handling
**Status**: Basic infrastructure exists
**Impact**: MEDIUM - Needed for process control

- [ ] **Complete signal delivery**
  - Files: `kernel/core/sig.c`
  - Implement: `sigaction`, `sigprocmask`, `sigsuspend`
  - Test: Ctrl+C kills processes, signal handlers work

- [ ] **Implement `alarm` and `pause`**
  - Files: `kernel/core/sig.c`, `kernel/core/clock.c`
  - Test: `sleep` command, timeouts

#### 4.2 Time and Date
**Status**: Basic clock exists
**Impact**: LOW-MEDIUM - Nice to have

- [ ] **Implement `time`/`gettimeofday`**
  - Files: `kernel/core/clock.c`, `kernel/core/sys.c`
  - Test: `date` command, timestamps

- [ ] **Implement `sleep`/`nanosleep`**
  - Files: `kernel/core/clock.c`
  - Test: `sleep 5`

#### 4.3 TTY and Terminal Control
**Status**: Basic console I/O works
**Impact**: MEDIUM - Needed for interactive programs

- [ ] **Implement basic `termios`**
  - Files: `kernel/drivers/char/console.c`, `kernel/core/tty.c`
  - Support: `tcgetattr`, `tcsetattr`, `tcflush`
  - Test: Raw mode, canonical mode

- [ ] **Implement basic `ioctl`**
  - Files: `kernel/core/sys.c`
  - Support: terminal size, control operations

---

### Phase 5: Filesystem Persistence (OPTIONAL - Week 9+)

#### 5.1 Disk Persistence
**Status**: Ramdisk only
**Impact**: MEDIUM - Data is lost on reboot

- [ ] **Enable disk writes to IDE**
  - Files: `kernel/drivers/block/ide.c`
  - Currently read-only
  - Test: Write files, reboot, verify persistence

- [ ] **Implement proper mount/umount**
  - Files: `kernel/fs/iget.c`
  - Support: mounting disk partitions
  - Test: `mount /dev/hda1 /mnt`

---

## Testing Strategy

### Milestone Tests for V0.01

**Test 1: Basic File Operations**
```bash
$ mkdir test
$ cd test
$ echo "Hello World" > hello.txt
$ cat hello.txt
Hello World
$ cp hello.txt hello2.txt
$ ls
hello.txt hello2.txt
$ rm hello.txt
$ ls
hello2.txt
```

**Test 2: Shell Redirection and Pipes**
```bash
$ ls > files.txt
$ cat files.txt
$ ls | cat
$ echo "test" | cat > output.txt
```

**Test 3: Process Management**
```bash
$ sleep 5 &
[1] 123
$ ps
PID  CMD
1    init
2    sh
123  sleep
$ wait
```

**Test 4: Text Processing**
```bash
$ cat file.txt | grep "pattern" | wc -l
$ head -n 10 large_file.txt
$ tail -n 5 large_file.txt
```

**Test 5: Build System**
```bash
$ cat > hello.c
#include <stdio.h>
int main() { printf("Hello\n"); return 0; }
^D
$ make hello
$ ./hello
Hello
```

---

## What Makes V0.01 "Usable"?

A usable OS V0.01 should support:

✅ **File Management**: Create, read, write, delete, copy, move files and directories  
✅ **Shell Operations**: Redirection, pipes, background jobs  
✅ **Text Processing**: View, search, and manipulate text files  
✅ **Process Control**: Run programs, manage processes, handle signals  
✅ **Basic Development**: Edit files, run scripts, basic compilation  
✅ **System Stability**: No crashes, proper error handling, clean shutdown  

---

## Current Strengths (Already Done! ✓)

- ✅ Bootable kernel with GRUB
- ✅ Process management (fork, exec, wait)
- ✅ Basic filesystem (ramdisk with V6 filesystem)
- ✅ Init process and shell
- ✅ Basic utilities (ls, cat, echo, pwd, ps)
- ✅ Memory management (segmentation-based)
- ✅ Basic I/O (console, keyboard)
- ✅ System call interface (int 0x80)
- ✅ Custom libc with syscall wrappers

---

## Recommended Next Steps (Start Here!)

### Week 1 Priority Tasks:

1. **Fix `stat`/`fstat`** - This unlocks `ls -l` and many utilities
2. **Implement `unlink`** - This enables `rm` command
3. **Implement `mkdir`/`rmdir`** - This enables directory management
4. **Test and fix `pipe`** - This enables shell pipes
5. **Implement `dup2`** - This enables I/O redirection

### Quick Wins (Low-hanging fruit):

- **Write `rm` utility** (once `unlink` works)
- **Write `mkdir` utility** (once `mkdir` syscall works)
- [x] **Write `cp` utility** (uses existing syscalls)
- [x] **Enhance `ls` with `-l` flag** (once `stat` works)

### Testing Approach:

1. Implement one syscall at a time
2. Write a test utility for each syscall
3. Test in QEMU before moving to next feature
4. Keep a test script that runs all tests

---

## Estimated Timeline

- **Phase 1** (Core Stability): 2 weeks
- **Phase 2** (Usability): 2 weeks
- **Phase 3** (Developer Tools): 2 weeks
- **Phase 4** (Robustness): 2 weeks
- **Total to V0.01**: ~8 weeks of focused development

---

## Success Criteria for V0.01 Release

- [ ] Can create, edit, and delete files from shell
- [ ] Can use pipes and redirection in shell
- [ ] Can run background processes
- [ ] Can compile and run simple C programs
- [ ] Can write and run shell scripts
- [ ] System runs stably for extended periods
- [ ] Basic utilities work reliably
- [ ] Documentation exists for all features

**Once these are complete, you'll have a genuinely usable Unix-like OS!**
