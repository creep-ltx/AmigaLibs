/* port.h - what json.c needs from the system: memory and files. On the
 * Amiga exec and dos; on the PC (tests, fuzzing) the C library, so the
 * same json.c is tested there. */
#ifndef JSON_PORT_H
#define JSON_PORT_H

#ifdef __amigaos__
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/json.h>
#define jalloc(n)  AllocVec((n), MEMF_ANY)
#define jfree(p)   FreeVec(p)
#ifdef JSON_LIB
/* the library has no C library: lib.c has these */
void *memcpy(void *, const void *, unsigned long);
void *memset(void *, int, unsigned long);
int memcmp(const void *, const void *, unsigned long);
unsigned long strlen(const char *);
#else
#include <string.h>
#endif

#else /* the PC */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint16_t UWORD;
typedef uint8_t UBYTE;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
typedef void *APTR;
typedef FILE *BPTR;
#define VOID void
#define TRUE 1
#define FALSE 0
#define jalloc(n)  malloc(n)
#define jfree(p)   free(p)
/* the library's own header: its exec/types.h and dos/dos.h are empty
 * stand-ins in tests/pcinc */
#include "../include/libraries/json.h"
#endif

/* json.c's calls (the library's, and a program's that builds it in) */
JsonDoc *JsonParse(CONST_STRPTR text, LONG len, ULONG flags, struct JsonError *err);
JsonDoc *JsonLoad(CONST_STRPTR name, ULONG flags, struct JsonError *err);
void JsonFree(JsonDoc *doc);
JsonNode *JsonRoot(JsonDoc *doc);
LONG JsonType(JsonNode *n);
JsonNode *JsonGet(JsonNode *o, CONST_STRPTR name);
JsonNode *JsonAt(JsonNode *a, LONG i);
JsonNode *JsonFind(JsonNode *n, CONST_STRPTR path);
LONG JsonCount(JsonNode *n);
JsonNode *JsonFirst(JsonNode *n);
JsonNode *JsonNext(JsonNode *n);
JsonNode *JsonParent(JsonNode *n);
CONST_STRPTR JsonName(JsonNode *n);
CONST_STRPTR JsonString(JsonNode *n);
LONG JsonLength(JsonNode *n);
CONST_STRPTR JsonNumText(JsonNode *n);
LONG JsonInt(JsonNode *n, LONG def);
LONG JsonScaled(JsonNode *n, LONG places, LONG def);
LONG JsonBool(JsonNode *n, LONG def);
LONG JsonLatin1(JsonNode *n, STRPTR buf, LONG size);
JsonDoc *JsonNewDoc(void);
void JsonSetRoot(JsonDoc *doc, JsonNode *n);
JsonNode *JsonNewObject(JsonDoc *doc);
JsonNode *JsonNewArray(JsonDoc *doc);
JsonNode *JsonNewString(JsonDoc *doc, CONST_STRPTR s, LONG len, ULONG flags);
JsonNode *JsonNewInt(JsonDoc *doc, LONG v);
JsonNode *JsonNewNumber(JsonDoc *doc, CONST_STRPTR text);
JsonNode *JsonNewBool(JsonDoc *doc, LONG v);
JsonNode *JsonNewNull(JsonDoc *doc);
LONG JsonAdd(JsonNode *parent, CONST_STRPTR name, JsonNode *n);
void JsonRemove(JsonNode *n);
LONG JsonWrite(JsonNode *n, STRPTR buf, LONG size, ULONG flags);
LONG JsonWriteFH(JsonNode *n, BPTR fh, ULONG flags);

/* for rexx.c */
LONG json_tolatin1(const UBYTE *s, LONG len, STRPTR buf, LONG size);
JsonNode *json_parse_into(JsonDoc *d, const UBYTE *text, LONG len, ULONG flags, struct JsonError *err);
void json_replace(JsonNode *old, JsonNode *n);
UBYTE *json_latin1_to_u8(const UBYTE *s, LONG len, LONG *outlen);
LONG json_isnumber(const UBYTE *s, LONG len);

#endif
