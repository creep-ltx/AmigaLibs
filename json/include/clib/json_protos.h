/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef CLIB_JSON_PROTOS_H
#define CLIB_JSON_PROTOS_H

/*
**   $VER: json_protos.h 0.1 $Id: json.sfd 0.1 $
**
**   C prototypes. For use with 32 bit integers only.
**
**   Copyright (c) 2001 Amiga, Inc.
**       All Rights Reserved
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/json.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

JsonDoc * JsonParse(CONST_STRPTR text, LONG len, ULONG flags, struct JsonError * err);
JsonDoc * JsonLoad(CONST_STRPTR name, ULONG flags, struct JsonError * err);
VOID JsonFree(JsonDoc * doc);
JsonNode * JsonRoot(JsonDoc * doc);
LONG JsonType(JsonNode * node);
JsonNode * JsonGet(JsonNode * object, CONST_STRPTR name);
JsonNode * JsonAt(JsonNode * array, LONG index);
JsonNode * JsonFind(JsonNode * node, CONST_STRPTR path);
LONG JsonCount(JsonNode * node);
JsonNode * JsonFirst(JsonNode * node);
JsonNode * JsonNext(JsonNode * node);
JsonNode * JsonParent(JsonNode * node);
CONST_STRPTR JsonName(JsonNode * node);
CONST_STRPTR JsonString(JsonNode * node);
LONG JsonLength(JsonNode * node);
CONST_STRPTR JsonNumText(JsonNode * node);
LONG JsonInt(JsonNode * node, LONG def);
LONG JsonScaled(JsonNode * node, LONG places, LONG def);
LONG JsonBool(JsonNode * node, LONG def);
LONG JsonLatin1(JsonNode * node, STRPTR buf, LONG size);
JsonDoc * JsonNewDoc(void);
VOID JsonSetRoot(JsonDoc * doc, JsonNode * node);
JsonNode * JsonNewObject(JsonDoc * doc);
JsonNode * JsonNewArray(JsonDoc * doc);
JsonNode * JsonNewString(JsonDoc * doc, CONST_STRPTR s, LONG len, ULONG flags);
JsonNode * JsonNewInt(JsonDoc * doc, LONG value);
JsonNode * JsonNewNumber(JsonDoc * doc, CONST_STRPTR text);
JsonNode * JsonNewBool(JsonDoc * doc, LONG value);
JsonNode * JsonNewNull(JsonDoc * doc);
LONG JsonAdd(JsonNode * parent, CONST_STRPTR name, JsonNode * node);
VOID JsonRemove(JsonNode * node);
LONG JsonWrite(JsonNode * node, STRPTR buf, LONG size, ULONG flags);
LONG JsonWriteFH(JsonNode * node, BPTR fh, ULONG flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_JSON_PROTOS_H */
