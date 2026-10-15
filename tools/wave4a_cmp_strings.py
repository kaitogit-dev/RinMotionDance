import re, sys

def extract(path):
    src = open(path, encoding='utf-8').read()
    out = {}
    no_comment = re.sub(r'//[^\n]*', '', src)
    for m in re.finditer(
            r'const\s+(?:wchar_t|char)\s+(\w+)\s*\[\s*\]\s*=\s*((?:L?"(?:[^"\\]|\\.)*"\s*)+);',
            no_comment):
        name, body = m.group(1), m.group(2)
        wide = 'L"' == body[:2]
        parts = re.findall(r'L?"((?:[^"\\]|\\.)*)"', body)
        b = bytearray()
        for p in parts:
            i = 0
            while i < len(p):
                c = p[i]
                if c == '\\':
                    n = p[i+1]
                    if n == 'x':
                        j = i + 2
                        h = ''
                        while j < len(p) and len(h) < 2 and p[j] in '0123456789abcdefABCDEF':
                            h += p[j]; j += 1
                        b.append(int(h, 16)); i = j
                    elif n in '01234567':
                        j = i + 1; o = ''
                        while j < len(p) and len(o) < 3 and p[j] in '01234567':
                            o += p[j]; j += 1
                        b.append(int(o, 8)); i = j
                    else:
                        b.append({'n':10,'t':9,'r':13,'\\':92,'"':34,'a':7}[n]); i += 2
                else:
                    b.append(ord(c) & 0xFF); i += 1
            if wide:
                pass  
        if wide:
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
                for u in units:
                    b += u.to_bytes(2, 'little')
        out[name] = bytes(b)
    return out

v1 = extract(r'src\io\pmm_load_v1.cpp')
v2 = extract(r'src\io\pmm_load_v2.cpp')

print(f"v1 constants: {len(v1)}, v2 constants: {len(v2)}")
print("\n== same-named pairs ==")
ident, differ, only = [], [], []
for k in sorted(set(v1) & set(v2)):
    if v1[k] == v2[k]:
        ident.append(k)
    else:
        differ.append(k)
for k in sorted(set(v1) - set(v2)): only.append(('v1-only', k))
for k in sorted(set(v2) - set(v1)): only.append(('v2-only', k))
print("IDENTICAL:", ', '.join(ident))
print("DIFFER  :")
for k in differ:
    a, b = v1[k], v2[k]
    print(f"  {k}: v1 {len(a)}B v2 {len(b)}B")
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else None
        y = b[i] if i < len(b) else None
        if x != y:
            xs = f"{x:02X}" if x is not None else "--"
            ys = f"{y:02X}" if y is not None else "--"
            print(f"    @{i}: v1={xs} v2={ys}")
            print(f"    ctx v1: {a[max(0,i-6):i+6].hex(' ')}")
            print(f"    ctx v2: {b[max(0,i-6):i+6].hex(' ')}")
print("ONLY    :", only)
