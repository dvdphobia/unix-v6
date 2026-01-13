/* types.h - Unix V6 x86 Type Definitions
 * 
 * Based on original V6 types, adapted for 32-bit x86
 */

#ifndef _UNIX_TYPES_H
#define _UNIX_TYPES_H

/* Standard integer types */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef signed short        int16_t;
typedef unsigned short      uint16_t;
typedef signed int          int32_t;
typedef unsigned int        uint32_t;
typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;

/* Size types */
typedef uint32_t            size_t;
typedef int32_t             ssize_t;
typedef int32_t             off_t;

/* Unix V6 types */
typedef int32_t             daddr_t;    /* Disk address */
typedef int32_t             blkno_t;    /* Block number */
typedef uint32_t            ino_t;      /* Inode number */
typedef uint32_t            time_t;     /* Time in seconds */
typedef int16_t             dev_t;      /* Device number */
typedef uint16_t            uid_t;      /* User ID */
typedef uint16_t            gid_t;      /* Group ID */
typedef uint16_t            mode_t;     /* File mode */
typedef uint16_t            nlink_t;    /* Link count */

/* Macros for device numbers */
#define major(x)    ((int)(((unsigned)(x) >> 8) & 0xFF))
#define minor(x)    ((int)((x) & 0xFF))
#define makedev(x,y) ((dev_t)(((x) << 8) | (y)))

/* Null pointer */
#ifndef NULL
#define NULL    ((void *)0)
#endif

/* Boolean */
#define TRUE    1
#define FALSE   0

#endif /* _UNIX_TYPES_H */
