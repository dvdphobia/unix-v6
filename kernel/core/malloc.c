/* malloc.c - Unix V6 x86 Port Memory Allocation
 * Ported from original V6 ken/malloc.c for PDP-11
 * Manages core map and swap map allocation
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/systm.h"

struct map {
    int m_size;
    int m_addr;
};

/*
 * malloc - Allocate space in map
 *
 * mp: map structure
 * size: size to allocate
 *
 * Returns positive base address/index if successful,
 * 0 if no space.
 */
uint32_t malloc(uint32_t *pmap, int size) {
    struct map *mp = (struct map *)pmap;
    register int a;
    register struct map *bp;

    for (bp = mp; bp->m_size; bp++) {
        if (bp->m_size >= size) {
            a = bp->m_addr;
            bp->m_addr += size;
            if ((bp->m_size -= size) == 0) {
                do {
                    bp++;
                    (bp-1)->m_addr = bp->m_addr;
                } while (((bp-1)->m_size = bp->m_size));
            }
            return((uint32_t)a);
        }
    }
    return(0);
}

/*
 * mfree - Free space into map
 *
 * mp: map structure
 * size: size to free
 * addr: base address/index
 */
void mfree(uint32_t *pmap, int size, uint32_t addr) {
    struct map *mp = (struct map *)pmap;
    register struct map *bp;
    register int t;

    bp = mp;
    for (; bp->m_addr <= addr && bp->m_size != 0; bp++);
    
    if (bp > mp && (bp-1)->m_addr + (bp-1)->m_size == addr) {
        (bp-1)->m_size += size;
        if (addr + size == bp->m_addr) {
            (bp-1)->m_size += bp->m_size;
            while (bp->m_size) {
                bp++;
                (bp-1)->m_addr = bp->m_addr;
                (bp-1)->m_size = bp->m_size;
            }
        }
    } else {
        if (addr + size == bp->m_addr && bp->m_size) {
            bp->m_addr -= size;
            bp->m_size += size;
        } else if (size) {
            do {
                t = bp->m_addr;
                bp->m_addr = addr;
                addr = t;
                t = bp->m_size;
                bp->m_size = size;
                bp++;
            } while ((size = t));
        }
    }
}
