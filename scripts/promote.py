#!/usr/bin/env python3
"""Offset-promotion ledger and coverage report for the raw<T>(offset) cleanup.

The port reaches the application state blob through
``app->raw<T>(offsets::kXxx)`` / ``app->at(offset)``.  The cleanup campaign
promotes each byte offset to a real named member of ``MMDAppState``
(include/rinmotiondance/app_layout.hpp) and rewrites the call sites, until
``raw<`` / ``at(`` disappear entirely and offsets.hpp / offsets_xlate.hpp are
deleted.

Modes:
  report   remaining raw< / at( call-site counts (the convergence metric)
  ledger   per-constant worklist: usage class, counts, accessor name
           (constants grouped by byte offset; aliases visible)
  apply    codemod: rewrite raw<T>(ns::kX) / at(ns::kX) call sites to the
           member expressions given in a map file (lines of
           ``kConstantName = expression-with-state.``).  Only exact-offset
           call forms are rewritten; ``kX + i`` family arithmetic is left
           alone.  After the sweep the script reports any map constant
           still referenced anywhere.

Usage:
  python scripts/promote.py report
  python scripts/promote.py ledger [--class dead|accessor-only|src-raw]
  python scripts/promote.py apply --map cluster_a.map
"""
import argparse
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INCLUDE = os.path.join(ROOT, "include")
SRC = os.path.join(ROOT, "src")

# ---------------------------------------------------------------------------
# source harvesting
# ---------------------------------------------------------------------------

def iter_sources():
    for base in (SRC, os.path.join(INCLUDE, "rinmotiondance")):
        for dirpath, _, files in os.walk(base):
            for f in files:
                if f.endswith((".cpp", ".hpp", ".inc")):
                    yield os.path.join(dirpath, f)


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return fh.read()


CONST_RE = re.compile(
    r"^\s*constexpr\s+std::size_t\s+(k\w+)\s*=\s*(\d+)\s*;\s*(?://\s*(.*))?$",
    re.M)


def parse_offsets():
    """[(name, decimal_value, comment)] from offsets.hpp, in file order."""
    text = read(os.path.join(INCLUDE, "rinmotiondance", "offsets.hpp"))
    return [(m.group(1), int(m.group(2)), m.group(3) or "")
            for m in CONST_RE.finditer(text)]


def usage_index():
    """{constant_name: {"mmd_app": n, "src": n, "other_include": n, "files": set}}"""
    usage = defaultdict(lambda: {"mmd_app": 0, "src": 0,
                                 "other_include": 0, "files": set()})
    word = {}
    for name, _, _ in parse_offsets():
        word[name] = re.compile(r"\b" + re.escape(name) + r"\b")
    skip = {"include/rinmotiondance/offsets.hpp"}
    for path in iter_sources():
        text = read(path)
        rel = os.path.relpath(path, ROOT).replace("\\", "/")
        if rel in skip:
            continue  # a constant's own definition is not a usage
        is_app = rel == "include/rinmotiondance/mmd_app.hpp"
        in_src = rel.startswith("src")
        for name, rx in word.items():
            n = len(rx.findall(text))
            if n:
                bucket = ("mmd_app" if is_app
                          else "src" if in_src else "other_include")
                usage[name][bucket] += n
                usage[name]["files"].add(rel)
    return usage


# ---------------------------------------------------------------------------
# report mode
# ---------------------------------------------------------------------------

def cmd_report():
    per_file = []
    total_raw = total_at = 0
    for path in iter_sources():
        text = read(path)
        n_raw = len(re.findall(r"\braw<", text))
        n_at = len(re.findall(r"->at\(|\.at\(|\bat\(", text))
        if n_raw or n_at:
            per_file.append((n_raw + n_at, n_raw, n_at,
                             os.path.relpath(path, ROOT)))
            total_raw += n_raw
            total_at += n_at
    per_file.sort(reverse=True)
    print(f"{'raw<':>6} {'at(':>5}  file")
    for tot, n_raw, n_at, rel in per_file:
        print(f"{n_raw:>6} {n_at:>5}  {rel}")
    print(f"\ntotal: {total_raw} raw< + {total_at} at( "
          f"= {total_raw + total_at} sites in {len(per_file)} files")
    return 0


# ---------------------------------------------------------------------------
# ledger mode
# ---------------------------------------------------------------------------

def classify(entry):
    if entry["mmd_app"] == 0 and entry["src"] == 0 \
            and entry["other_include"] == 0:
        return "dead"
    if entry["src"] == 0 and entry["other_include"] == 0 \
            and entry["mmd_app"] > 0:
        return "accessor-only"
    return "src-raw"


def cmd_ledger(want_class=None):
    usage = usage_index()
    by_value = defaultdict(list)
    for name, value, comment in parse_offsets():
        by_value[value].append((name, comment))

    rows = []
    for value in sorted(by_value):
        for name, comment in by_value[value]:
            e = usage[name]
            cls = classify(e)
            if want_class and cls != want_class:
                continue
            rows.append((value, name, cls, e["mmd_app"], e["src"],
                         e["other_include"], comment))

    print(f"{'offset':>8}  {'class':<14} {'acc':>4} {'src':>4} {'inc':>4}  name")
    for value, name, cls, acc, src, inc, comment in rows:
        note = f"  // {comment}" if comment else ""
        print(f"{value:>8}  {cls:<14} {acc:>4} {src:>4} {inc:>4}  {name}{note}")

    n = defaultdict(int)
    for _, _, cls, *_ in rows:
        n[cls] += 1
    print(f"\ncounts: {dict(n)} (of {len(parse_offsets())} constants)")
    return 0


def cmd_apply(map_path):
    mapping = {}
    with open(map_path, encoding="utf-8") as fh:
        for ln in fh:
            ln = ln.split("#")[0].strip()
            if not ln:
                continue
            name, _, expr = ln.partition("=")
            mapping[name.strip()] = expr.strip()
    # expand aliases: any constant sharing a mapped constant's byte offset
    by_value = defaultdict(list)
    for name, value, _ in parse_offsets():
        by_value[value].append(name)
    expanded = dict(mapping)
    for name, expr in mapping.items():
        value = next((v for n, v, _ in parse_offsets() if n == name), None)
        if value is None:
            print(f"  WARN: {name} not in offsets.hpp")
            continue
        for alias in by_value[value]:
            expanded.setdefault(alias, expr)

    changed = {}
    for path in iter_sources():
        text = read(path)
        orig = text
        for name, expr in expanded.items():
            # receiver-attached or bare call forms; exact offset only;
            # \s* lets multi-line calls (constant on the next line) match
            ns = r'\s*(?:rinmotiondance::)?(?:offsets|off)::'
            for pat, rep in (
                (r'(?:->|\.)?raw<[^>()+]*>\(' + ns + name + r'\s*\)', expr),
                (r'(?:->|\.)?at\(' + ns + name + r'\s*\)', expr),
            ):
                text = re.sub(pat, lambda m, e=expr: _attach(m.group(0), e), text)
        if text != orig:
            open(path, 'w', encoding='utf-8', newline='').write(text)
            changed[os.path.relpath(path, ROOT)] = sum(
                1 for name in expanded
                if re.search(r'\b' + name + r'\b', orig))

    for rel in sorted(changed):
        print(f"  {rel}")
    left = [n for n in expanded
            if any(re.search(r'\b' + n + r'\b', read(p))
                   for p in iter_sources())]
    print(f"\nrewrote {len(changed)} files; still referenced: {left}")
    return 0


def _attach(matched, expr):
    """expr like ``state.foo`` -> ``->state.foo`` / ``.state.foo`` / bare.

    Cast expressions (``reinterpret_cast<T&>(state.x)``) cannot take a
    receiver prefix; the receiver goes inside the parentheses instead.
    """
    m = re.match(r'reinterpret_cast(<[^()]*>)\((.*)$', expr, re.S)
    if m:
        inner = m.group(2)
        if matched.startswith(('->', '.')):
            inner = matched + inner
        return f'reinterpret_cast{m.group(1)}({inner}'
    if matched.startswith('->'):
        return '->' + expr
    if matched.startswith('.'):
        return '.' + expr
    return expr


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("cmd", choices=["report", "ledger", "apply"])
    ap.add_argument("--class", dest="klass",
                    choices=["dead", "accessor-only", "src-raw"])
    ap.add_argument("--map", dest="map_path")
    args = ap.parse_args()
    if args.cmd == "report":
        return cmd_report()
    if args.cmd == "apply":
        if not args.map_path:
            ap.error("apply requires --map FILE")
        return cmd_apply(args.map_path)
    return cmd_ledger(args.klass)


if __name__ == "__main__":
    sys.exit(main())
