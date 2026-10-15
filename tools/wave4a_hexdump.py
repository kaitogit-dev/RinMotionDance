import re, sys, struct

def extract(path, name):
    src = re.sub(r'//[^\n]*', '', open(path, encoding='utf-8').read())
    m = re.search(r'const\s+char\s+' + name + r'\s*\[\s*\]\s*=\s*((?:L?"(?:[^"\\]|\\.)*"\s*)+);', src)
    if not m:
        return None
    parts = re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))
    b = bytearray()
    for p in parts:
        i = 0
        while i < len(p):
            if p[i] == '\\':
                n = p[i+1]
                if n == 'x':
                    j = i+2; h=''
                    while j < len(p) and len(h) < 2 and p[j] in '0123456789abcdefABCDEF':
                        h += p[j]; j += 1
                    b.append(int(h,16)); i = j
                elif n in '01234567':
                    j = i+1; o=''
                    while j < len(p) and len(o) < 3 and p[j] in '01234567':
                        o += p[j]; j += 1
                    b.append(int(o,8)); i = j
                else:
                    b.append({'n':10,'t':9,'r':13,'\\':92,'"':34,'a':7}[n]); i += 2
            else:
                b.append(ord(p[i]) & 0xFF); i += 1
    return bytes(b)

EXE = r'C:\Users\jstzw\Documents\github\OpenMMD\MikuMikuDanceE_v932\MikuMikuDance.exe'

def load_sections():
    exe = open(EXE,'rb').read()
    pe = struct.unpack_from('<I', exe, 0x3C)[0]
    nsec = struct.unpack_from('<H', exe, pe+6)[0]
    opt = struct.unpack_from('<H', exe, pe+20)[0]
    secs = []
    for i in range(nsec):
        off = pe+24+opt+i*40
        vsz, va, rsz, ro = struct.unpack_from('<IIII', exe, off+8)
        secs.append((va, vsz, ro, rsz))
    return exe, secs

def rva2off(secs, rva):
    for va, vsz, ro, rsz in secs:
        if va <= rva < va+max(vsz,rsz):
            return ro + (rva-va)
    raise ValueError(hex(rva))

def rd_cstr(exe, secs, rva):
    o = rva2off(secs, rva); e = exe.index(b'\0', o)
    return exe[o:e]

NAMES = [('kJpChainCapDisp',0x12D770), ('kJpChainFmtPhys',0x12D788),
         ('kJpChainFmtDisp',0x12D848), ('kJpCannotOpenAcc',0x12DAE0)]

exe, secs = load_sections()
for nm, rva in NAMES:
    orig = rd_cstr(exe, secs, rva)
    a = extract(r'src\io\pmm_load_v1.cpp', nm)
    b = extract(r'src\io\pmm_load_v2.cpp', nm)
    print(f'== {nm} @VA 0x{rva+0x400000:X}: orig={len(orig)}B v1={len(a) if a else 0}B v2={len(b) if b else 0}B')
    print('   orig hex: ' + ' '.join(f'{x:02X}' for x in orig))
    for tag, arr in (('v1',a),('v2',b)):
        if arr is None:
            print(f'   {tag}: MISSING'); continue
        def g(x, i):
            return f'{x[i]:02X}' if i < len(x) else '--'
        diffs = [f'@{i}:{tag}={g(arr,i)},orig={g(orig,i)}' for i in range(max(len(arr),len(orig)))
                 if (arr[i] if i<len(arr) else None) != (orig[i] if i<len(orig) else None)]
        print(f'   {tag}: ' + ('EXACT MATCH' if not diffs else ' '.join(diffs)))
