#!/usr/bin/env bash
# run.sh [REV] - core.c's drawing half on Linux, the working copy against
# git revision REV (default HEAD): the same random screens through both
# (colours of every kind, attributes, wide characters, accents, a plain
# console and CTerm, screens of all different 24-bit colours) must send the
# terminal the same bytes. For a change inside the grid that should show
# nothing different (0.5b3's 6-byte cells were checked this way).
set -e
here=$(cd "$(dirname "$0")" && pwd); tui=$(cd "$here/../.." && pwd)
rev=${1:-HEAD}; tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/inc/exec" "$tmp/inc/dos" "$tmp/inc/proto"
for h in exec/memory.h exec/types.h dos/dosextens.h dos/dos.h proto/exec.h proto/dos.h proto/intuition.h; do
    echo '#include "amishim.h"' > "$tmp/inc/$h"
done
git -C "$tui" show "$rev:./src/core.c" > "$tmp/core_old.c"
for v in old new; do
    src=$([ $v = old ] && echo "$tmp/core_old.c" || echo "$tui/src/core.c")
    gcc -O1 -w -I"$tmp/inc" -I"$here/inc" -I"$tui/include" -DCORE="\"$src\"" "$here/drive.c" -o "$tmp/$v"
done
fail=0
for a in "1 1 0" "1 1 1" "1 1 2" "0 0 0" "0 0 1" "0 0 2" "1 0 1" "0 1 2"; do
    o=$("$tmp/old" $a 2000 | cut -d, -f1-2); n=$("$tmp/new" $a 2000 | cut -d, -f1-2)
    if [ "$o" = "$n" ]; then echo "same: $n"; else echo "DIFFERENT: $o / $n"; fail=1; fi
done
exit $fail
