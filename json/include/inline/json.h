/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef _INLINE_JSON_H
#define _INLINE_JSON_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef JSON_BASE_NAME
#define JSON_BASE_NAME JsonBase
#endif /* !JSON_BASE_NAME */

#define JsonParse(___text, ___len, ___flags, ___err) \
      LP4(0x24, JsonDoc *, JsonParse , CONST_STRPTR, ___text, a0, LONG, ___len, d0, ULONG, ___flags, d1, struct JsonError *, ___err, a1,\
      , JSON_BASE_NAME)

#define JsonLoad(___name, ___flags, ___err) \
      LP3(0x2a, JsonDoc *, JsonLoad , CONST_STRPTR, ___name, a0, ULONG, ___flags, d0, struct JsonError *, ___err, a1,\
      , JSON_BASE_NAME)

#define JsonFree(___doc) \
      LP1NR(0x30, JsonFree , JsonDoc *, ___doc, a0,\
      , JSON_BASE_NAME)

#define JsonRoot(___doc) \
      LP1(0x36, JsonNode *, JsonRoot , JsonDoc *, ___doc, a0,\
      , JSON_BASE_NAME)

#define JsonType(___node) \
      LP1(0x3c, LONG, JsonType , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonGet(___object, ___name) \
      LP2(0x42, JsonNode *, JsonGet , JsonNode *, ___object, a0, CONST_STRPTR, ___name, a1,\
      , JSON_BASE_NAME)

#define JsonAt(___array, ___index) \
      LP2(0x48, JsonNode *, JsonAt , JsonNode *, ___array, a0, LONG, ___index, d0,\
      , JSON_BASE_NAME)

#define JsonFind(___node, ___path) \
      LP2(0x4e, JsonNode *, JsonFind , JsonNode *, ___node, a0, CONST_STRPTR, ___path, a1,\
      , JSON_BASE_NAME)

#define JsonCount(___node) \
      LP1(0x54, LONG, JsonCount , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonFirst(___node) \
      LP1(0x5a, JsonNode *, JsonFirst , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonNext(___node) \
      LP1(0x60, JsonNode *, JsonNext , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonParent(___node) \
      LP1(0x66, JsonNode *, JsonParent , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonName(___node) \
      LP1(0x6c, CONST_STRPTR, JsonName , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonString(___node) \
      LP1(0x72, CONST_STRPTR, JsonString , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonLength(___node) \
      LP1(0x78, LONG, JsonLength , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonNumText(___node) \
      LP1(0x7e, CONST_STRPTR, JsonNumText , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonInt(___node, ___def) \
      LP2(0x84, LONG, JsonInt , JsonNode *, ___node, a0, LONG, ___def, d0,\
      , JSON_BASE_NAME)

#define JsonScaled(___node, ___places, ___def) \
      LP3(0x8a, LONG, JsonScaled , JsonNode *, ___node, a0, LONG, ___places, d0, LONG, ___def, d1,\
      , JSON_BASE_NAME)

#define JsonBool(___node, ___def) \
      LP2(0x90, LONG, JsonBool , JsonNode *, ___node, a0, LONG, ___def, d0,\
      , JSON_BASE_NAME)

#define JsonLatin1(___node, ___buf, ___size) \
      LP3(0x96, LONG, JsonLatin1 , JsonNode *, ___node, a0, STRPTR, ___buf, a1, LONG, ___size, d0,\
      , JSON_BASE_NAME)

#define JsonNewDoc() \
      LP0(0x9c, JsonDoc *, JsonNewDoc ,\
      , JSON_BASE_NAME)

#define JsonSetRoot(___doc, ___node) \
      LP2NR(0xa2, JsonSetRoot , JsonDoc *, ___doc, a0, JsonNode *, ___node, a1,\
      , JSON_BASE_NAME)

#define JsonNewObject(___doc) \
      LP1(0xa8, JsonNode *, JsonNewObject , JsonDoc *, ___doc, a0,\
      , JSON_BASE_NAME)

#define JsonNewArray(___doc) \
      LP1(0xae, JsonNode *, JsonNewArray , JsonDoc *, ___doc, a0,\
      , JSON_BASE_NAME)

#define JsonNewString(___doc, ___s, ___len, ___flags) \
      LP4(0xb4, JsonNode *, JsonNewString , JsonDoc *, ___doc, a0, CONST_STRPTR, ___s, a1, LONG, ___len, d0, ULONG, ___flags, d1,\
      , JSON_BASE_NAME)

#define JsonNewInt(___doc, ___value) \
      LP2(0xba, JsonNode *, JsonNewInt , JsonDoc *, ___doc, a0, LONG, ___value, d0,\
      , JSON_BASE_NAME)

#define JsonNewNumber(___doc, ___text) \
      LP2(0xc0, JsonNode *, JsonNewNumber , JsonDoc *, ___doc, a0, CONST_STRPTR, ___text, a1,\
      , JSON_BASE_NAME)

#define JsonNewBool(___doc, ___value) \
      LP2(0xc6, JsonNode *, JsonNewBool , JsonDoc *, ___doc, a0, LONG, ___value, d0,\
      , JSON_BASE_NAME)

#define JsonNewNull(___doc) \
      LP1(0xcc, JsonNode *, JsonNewNull , JsonDoc *, ___doc, a0,\
      , JSON_BASE_NAME)

#define JsonAdd(___parent, ___name, ___node) \
      LP3(0xd2, LONG, JsonAdd , JsonNode *, ___parent, a0, CONST_STRPTR, ___name, a1, JsonNode *, ___node, a2,\
      , JSON_BASE_NAME)

#define JsonRemove(___node) \
      LP1NR(0xd8, JsonRemove , JsonNode *, ___node, a0,\
      , JSON_BASE_NAME)

#define JsonWrite(___node, ___buf, ___size, ___flags) \
      LP4(0xde, LONG, JsonWrite , JsonNode *, ___node, a0, STRPTR, ___buf, a1, LONG, ___size, d0, ULONG, ___flags, d1,\
      , JSON_BASE_NAME)

#define JsonWriteFH(___node, ___fh, ___flags) \
      LP3(0xe4, LONG, JsonWriteFH , JsonNode *, ___node, a0, BPTR, ___fh, d1, ULONG, ___flags, d0,\
      , JSON_BASE_NAME)

#endif /* !_INLINE_JSON_H */
