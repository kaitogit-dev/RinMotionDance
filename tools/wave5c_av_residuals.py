import re, os, sys

ROOT = r'C:\Users\jstzw\Documents\github\RinMotionDance\src'
ID = re.compile(r'\b(?:a[1-9]|v[0-9]+)\b')

def strip_comments_strings(text):
    out = []
    i, n = 0, len(text)
    line = 1
    state = 'code'
    while i < n:
        c = text[i]
        nxt = text[i+1] if i+1 < n else ''
        if c == '\n':
            if state == 'line':
                state = 'code'
            line += 1
            out.append(c); i += 1; continue
        if state == 'code':
            if c == '/' and nxt == '/':
                state = 'line'; out.append('  '); i += 2; continue
            if c == '/' and nxt == '*':
                state = 'block'; out.append('  '); i += 2; continue
            if c == '"':
                state = 'str'; out.append(' '); i += 1; continue
            if c == "'":
                state = 'chr'; out.append(' '); i += 1; continue
            out.append(c); i += 1
        elif state == 'line':
            out.append(' '); i += 1
        elif state == 'block':
            if c == '*' and nxt == '/':
                state = 'code'; out.append('  '); i += 2
            else:
                out.append(' ' if c != '\n' else '\n'); i += 1
        elif state == 'str':
            if c == '\\':
                out.append('  '); i += 2
            elif c == '"':
                state = 'code'; out.append(' '); i += 1
            else:
                out.append(' '); i += 1
        elif state == 'chr':
            if c == '\\':
                out.append('  '); i += 2
            elif c == "'":
                state = 'code'; out.append(' '); i += 1
            else:
                out.append(' '); i += 1
    return ''.join(out)

hits = []
nfiles = 0
for dirpath, _, files in os.walk(ROOT):
    for fn in files:
        if not fn.endswith(('.cpp', '.hpp')):
            continue
        nfiles += 1
        path = os.path.join(dirpath, fn)
        text = open(path, encoding='utf-8', errors='replace').read()
        code = strip_comments_strings(text)
        if len(code) != len(text):
            print(f'!! LENGTH MISMATCH {fn}: {len(text)} -> {len(code)}', file=sys.stderr)
        for m in ID.finditer(code):
            ln = code.count('\n', 0, m.start()) + 1
            src_line = text.splitlines()[ln-1].strip()
            hits.append((path, ln, m.group(0), src_line))

for path, ln, tok, src in hits:
    rel = os.path.relpath(path, os.path.dirname(ROOT))
    print(f'{rel}:{ln}: [{tok}] {src[:150]}')
print(f'TOTAL code-token hits: {len(hits)}')
