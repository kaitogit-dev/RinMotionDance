#!/usr/bin/env python3
"""Wave3 Task A: replace hex Win32 message constants with verified macros.

All mappings verified against Windows SDK 10.0.26100.0 headers
(um/winuser.h, um/commctrl.h) on 2026-09-03:

  0x00B1 EM_SETSEL        0x0143 CB_ADDSTRING      0x0180 LB_ADDSTRING
  0x00C2 EM_REPLACESEL    0x0144 CB_DELETESTRING   0x0181 LB_INSERTSTRING
  0x00F0 BM_GETCHECK      0x0146 CB_GETCOUNT       0x0182 LB_DELETESTRING
  0x00F1 BM_SETCHECK      0x0147 CB_GETCURSEL      0x0186 LB_SETCURSEL
  0x0111 WM_COMMAND       0x0148 CB_GETLBTEXT      0x0188 LB_GETCURSEL
  0x0115 WM_VSCROLL       0x014A CB_INSERTSTRING   0x0189 LB_GETTEXT
                          0x014B CB_RESETCONTENT
  0x0400 TBM_GETPOS       0x014E CB_SETCURSEL
  0x0405 TBM_SETPOS
  0x0407 TBM_SETRANGEMIN  0x0414 TBM_SETTICFREQ
  0x0408 TBM_SETRANGEMAX

Only the message ARGUMENT of SendMessage/PostMessage/SendDlgItemMessage is
rewritten (control IDs like GetDlgItem(hwnd, 0x1D7) are never touched --
the parser walks argument lists).  An inline /*MACRO*/ comment immediately
following the hex token is dropped (the information now lives in the macro
name).  Trailing // MACRO comments that merely repeat the inserted macro
are also dropped.
"""
import re
import sys
import pathlib

MAP = {
    '0xB1': 'EM_SETSEL', '0xC2': 'EM_REPLACESEL',
    '0xF0': 'BM_GETCHECK', '0xF1': 'BM_SETCHECK',
    '0x111': 'WM_COMMAND', '0x115': 'WM_VSCROLL',
    '0x143': 'CB_ADDSTRING', '0x144': 'CB_DELETESTRING',
    '0x146': 'CB_GETCOUNT', '0x147': 'CB_GETCURSEL',
    '0x148': 'CB_GETLBTEXT', '0x14A': 'CB_INSERTSTRING',
    '0x14B': 'CB_RESETCONTENT', '0x14E': 'CB_SETCURSEL',
    '0x180': 'LB_ADDSTRING', '0x181': 'LB_INSERTSTRING',
    '0x182': 'LB_DELETESTRING', '0x186': 'LB_SETCURSEL',
    '0x188': 'LB_GETCURSEL', '0x189': 'LB_GETTEXT',
    '0x400': 'TBM_GETPOS', '0x405': 'TBM_SETPOS',
    '0x407': 'TBM_SETRANGEMIN', '0x408': 'TBM_SETRANGEMAX',
    '0x414': 'TBM_SETTICFREQ',
}

CALL = re.compile(r'(SendDlgItemMessage|SendMessage|PostMessage)(A|W)?\s*\(')
HEX_TOKEN = re.compile(r'0[xX][0-9A-Fa-f]+[uUlL]*')


def skip_ws_comments(text, i):
    """Return index past whitespace and /*..*/ or //.. comments starting at i."""
    n = len(text)
    while i < n:
        c = text[i]
        if c in ' \t\r\n':
            i += 1
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j
            continue
        break
    return i


def match_paren(text, open_pos):
    """Index of the ')' matching the '(' at open_pos, comment/string aware."""
    depth = 0
    i = open_pos
    n = len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1:i + 2] == '*':
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j
            continue
        if c == '"':
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == '"':
                    break
                j += 1
            i = j + 1
            continue
        if c == "'":
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == "'":
                    break
                j += 1
            i = j + 1
            continue
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def split_args(text, start, end):
    """Top-level comma spans of text[start:end] (comment/string aware).

    Yields (a_start, a_end) spans covering each argument (comments inside
    an argument span are kept; callers strip them when tokenizing).
    """
    spans = []
    depth = 0
    i = start
    arg_start = start
    n = end
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j
            continue
        if c in '"\'':
            q = c
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == q:
                    break
                j += 1
            i = j + 1
            continue
        if c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        elif c == ',' and depth == 0:
            spans.append((arg_start, i))
            arg_start = i + 1
        i += 1
    spans.append((arg_start, n))
    return spans


def hex_key(token):
    t = token.rstrip('uUlL')
    v = int(t, 16)
    return f'0x{v:X}'


def process(path):
    text = path.read_text(encoding='utf-8', errors='surrogateescape')
    edits = []  
    stats = {}
    for m in CALL.finditer(text):
        open_pos = m.end() - 1
        close = match_paren(text, open_pos)
        if close < 0:
            continue
        base = m.group(1)
        arg_spans = split_args(text, open_pos + 1, close)
        mi = 2 if base == 'SendDlgItemMessage' else 1
        if len(arg_spans) <= mi:
            continue
        s, e = arg_spans[mi]
        stripped = re.sub(r'/\*.*?\*/', ' ', text[s:e], flags=re.S)
        stripped = re.sub(r'//[^\n]*', ' ', stripped)
        stripped = stripped.strip()
        if not HEX_TOKEN.fullmatch(stripped):
            continue
        key = hex_key(stripped)
        if key not in MAP:
            continue
        macro = MAP[key]
        h = HEX_TOKEN.search(re.sub(r'/\*.*?\*/', lambda _: ' ' * (len(_.group(0))), text[s:e], flags=re.S))
        if not h:
            continue
        tok_s = s + h.start()
        tok_e = s + h.end()
        k = tok_e
        while k < e and text[k] in ' \t':
            k += 1
        end = tok_e
        if k < e and text.startswith('/*', k):
            j = text.find('*/', k)
            if j != -1 and text[j + 2:k].strip() == '':
                rest = text[j + 2:e]
                if rest.strip() == '':
                    end = j + 2
        edits.append((tok_s, end, macro))
        stats[macro] = stats.get(macro, 0) + 1
    if not edits:
        return None, {}
    for s, e, rep in sorted(edits, key=lambda t: -t[0]):
        text = text[:s] + rep + text[e:]
    out_lines = []
    for ln in text.split('\n'):
        mm = re.search(r'\s+//\s*([A-Z_0-9]+(?:\s*\|\s*[A-Z_0-9]+)*)\s*$',
                       ln.rstrip('\r'))
        if mm:
            name = re.sub(r'\s+', '', mm.group(1))
            core = ln[:mm.start()]
            if re.search(r'\b' + re.escape(mm.group(1).strip()) + r'\b', core) and '|' not in name:
                ln = core.rstrip() + '\n' if ln.endswith('\n') else core.rstrip()
        out_lines.append(ln)
    text = '\n'.join(out_lines)
    path.write_text(text, encoding='utf-8', errors='surrogateescape')
    return len(edits), stats


def main():
    grand = {}
    total = 0
    for p in pathlib.Path('.').rglob('*'):
        if p.suffix.lower() not in ('.cpp', '.hpp', '.h', '.cc', '.inc'):
            continue
        s = str(p)
        parts = set(s.replace('/', '\\').split('\\'))
        if parts & {'build', 'build-x64', 'build-x86', '.git', 'tools'}:
            continue
        n, stats = process(p)
        if n:
            print(f'{s}: {n} edits  {stats}')
            total += n
            for k, v in stats.items():
                grand[k] = grand.get(k, 0) + v
    print(f'TOTAL: {total}')
    for k in sorted(grand, key=lambda k: -grand[k]):
        print(f'  {k}: {grand[k]}')


if __name__ == '__main__':
    main()
