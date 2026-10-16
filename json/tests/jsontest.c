/* jsontest - json.library tried on the Amiga, through the library: every
 * call once, the answers checked. Prints what failed; returns 0 when all
 * passed, 10 otherwise (20 without the library).
 *
 *   jsontest [FILE]     FILE: a JSON file to read and write back as well */
#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/json.h>

struct Library *JsonBase;
static int fails;

#define CHECK(c) do { if (!(c)) { printf("line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(int argc, char **argv)
{
    struct JsonError e;
    JsonDoc *d, *b;
    JsonNode *r, *n, *o, *a;
    char buf[256];
    LONG len;
    BPTR fh;

    if (!(JsonBase = OpenLibrary(JSONNAME, 0))) {
        printf("jsontest: needs json.library\n");
        return 20;
    }
    printf("json.library %d.%d\n", JsonBase->lib_Version, JsonBase->lib_Revision);

    /* reading */
    d = JsonParse("{\"weather\":{\"city\":\"G\xc3\xb6teborg\",\"temp\":8.25,\"days\":[1,2,3],\"ok\":true}}",
                  -1, 0, &e);
    CHECK(d != NULL);
    r = JsonRoot(d);
    CHECK(JsonType(r) == JT_OBJECT);
    CHECK(JsonCount(JsonGet(r, "weather")) == 4);
    n = JsonFind(r, "weather.city");
    CHECK(JsonType(n) == JT_STRING);
    CHECK(JsonLength(n) == 9);
    CHECK(JsonLatin1(n, buf, sizeof(buf)) == 8 && !strcmp(buf, "G\xf6teborg"));
    CHECK(!strcmp(JsonString(n), "G\xc3\xb6teborg"));
    CHECK(!strcmp(JsonName(n), "city"));
    CHECK(JsonParent(n) == JsonGet(r, "weather"));
    CHECK(JsonInt(JsonFind(r, "weather.temp"), 0) == 8);
    CHECK(JsonScaled(JsonFind(r, "weather.temp"), 2, 0) == 825);
    CHECK(!strcmp(JsonNumText(JsonFind(r, "weather.temp")), "8.25"));
    CHECK(JsonBool(JsonFind(r, "weather.ok"), 0) == 1);
    CHECK(JsonInt(JsonAt(JsonFind(r, "weather.days"), -1), 0) == 3);
    CHECK(JsonInt(JsonNext(JsonFirst(JsonFind(r, "weather.days"))), 0) == 2);
    JsonFree(d);

    /* an error, where it is */
    CHECK(JsonParse("{\n  \"a\" 1\n}", -1, 0, &e) == NULL);
    CHECK(e.Line == 2 && e.Col == 7 && !strcmp(e.Message, "expected : after a name"));
    printf("error as reported: line %ld col %ld: %s\n", (long)e.Line, (long)e.Col, e.Message);

    /* relaxed */
    d = JsonParse("// settings\n{\"a\":[1,2,],/* x */\"b\":\"\xe5\"}", -1, JPF_RELAXED, &e);
    CHECK(d != NULL);
    JsonWrite(JsonRoot(d), buf, sizeof(buf), 0);
    CHECK(!strcmp(buf, "{\"a\":[1,2],\"b\":\"\xc3\xa5\"}"));
    JsonFree(d);

    /* building */
    b = JsonNewDoc();
    o = JsonNewObject(b);
    JsonSetRoot(b, o);
    CHECK(JsonAdd(o, "city", JsonNewString(b, "G\xf6teborg", -1, JSF_LATIN1)));
    CHECK(JsonAdd(o, "n", JsonNewInt(b, -42)));
    CHECK(JsonAdd(o, "pi", JsonNewNumber(b, "3.14")));
    CHECK(JsonAdd(o, "t", JsonNewBool(b, 1)));
    CHECK(JsonAdd(o, "x", JsonNewNull(b)));
    a = JsonNewArray(b);
    CHECK(JsonAdd(o, "list", a));
    CHECK(JsonAdd(a, NULL, JsonNewString(b, "\xe2\x82\xac", -1, 0)));
    len = JsonWrite(o, buf, sizeof(buf), 0);
    CHECK(!strcmp(buf, "{\"city\":\"G\xc3\xb6teborg\",\"n\":-42,\"pi\":3.14,\"t\":true,\"x\":null,\"list\":[\"\xe2\x82\xac\"]}"));
    CHECK(len == (LONG)strlen(buf));
    CHECK(JsonWrite(o, NULL, 0, 0) == len);
    JsonWrite(a, buf, sizeof(buf), JWF_ASCII);
    CHECK(!strcmp(buf, "[\"\\u20ac\"]"));
    JsonRemove(JsonGet(o, "x"));
    CHECK(JsonGet(o, "x") == NULL);
    JsonWrite(o, buf, sizeof(buf), JWF_PRETTY);
    printf("built, pretty:\n%s\n", buf);

    /* a file, written and read back */
    if ((fh = Open("T:jsontest.json", MODE_NEWFILE))) {
        CHECK(JsonWriteFH(o, fh, JWF_PRETTY));
        Close(fh);
        d = JsonLoad("T:jsontest.json", 0, &e);
        CHECK(d != NULL);
        CHECK(JsonScaled(JsonGet(JsonRoot(d), "pi"), 2, 0) == 314);
        JsonFree(d);
        DeleteFile("T:jsontest.json");
    } else
        CHECK(!"T: is writable");
    JsonFree(b);

    /* a file of the user's, read and written back to the screen */
    if (argc > 1) {
        d = JsonLoad(argv[1], JPF_RELAXED, &e);
        if (!d)
            printf("%s: line %ld col %ld: %s\n", argv[1], (long)e.Line, (long)e.Col, e.Message);
        else {
            JsonWriteFH(JsonRoot(d), Output(), JWF_PRETTY);
            JsonFree(d);
        }
    }

    CloseLibrary(JsonBase);
    if (fails) {
        printf("jsontest: %d failed\n", fails);
        return 10;
    }
    printf("jsontest: all passed\n");
    return 0;
}
