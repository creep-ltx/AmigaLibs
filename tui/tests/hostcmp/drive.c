/* drive.c - drives tui core.c's drawing half on Linux with random screens
 * and prints a checksum of everything it sent (run.sh compares two core.c's).
 *   drive CTERM UTF8 MODE ROUNDS [DUMPFILE]   MODE 0 few colours, 1 many, 2 + full 24-bit screens */
#include CORE
#include <stdio.h>
static unsigned long long allocated, peak;
APTR AllocVec(ULONG n, ULONG f){ size_t *q = calloc(1, n + 16); q[0]=n; allocated+=n; if(allocated>peak)peak=allocated; return q+2; }
void FreeVec(APTR a){ size_t *q=(size_t*)a-2; allocated-=q[0]; free(q); }
static unsigned long long hsum=1469598103934665603ULL, total;
static FILE *dump;
LONG Write(BPTR f, const void *b, LONG n){ const unsigned char *c=b; for(LONG i=0;i<n;i++){hsum^=c[i];hsum*=1099511628211ULL;} total+=n; if(dump)fwrite(b,1,n,dump); return n; }
LONG Read(BPTR f, void *b, LONG n){return 0;} LONG SetMode(BPTR f, LONG m){return 1;}
BPTR Input(void){return 0;} BPTR Output(void){return 1;}
ULONG Wait(ULONG s){return s;} LONG WaitForChar(BPTR f, LONG t){return 0;} ULONG SetSignal(ULONG a, ULONG b){return 0;}
LONG IsInteractive(BPTR f){return 1;} void FreeDosObject(ULONG t, APTR a){} APTR AllocDosObject(ULONG t, APTR a){return 0;}
void DeleteMsgPort(struct MsgPort *p){} struct MsgPort *CreateMsgPort(void){return 0;}
void CopyMem(const void *a, void *b, ULONG n){memcpy(b,a,n);} void CloseLibrary(struct Library *l){}
struct Library *OpenLibrary(const char *n, ULONG v){return 0;} struct Message *WaitPort(struct MsgPort *p){return 0;}
struct Message *GetMsg(struct MsgPort *p){return 0;} void SendPkt(struct DosPacket *d, struct MsgPort *a, struct MsgPort *b){}
void Forbid(void){} void Permit(void){} LONG DoPkt(struct MsgPort *p, LONG a, LONG b, LONG c, LONG d, LONG e, LONG f){return 0;}
void CurrentTime(ULONG *a, ULONG *b){*a=*b=0;}
static unsigned rs=12345; static unsigned rnd(void){ rs = rs*1103515245u+12345u; return rs>>8; }
static LONG colour(int mode){
    switch (rnd()%(mode?6:4)) { case 0: return rnd()%17; case 1: return TC_X(rnd()%256); case 2: return TC_PEN(rnd()%8);
    case 3: return TC_RGB(rnd()%4*85, rnd()%3*127, rnd()%2*255);   /* a few */
    default: return TC_RGB(rnd()%256, rnd()%256, rnd()%256); }      /* many */
}
int main(int argc, char **argv){
    int cterm = atoi(argv[1]), utf8 = atoi(argv[2]), mode = atoi(argv[3]), rounds = atoi(argv[4]);
    Priv *p; int i;
    if (argc > 5) dump = fopen(argv[5], "wb");
    rs = 1000 + cterm*7 + utf8*3 + mode;
    p = AllocVec(sizeof(*p), MEMF_CLEAR);
    p->pub.Cols = 80; p->pub.Rows = 32; p->pub.CTerm = cterm; p->pub.UTF8 = utf8; p->tout = 1;
    forget(p); gridalloc(p); TuiRedraw(&p->pub);
    static const char *u8s[] = { "\xe4\xb8\xad\xe6\x96\x87 wide", "e\xcc\x81 accent", "\xc5\x81\xc3\xb3" "d\xc5\xba", "\xf0\x9f\x98\x80 emoji \xe2\x94\x80\xe2\x94\x82", "plain \xe5\xe4\xf6" };
    for (i = 0; i < rounds; i++) {
        int k, n = 1 + rnd()%40;
        for (k = 0; k < n; k++) {
            int op = rnd()%8, col = (int)(rnd()%90) - 5, row = (int)(rnd()%34) - 1;
            LONG fg = colour(mode), bg = colour(mode), at = rnd()%64;
            char buf[64]; int j, len = rnd()%60;
            for (j = 0; j < len; j++) buf[j] = (char)(32 + rnd()%224);
            switch (op) {
            case 0: case 1: TuiPut(&p->pub, col, row, (CONST_STRPTR)buf, len, fg, bg, at); break;
            case 2: TuiFill(&p->pub, col, row, rnd()%90, rnd()%3 ? ' ' + rnd()%95 : 0x2588, fg, bg, at); break;
            case 3: TuiPutC(&p->pub, col, row, rnd()%2 ? 0x4E2D + rnd()%100 : rnd()%0x3000, fg, bg, at); break;
            case 4: TuiPutUTF8(&p->pub, col, row, (CONST_STRPTR)u8s[rnd()%5], -1, fg, bg, at); break;
            case 5: if (rnd()%20 == 0) TuiClear(&p->pub); else TuiDirty(&p->pub, col, row, rnd()%10, rnd()%5); break;
            case 6: if (mode == 2) { int r, c; for (r = 0; r < 32; r++) for (c = 0; c < 80; c++) TuiPutC(&p->pub, c, r, 'a' + (r + c + i) % 26, TC_RGB(c * 3, r * 7, i * 13), TC_DEFAULT, 0); } break;
            case 7: TuiCursor(&p->pub, col, row, rnd()%2); if (rnd()%30 == 0) TuiRedraw(&p->pub); break;
            }
        }
        TuiFlush(&p->pub);
    }
    printf("cterm %d utf8 %d mode %d: %llu bytes sent, sum %016llx, peak %llu bytes\n", cterm, utf8, mode, total, hsum, peak);
    return 0;
}
