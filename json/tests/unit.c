/* unit.c - json.c on the PC: every call, the edges, the promises in the
 * guide. Built with the address and undefined-behaviour sanitizers
 * (make pctest). */
#include "../src/port.h"
#include <assert.h>

static int fails;

#define CHECK(c) do { if (!(c)) { printf("%s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define STREQ(a, b) do { const char *x_ = (const char *)(a), *y_ = (b); \
    if (!x_ || strcmp(x_, y_)) { printf("%s:%d: \"%s\" is not \"%s\"\n", __FILE__, __LINE__, x_ ? x_ : "(null)", y_); fails++; } } while (0)

static char out[4096];

static const char *w(JsonNode *n, ULONG flags)
{
    JsonWrite(n, out, sizeof(out), flags);
    return out;
}

/* parse, write compact, compare */
static void same(const char *in, const char *want, ULONG flags)
{
    struct JsonError e;
    JsonDoc *d = JsonParse(in, -1, flags, &e);
    if (!d) {
        printf("parse failed: %s (%s at %d)\n", in, e.Message, (int)e.Pos);
        fails++;
        return;
    }
    STREQ(w(JsonRoot(d), 0), want);
    JsonFree(d);
}

static void bad(const char *in, ULONG flags, const char *msg, LONG line, LONG col)
{
    struct JsonError e;
    JsonDoc *d = JsonParse(in, -1, flags, &e);
    if (d) {
        printf("should fail: %s\n", in);
        fails++;
        JsonFree(d);
        return;
    }
    if (msg) STREQ(e.Message, msg);
    if (line) CHECK(e.Line == line);
    if (col) {
        if (e.Col != col) printf("col %d, wanted %d: %s\n", (int)e.Col, (int)col, in);
        CHECK(e.Col == col);
    }
}

static void reading(void)
{
    JsonDoc *d;
    JsonNode *r, *n;
    char b[64];

    same("  {\"a\" : [1, 2.5, -3e2, true, false, null, \"x\"] }  ", "{\"a\":[1,2.5,-3e2,true,false,null,\"x\"]}", 0);
    same("[]", "[]", 0);
    same("{}", "{}", 0);
    same("\"\\u00e5\\ud83d\\ude00\\n\\/\"", "\"\xc3\xa5\xf0\x9f\x98\x80\\n/\"", 0);
    same("42", "42", 0);
    same("\xef\xbb\xbf[1]", "[1]", 0);                     /* a byte order mark */
    same("[\"\\ud800\"]", "[\"\xef\xbf\xbd\"]", 0);         /* half a pair: U+FFFD */

    bad("", 0, "the text ends too soon", 1, 1);
    bad("[1,2", 0, "the text ends too soon", 0, 0);
    bad("[1 2]", 0, "expected , or ]", 1, 4);
    bad("{\"a\" 1}", 0, "expected : after a name", 1, 6);
    bad("{a:1}", 0, "expected a name in quotes", 1, 2);
    bad("[1,]", 0, "expected a value", 1, 4);
    bad("[01]", 0, "expected , or ]", 1, 3);
    bad("[1.]", 0, "a number is wrong", 1, 2);
    bad("[-]", 0, "a number is wrong", 0, 0);
    bad("[\"a\nb\"]", 0, "a control character in a string", 0, 0);
    bad("[\"\\x\"]", 0, "an unknown \\ escape", 0, 0);
    bad("[\"\\u12\"]", 0, "a \\u escape needs four hex digits", 0, 0);
    bad("[\"\xe5\"]", 0, "text that is not UTF-8", 0, 0);
    bad("[1] x", 0, "more text after the end", 1, 5);
    bad("{\n  \"a\": 1,\n  \"b\" 2\n}", 0, "expected : after a name", 3, 7);
    bad("[1] // c", 0, "more text after the end", 0, 0);
    bad("[tru]", 0, "expected a value", 0, 0);

    /* relaxed: comments, trailing commas, Latin-1 */
    same("// settings\n{ \"a\": 1, /* two */ \"b\": [2,], }", "{\"a\":1,\"b\":[2]}", JPF_RELAXED);
    same("[\"\xe5\xe4\xf6\"]", "[\"\xc3\xa5\xc3\xa4\xc3\xb6\"]", JPF_LATIN1);
    bad("[1] /* open", JPF_COMMENTS, "a comment is not closed", 0, 0);
    bad("[1,,]", JPF_TRAILING, "expected a value", 0, 0);
    bad("{,}", JPF_TRAILING, "expected a name in quotes", 0, 0);

    /* walking */
    d = JsonParse("{\"weather\":{\"city\":\"G\xc3\xb6teborg\",\"temp\":8.25,\"wind\":\"7\","
                  "\"days\":[{\"t\":1},{\"t\":2},{\"t\":3}],\"a.b\":5,\"ok\":true,\"x\":null},"
                  "\"dup\":1,\"dup\":2}", -1, 0, NULL);
    CHECK(d != NULL);
    r = JsonRoot(d);
    CHECK(JsonType(r) == JT_OBJECT);
    CHECK(JsonCount(r) == 3);
    CHECK(JsonInt(JsonGet(r, "dup"), 0) == 2);             /* the last one */
    n = JsonFind(r, "weather.city");
    STREQ(JsonString(n), "G\xc3\xb6teborg");
    CHECK(JsonLength(n) == 9);
    CHECK(JsonLatin1(n, b, sizeof(b)) == 8);
    STREQ(b, "G\xf6teborg");
    CHECK(JsonLatin1(n, b, 4) == 8);                         /* cut, the full length back */
    STREQ(b, "G\xf6t");
    CHECK(JsonLatin1(JsonGet(r, "weather"), b, sizeof(b)) == -1);
    CHECK(JsonInt(JsonFind(r, "weather.temp"), -1) == 8);
    CHECK(JsonScaled(JsonFind(r, "weather.temp"), 2, -1) == 825);
    CHECK(JsonScaled(JsonFind(r, "weather.temp"), 1, -1) == 83);   /* 82.5: half up */
    CHECK(JsonInt(JsonFind(r, "weather.wind"), -1) == 7);          /* a string that is a number */
    CHECK(JsonInt(JsonFind(r, "weather.city"), -1) == -1);
    CHECK(JsonInt(JsonFind(r, "weather.days[1].t"), 0) == 2);
    CHECK(JsonInt(JsonFind(r, "weather.days[-1].t"), 0) == 3);
    CHECK(JsonFind(r, "weather.days[3]") == NULL);
    CHECK(JsonFind(r, "weather.days[x]") == NULL);
    CHECK(JsonInt(JsonFind(r, "weather[\"a.b\"]"), 0) == 5);
    CHECK(JsonBool(JsonFind(r, "weather.ok"), 0) == 1);
    CHECK(JsonBool(JsonFind(r, "weather.x"), 7) == 7);
    CHECK(JsonType(JsonFind(r, "weather.x")) == JT_NULL);
    CHECK(JsonType(JsonFind(r, "nothing.here")) == JT_NONE);
    CHECK(JsonFind(r, "") == r);
    STREQ(JsonName(JsonFind(r, "weather.temp")), "temp");
    STREQ(JsonNumText(JsonFind(r, "weather.temp")), "8.25");
    CHECK(JsonParent(JsonFind(r, "weather.temp")) == JsonGet(r, "weather"));
    {
        int i = 0;
        for (n = JsonFirst(JsonFind(r, "weather.days")); n; n = JsonNext(n)) i++;
        CHECK(i == 3);
    }
    JsonFree(d);

    /* NULL everywhere is safe */
    CHECK(JsonType(NULL) == JT_NONE);
    CHECK(JsonGet(NULL, "a") == NULL);
    CHECK(JsonFind(NULL, "a") == NULL);
    CHECK(JsonInt(NULL, 5) == 5);
    CHECK(JsonCount(NULL) == 0);
    CHECK(JsonParse(NULL, 0, 0, NULL) == NULL);
    JsonFree(NULL);
}

static void numbers(void)
{
    static const struct { const char *in; LONG places, want; } t[] = {
        { "0", 0, 0 }, { "-0", 0, 0 }, { "7", 3, 7000 }, { "1.5", 0, 2 }, { "-1.5", 0, -2 },
        { "2.4999", 0, 2 }, { "0.005", 2, 1 }, { "0.004", 2, 0 }, { "1e3", 0, 1000 },
        { "1E+2", 1, 1000 }, { "12345e-2", 0, 123 }, { "12355e-2", 1, 1236 },
        { "2147483647", 0, 2147483647 }, { "2147483648", 0, 2147483647 },
        { "-2147483648", 0, -2147483647 - 1 }, { "-2147483649", 0, -2147483647 - 1 },
        { "1e100000000", 0, 2147483647 }, { "1e-100000000", 0, 0 }, { "-1e400", 0, -2147483647 - 1 },
        { "0.000000001", 9, 1 }, { "99999999999999999999", 0, 2147483647 },
        { "3.14159", 4, 31416 }, { "0e5", 0, 0 }, { "0.5", 0, 1 }, { "-0.5", 0, -1 },
    };
    unsigned i;
    for (i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
        JsonDoc *d = JsonParse(t[i].in, -1, 0, NULL);
        LONG got = JsonScaled(JsonRoot(d), t[i].places, -99);
        if (got != t[i].want) {
            printf("%s with %d places: %d, wanted %d\n", t[i].in, (int)t[i].places, (int)got, (int)t[i].want);
            fails++;
        }
        JsonFree(d);
    }
    {
        JsonDoc *d = JsonParse("1", -1, 0, NULL);
        CHECK(JsonScaled(JsonRoot(d), 10, -9) == -9);       /* places 0-9 only */
        CHECK(JsonScaled(JsonRoot(d), -1, -9) == -9);
        JsonFree(d);
    }
}

static void building(void)
{
    JsonDoc *d = JsonNewDoc();
    JsonNode *o, *a, *s, *x;
    char small[8];
    LONG need;

    o = JsonNewObject(d);
    JsonSetRoot(d, o);
    CHECK(JsonAdd(o, "name", JsonNewString(d, "G\xf6teborg", -1, JSF_LATIN1)));
    CHECK(JsonAdd(o, "n", JsonNewInt(d, -2147483647 - 1)));
    CHECK(JsonAdd(o, "zero", JsonNewInt(d, 0)));
    CHECK(JsonAdd(o, "pi", JsonNewNumber(d, "3.14")));
    CHECK(!JsonAdd(o, "bad", JsonNewNumber(d, "3.")));       /* not a number: NULL, refused */
    CHECK(JsonAdd(o, "t", JsonNewBool(d, 5)));
    CHECK(JsonAdd(o, "nil", JsonNewNull(d)));
    a = JsonNewArray(d);
    CHECK(JsonAdd(o, "list", a));
    CHECK(JsonAdd(a, NULL, JsonNewString(d, "q\"\\\x01\n\xe2\x82\xac", -1, 0)));
    CHECK(JsonAdd(a, "ignored", JsonNewObject(d)));
    CHECK(JsonAdd(a, NULL, JsonNewArray(d)));
    STREQ(w(o, 0), "{\"name\":\"G\xc3\xb6teborg\",\"n\":-2147483648,\"zero\":0,\"pi\":3.14,\"t\":true,"
                   "\"nil\":null,\"list\":[\"q\\\"\\\\\\u0001\\n\xe2\x82\xac\",{},[]]}");
    STREQ(w(a, JWF_ASCII), "[\"q\\\"\\\\\\u0001\\n\\u20ac\",{},[]]");
    STREQ(w(JsonNewString(d, "\xf0\x9f\x98\x80", -1, 0), JWF_ASCII), "\"\\ud83d\\ude00\"");
    STREQ(w(JsonNewString(d, "a\xe5", -1, 0), 0), "\"a\xc3\xa5\"");   /* bad UTF-8 taken as Latin-1 */

    /* a name once: adding it again replaces */
    CHECK(JsonAdd(o, "pi", JsonNewInt(d, 3)));
    CHECK(JsonInt(JsonGet(o, "pi"), 0) == 3);
    CHECK(JsonCount(o) == 7);

    /* the rules */
    s = JsonNewString(d, "x", -1, 0);
    CHECK(JsonAdd(a, NULL, s));
    CHECK(!JsonAdd(a, NULL, s));                             /* already in */
    CHECK(!JsonAdd(o, NULL, JsonNewNull(d)));                /* an object needs a name */
    CHECK(!JsonAdd(s, NULL, JsonNewNull(d)));                /* not into a string */
    CHECK(!JsonAdd(a, NULL, a));                             /* not into itself */
    CHECK(!JsonAdd(a, NULL, o));                             /* nor its own root */
    x = JsonNewArray(d);
    CHECK(JsonAdd(x, NULL, JsonNewInt(d, 1)));
    CHECK(!JsonAdd(JsonFirst(x), NULL, x));
    {
        JsonDoc *d2 = JsonNewDoc();
        CHECK(!JsonAdd(a, NULL, JsonNewNull(d2)));           /* not from another document */
        JsonFree(d2);
    }
    CHECK(!JsonAdd(a, NULL, NULL));

    /* remove and add again */
    JsonRemove(s);
    CHECK(JsonCount(a) == 3);
    CHECK(JsonAdd(o, "moved", s));
    STREQ(w(JsonGet(o, "moved"), 0), "\"x\"");
    JsonRemove(JsonGet(o, "name"));
    CHECK(JsonGet(o, "name") == NULL);
    JsonRemove(JsonAt(a, 0));
    JsonRemove(JsonAt(a, -1));
    STREQ(w(a, 0), "[{}]");

    /* sizes */
    need = JsonWrite(o, NULL, 0, 0);
    CHECK(need == (LONG)strlen(w(o, 0)));
    CHECK(JsonWrite(o, small, sizeof(small), 0) == need);
    CHECK(strlen(small) == 7);
    CHECK(JsonWrite(NULL, small, sizeof(small), 0) == 0 && small[0] == 0);

    /* pretty */
    {
        JsonDoc *p = JsonParse("{\"a\":[1,{\"b\":[]}],\"c\":{}}", -1, 0, NULL);
        STREQ(w(JsonRoot(p), JWF_PRETTY), "{\n  \"a\": [\n    1,\n    {\n      \"b\": []\n    }\n  ],\n  \"c\": {}\n}");
        STREQ(w(JsonFind(JsonRoot(p), "a[1]"), 0), "{\"b\":[]}");
        JsonFree(p);
    }
    JsonFree(d);
}

static void files(void)
{
    struct JsonError e;
    JsonDoc *d;
    FILE *f;
    char big[200000];
    int i, k = 0;

    CHECK(JsonLoad("/nonexistent/x.json", 0, &e) == NULL);
    STREQ(e.Message, "cannot open the file");

    /* bigger than the first read, and a big string in a block of its own */
    k += sprintf(big + k, "{\"s\":\"");
    for (i = 0; i < 60000; i++) big[k++] = 'a' + i % 26;
    k += sprintf(big + k, "\",\"n\":[");
    for (i = 0; i < 5000; i++) k += sprintf(big + k, "%s%d", i ? "," : "", i);
    k += sprintf(big + k, "]}");
    f = fopen("/tmp/jsonunit.json", "wb");
    fwrite(big, 1, k, f);
    fclose(f);
    d = JsonLoad("/tmp/jsonunit.json", 0, &e);
    CHECK(d != NULL);
    CHECK(JsonLength(JsonFind(JsonRoot(d), "s")) == 60000);
    CHECK(JsonInt(JsonFind(JsonRoot(d), "n[4999]"), 0) == 4999);
    f = fopen("/tmp/jsonunit2.json", "wb");
    CHECK(JsonWriteFH(JsonRoot(d), f, 0));
    fclose(f);
    JsonFree(d);
    f = fopen("/tmp/jsonunit2.json", "rb");
    i = fread(big + k + 1, 1, sizeof(big) - k - 1, f);
    fclose(f);
    CHECK(i == k + 1 && !memcmp(big, big + k + 1, k) && big[k + 1 + k] == '\n');
    remove("/tmp/jsonunit.json");
    remove("/tmp/jsonunit2.json");

    /* deep nesting: no recursion, so no stack trouble */
    {
        static char deep[2000002];
        for (i = 0; i < 1000000; i++) deep[i] = '[';
        for (i = 0; i < 1000000; i++) deep[1000000 + i] = ']';
        d = JsonParse(deep, 2000000, 0, NULL);
        CHECK(d != NULL);
        CHECK(JsonWrite(JsonRoot(d), NULL, 0, 0) == 2000000);
        JsonFree(d);
        d = JsonParse(deep + 1000000 - 3000, 6000, 0, NULL);    /* pretty: indent grows */
        CHECK(JsonWrite(JsonRoot(d), NULL, 0, JWF_PRETTY) == 18000000);   /* as Python json.dumps(indent=2) */
        JsonFree(d);
    }
}

int main(void)
{
    reading();
    numbers();
    building();
    files();
    if (fails) {
        printf("unit: %d failed\n", fails);
        return 1;
    }
    printf("unit: all passed\n");
    return 0;
}
