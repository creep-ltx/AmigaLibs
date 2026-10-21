# json.library - read, build and write JSON (README.md).
# Cross-built with Bebbo's m68k-amigaos-gcc, the same way as tui.library.
#
#   make            the library three times (below) and build/jsontest
#   make headers    clib/ proto/ inline/ again from json.sfd (after adding a call)
#   make pctest     json.c on the PC with the sanitizers: the unit tests, and
#                   JSONTestSuite when SUITE=.../JSONTestSuite/test_parsing
#   make fuzz       json.c under libFuzzer (clang) for FUZZTIME seconds
#
# The library, one per processor; install one as LIBS:json.library:
#   build/000/json.library   every Amiga (68000 and up)
#   build/020/json.library   68020, 68030, 68040 (and Emu68)
#   build/060/json.library   68060: without the 64-bit multiply and divide the
#                           060 has only in software (runs on a 68040 too).
#                           Not the 000/020/040 trio of programs with floating
#                           point: this library has none, so only the 060 differs
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
LIBFLAGS = -fno-builtin -ffreestanding -DJSON_LIB
NIX13   = $(AMIGA)/m68k-amigaos/libnix/lib/libnix13.a
CPUS    = 000 020 060
# the compiler's processor for each build
CPU_000 = -m68000
CPU_020 = -m68020
CPU_060 = -m68060
ARITH_000 = $(NIX13)
OBJS    = lib json rexx

PCCC    = gcc -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -g -O1 -Itests/pcinc \
          -fsanitize=address,undefined -fno-sanitize-recover=all
FUZZTIME ?= 240

all: $(foreach c,$(CPUS),build/$(c)/json.library) build/jsontest check060

build/start.o: src/start.s
	@mkdir -p build
	$(CC) -c $< -o $@

# one library per processor: build/CPU/json.library
define LIBRARY
build/$(1)/%.o: src/%.c src/port.h src/base.h src/version.h include/libraries/json.h
	@mkdir -p build/$(1)
	$$(CC) $$(WARN) $$(CPU_$(1)) $$(LIBFLAGS) -c $$< -o $$@

build/$(1)/json.library: build/start.o $(OBJS:%=build/$(1)/%.o)
	$$(CC) $$(CPU_$(1)) -noixemul -nostartfiles -nostdlib -s -Wl,-Map,build/$(1)/json.map -o $$@ $$^ $$(ARITH_$(1)) -lgcc
	@! grep -E 'libnix|libstubs|libamiga' build/$(1)/json.map | grep -v -E '^LOAD |libnix13.a\(__u?(mul|div|mod)si[34].o\)' \
	    || (echo "C library code got into $$@"; false)
endef
$(foreach c,$(CPUS),$(eval $(call LIBRARY,$(c))))

# the 060 one must not have the 64-bit forms of muls.l/mulu.l/divs.l/divu.l
check060: build/060/json.library
	@! $(OBJDUMP) -m m68k:68060 -d $(OBJS:%=build/060/%.o) | grep -E '\s(mul|div)[su]l\s+[^ ]*,%?d[0-7],%?d[0-7]' \
	    || (echo "build/060 has a 64-bit multiply or divide"; false)
	@echo "build/060: no 64-bit multiply or divide"

# the Amiga test: every call through the library (tests/jsontest.c)
build/jsontest: tests/jsontest.c include/libraries/json.h include/inline/json.h
	$(CC) $(CFLAGS) -s -o $@ $<

headers: json.sfd
	$(SFDC) --mode=clib --output=include/clib/json_protos.h json.sfd
	$(SFDC) --mode=proto --output=include/proto/json.h json.sfd
	$(SFDC) --target=m68k-amigaos --mode=macros --output=include/inline/json.h json.sfd

pctest:
	@mkdir -p build/pc
	$(PCCC) -o build/pc/unit tests/unit.c src/json.c
	build/pc/unit
	$(PCCC) -o build/pc/suite tests/suite.c src/json.c
	@if [ -n "$(SUITE)" ]; then build/pc/suite $(SUITE); else echo "(SUITE= not given: JSONTestSuite skipped)"; fi

fuzz:
	@mkdir -p build/pc/corpus
	clang -g -O1 -Itests/pcinc -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all \
	    -o build/pc/fuzz tests/fuzz.c src/json.c
	build/pc/fuzz build/pc/corpus -max_total_time=$(FUZZTIME) -rss_limit_mb=1024 -max_len=8192

clean:
	rm -rf build

.PHONY: all headers clean check060 pctest fuzz

# the Markdown docs as the Amiga reads them: docs/json.doc (Autodocs),
# docs/json_guide.doc and docs/json.guide (AmigaGuide), Latin-1
docs: docs/guide.md docs/reference.md ../tools/mkdocs.py
	python3 -I ../tools/mkdocs.py . docs

.PHONY: docs
