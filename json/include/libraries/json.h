/* libraries/json.h - json.library: read, build and write JSON.
 *
 * JsonParse() or JsonLoad() turns JSON text into a tree of nodes in a
 * JsonDoc; JsonGet(), JsonAt(), JsonFind() walk it; JsonString(),
 * JsonInt(), JsonScaled(), JsonBool() read the values. JsonNewDoc() and
 * JsonNew...() build a tree, JsonAdd() puts nodes together, JsonWrite()
 * and JsonWriteFH() turn it into text. JsonFree() frees a document and
 * every node in it.
 *
 * Text is UTF-8 inside the library, as JSON is. JsonLatin1() gives a
 * string as Amiga (Latin-1) text; JsonNewString(..., JSF_LATIN1) takes
 * Amiga text.
 *
 * No floating point: numbers keep their text (JsonNumText) and give whole
 * numbers (JsonInt) or fixed decimals (JsonScaled: 8.25 with 2 places is
 * 825), so the library needs no FPU and no maths library.
 *
 * ARexx: the library is also an ARexx function library
 * (ADDLIB('json.library', 0, -30, 0)); see the guide.
 *
 * The rule of a library: a function's place in the table never moves.
 * New calls are added at the end of json.sfd and the revision goes up. */
#ifndef LIBRARIES_JSON_H
#define LIBRARIES_JSON_H

#include <exec/types.h>
#include <dos/dos.h>

#define JSONNAME     "json.library"
#define JSONVERSION  0          /* the version and revision this header */
#define JSONREVISION 1          /* describes: 0.1 (while the version is 0,
                                 * check lib_Revision for newer calls) */

/* a document (it owns all its nodes) and a node in it: both private */
typedef struct JsonDoc JsonDoc;
typedef struct JsonNode JsonNode;

/* JsonType() */
#define JT_NONE   0             /* no node (NULL) */
#define JT_NULL   1
#define JT_BOOL   2
#define JT_NUMBER 3
#define JT_STRING 4
#define JT_ARRAY  5
#define JT_OBJECT 6

/* JsonParse() and JsonLoad() flags. Without any, the text must be strict
 * JSON (RFC 8259). JPF_RELAXED is for settings files people write. */
#define JPF_COMMENTS 0x01       /* // and slash-star comments */
#define JPF_TRAILING 0x02       /* a comma before ] or } */
#define JPF_LATIN1   0x04       /* bytes that are not UTF-8 are Latin-1
                                 * (Amiga text), not an error */
#define JPF_RELAXED  (JPF_COMMENTS | JPF_TRAILING | JPF_LATIN1)

/* JsonNewString() flags */
#define JSF_LATIN1   0x01       /* the text is Latin-1 (Amiga text) */

/* JsonWrite() and JsonWriteFH() flags */
#define JWF_PRETTY   0x01       /* one value a line, indented by 2 */
#define JWF_ASCII    0x02       /* every character beyond ASCII as \uXXXX */

/* where JsonParse() or JsonLoad() stopped, when it failed */
struct JsonError {
    LONG Pos;                   /* the byte, from 0 */
    LONG Line, Col;             /* from 1 (Col counts bytes) */
    CONST_STRPTR Message;       /* what was wrong, in English */
};

#endif
