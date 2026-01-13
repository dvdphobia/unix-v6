/* types.h - Unix V6 x86 Port Type Definitions
 * Modernized for 32-bit x86 architecture
 * Original: No equivalent (types were implicit in V6)
 */

#ifndef _TYPES_H_
#define _TYPES_H_

/* Standard integer types for x86 */
typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
typedef unsigned long long  uint64_t;
typedef signed long long    int64_t;

/* Size and pointer types */
typedef uint32_t            size_t;
typedef int32_t             ssize_t;
typedef int32_t             off_t;
typedef uint32_t            uintptr_t;
typedef int32_t             intptr_t;

/* Unix V6 compatible types */
typedef uint16_t            dev_t;      /* Device number (major/minor) */
typedef uint16_t            ino_t;      /* Inode number */
typedef uint16_t            mode_t;     /* File mode */
typedef uint16_t            nlink_t;    /* Link count */
typedef uint16_t            uid_t;      /* User ID */
typedef uint16_t            gid_t;      /* Group ID */
typedef int32_t             time_t;     /* Time in seconds */
typedef int32_t             blkno_t;    /* Block number */
typedef int32_t             daddr_t;    /* Disk address */
typedef int16_t             pid_t;      /* Process ID */

/* Pointer type that matches PDP-11's 16-bit pointers mapped to 32-bit */
typedef void*               caddr_t;    /* Core address */

/* Boolean type */
typedef int                 bool_t;
#define TRUE    1
#define FALSE   0

/* NULL pointer */
#ifndef NULL
#define NULL    ((void*)0)
#endif

/* Device number macros */
#define major(x)    ((uint16_t)(((x) >> 8) & 0xFF))
#define minor(x)    ((uint16_t)((x) & 0xFF))
#define makedev(maj, min)   ((dev_t)(((maj) << 8) | ((min) & 0xFF)))

/* Structure to access bytes in an integer (V6 compatibility) */
struct lobyte_hibyte {
    uint8_t lobyte;
    uint8_t hibyte;
};

/* Structure to access integer */
struct integ {
    int32_t integ;
};

#endif /* _TYPES_H_ */
