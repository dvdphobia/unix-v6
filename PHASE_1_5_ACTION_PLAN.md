# POSIX Implementation - Action Plan for Completion

## Executive Summary

**Current State**: Phase 1 complete (30% POSIX compliance)
**Critical Blockers**: 5 major issues preventing common programs from running
**Effort to Fix**: 2-3 weeks for critical fixes, 6-12 weeks for full Phase 2-3

---

## CRITICAL BLOCKERS (Fix Immediately)

### 1️⃣ PRIORITY 1: Fix getcwd() - Returns "/" Always

**Problem**: 
- `getcwd()` always returns "/" regardless of current directory
- `pwd` command doesn't work correctly
- Relative paths fail
- Shell working directory is broken

**Current Code Issue**:
- kernel/posix.c `sys_getcwd()` doesn't track actual path
- Only returns root directory path

**Impact**: CRITICAL - Many programs depend on correct cwd

**Estimated Time**: 1-2 hours

**Solution Approach**:
```c
// Need to:
1. Track current directory in proc structure
2. Maintain inode path from root
3. Build path string from inode chain
4. Return full path to caller
```

**Files to Modify**:
- `kernel/posix.c` - Rewrite sys_getcwd()
- `kernel/include/proc.h` - Add current directory tracking
- Test with `pwd` command in shell

---

### 2️⃣ PRIORITY 2: Fix Signal Masking

**Problem**:
- `sigprocmask()` is skeleton implementation
- Signal blocking doesn't work
- Signals can't be masked properly
- Background jobs, signal handling unreliable

**Current Code Issue**:
- kernel/posix_sig.c has `sys_sigprocmask()` but it's incomplete
- Signal blocking not actually implemented in kernel

**Impact**: HIGH - Signal-dependent programs fail

**Estimated Time**: 2-4 hours

**Solution Approach**:
```c
// Need to:
1. Add signal mask to user structure (u_sigmask)
2. Check mask in signal delivery code
3. Queue masked signals
4. Process queued signals when mask cleared
5. Handle signal interruption properly
```

**Files to Modify**:
- `kernel/include/user.h` - Add u_sigmask field
- `kernel/posix_sig.c` - Complete sigprocmask()
- `kernel/trap.c` - Check signal mask in delivery
- Test with signal masking programs

---

### 3️⃣ PRIORITY 3: Implement sleep() and _exit()

**Problem**:
- No `sleep()` syscall (many programs use this)
- No `_exit()` syscall (bypasses cleanup)
- Programs waiting for delays don't work

**Current Code Issue**:
- `sleep()` not in syscall table
- `_exit()` not in syscall table
- Both are essential POSIX syscalls

**Impact**: HIGH - Many programs need these

**Estimated Time**: 1-2 hours total

**Solution Approach**:

For `sleep()`:
```c
// Add syscall:
int sys_sleep(void) {
    int seconds = u.u_ar0[3];  // First argument
    alarm(seconds);
    pause();  // Wait for SIGALRM
    return 0;
}
```

For `_exit()`:
```c
// Add syscall:
void sys_exit(void) {
    int status = u.u_ar0[3];
    u.u_r.r_val1 = status;
    exit(status);  // Call existing exit
}
```

**Files to Modify**:
- `kernel/posix.c` - Add sys_sleep(), sys_exit()
- `kernel/sysent.c` - Add table entries (use free slots)
- `userspace/syscalls.c` - Add wrappers
- Test with `sleep` command

---

### 4️⃣ PRIORITY 4: Add Missing Core Syscalls

**Problem**:
- `rename()` - Can't move/rename files
- `symlink()` / `readlink()` - No symbolic links
- `getgroups()` / `setgroups()` - No group management
- `seteuid()` / `setegid()` - Can't set effective IDs

**Estimated Time**: 4-6 hours total (can do in parallel)

**Files to Add/Modify**:
- `kernel/posix.c` - Add these syscalls
- `kernel/sysent.c` - Add table entries
- `userspace/syscalls.c` - Add wrappers

---

### 5️⃣ PRIORITY 5: Fix Signal Delivery Reliability

**Problem**:
- Signals sometimes don't interrupt syscalls
- Signal delivery timing issues
- Multiple signals lost
- Race conditions possible

**Estimated Time**: 2-4 hours

**Files to Modify**:
- `kernel/trap.c` - Signal delivery code
- `kernel/posix_sig.c` - Signal handling
- Need proper signal queue

---

## WORK BREAKDOWN

### Phase 1.5: Critical Fixes (Week 1-2)

```
Day 1-2:  Fix getcwd() path tracking
Day 2-3:  Implement signal masking properly
Day 3-4:  Add sleep() and _exit() syscalls
Day 4-5:  Add symlink/readlink/rename
Day 5:    Testing and bug fixes
```

**Total Effort**: ~40 hours
**Team Size**: 1-2 developers
**Testing**: Use posix_test + manual shell testing

---

### Phase 2: Essential Features (Week 3-6)

#### Batch 1: File System Features
```
- [ ] Implement fcntl file locking (F_GETLK, F_SETLK, F_SETLKW)
- [ ] Implement truncate() / ftruncate()
- [ ] Implement fsync() / fdatasync()
- [ ] Implement utime() / utimes()
- [ ] Expand directory support beyond 16 files
- [ ] Add telldir() / seekdir()
Effort: ~20 hours
```

#### Batch 2: User/Group Functions
```
- [ ] Implement getpwnam() / getpwuid()
- [ ] Implement getgrnam() / getgrgid()
- [ ] Implement getgroups() / setgroups()
- [ ] Implement seteuid() / setegid()
- [ ] Parse /etc/passwd and /etc/group
Effort: ~15 hours
```

#### Batch 3: Terminal I/O (termios) - CRITICAL
```
- [ ] Implement tcgetattr() / tcsetattr()
- [ ] Implement canonical/non-canonical modes
- [ ] Implement tcsendbreak() / tcdrain()
- [ ] Implement tcflush()
- [ ] Implement speed functions (cfgetispeed, etc.)
Effort: ~40 hours
```

**Total Phase 2**: ~75 hours (2-3 weeks for 1 developer)

---

### Phase 3: Advanced Features (Week 7-10)

#### Batch 1: I/O Multiplexing
```
- [ ] Implement select() system call
- [ ] Implement poll() system call
- [ ] Add file descriptor polling infrastructure
Effort: ~25 hours
```

#### Batch 2: Memory Management
```
- [ ] Implement mmap() / munmap()
- [ ] Implement mprotect()
- [ ] Implement msync() / madvise()
Effort: ~40 hours
```

**Total Phase 3**: ~65 hours (2-3 weeks)

---

## Testing Strategy

### Phase 1.5 Testing
```
1. Fix getcwd() → Test with `pwd` command
2. Fix signals → Test with signal programs
3. Add sleep/exit → Test with sleep command
4. Add symlinks → Test with ln -s
5. Integration tests → Run posix_test again
```

### Phase 2 Testing
```
1. File system features → Test with system calls
2. User/group functions → Test getpwuid, getgrnam
3. termios → Test with interactive shell, vi
4. Full integration test
```

### Phase 3 Testing
```
1. select/poll → Test multiplexing programs
2. mmap → Test large file access
3. Full compliance tests
```

---

## Implementation Sequence (Recommended)

### Week 1: Critical Fixes
```
Priority Order (by dependency):
1. Fix getcwd()               ← Many things depend on this
2. Implement _exit()          ← Simple, needed by many
3. Implement sleep()          ← Simple, needed by many
4. Fix signal masking         ← Important for reliability
5. Add symlink/readlink       ← File system feature
6. Add rename()               ← File system feature
7. Add getgroups/setgroups    ← User management
```

### Week 2-3: Essential Features
```
1. Implement termios          ← CRITICAL for shell/vi
2. Expand directory support   ← Fix limitations
3. Add file locking           ← Important for DBs
4. Add user/group DB access   ← Authentication
5. Add select/poll            ← I/O multiplexing
```

### Week 4-6: Polish & Test
```
1. Comprehensive testing
2. Port standard utilities
3. Performance optimization
4. Bug fixes
```

---

## Code Changes Required

### Estimated Lines of Code to Add/Modify

| Feature | New | Modified | Total |
|---------|-----|----------|-------|
| getcwd fix | 30 | 40 | 70 |
| Signal masking | 80 | 60 | 140 |
| sleep/_exit | 40 | 20 | 60 |
| symlink/readlink | 60 | 20 | 80 |
| rename | 40 | 20 | 60 |
| termios | 300+ | 100+ | 400+ |
| File locking | 100 | 50 | 150 |
| User/group DB | 150 | 50 | 200 |
| select/poll | 200 | 100 | 300 |
| mmap | 250 | 100 | 350 |
| **TOTAL** | **1,250+** | **560+** | **1,810+** |

---

## Resource Requirements

### For Phase 1.5 (Critical Fixes)
- **Team**: 1-2 developers
- **Time**: 2 weeks
- **Tools**: Cross-compiler, QEMU, debugger
- **Testing**: Manual + automated

### For Phase 2 (Essential)
- **Team**: 1-2 developers
- **Time**: 4 weeks
- **Tools**: Same as above

### For Phase 3 (Advanced)
- **Team**: 1-2 developers
- **Time**: 3-4 weeks
- **Tools**: Same as above

---

## Success Criteria

### Phase 1.5 Complete When:
- ✅ `pwd` command works correctly
- ✅ Signal masking works reliably
- ✅ `sleep 5` and `_exit` work
- ✅ Symlinks can be created and used
- ✅ Files can be renamed
- ✅ `posix_test` passes all tests
- ✅ Basic shell operations work

### Phase 2 Complete When:
- ✅ Interactive shell works (sh with termios)
- ✅ `vi` text editor works
- ✅ File locking functions
- ✅ User authentication works
- ✅ Multiple files can be in directories
- ✅ select()/poll() multiplexing works

### Phase 3 Complete When:
- ✅ mmap works for large files
- ✅ 80% POSIX compliance achieved
- ✅ Standard utilities work (cp, mv, rm, etc.)
- ✅ Complex scripts work

---

## Quick Start for Phase 1.5

To begin fixing the critical issues:

```bash
# 1. Create a branch for Phase 1.5 fixes
git checkout -b posix-phase-1.5-fixes

# 2. Start with getcwd() fix
# File: kernel/posix.c, function: sys_getcwd()
# - Add directory tracking to proc structure
# - Implement path building from inode chain

# 3. Test after each fix
cd /workspaces/unix-v6
make clean && make iso
qemu-system-i386 -m 256 -cdrom kernel/unix_v6.iso

# 4. Commit incrementally
git add -A
git commit -m "Fix getcwd() to return actual path"
```

---

## Documentation Needed

- [ ] Update POSIX_STATUS_OVERVIEW.md with progress
- [ ] Create PHASE_1.5_FIXES.md with details
- [ ] Document each fix with before/after code
- [ ] Add test cases for each fix

---

## Risk Assessment

### Risks to Consider

1. **Regression Risk**: Changes to signal handling could break other syscalls
   - Mitigation: Comprehensive testing, backup original code

2. **Timing Risk**: Signal masking changes could affect timing
   - Mitigation: Test under load, measure performance

3. **Compatibility Risk**: Changes to syscall table could break existing code
   - Mitigation: Use free syscall slots, test thoroughly

### Assumptions

1. Kernel compilation works correctly ✅
2. Cross-compiler is available ✅
3. QEMU emulator available ✅
4. Current code is stable baseline ✅

---

## Timeline Summary

```
Week 1:    Phase 1.5 - Critical Fixes (getcwd, signals, sleep, _exit, symlinks)
           Estimated: 40-50 hours
           
Week 2-3:  Phase 2.1 - File System Features (truncate, fsync, termios prep)
           Estimated: 50-60 hours
           
Week 4-5:  Phase 2.2 - Terminal I/O (termios, canonical modes)
           Estimated: 40-50 hours
           
Week 6-8:  Phase 3 - I/O Multiplexing (select, poll)
           Estimated: 40-50 hours
           
Week 9-10: Phase 3 - Memory Management (mmap, mprotect)
           Estimated: 40-50 hours
           
Total:     ~250+ hours = 6-8 weeks for ~80% POSIX compliance
```

---

## Key Milestones

- [ ] **Milestone 1 (Week 1)**: All Phase 1.5 critical fixes working
- [ ] **Milestone 2 (Week 3)**: Interactive shell with termios
- [ ] **Milestone 3 (Week 5)**: Text editor (vi) working
- [ ] **Milestone 4 (Week 8)**: I/O multiplexing operational
- [ ] **Milestone 5 (Week 10)**: 80% POSIX compliance, standard utils working

---

**Ready to start? Begin with Phase 1.5 critical fixes in Week 1!**

For detailed implementation instructions, see individual fix guides in subsequent commits.
