"""Wave5-C helper: rename identifiers ONLY in code regions (never inside
comments or string/char literals), preserving the file byte-for-byte
elsewhere (CRLF-safe: newline='' on both ends).

Usage: python wave5c_rename_code.py <file> <old> <new> [<old> <new> ...]
"""
import re, sys

def code_mask(text):
    """Return a boolean list: True where text[i] is live code."""
    n = len(text)
    mask = [True] * n
    i = 0
    state = 'code'
    while i < n:
        c = text[i]
        nxt = text[i+1] if i+1 < n else ''
        if state == 'code':
            if c == '/' and nxt == '/':
                state = 'line'
                mask[i] = mask[i+1] = False
                i += 2
                continue
            if c == '/' and nxt == '*':
                state = 'block'
                mask[i] = mask[i+1] = False
                i += 2
                continue
            if c == '"':
                state = 'str'
                mask[i] = False
                i += 1
                continue
            if c == "'":
                state = 'chr'
                mask[i] = False
                i += 1
                continue
            i += 1
            continue
        if state == 'line':
            if c == '\n':
                state = 'code'
            else:
                mask[i] = False
            i += 1
            continue
        if state == 'block':
            if c == '*' and nxt == '/':
                mask[i] = mask[i+1] = False
                state = 'code'
                i += 2
                continue
            if c != '\n':
                mask[i] = False
            i += 1
            continue
        if c == '\\':
            mask[i] = mask[i+1] if i+1 < n else False
            i += 2
            continue
        mask[i] = False
        if (state == 'str' and c == '"') or (state == 'chr' and c == "'"):
            state = 'code'
        i += 1
    return mask

def rename_code(path, mapping):
    text = open(path, encoding='utf-8', newline='').read()
    changed = 0
    for old, new in mapping:
        mask = code_mask(text)
        pat = re.compile(r'(?<![\w])' + re.escape(old) + r'(?![\w])')
        spots = [m for m in pat.finditer(text) if all(mask[m.start():m.end()])]
        for m in reversed(spots):
            text = text[:m.start()] + new + text[m.end():]
            changed += 1
    open(path, 'w', encoding='utf-8', newline='').write(text)
    return changed

if __name__ == '__main__':
    path = sys.argv[1]
    pairs = list(zip(sys.argv[2::2], sys.argv[3::2]))
    n = rename_code(path, pairs)
    print(f'{path}: {n} code-site replacements')
