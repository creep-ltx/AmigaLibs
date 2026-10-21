#!/usr/bin/env python3
"""mkdocs: a library's Markdown docs (docs/guide.md, docs/reference.md) as
the Amiga reads docs:

  NAME.doc        the reference as Autodocs, like Commodore's exec.doc
  NAME_guide.doc  the programmer's guide as plain text
  NAME.guide      both as one AmigaGuide database, the calls linked

All three are Latin-1, 74 columns. Characters Latin-1 lacks go through
utf8.library's own Utf8ToCharset (tools/tolatin1.c, built here for the
PC): their nearest, other scripts spelt in Latin letters. The Markdown
stays the master copy; run this again after every change.

  mkdocs.py LIBDIR OUTDIR        (e.g. mkdocs.py tui build/docs)

Only the Markdown these docs use: headings, paragraphs, - and 1. lists,
``` code, | tables, `code`, **bold**, *italic*, [links](...)."""
import os, re, subprocess, sys, unicodedata

W = 74
HERE = os.path.dirname(os.path.abspath(__file__))
TOLATIN1 = None


def tolatin1(text):
    """UTF-8 text to Latin-1 bytes through utf8.library's own code"""
    global TOLATIN1
    if TOLATIN1 is None:
        TOLATIN1 = os.path.join(os.environ.get('TMPDIR', '/tmp'), 'mkdocs-tolatin1')
        src = [os.path.join(HERE, 'tolatin1.c'), os.path.join(HERE, '../utf8/src/utf8.c'),
               os.path.join(HERE, '../utf8/src/tables.c')]
        subprocess.run(['cc', '-std=c99', '-D_DEFAULT_SOURCE', '-O2', '-w',
                        '-I' + os.path.join(HERE, '../utf8/tests/pcinc'), '-o', TOLATIN1] + src, check=True)
    return subprocess.run([TOLATIN1], input=text.encode('utf-8'), capture_output=True, check=True).stdout


def slug(s):
    """GitHub's heading anchor"""
    s = re.sub(r'[`*]', '', s).strip().lower()
    s = re.sub(r'[^\w\- ]', '', s)
    return s.replace(' ', '-')


# ---- blocks ------------------------------------------------------------

def blocks(md):
    """Markdown into (kind, data): h (level, text), p text, ul/ol [items],
    code [lines], table [rows of cells], label text (a **Bold** line)"""
    out, lines, i = [], md.split('\n'), 0
    while i < len(lines):
        l = lines[i]
        if not l.strip():
            i += 1
        elif l.startswith('```'):
            j = i + 1
            while j < len(lines) and not lines[j].startswith('```'):
                j += 1
            out.append(('code', lines[i + 1:j]))
            i = j + 1
        elif l.startswith('    ') and (not out or out[-1][0] != 'p'):
            j = i
            while j < len(lines) and (lines[j].startswith('    ') or not lines[j].strip()):
                j += 1
            code = [x[4:] for x in lines[i:j]]
            while code and not code[-1].strip():
                code.pop()
            out.append(('code', code))
            i = j
        elif l.startswith('#'):
            m = re.match(r'(#+)\s*(.*)', l)
            out.append(('h', (len(m.group(1)), m.group(2).strip())))
            i += 1
        elif l.startswith('|'):
            rows = []
            while i < len(lines) and lines[i].startswith('|'):
                cells = [c.strip() for c in re.split(r'(?<!\\)\|', lines[i].strip()[1:-1])]
                if not all(re.fullmatch(r':?-+:?', c) for c in cells):
                    rows.append([c.replace('\\|', '|') for c in cells])
                i += 1
            out.append(('table', rows))
        elif re.match(r'\s*([-*]|\d+\.)\s', l):
            ordered = bool(re.match(r'\s*\d+\.', l))
            items = []
            while i < len(lines) and lines[i].strip():
                m = re.match(r'\s*([-*]|\d+\.)\s+(.*)', lines[i])
                if m and len(lines[i]) - len(lines[i].lstrip()) < 2:
                    items.append(m.group(2))
                elif m:                         # a nested item: kept inside its parent
                    items[-1] += ' / ' + m.group(2)
                else:
                    items[-1] += ' ' + lines[i].strip()
                i += 1
            out.append(('ol' if ordered else 'ul', items))
        elif re.fullmatch(r'\*\*[^*]+\*\*', l.strip()):
            out.append(('label', l.strip()[2:-2]))
            i += 1
        else:
            j = i
            while j < len(lines) and lines[j].strip() and not re.match(r'(#|```|\||\s*([-*]|\d+\.)\s)', lines[j]):
                j += 1
            out.append(('p', ' '.join(x.strip() for x in lines[i:j])))
            i = j
    return out


# ---- inline -------------------------------------------------------------

TOKEN = re.compile(r'`([^`]*)`|\*\*(.+?)\*\*|(?<![\w*])\*([^*\s][^*]*?)\*(?![\w*])|\[([^\]]+)\]\(([^)]+)\)')


def spans(text):
    """inline Markdown into (text, style, link) pieces; style '', 'b', 'i', 'c'"""
    out, pos = [], 0
    for m in TOKEN.finditer(text):
        if m.start() > pos:
            out.append((text[pos:m.start()], '', None))
        if m.group(1) is not None:
            out.append((m.group(1), 'c', None))
        elif m.group(2) is not None:
            out += [(t, 'b' if s == '' else s, l) for t, s, l in spans(m.group(2))]
        elif m.group(3) is not None:
            out.append((m.group(3), 'i', None))
        else:
            out.append((plain(m.group(4)), '', m.group(5)))
        pos = m.end()
    if pos < len(text):
        out.append((text[pos:], '', None))
    return out


def plain(text):
    return ''.join(t for t, _, _ in spans(text))


def words(text):
    """pieces into words for wrapping, each a list of (text, style, link);
    a link stays one word"""
    res = [[]]
    for t, s, l in spans(text):
        if l is not None:
            res[-1].append((t, s, l))
            continue
        for k, part in enumerate(t.split(' ')):
            if k:
                res.append([])
            if part:
                res[-1].append((part, s, None))
    return [w for w in res if w]


def wrap(text, width, render, indent='', first=None):
    """text wrapped to width (visible), each word through render"""
    first = indent if first is None else first
    lines, cur, n = [], first, len(first)
    for word in words(text):
        vis = sum(len(t) for t, _, _ in word)
        if n > len(indent) and n + 1 + vis > width and cur.strip():
            lines.append(cur)
            cur, n = indent, len(indent)
        elif n > len(first if not lines else indent):
            cur += ' '
            n += 1
        cur += ''.join(render(t, s, l) for t, s, l in word)
        n += vis
    lines.append(cur)
    return lines


# ---- rendering, text and AmigaGuide ---------------------------------------

class Out:
    """how one format writes inline pieces and code"""
    guide = False

    def __init__(self, resolve=None):
        self.resolve = resolve or (lambda target: None)

    def esc(self, s):
        return s

    def piece(self, t, s, l):
        if l is not None and re.match(r'https?://', l) and plain(t) != l:
            return t + ' (' + l + ')'
        return t


class GuideOut(Out):
    guide = True
    STYLE = {'b': ('@{b}', '@{ub}'), 'i': ('@{i}', '@{ui}'), 'c': ('', '')}

    def esc(self, s):
        return s.replace('\\', '\\\\').replace('@', '\\@')

    def piece(self, t, s, l):
        if l is not None:
            node = self.resolve(l)
            if node:
                return '@{"' + t.replace('"', "'") + '" link "' + node + '"}'
            return Out.piece(self, self.esc(t), s, l)
        a, b = self.STYLE.get(s, ('', ''))
        return a + self.esc(t) + b if t.strip() else self.esc(t)


def render_blocks(bl, out, ind=''):
    """blocks into lines of text (or AmigaGuide)"""
    lines = []
    width = W
    for kind, data in bl:
        if kind == 'p':
            lines += wrap(data, width, out.piece, ind) + ['']
        elif kind == 'label':
            lines += [ind + (('@{b}' + out.esc(plain(data)) + '@{ub}') if out.guide else plain(data).upper()), '']
        elif kind in ('ul', 'ol'):
            for k, item in enumerate(data):
                mark = '- ' if kind == 'ul' else '%d. ' % (k + 1)
                lines += wrap(item, width, out.piece, ind + ' ' * (len(mark) + 2), ind + '  ' + mark)
            lines.append('')
        elif kind == 'code':
            pad = ind + ('    ' if len(ind) < 8 else '')       # Autodocs: code at 8, as their text
            lines += [(pad + out.esc(x)).rstrip() for x in data] + ['']
        elif kind == 'table':
            lines += table(data, out, ind) + ['']
        elif kind == 'h':
            level, text = data
            t = plain(text)
            lines += [ind + (('@{b}' + out.esc(t) + '@{ub}') if out.guide else t), ind + '-' * len(t), '']
    return lines


def table(rows, out, ind):
    """a table as columns when it fits, else each row as a small block"""
    n = max(len(r) for r in rows)
    rows = [r + [''] * (n - len(r)) for r in rows]
    wid = [max(len(plain(r[k])) for r in rows) for k in range(n)]

    def cell(c, head):
        if head:
            return ('@{b}' + out.esc(plain(c)) + '@{ub}') if out.guide else plain(c)
        return ''.join(out.piece(t, s, l) for t, s, l in spans(c)) if out.guide else plain(c)

    lines = []
    if len(ind) + 2 + sum(wid) + 2 * (n - 1) <= W:
        for k, r in enumerate(rows):
            lines.append((ind + '  ' + '  '.join(cell(c, k == 0) + ' ' * (wid[i] - len(plain(c)))
                                                 for i, c in enumerate(r))).rstrip())
            if k == 0:
                lines.append(ind + '  ' + '  '.join('-' * w for w in wid))
        return lines
    head = rows[0]
    for r in rows[1:]:
        lines += wrap(r[0], W, out.piece, ind + '    ', ind + '  ')
        for i in range(1, n):
            if not r[i]:
                continue
            label = plain(head[i]) if n > 2 and plain(head[i]) else ''
            lines += wrap((label + ': ' if label else '') + r[i], W, out.piece, ind + '      ', ind + '    ')
    return lines


# ---- the reference as Autodocs -------------------------------------------

def sections(md):
    """reference.md into (name, blocks) per ## section"""
    res, cur = [], None
    for b in blocks(md):
        if b[0] == 'h' and b[1][0] == 2:
            cur = (plain(b[1][1]), [])
            res.append(cur)
        elif b[0] == 'h' and b[1][0] == 1:
            continue
        elif cur:
            cur[1].append(b)
    return res


def autodoc_parts(bl):
    """a call's blocks into Autodoc parts: [(HEADING, blocks)]"""
    parts, cur = [], None
    first = True
    for b in bl:
        if b[0] == 'p' and first:
            parts.append(('NAME', [b]))
            first = False
            continue
        first = False
        if b[0] == 'code' and not any(p[0] == 'SYNOPSIS' for p in parts) and len(parts) == 1:
            parts.append(('SYNOPSIS', [b]))
            continue
        if b[0] == 'label':
            cur = (plain(b[1]).upper(), [])
            parts.append(cur)
            continue
        if b[0] == 'p' and plain(b[1]).lower().startswith('see also'):
            parts.append(('SEE ALSO', [('p', re.sub(r'^\W*see also\W*', '', b[1], flags=re.I))]))
            cur = None
            continue
        if cur is None or cur[0] in ('NAME', 'SYNOPSIS', 'SEE ALSO'):
            cur = ('FUNCTION', [])
            parts.append(cur)
        cur[1].append(b)
    return parts


def autodoc_entry(lib, name, bl, out):
    """one call in Autodoc layout: headings at 3 spaces, text at 8"""
    lines = []
    for head, pb in autodoc_parts(bl):
        lines.append('   ' + (('@{b}' + head + '@{ub}') if out.guide else head))
        if head == 'NAME':
            lines += wrap(name + ' -- ' + plain(pb[0][1]), W, lambda t, s, l: out.esc(t), ' ' * 12, ' ' * 8)
            lines.append('')
        else:
            lines += render_blocks(pb, out, ' ' * 8)
    while lines and not lines[-1].strip():
        lines.pop()
    return lines


def autodoc(lib, ref):
    secs = sections(ref)
    names = []
    for name, _ in secs:
        names.append(lib + '/' + (name if re.fullmatch(r'\w+', name) else '--' + slug(name) + '--'))
    out = ['TABLE OF CONTENTS', ''] + names
    for (name, bl), full in zip(secs, names):
        out += ['@@FORMFEED@@', full + ' ' * max(1, W + 4 - 2 * len(full)) + full, '']
        out += autodoc_entry(lib, name, bl, Out())
        out.append('')
    return '\n'.join(x.rstrip() for x in out) + '\n'


# ---- the guide as text ---------------------------------------------------

def guidetext(guide):
    bl = blocks(guide)
    lines = []
    for kind, data in bl:
        if kind == 'h' and data[0] == 1:
            t = plain(data[1])
            lines += [t, '=' * len(t), '']
        elif kind == 'h' and data[0] == 2:
            t = plain(data[1]).upper()
            lines += ['', '', t, '-' * len(t), '']
        else:
            lines += render_blocks([(kind, data)], Out())
    return '\n'.join(x.rstrip() for x in lines) + '\n'


# ---- both as AmigaGuide ----------------------------------------------------

def amigaguide(lib, guide, ref, title, note=''):
    gbl = blocks(guide)
    secs = sections(ref)
    calls = {name: ('ref-' + slug(name) if not re.fullmatch(r'\w+', name) else name) for name, _ in secs}
    # the guide's chapters: ## headings; ### ones belong to their chapter
    chapters, intro, cur = [], [], None
    anchor = {}
    for b in gbl:
        if b[0] == 'h' and b[1][0] == 1:
            continue
        if b[0] == 'h' and b[1][0] == 2:
            cur = ('g-' + slug(b[1][1]), plain(b[1][1]), [])
            chapters.append(cur)
            anchor[slug(b[1][1])] = cur[0]
            continue
        if b[0] == 'h' and cur:
            anchor[slug(b[1][1])] = cur[0]
        (cur[2] if cur else intro).append(b)

    def resolve(target):
        f, _, frag = target.partition('#')
        f = os.path.basename(f)
        if f in ('', 'guide.md') and frag in anchor:
            return anchor[frag]
        if f == 'guide.md' and not frag:
            return 'main'
        if f == 'reference.md' or (f == '' and frag):
            for name, node in calls.items():
                if slug(name) == frag:
                    return node
            if f == 'reference.md':
                return 'reference'
        return None

    out = GuideOut(resolve)
    callre = re.compile(r'\b(' + '|'.join(re.escape(n) for n in calls if re.fullmatch(r'\w+', n)) + r')\(\)') if calls else None

    def linkcalls(lines, own=None):
        """TuiFlush() in the text becomes a link (not in code, not to itself)"""
        if not callre:
            return lines
        res = []
        for x in lines:
            if '@{' in x and 'link' in x:
                res.append(x)
                continue
            res.append(callre.sub(lambda m: m.group(0) if m.group(1) == own else
                                  '@{"' + m.group(0) + '" link "' + calls[m.group(1)] + '"}', x))
        return res

    base = lib.split('.')[0]
    g = ['@database ' + base + '.guide', '@$VER: ' + base + '.guide ' + title,
         '@author Tobias Karlsson', '@wordwrap', '']
    g += ['@node main "' + lib + '"', '']
    head = plain(next((b[1][1] for b in gbl if b[0] == 'h' and b[1][0] == 1), lib))
    g += ['@{b}' + out.esc(head) + '@{ub}', '']
    if note:
        g += note.rstrip('\n').split('\n') + ['']
    # the intro without its own list of links (the menu below replaces it)
    g += linkcalls(render_blocks([b for b in intro if b[0] != 'ul'], out))
    g += ['@{b}The programmer\'s guide@{ub}', '']
    g += ['  @{"' + c[1].replace('"', "'") + '" link "' + c[0] + '"}' for c in chapters]
    g += ['', '@{b}Reference@{ub}', '', '  @{"Every call" link "reference"}', '@endnode', '']
    for k, (node, name, bl) in enumerate(chapters):
        g += ['@node ' + node + ' "' + name.replace('"', "'") + '"']
        if k + 1 < len(chapters):
            g.append('@next ' + chapters[k + 1][0])
        if k:
            g.append('@prev ' + chapters[k - 1][0])
        g += ['', '@{b}' + out.esc(name) + '@{ub}', '']
        g += linkcalls(render_blocks(bl, out))
        g += ['@endnode', '']
    g += ['@node reference "' + lib + ' reference"', '', '@{b}Every call@{ub}', '']
    g += ['  @{"' + n + '" link "' + calls[n] + '"}' for n, _ in secs]
    g += ['@endnode', '']
    for k, (name, bl) in enumerate(secs):
        g += ['@node ' + calls[name] + ' "' + lib + '/' + name.replace('"', "'") + '"', '@toc reference']
        if k + 1 < len(secs):
            g.append('@next ' + calls[secs[k + 1][0]])
        if k:
            g.append('@prev ' + calls[secs[k - 1][0]])
        g += ['', '@{b}' + lib + '/' + out.esc(name) + '@{ub}', '']
        g += linkcalls(autodoc_entry(lib, name, bl, out), name)
        g += ['@endnode', '']
    text = '\n'.join(x.rstrip() for x in g) + '\n'
    text = text.replace('@{ub} @{b}', ' ').replace('@{ui} @{i}', ' ')     # one run, not a word at a time
    return text.replace('@wordwrap\n', '')     # pre-wrapped: no @wordwrap


def main():
    libdir, outdir = sys.argv[1], sys.argv[2]
    lib = os.path.basename(os.path.abspath(libdir))
    guide = open(os.path.join(libdir, 'docs/guide.md'), encoding='utf-8').read()
    ref = open(os.path.join(libdir, 'docs/reference.md'), encoding='utf-8').read()
    ver = ''
    vh = os.path.join(libdir, 'src/version.h')
    if os.path.exists(vh):
        v = re.search(r'VERSTR\s+"([^"]+)"', open(vh).read())
        d = re.search(r'_DATE\s+"([^"]+)"', open(vh).read())
        ver = (v.group(1) + ' (' + d.group(1) + ')') if v and d else ''
    os.makedirs(outdir, exist_ok=True)
    name = lib + '.library'
    # where the Markdown has characters Latin-1 lacks, say where the originals are
    other = any(ord(c) > 255 and unicodedata.category(c).startswith('L') and
                not unicodedata.name(c, '').startswith('LATIN') for c in guide + ref)
    note = ('(Other scripts are spelt in Latin letters here, as the Amiga shows them;\n'
            'docs/guide.md and docs/reference.md have the original characters.)\n\n') if other else ''
    # the conversion drops control characters: the form feed between entries goes in after it
    open(os.path.join(outdir, lib + '.doc'), 'wb').write(
        tolatin1(note + autodoc(name, ref)).replace(b'@@FORMFEED@@', b'\x0c'))
    open(os.path.join(outdir, lib + '_guide.doc'), 'wb').write(tolatin1(note + guidetext(guide)))
    open(os.path.join(outdir, lib + '.guide'), 'wb').write(
        tolatin1(amigaguide(name, guide, ref, ver or lib, note.replace('\n\n', '\n'))))


main()
