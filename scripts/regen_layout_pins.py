#!/usr/bin/env python3
import re
import sys

SRC = "include/rinmotiondance/layout_pins.hpp"

EXPR_FIXES = {
    "v9da24": "copiedBoneCount",
    "v9da24[8]": "clipboardCounts.accessories",
    "v9e178": "lightDirection[1]",
    "v9e17c": "lightDirection[2]",
    "lightDirection": "lightDirection",  
    "v9e1a8": "lightColor[1]",
    "v9e1ac": "lightColor[2]",
    "lightColor": "lightColor",
    "a03E4": "globalTrackSelected",
    "a03E5": "globalTrackSelected[1]",
    "a03E6": "globalTrackSelected[2]",
    "a03E7": "globalTrackSelected[3]",
    "a03E8": "selectionActiveLatch",
    "fontSubOrPtr": "pathWorkspace",
}

with open(SRC, encoding="utf-8") as f:
    text = f.read()

pin_re = re.compile(
    r"static_assert\(offsetof\(MMDAppState,\s*([A-Za-z_]\w*(?:\[\d+\])?)\)"
    r"\s*==\s*(\d+),")
delta_re = re.compile(
    r"static_assert\(offsetof\(MMDAppState,\s*(\w+)\)\s*-\s*"
    r"offsetof\(MMDAppState,\s*(\w+)\)\s*==\s*(\d+),")

lines = text.splitlines()
split = next(i for i, l in enumerate(lines) if l.strip() == "#else")

x86, x64 = [], []
for i, l in enumerate(lines):
    m = pin_re.search(l)
    if m:
        (x86 if i < split else x64).append((m.group(1), int(m.group(2))))

flat = re.sub(r"\s+", " ", text)
d = delta_re.search(flat)
assert d, "delta pin not found"
delta = (d.group(1), d.group(2), int(d.group(3)))

def fix(expr):
    return EXPR_FIXES.get(expr, expr)

x86 = [(fix(e), o) for e, o in x86]
x64 = [(fix(e), o) for e, o in x64]
delta = (delta[0], fix(delta[1]), delta[2])

print(f"x86 pins: {len(x86)}  x64 anchors: {len(x64)}  delta: {delta}")

out = []
out.append("""\
// ===========================================================================
// RinMotionDance - layout pin wall (MMDAppState)
// ===========================================================================
// Regression guard for MMDAppState, split from app_layout.hpp so the
// struct definition reads like the original author's code:
//   * x86: every member pinned to the original binary's byte offset
//     (the ground truth this port mirrors; recovered from IDA + probes).
//     The wall below is ordered by offset, i.e. by struct order; one
//     RINMOTIONDANCE_APP_OFF line per pinned field (array elements get
//     their own line, e.g. dialogFlags[17]).
//   * x64: anchor members only - instruction-level ground truth
//     (vote-grade twin reads); the rest keep their spacing relative to
//     the last anchor, and the size is bounded near the original truth
//     (see app_layout.hpp).
// Included at the bottom of app_layout.hpp, inside namespace
// rinmotiondance - requires MMDAppState in scope.  Hand-maintained
// alongside the struct.  Architecture switches use the canonical
// #if defined(_M_X64) / #if !defined(_M_X64) forms.
// ===========================================================================
#pragma once

#include <cstddef>

#if !defined(_M_X64)
// ---- x86 wall: every field pinned to the original x86 binary ----------
#define RINMOTIONDANCE_APP_OFF(f, off)                                        \\
    static_assert(offsetof(MMDAppState, f) == (off),                          \\
                  #f " x86 offset must match the original binary")
""")
for e, o in sorted(x86, key=lambda t: t[1]):
    out.append(f"RINMOTIONDANCE_APP_OFF({e}, {o});")
out.append("""
#undef RINMOTIONDANCE_APP_OFF

// The inline path-resolution workspace (was fontSubOrPtr + 3535 pad bytes)
// must fill the blob region up to the next live field.
static_assert(offsetof(MMDAppState, leftViewportVertices) -
                  offsetof(MMDAppState, pathWorkspace) == %d,
              "pathWorkspace x86 span must end at the next app field");
#else
// ---- x64 anchors --------------------------------------------------------
#define RINMOTIONDANCE_APP_OFF64(f, off)                                      \\
    static_assert(offsetof(MMDAppState, f) == (off),                          \\
                  #f " x64 anchor must match the original binary")
""" % delta[2])
for e, o in x64:
    out.append(f"RINMOTIONDANCE_APP_OFF64({e}, {o});")
out.append("""
#undef RINMOTIONDANCE_APP_OFF64
#endif
""")

with open(SRC, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out))

with open(SRC, encoding="utf-8") as f:
    gen = f.read()
glines = gen.splitlines()
gsplit = next(i for i, l in enumerate(glines) if l.strip() == "#else")
gx86, gx64 = [], []
for i, l in enumerate(glines):
    m = re.search(
        r"RINMOTIONDANCE_APP_OFF(?:64)?\(([^,]+),\s*(\d+)\);", l)
    if m:
        (gx86 if i < gsplit else gx64).append((m.group(1), int(m.group(2))))

assert gx86 == sorted(x86, key=lambda t: t[1]), "x86 multiset mismatch"
assert gx64 == x64, "x64 multiset mismatch"
assert len(gx86) == len(x86) and len(gx64) == len(x64)
print(f"regenerated OK: {len(gx86)} x86 + {len(gx64)} x64 pins, "
      f"{len(gx86) + len(gx64)} total (delta guard kept)")
