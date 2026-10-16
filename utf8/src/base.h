/* base.h - utf8.library's base. The library keeps nothing between calls;
 * the lock is for opening rexxsyslib.library once. */
#ifndef U8_BASE_H
#define U8_BASE_H

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

struct Utf8LibBase {
    struct Library lib;
    BPTR seglist;
    struct SignalSemaphore lock;
};

void rexx_close(void);

#endif
