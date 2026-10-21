# utf8.library - UTF-8 text on a Latin-1 Amiga (README.md).
# Cross-built with Bebbo's m68k-amigaos-gcc, the same way as json.library.
#
#   make            the library three times (below) and build/utf8test
#   make headers    clib/ proto/ inline/ again from utf8.sfd (after adding a call)
#   make tables     src/tables.c again from Python's Unicode database
#   make pctest     utf8.c on the PC with the sanitizers: the unit tests
#   make pccheck    every code point and 8-bit set held against Python's
#                   own Unicode database and codecs
#   make fuzz       utf8.c under libFuzzer (clang) for FUZZTIME seconds
#
# The library, one per processor; install one as LIBS:utf8.library:
#   build/000/utf8.library   every Amiga (68000 and up)
#   build/020/utf8.library   68020, 68030, 68040 (and Emu68)
#   build/060/utf8.library   68060: without the 64-bit multiply and divide the
#                           060 has only in software (runs on a 68040 too)
# No FPU build: the library does no floating point at all.
#
# The library links nothing but its own code (-nostartfiles -nostdlib); the
# 68000 one also takes libnix 1.3's plain multiply and divide. The build
# fails if any other C library code gets in.

AMIGA   ?= $(firstword $(wildcard /opt/amiga $(HOME)/opt/amiga))
CC      = $(AMIGA)/bin/m68k-amigaos-gcc
OBJDUMP = $(AMIGA)/bin/m68k-amigaos-objdump
SFDC    = $(AMIGA)/bin/sfdc
WARN    = -Os -noixemul -fomit-frame-pointer -fno-common -fbbb=- -Wall -Wno-pointer-sign -Iinclude -Isrc
CFLAGS  = $(WARN) -m68020
LIBFLAGS = -fno-builtin -ffreestanding -DUTF8_LIB
NIX13   = $(AMIGA)/m68k-amigaos/libnix/lib/libnix13.a
CPUS    = 000 020 060
# the compiler's processor for each build
CPU_000 = -m68000
CPU_020 = -m68020
CPU_060 = -m68060
ARITH_000 = $(NIX13)
OBJS    = lib utf8 tables rexx

PCCC    = gcc -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -g -O1 -Itests/pcinc \
          -fsanitize=address,undefined -fno-sanitize-recover=all
PCSRC   = src/utf8.c src/tables.c
FUZZTIME ?= 240

all: $(foreach c,$(CPUS),build/$(c)/utf8.library) build/utf8test check060

build/start.o: src/start.s
	@mkdir -p build
	$(CC) -c $< -o $@

# one library per processor: build/CPU/utf8.library
define LIBRARY
build/$(1)/%.o: src/%.c src/port.h src/base.h src/tables.h src/version.h include/libraries/utf8.h
	@mkdir -p build/$(1)
	$$(CC) $$(WARN) $$(CPU_$(1)) $$(LIBFLAGS) -c $$< -o $$@

build/$(1)/utf8.library: build/start.o $(OBJS:%=build/$(1)/%.o)
	$$(CC) $$(CPU_$(1)) -noixemul -nostartfiles -nostdlib -s -Wl,-Map,build/$(1)/utf8.map -o $$@ $$^ $$(ARITH_$(1)) -lgcc
	@! grep -E 'libnix|libstubs|libamiga' build/$(1)/utf8.map | grep -v -E '^LOAD |libnix13.a\(__u?(mul|div|mod)si[34].o\)' \
	    || (echo "C library code got into $$@"; false)
endef
$(foreach c,$(CPUS),$(eval $(call LIBRARY,$(c))))

# the 060 one must not have the 64-bit forms of muls.l/mulu.l/divs.l/divu.l
check060: build/060/utf8.library
	@! $(OBJDUMP) -m m68k:68060 -d $(OBJS:%=build/060/%.o) | grep -E '\s(mul|div)[su]l\s+[^ ]*,%?d[0-7],%?d[0-7]' \
	    || (echo "build/060 has a 64-bit multiply or divide"; false)
	@echo "build/060: no 64-bit multiply or divide"

# the Amiga test: every call through the library (tests/utf8test.c)
build/utf8test: tests/utf8test.c tests/unit.c include/libraries/utf8.h include/inline/utf8.h
	@mkdir -p build
	$(CC) $(CFLAGS) -s -o $@ $<

headers: utf8.sfd
	@mkdir -p include/clib include/proto include/inline
	$(SFDC) --mode=clib --output=include/clib/utf8_protos.h utf8.sfd
	$(SFDC) --mode=proto --output=include/proto/utf8.h utf8.sfd
	$(SFDC) --target=m68k-amigaos --mode=macros --output=include/inline/utf8.h utf8.sfd

tables:
	python3 -I tools/gen.py > src/tables.c

pctest:
	@mkdir -p build/pc
	$(PCCC) -o build/pc/unit tests/unit.c $(PCSRC)
	build/pc/unit

pccheck:
	@mkdir -p build/pc
	gcc -std=c99 -O2 -Itests/pcinc -o build/pc/dump tests/dump.c $(PCSRC)
	build/pc/dump | python3 -I tools/check.py

# the quick lane of Utf8ToCharset gives what the full way gives
lanecheck:
	@mkdir -p build/pc
	gcc -std=c99 -O2 -Itests/pcinc -o build/pc/lane tests/lane.c $(PCSRC)
	gcc -std=c99 -O2 -Itests/pcinc -DNOQUICKLANE -o build/pc/lanefull tests/lane.c $(PCSRC)
	build/pc/lane > build/pc/lane.out
	build/pc/lanefull > build/pc/lanefull.out
	cmp build/pc/lane.out build/pc/lanefull.out && echo "quick lane: the same for every code point"

fuzz:
	@mkdir -p build/pc/corpus
	clang -g -O1 -Itests/pcinc -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all \
	    -o build/pc/fuzz tests/fuzz.c $(PCSRC)
	build/pc/fuzz build/pc/corpus -max_total_time=$(FUZZTIME) -rss_limit_mb=1024 -max_len=4000

clean:
	rm -rf build

.PHONY: all headers tables clean check060 pctest pccheck fuzz lanecheck

# the Markdown docs as the Amiga reads them: docs/utf8.doc (Autodocs),
# docs/utf8_guide.doc and docs/utf8.guide (AmigaGuide), Latin-1
docs: docs/guide.md docs/reference.md ../tools/mkdocs.py
	python3 -I ../tools/mkdocs.py . docs

.PHONY: docs
