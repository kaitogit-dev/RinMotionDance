#!/usr/bin/env python3
"""Wave3 scan: extract the true message argument from every SendMessage/PostMessage call."""
import re
import pathlib

root = pathlib.Path('.')
pat = re.compile(r'(SendDlgItemMessage|SendMessage|PostMessage)(A|W)?\s*\(', re.S)


def calls(text):
    for m in pat.finditer(text):
        i = m.end() - 1
        depth = 0
        start = i
        while i < len(text):
            c = text[i]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    break
            i += 1
        inner = text[start + 1:i]
        args, depth, cur, inq = [], 0, '', None
        j = 0
        while j < len(inner):
            c = inner[j]
            if inq:
                cur += c
                if c == inq and inner[j - 1] != chr(92):
                    inq = None
            elif c in '"\'' + chr(39):
                cur += c
                inq = c
            elif c in '([':
                depth += 1
                cur += c
            elif c in ')]':
                depth -= 1
                cur += c
            elif c == ',' and depth == 0:
                args.append(cur.strip())
                cur = ''
            else:
                cur += c
            j += 1
        args.append(cur.strip())
        yield m.start(), m.group(0), args


total_hex = {}
total_dec = {}
for p in root.rglob('*'):
    if p.suffix.lower() not in ('.cpp', '.hpp', '.h', '.cc', '.inc'):
        continue
    s = str(p)
    parts = set(s.replace('/', '\\').split('\\'))
    if parts & {'build', 'build-x64', 'build-x86', '.git'}:
        continue
    try:
        text = p.read_text(encoding='utf-8', errors='replace')
    except Exception:
        continue
    text2 = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text2 = re.sub(r'//[^\n]*', ' ', text2)
    for pos, matched, args in calls(text2):
        base = re.search(r'(\w+)\s*$', matched.rstrip('(')).group(1)
        stem = base.lower()
        if stem.endswith(('a', 'w')) and stem[:-1].endswith('message'):
            stem = stem[:-1]
        mi = 2 if stem == 'senddlgitemmessage' else 1
        if len(args) < mi + 1:
            continue
        msg = args[mi]
        hm = re.fullmatch(r'(0[xX][0-9A-Fa-f]+)([uUlL]*)', msg)
        dm = re.fullmatch(r'(\d+)([uUlL]*)', msg)
        line = text2.count('\n', 0, pos) + 1
        if hm:
            total_hex.setdefault(msg.upper(), []).append((s, line))
        elif dm and int(msg.rstrip('uUlL')) not in (0, 1):
            total_dec.setdefault(int(msg.rstrip('uUlL')), []).append(
                (s, line, f'{base}({args[0][:30]})'))

print("HEX message constants:")
grand = 0
for k in sorted(total_hex, key=lambda k: -len(total_hex[k])):
    print(f"  {k}: {len(total_hex[k])}")
    grand += len(total_hex[k])
print(f"  TOTAL: {grand}")
print("LOCATIONS:")
for k in sorted(total_hex):
    for f, l in total_hex[k]:
        print(f"  {k} {f}:{l}")
print("DEC (non-0/1) message constants:")
for k, v in sorted(total_dec.items()):
    for f, l, a in v:
        print(f"  {k}: {f}:{l} firstarg={a}")
