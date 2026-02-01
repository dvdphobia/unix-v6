# 🎯 POSIX Implementation - Next Steps (Prioritized)

## Current Status
- ✅ **Phases 1.5-2 COMPLETE**: 50% POSIX compliance achieved
- ✅ Kernel boots successfully
- ✅ Basic system operations work
- ❌ Still missing critical features for production use

---

## Phase 3: Critical Fixes (1-2 weeks)
**Why**: These block many programs from running

### 1. Fix getcwd() Path Tracking 🔴 **HIGHEST PRIORITY**
**Status**: getcwd() returns "/" always  
**Issue**: Current directory not properly tracked  
**Impact**: `pwd` command broken, many programs fail  
**Effort**: 4-8 hours

```c
// Current: Always returns "/"
// Needed: Walk inode chain to build actual path
// Example: /home/user/projects → should return this path, not "/"
```

**Tasks**:
- [ ] Track actual current directory in inode chain
- [ ] Walk parent inodes to build full path
- [ ] Store path in user structure
- [ ] Test with pwd command

---

### 2. Implement sleep() and _exit() 🔴 **CRITICAL**
**Status**: Missing or incomplete  
**Impact**: Programs can't sleep or exit cleanly  
**Effort**: 4-6 hours

```c
// Need these syscalls:
int sleep(unsigned int seconds);           // Entry 80
void _exit(int status);                    // Entry 81
```

**Tasks**:
- [ ] Implement sleep() using alarm mechanism
- [ ] Implement _exit() for immediate termination
- [ ] Add to syscall table
- [ ] Test with sleep command

---

### 3. Signal Masking Fix 🔴 **CRITICAL**
**Status**: Partially implemented but unreliable  
**Issue**: Signals may not be properly blocked  
**Impact**: Signal handling unreliable  
**Effort**: 6-10 hours

**Tasks**:
- [ ] Ensure sigprocmask() correctly blocks signals
- [ ] Fix signal delivery to respect mask
- [ ] Test Ctrl+C masking/unmasking
- [ ] Verify signal interruption behavior

---

### 4. File System Essentials 🟡 **IMPORTANT**
**Status**: Missing truncate, utime, fsync  
**Effort**: 8-12 hours

```c
int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);
int fsync(int fd);
int utime(const char *path, const struct utimbuf *times);
```

**Tasks**:
- [ ] Implement truncate/ftruncate
- [ ] Implement fsync (at minimum, make it no-op)
- [ ] Implement utime for timestamps
- [ ] Add 4 new syscalls to table

---

## Phase 3 (Continued): Enhanced Features (2-4 weeks)

### 5. Terminal I/O Control (termios) 🟡 **HIGH PRIORITY**
**Status**: Not implemented  
**Impact**: Shell can't control terminal, vi/nano won't work  
**Effort**: 1-2 weeks

```c
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
int tcsendbreak(int fd, int duration);
int cfgetispeed(const struct termios *termios_p);
int cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
```

**Tasks**:
- [ ] Define termios structure
- [ ] Implement tcgetattr/tcsetattr
- [ ] Support canonical/non-canonical modes
- [ ] Implement flow control
- [ ] Test with shell input handling

---

### 6. I/O Multiplexing (select/poll) 🟡 **IMPORTANT**
**Status**: Not implemented  
**Impact**: Can't handle multiple I/O streams  
**Effort**: 1-2 weeks

```c
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

**Tasks**:
- [ ] Implement select with fd_set
- [ ] Implement poll alternative
- [ ] Add timeout support
- [ ] Test multiplexing capability

---

### 7. User/Group Database Functions 🟡 **IMPORTANT**
**Status**: Not implemented  
**Impact**: Can't authenticate users, permissions limited  
**Effort**: 1-2 weeks

```c
struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);
struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);
int getgroups(int size, gid_t list[]);
int setgroups(size_t size, const gid_t *list[]);
```

**Tasks**:
- [ ] Parse /etc/passwd file
- [ ] Parse /etc/group file
- [ ] Implement getpw* functions
- [ ] Implement getgr* functions
- [ ] Implement group functions

---

### 8. Enhanced fcntl() - File Locking 🟡 **USEFUL**
**Status**: Partially implemented (no locking)  
**Effort**: 1 week

```c
int fcntl(int fd, int cmd, ...);
// Missing: F_GETLK, F_SETLK, F_SETLKW (advisory locking)
```

**Tasks**:
- [ ] Add struct flock definition
- [ ] Implement F_GETLK (get lock info)
- [ ] Implement F_SETLK (set non-blocking lock)
- [ ] Implement F_SETLKW (set blocking lock)

---

## Phase 4: Advanced Features (3-6 months)

### 9. Memory Management (mmap, etc.)
- mmap() / munmap() - Memory mapping
- mprotect() - Change protection
- madvise() - Memory hints
**Effort**: 2-3 weeks

---

### 10. IPC Mechanisms
- Message queues
- Semaphores  
- Shared memory
**Effort**: 3-4 weeks

---

### 11. Networking (Socket API)
- socket(), bind(), listen(), accept()
- TCP/IP stack integration
- DNS resolution
**Effort**: 4-6 weeks

---

### 12. Threading (pthreads)
- pthread_create/join
- Mutexes, condition variables
- Thread-local storage
**Effort**: 6-8 weeks

---

## Recommended Development Order

### Week 1-2: Critical Fixes
1. ✅ Fix structure size overflow (DONE)
2. ⏳ Fix getcwd() path tracking
3. ⏳ Implement sleep() / _exit()
4. ⏳ Fix signal masking reliability

### Week 3-4: Core Functionality
5. ⏳ Implement truncate/ftruncate/fsync/utime
6. ⏳ Add advisory file locking (fcntl)
7. ⏳ Implement user/group database

### Week 5-8: Terminal & I/O
8. ⏳ Implement termios (terminal control)
9. ⏳ Implement select/poll (I/O multiplexing)

### Later: Advanced Features
10. mmap and memory management
11. IPC mechanisms
12. Networking
13. Threading

---

## Quick Assessment Matrix

| Feature | Priority | Effort | Impact | Users Blocked |
|---------|----------|--------|--------|-----------------|
| getcwd() fix | 🔴 | 4-8h | Shell basics | All pwd users |
| sleep()/_exit() | 🔴 | 4-6h | Many programs | Most background tasks |
| Signal masking | 🔴 | 6-10h | Reliability | Signal handlers |
| File truncate/fsync | 🟡 | 8-12h | File ops | Editor users |
| termios | 🟡 | 1-2w | Terminal ops | vi/nano, shell |
| select/poll | 🟡 | 1-2w | I/O | Servers, shells |
| User/group DB | 🟡 | 1-2w | Auth | Multi-user systems |
| fcntl locking | 🟡 | 1w | File safety | Database apps |

---

## Success Criteria for Each Phase

### Phase 3 Complete (4 weeks):
- ✅ getcwd() returns actual paths
- ✅ sleep() and _exit() work
- ✅ Signal masking reliable
- ✅ File operations (truncate, fsync)
- ✅ termios for terminal control
- ✅ select/poll for I/O
- ✅ User/group functions

**Result**: ~75% POSIX compliance, shell fully functional

### Phase 4 Complete (3-6 months):
- ✅ Memory mapping (mmap)
- ✅ IPC (queues, semaphores)
- ✅ Basic networking
- ✅ Threading support

**Result**: ~90% POSIX compliance, production-ready

---

## Recommended Next Task

**START HERE**: Fix getcwd() path tracking

This will unblock:
- ✅ pwd command  
- ✅ Relative path operations
- ✅ Many shell features
- ✅ File operation reliability

Estimated time: 4-8 hours  
Impact: Very high (shell usability)

---

## Quick Links to Related Docs
- [INCOMPLETE_FEATURES.md](INCOMPLETE_FEATURES.md) - Detailed feature list
- [PHASE_1_5_ACTION_PLAN.md](PHASE_1_5_ACTION_PLAN.md) - Phase 1.5 details
- [PHASE_2_COMPLETE.md](PHASE_2_COMPLETE.md) - Phase 2 details
