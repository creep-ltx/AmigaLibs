/* base.h - fuzzy.library's base: the Library, and what ARexx needs
 * between calls (the last error, rexxsyslib). */
#ifndef FUZZY_BASE_H
#define FUZZY_BASE_H

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

struct FuzzyBase {
    struct Library lib;
    BPTR seglist;
    struct SignalSemaphore lock;    /* opening rexxsyslib */
    char lasterr[96];               /* FUZZYERROR() */
};

void rexx_close(struct FuzzyBase *b);

#endif
