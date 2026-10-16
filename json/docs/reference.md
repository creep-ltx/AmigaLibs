# json.library reference

Every call, in the order of the alphabet. The number after each name is the revision it came in
(see [Versions](guide.md#versions)); the registers are for assembler and other languages.
How to use them together, and the ARexx functions: the [programmer's guide](guide.md).

All calls take NULL for a node or document and answer "nothing" (NULL, 0, `JT_NONE` or the
default given), so calls can be chained without checks in between.

[JsonAdd](#jsonadd) · [JsonAt](#jsonat) · [JsonBool](#jsonbool) · [JsonCount](#jsoncount) · [JsonFind](#jsonfind) · [JsonFirst](#jsonfirst) · [JsonFree](#jsonfree) · [JsonGet](#jsonget) · [JsonInt](#jsonint) · [JsonLatin1](#jsonlatin1) · [JsonLength](#jsonlength) · [JsonLoad](#jsonload) · [JsonName](#jsonname) · [JsonNewArray](#jsonnewarray) · [JsonNewBool](#jsonnewbool) · [JsonNewDoc](#jsonnewdoc) · [JsonNewInt](#jsonnewint) · [JsonNewNull](#jsonnewnull) · [JsonNewNumber](#jsonnewnumber) · [JsonNewObject](#jsonnewobject) · [JsonNewString](#jsonnewstring) · [JsonNext](#jsonnext) · [JsonNumText](#jsonnumtext) · [JsonParent](#jsonparent) · [JsonParse](#jsonparse) · [JsonRemove](#jsonremove) · [JsonRoot](#jsonroot) · [JsonScaled](#jsonscaled) · [JsonSetRoot](#jsonsetroot) · [JsonString](#jsonstring) · [JsonType](#jsontype) · [JsonWrite](#jsonwrite) · [JsonWriteFH](#jsonwritefh)

## JsonAdd

Put a node into an object or array (0.1).

```c
ok = JsonAdd(parent, name, node)
D0          A0      A1    A2

LONG JsonAdd(JsonNode *, CONST_STRPTR, JsonNode *);
```

Adds node as the last member of parent. In an object it needs a name (UTF-8; a byte that is not UTF-8 is taken as Latin-1); a member of that name already there is replaced. In an array the name is ignored (pass NULL).

**Inputs**

```
parent - an object or array
name   - the member's name (objects), or NULL
node   - from JsonNew...(), or taken out with JsonRemove()
```

**Result**

```
ok - TRUE, or FALSE: node NULL, already in a tree, from another
     document, parent not an object or array (or no name for an
     object), or node is parent or holds it
```

**Notes:** Since a failed JsonNew...() is NULL and JsonAdd() refuses NULL, JsonAdd(o, "x", JsonNewInt(doc, 5)) needs no checks.

**See also:** [JsonRemove](#jsonremove), [JsonSetRoot](#jsonsetroot)

## JsonAt

An array's item by number (0.1).

```c
node = JsonAt(array, index)
D0            A0     D0

JsonNode *JsonAt(JsonNode *, LONG);
```

Returns the item at index, counting from 0; a negative index counts from the end (-1 is the last).

**Inputs**

```
array - an array
index - which item
```

**Result**

```
node - the item, or NULL (not an array, or no such item)
```

**Notes:** It walks from the start: in a loop over a big array use JsonFirst() and JsonNext().

**See also:** [JsonFirst](#jsonfirst), [JsonFind](#jsonfind)

## JsonBool

true or false (0.1).

```c
value = JsonBool(node, def)
D0               A0    D0

LONG JsonBool(JsonNode *, LONG);
```

Returns 1 for true and 0 for false.

**Inputs**

```
node - a bool
def  - what to return for anything else
```

**Result**

```
value - 1, 0, or def
```

**See also:** [JsonNewBool](#jsonnewbool)

## JsonCount

How many members (0.1).

```c
count = JsonCount(node)
D0                A0

LONG JsonCount(JsonNode *);
```

Returns how many members an object or array has.

**Inputs**

```
node - an object or array
```

**Result**

```
count - the members; 0 for anything else
```

**See also:** [JsonFirst](#jsonfirst)

## JsonFind

A node by path (0.1).

```c
node = JsonFind(start, path)
D0              A0     A1

JsonNode *JsonFind(JsonNode *, CONST_STRPTR);
```

Follows a path from start: names joined by dots, [n] for an array's n-th item (from 0, negative from the end), ["name"] for a name with dots or brackets (\" and \\ inside). An empty path is start itself.

**Inputs**

```
start - where the path begins (usually JsonRoot(doc))
path  - like "weather.days[0].temp" (UTF-8)
```

**Result**

```
node - what the path leads to, or NULL
```

**See also:** [JsonGet](#jsonget), [JsonAt](#jsonat)

## JsonFirst

The first member (0.1).

```c
member = JsonFirst(node)
D0                A0

JsonNode *JsonFirst(JsonNode *);
```

Returns the first member of an object or array; JsonNext() gives the rest.

**Inputs**

```
node - an object or array
```

**Result**

```
member - the first member, or NULL (empty, or not an object or array)
```

**Notes:** for (m = JsonFirst(o); m; m = JsonNext(m)) ...

**See also:** [JsonNext](#jsonnext), [JsonName](#jsonname)

## JsonFree

Free a document (0.1).

```c
JsonFree(doc)
         A0

VOID JsonFree(JsonDoc *);
```

Frees the document and every node and string in it. Nothing from it may be used afterwards.

**Inputs**

```
doc - from JsonParse(), JsonLoad() or JsonNewDoc(); NULL is safe
```

**See also:** [JsonParse](#jsonparse), [JsonNewDoc](#jsonnewdoc)

## JsonGet

An object's member by name (0.1).

```c
node = JsonGet(object, name)
D0             A0      A1

JsonNode *JsonGet(JsonNode *, CONST_STRPTR);
```

Returns the member called name. If the name is there more than once, the last one (as JavaScript does).

**Inputs**

```
object - an object
name   - the name, UTF-8
```

**Result**

```
node - the member, or NULL
```

**See also:** [JsonFind](#jsonfind), [JsonAt](#jsonat)

## JsonInt

A number as a whole number (0.1).

```c
value = JsonInt(node, def)
D0              A0    D0

LONG JsonInt(JsonNode *, LONG);
```

Returns the number rounded to the nearest whole number (halves away from zero: 8.5 is 9, -8.5 is -9), kept between -2147483648 and 2147483647. A string that is exactly a JSON number ("8.5") counts as that number.

**Inputs**

```
node - a number
def  - what to return for anything else
```

**Result**

```
value - the number, or def
```

**See also:** [JsonScaled](#jsonscaled), [JsonNumText](#jsonnumtext)

## JsonLatin1

A string as Amiga text (0.1).

```c
length = JsonLatin1(node, buf, size)
D0                  A0    A1   D0

LONG JsonLatin1(JsonNode *, STRPTR, LONG);
```

Copies a string (or a number's text) into buf as Latin-1, ended with a 0. Characters Latin-1 does not have become the nearest it has (Ł as L, € as E, curly quotes as ", dashes as -), else ?.

**Inputs**

```
node - a string or number
buf  - where to put it; NULL only measures
size - buf's size, the 0 included
```

**Result**

```
length - the whole text's length, even if buf was too small;
         -1 if node is not a string or number (buf gets "")
```

**Notes:** JsonLatin1(n, NULL, 0) + 1 is the buffer size needed.

**See also:** [JsonString](#jsonstring), [JsonNewString](#jsonnewstring)

## JsonLength

A string's length (0.1).

```c
length = JsonLength(node)
D0                  A0

LONG JsonLength(JsonNode *);
```

Returns the length in bytes (UTF-8) of a string, or of a number's text.

**Inputs**

```
node - a string or number
```

**Result**

```
length - the bytes, or 0 for anything else
```

**Notes:** A string can hold a 0 byte (\u0000 in the JSON): JsonString() stops there, JsonLength() does not.

**See also:** [JsonString](#jsonstring)

## JsonLoad

Read a JSON file (0.1).

```c
doc = JsonLoad(name, flags, err)
D0            A0    D0     A1

JsonDoc *JsonLoad(CONST_STRPTR, ULONG, struct JsonError *);
```

Reads a whole file (or pipe) and reads it as JsonParse() does.

**Inputs**

```
name  - the file
flags - as JsonParse()
err   - filled in when it fails; may be NULL
```

**Result**

```
doc - the document, or NULL (Message: "cannot open the file",
      "cannot read the file", "out of memory", or what was
      wrong with the JSON)
```

**See also:** [JsonParse](#jsonparse), [JsonWriteFH](#jsonwritefh)

## JsonName

A member's name (0.1).

```c
name = JsonName(node)
D0              A0

CONST_STRPTR JsonName(JsonNode *);
```

Returns the name of an object's member (UTF-8, read-only, valid while the document is).

**Inputs**

```
node - a member of an object
```

**Result**

```
name - the name, or NULL (not in an object)
```

**See also:** [JsonFirst](#jsonfirst), [JsonLatin1](#jsonlatin1)

## JsonNewArray

A new, empty array (0.1).

```c
node = JsonNewArray(doc)
D0                  A0

JsonNode *JsonNewArray(JsonDoc *);
```

Makes an empty array in doc, not yet in the tree.

**Inputs**

```
doc - the document
```

**Result**

```
node - the array, or NULL (out of memory)
```

**See also:** [JsonAdd](#jsonadd)

## JsonNewBool

A new true or false (0.1).

```c
node = JsonNewBool(doc, value)
D0                 A0   D0

JsonNode *JsonNewBool(JsonDoc *, LONG);
```

Makes true (value not 0) or false.

**Inputs**

```
doc   - the document
value - TRUE or FALSE
```

**Result**

```
node - the bool, or NULL
```

**See also:** [JsonAdd](#jsonadd)

## JsonNewDoc

A new, empty document (0.1).

```c
doc = JsonNewDoc()
D0

JsonDoc *JsonNewDoc(void);
```

Makes an empty document to build in. Give it a top value with JsonSetRoot().

**Result**

```
doc - the document, or NULL (out of memory)
```

**See also:** [JsonSetRoot](#jsonsetroot), [JsonFree](#jsonfree)

## JsonNewInt

A new number from a LONG (0.1).

```c
node = JsonNewInt(doc, value)
D0                A0   D0

JsonNode *JsonNewInt(JsonDoc *, LONG);
```

Makes a whole number.

**Inputs**

```
doc   - the document
value - the number
```

**Result**

```
node - the number, or NULL
```

**See also:** [JsonNewNumber](#jsonnewnumber)

## JsonNewNull

A new null (0.1).

```c
node = JsonNewNull(doc)
D0                 A0

JsonNode *JsonNewNull(JsonDoc *);
```

Makes null.

**Inputs**

```
doc - the document
```

**Result**

```
node - the null, or NULL (out of memory)
```

**See also:** [JsonAdd](#jsonadd)

## JsonNewNumber

A new number from its text (0.1).

```c
node = JsonNewNumber(doc, text)
D0                   A0   A1

JsonNode *JsonNewNumber(JsonDoc *, CONST_STRPTR);
```

Makes a number written as text, kept exactly: "3.14", "-2e-3". For decimals without floating point.

**Inputs**

```
doc  - the document
text - a JSON number
```

**Result**

```
node - the number, or NULL (out of memory, or text is not a JSON
       number: "3.", "01", "+1", "1,5" are not)
```

**See also:** [JsonNewInt](#jsonnewint)

## JsonNewObject

A new, empty object (0.1).

```c
node = JsonNewObject(doc)
D0                   A0

JsonNode *JsonNewObject(JsonDoc *);
```

Makes an empty object in doc, not yet in the tree.

**Inputs**

```
doc - the document
```

**Result**

```
node - the object, or NULL (out of memory)
```

**See also:** [JsonAdd](#jsonadd)

## JsonNewString

A new string (0.1).

```c
node = JsonNewString(doc, text, len, flags)
D0                   A0   A1    D0   D1

JsonNode *JsonNewString(JsonDoc *, CONST_STRPTR, LONG, ULONG);
```

Makes a string, copying the text.

**Inputs**

```
doc   - the document
text  - the text
len   - its length in bytes, or -1 for up to its 0
flags - JSF_LATIN1: the text is Amiga text. Without it the text is
        UTF-8, and a byte that is not UTF-8 is still taken as Latin-1
```

**Result**

```
node - the string, or NULL
```

**See also:** [JsonLatin1](#jsonlatin1), [JsonAdd](#jsonadd)

## JsonNext

The next member (0.1).

```c
member = JsonNext(node)
D0                A0

JsonNode *JsonNext(JsonNode *);
```

Returns the member after node in its object or array.

**Inputs**

```
node - a member
```

**Result**

```
member - the next one, or NULL at the end
```

**See also:** [JsonFirst](#jsonfirst)

## JsonNumText

A number's text (0.1).

```c
text = JsonNumText(node)
D0                 A0

CONST_STRPTR JsonNumText(JsonNode *);
```

Returns a number exactly as it was written ("8.25", "1e-3"), for a program with its own floating-point maths.

**Inputs**

```
node - a number
```

**Result**

```
text - the text (read-only), or NULL for anything else
```

**See also:** [JsonInt](#jsonint), [JsonScaled](#jsonscaled)

## JsonParent

The object or array a node is in (0.1).

```c
parent = JsonParent(node)
D0                  A0

JsonNode *JsonParent(JsonNode *);
```

Returns the object or array that holds node.

**Inputs**

```
node - any node
```

**Result**

```
parent - the object or array, or NULL (the top, or not in a tree)
```

**See also:** [JsonFirst](#jsonfirst)

## JsonParse

Read JSON text (0.1).

```c
doc = JsonParse(text, len, flags, err)
D0             A0    D0   D1     A1

JsonDoc *JsonParse(CONST_STRPTR, LONG, ULONG, struct JsonError *);
```

Reads JSON text into a new document. Without flags the text must be strict JSON (RFC 8259); a UTF-8 byte order mark at the start is allowed.

**Inputs**

```
text  - the JSON text
len   - its length, or -1 for up to its 0
flags - 0, or any of:
          JPF_COMMENTS  // and slash-star comments
          JPF_TRAILING  a comma before ] or }
          JPF_LATIN1    bytes that are not UTF-8 are Latin-1
          JPF_RELAXED   all three
err   - filled in when it fails; may be NULL
```

**Result**

```
doc - the document, or NULL. err then says where and why:
      Message (English), Line and Col (from 1), Pos (from 0)
```

**Notes:** The text is copied: it may be freed straight after.

**See also:** [JsonLoad](#jsonload), [JsonRoot](#jsonroot), [JsonFree](#jsonfree)

## JsonRemove

Take a node out (0.1).

```c
JsonRemove(node)
           A0

VOID JsonRemove(JsonNode *);
```

Takes node out of its object or array (or off the top of its document). It stays in the document and can be added again elsewhere; its memory goes with JsonFree().

**Inputs**

```
node - any node; NULL is safe
```

**See also:** [JsonAdd](#jsonadd)

## JsonRoot

A document's top value (0.1).

```c
node = JsonRoot(doc)
D0              A0

JsonNode *JsonRoot(JsonDoc *);
```

Returns the value at the top of the document.

**Inputs**

```
doc - the document
```

**Result**

```
node - the top value, or NULL (none yet)
```

**See also:** [JsonSetRoot](#jsonsetroot)

## JsonScaled

A number as fixed point (0.1).

```c
value = JsonScaled(node, places, def)
D0                 A0    D0      D1

LONG JsonScaled(JsonNode *, LONG, LONG);
```

Returns the number times 10 to the power places, rounded to the nearest whole number (halves away from zero) and kept within a LONG: 8.25 with 2 places is 825, with 1 place 83. A string that is exactly a JSON number counts as that number.

**Inputs**

```
node   - a number
places - 0 to 9
def    - what to return for anything else (or places out of range)
```

**Result**

```
value - the scaled number, or def
```

**See also:** [JsonInt](#jsonint), [JsonNumText](#jsonnumtext)

## JsonSetRoot

Set a document's top value (0.1).

```c
JsonSetRoot(doc, node)
            A0   A1

VOID JsonSetRoot(JsonDoc *, JsonNode *);
```

Makes node the document's top value. The old one (if any) stays in the document, out of the tree.

**Inputs**

```
doc  - the document
node - a node of doc that is in no object or array; NULL empties it
```

**See also:** [JsonRoot](#jsonroot), [JsonNewDoc](#jsonnewdoc)

## JsonString

A string's text (0.1).

```c
text = JsonString(node)
D0                A0

CONST_STRPTR JsonString(JsonNode *);
```

Returns a string's text, UTF-8, ended with a 0 (read-only, valid while the document is).

**Inputs**

```
node - a string
```

**Result**

```
text - the text, or NULL for anything else
```

**Notes:** For Amiga text use JsonLatin1().

**See also:** [JsonLatin1](#jsonlatin1), [JsonLength](#jsonlength)

## JsonType

What a node is (0.1).

```c
type = JsonType(node)
D0              A0

LONG JsonType(JsonNode *);
```

Returns the node's type.

**Inputs**

```
node - any node, or NULL
```

**Result**

```
type - JT_OBJECT, JT_ARRAY, JT_STRING, JT_NUMBER, JT_BOOL, JT_NULL,
       or JT_NONE for NULL
```

## JsonWrite

A node as JSON text (0.1).

```c
length = JsonWrite(node, buf, size, flags)
D0                 A0    A1   D0    D1

LONG JsonWrite(JsonNode *, STRPTR, LONG, ULONG);
```

Writes node (and everything in it) as JSON text into buf, as snprintf does: at most size - 1 bytes and a 0.

**Inputs**

```
node  - any node
buf   - where to write; NULL only measures
size  - buf's size
flags - 0, or any of:
          JWF_PRETTY  one value a line, indented by two spaces
          JWF_ASCII   every character beyond ASCII as \uXXXX
```

**Result**

```
length - the whole text's length, even if buf was too small
```

**Notes:** JsonWrite(n, NULL, 0, flags) + 1 is the buffer size needed.

**See also:** [JsonWriteFH](#jsonwritefh)

## JsonWriteFH

A node as JSON text, into a file (0.1).

```c
ok = JsonWriteFH(node, fh, flags)
D0              A0    D1  D0

LONG JsonWriteFH(JsonNode *, BPTR, ULONG);
```

Writes node as JSON text to an open file, ended with a newline.

**Inputs**

```
node  - any node
fh    - a file opened for writing (or Output())
flags - as JsonWrite()
```

**Result**

```
ok - TRUE, or FALSE if writing failed
```

**See also:** [JsonWrite](#jsonwrite), [JsonLoad](#jsonload)

## The ARexx entry

The library's first entry (offset -30, before `JsonParse`) is not a C call: it is the function
ARexx calls for `ADDLIB('json.library', 0, -30, 0)`, with a RexxMsg in A0. It returns an error
code in D0 and the result string in A0 (and A1). Do not call it from C. The ARexx functions are in
the [guide](guide.md#arexx).

