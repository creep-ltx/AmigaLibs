/* just enough AmigaOS for compiling tui core.c on Linux (a test harness) */
#ifndef AMISHIM_H
#define AMISHIM_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
typedef uint32_t ULONG; typedef int32_t LONG; typedef uint16_t UWORD; typedef int16_t WORD;
typedef uint8_t UBYTE; typedef int8_t BYTE; typedef void *APTR; typedef long BPTR;
typedef unsigned char *STRPTR; typedef const unsigned char *CONST_STRPTR; typedef short BOOL;
#define VOID void
#define CONST const
#ifndef NULL
#define NULL ((void*)0)
#endif
#define TRUE 1
#define FALSE 0
struct Library { int lib_Version, lib_Revision; };
struct MsgPort { int mp_SigBit; };
struct Message { int x; };
struct DosPacket { LONG dp_Type, dp_Res1, dp_Res2, dp_Arg1, dp_Arg2, dp_Arg3; };
struct FileHandle { struct MsgPort *fh_Type; };
struct InfoData { ULONG id_DiskType; };
#define BADDR(x) ((void *)(x))
#define MKBADDR(x) ((BPTR)(x))
#define MEMF_ANY 0
#define MEMF_PUBLIC 1
#define MEMF_CLEAR 0x10000
#define ACTION_DISK_INFO 25
#define ACTION_WAIT_CHAR 20
#define DOS_STDPKT 0
#define SIGBREAKF_CTRL_C (1<<12)
#define SIGBREAKF_CTRL_D (1<<13)
#define SIGBREAKF_CTRL_E (1<<14)
#define SIGBREAKF_CTRL_F (1<<15)
APTR AllocVec(ULONG, ULONG); void FreeVec(APTR);
LONG Write(BPTR, const void *, LONG); LONG Read(BPTR, void *, LONG);
LONG SetMode(BPTR, LONG); BPTR Input(void); BPTR Output(void);
ULONG Wait(ULONG); LONG WaitForChar(BPTR, LONG); ULONG SetSignal(ULONG, ULONG);
LONG IsInteractive(BPTR); void FreeDosObject(ULONG, APTR); APTR AllocDosObject(ULONG, APTR);
void DeleteMsgPort(struct MsgPort *); struct MsgPort *CreateMsgPort(void);
void CopyMem(const void *, void *, ULONG); void CloseLibrary(struct Library *);
struct Library *OpenLibrary(const char *, ULONG); struct Message *WaitPort(struct MsgPort *);
struct Message *GetMsg(struct MsgPort *); void SendPkt(struct DosPacket *, struct MsgPort *, struct MsgPort *);
void Forbid(void); void Permit(void); LONG DoPkt(struct MsgPort *, LONG, LONG, LONG, LONG, LONG, LONG);
void CurrentTime(ULONG *, ULONG *);
#endif
