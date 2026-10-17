/* prof2.c - the reading cost split up, on the Amiga: the index file read
 * whole into memory, then decoded in steps, each timed.  prof2 INDEX */
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
static ULONG ticks(void) { struct DateStamp d; DateStamp(&d); return (ULONG)d.ds_Minute * 3000 + d.ds_Tick; }
static UBYTE path[1100];
#define GETVAR(v) do { ULONG r_ = 0; int sh_ = 0; UBYTE c_; do { c_ = *q++; r_ |= (ULONG)(c_ & 0x7F) << sh_; sh_ += 7; } while (c_ & 0x80); (v) = r_; } while (0)
int main(int argc, char **argv)
{
    BPTR f = Open(argv[1], MODE_OLDFILE);
    LONG size, n, step, plen = 0, sum = 0;
    UBYTE *buf, *q, *end, *hdr;
    ULONG t0;
    Seek(f, 0, OFFSET_END); size = Seek(f, 0, OFFSET_BEGINNING);
    buf = AllocVec(size, MEMF_ANY); Read(f, buf, size); Close(f);
    hdr = buf + 18; hdr += 1 + hdr[0] + 4;             /* skip the header (one volume) */
    { UWORD nv = (buf[16] << 8) | buf[17]; LONG i; hdr = buf + 18; for (i = 0; i < nv; i++) hdr += 1 + hdr[0] + 4; }
    end = buf + size;
    for (step = 0; step < 4; step++) {
        n = 0; plen = 0; q = hdr; t0 = ticks();
        while (q < end) {
            ULONG sh, rest, s, d;
            GETVAR(sh); GETVAR(rest);
            if (step >= 1) { UBYTE *dd = path + sh; LONG k = rest; while (k--) *dd++ = *q++; plen = sh + rest; }
            else q += rest;
            q += 2;
            GETVAR(s); GETVAR(d);
            if (step >= 2) { LONG i = plen; while (i > 0 && path[i - 1] != '/' && path[i - 1] != ':') i--; sum += i; }
            if (step >= 3) { LONG i; for (i = 0; i < plen; i++) sum += path[i]; }
            n++;
        }
        printf("step %ld (%s): %ld entries, %lu ms\n", (long)step,
               step == 0 ? "numbers only" : step == 1 ? "+ copy the new bytes" : step == 2 ? "+ find the name" : "+ touch every byte of the path",
               (long)n, (ticks() - t0) * 20);
    }
    printf("(%ld)\n", (long)sum);
    return 0;
}
