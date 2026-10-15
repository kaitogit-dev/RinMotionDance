import re, subprocess, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def extract(src):
    src = re.sub(r'//[^\n]*', '', src)
    out = {}
    for m in re.finditer(
            r'inline\s+const\s+(wchar_t|char)\s+(\w+)\s*\[\s*\]\s*=\s*'
            r'((?:L?"(?:[^"\\]|\\.)*"\s*)+);|'
            r'const\s+(wchar_t|char)\s+(\w+)\s*\[\s*\]\s*=\s*'
            r'((?:L?"(?:[^"\\]|\\.)*"\s*)+);',
            src):
        typ = m.group(1) or m.group(4)
        name = m.group(2) or m.group(5)
        body = m.group(3) or m.group(6)
        wide = body.startswith('L')
        parts = re.findall(r'L?"((?:[^"\\]|\\.)*)"', body)
        b = bytearray()
        for p in parts:
            i = 0
            units = []
            while i < len(p):
                c = p[i]
                if c == '\\':
                    n = p[i+1]
                    if n == 'x':
                        j = i + 2; h = ''
                        while j < len(p) and len(h) < 4 and p[j] in '0123456789abcdefABCDEF':
                            h += p[j]; j += 1
                        units.append(int(h, 16)); i = j
                    elif n in '01234567':
                        j = i + 1; o = ''
                        while j < len(p) and len(o) < 3 and p[j] in '01234567':
                            o += p[j]; j += 1
                        units.append(int(o, 8)); i = j
                    else:
                        units.append({'n':10,'t':9,'r':13,'\\':92,'"':34,'a':7}[n]); i += 2
                else:
                    units.append(ord(c)); i += 1
            if wide:
                for u in units:
                    b += u.to_bytes(2, 'little')
            else:
                for u in units:
                    b.append(u & 0xFF)
        out[name] = bytes(b)
    return out

def git_show(path):
    return subprocess.run(['git', 'show', 'HEAD:' + path], cwd=ROOT,
                          capture_output=True, text=True, encoding='utf-8').stdout

hdr = extract(open(os.path.join(ROOT, 'src/io/pmm_io_common.hpp'),
                   encoding='utf-8').read())
v1 = extract(git_show('src/io/pmm_load_v1.cpp'))
v2 = extract(git_show('src/io/pmm_load_v2.cpp'))
sv = extract(open(os.path.join(ROOT, 'src/io/pmm_save.cpp'),
                  encoding='utf-8').read())

fails = 0
for name in sorted(hdr):
    h = hdr[name]
    for tag, other in (('v1', v1), ('v2', v2)):
        if name in other:
            if h != other[name]:
                fails += 1
                print(f'DIFFER {name}: header {len(h)}B vs HEAD-{tag} '
                      f'{len(other[name])}B')
            else:
                print(f'OK     {name} == HEAD-{tag} ({len(h)}B)')
        else:
            print(f'note   {name} not in HEAD-{tag} (fine)')

title = 'RinMotionDance [%s]'.encode('utf-16-le')
for tag, src, pat in (('v1-load', git_show('src/io/pmm_load_v1.cpp'),
                       'L"RinMotionDance [%s]"'),
                      ('v2-load', git_show('src/io/pmm_load_v2.cpp'),
                       'L"RinMotionDance [%s]"'),
                      ('save', git_show('src/io/pmm_save.cpp'),
                       'L"RinMotionDance [%s]"')):
    present = pat in src
    removed_now = pat not in {
        'v1-load': open(os.path.join(ROOT, 'src/io/pmm_load_v1.cpp'),
                        encoding='utf-8').read(),
        'v2-load': open(os.path.join(ROOT, 'src/io/pmm_load_v2.cpp'),
                        encoding='utf-8').read(),
        'save': open(os.path.join(ROOT, 'src/io/pmm_save.cpp'),
                     encoding='utf-8').read(),
    }[tag]
    ok = (hdr.get('kAppTitleFormat') == title) and present and removed_now
    print(('OK    ' if ok else 'FAIL  ') + f'title {tag}: HEAD had literal, '
          f'working tree uses kAppTitleFormat, bytes match')
    if not ok:
        fails += 1
print('RESULT:', 'ALL MOVED CONSTANTS BYTE-IDENTICAL' if fails == 0
      else f'{fails} FAILURES')

print()
for path in ('src/io/pmm_load_v1.cpp', 'src/io/pmm_load_v2.cpp'):
    cur = extract(open(os.path.join(ROOT, path), encoding='utf-8').read())
    old = extract(git_show(path))
    for name, val in sorted(cur.items()):
        if name in old:
            same = val == old[name]
            if not same:
                fails += 1
            print(('OK      ' if same else 'CHANGED!')
                  + f' kept {path.split("/")[-1]} {name} ({len(val)}B)')
print('KEPT STRINGS:', 'ALL UNCHANGED' if fails == 0 else f'{fails} FAILURES')
raise SystemExit(0 if fails == 0 else 1)
