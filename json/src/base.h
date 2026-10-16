/* base.h - json.library's base: the Library, and what ARexx needs to
 * keep between calls (the documents' handles, the last error). */
#ifndef JSON_BASE_H
#define JSON_BASE_H

#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

struct JsonBase {
    struct Library lib;
    BPTR seglist;
    struct SignalSemaphore lock;    /* the handles and opening rexxsyslib */
    struct MinList handles;         /* ARexx's documents */
    ULONG nextid;
    char lasterr[96];               /* JSONERROR() */
};

void rexx_freeall(struct JsonBase *b);

#endif
