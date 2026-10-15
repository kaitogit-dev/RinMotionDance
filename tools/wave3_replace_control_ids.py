#!/usr/bin/env python3
"""Wave3 Task B: replace bare control-ID literals with panel:: constants.

Rewrites only the ID (2nd) argument of GetDlgItem / SendDlgItemMessage /
CheckDlgButton / IsDlgButtonChecked calls when it is exactly a decimal or
hexadecimal literal whose value is in the map from
include/rinmotiondance/panel_controls.hpp.  Arithmetic forms
(GetDlgItem(h, 0x1C7 + ch)) are left alone.  Call sites inside comments are
skipped.  Existing local constants/tables are never touched (only bare
literals are rewritten).  Adds the panel_controls.hpp include where needed.

Run with --dry to list sites without editing.
"""
import re
import sys
import pathlib

DRY = '--dry' in sys.argv

MAP = {
    400: 'kUndoButton', 401: 'kRedoButton', 408: 'kPlayButton',
    409: 'kPlayStartFrameEdit', 410: 'kPlayStopFrameEdit',
    412: 'kCameraRefModelCheckbox', 417: 'kCurrentFrameEdit',
    421: 'kPasteButton', 422: 'kReversePasteButton', 424: 'kExpandShrinkButton',
    425: 'kRangeStartEdit', 426: 'kRangeStopEdit',
    427: 'kTimelineVScroll', 428: 'kTimelineHScroll',
    433: 'kInterpCurveCombo', 434: 'kRegisterScopeCombo', 436: 'kMainComboModel',
    439: 'kModelVisibleCheckbox', 440: 'kShadowCheckbox', 441: 'kAddBlendCheckbox',
    443: 'kIkChainCombo', 444: 'kIkOnRadio', 445: 'kIkOffRadio',
    446: 'kPerspectiveCheckbox', 447: 'kFovSlider', 448: 'kFovEdit',
    449: 'kMainComboNormal', 450: 'kBoneRegisterCombo',
    455: 'kLightColorSliderR', 456: 'kLightColorSliderG', 457: 'kLightColorSliderB',
    458: 'kLightDirSliderX', 459: 'kLightDirSliderY', 460: 'kLightDirSliderZ',
    461: 'kLightColorEditR', 462: 'kLightColorEditG', 463: 'kLightColorEditB',
    464: 'kLightDirEditX', 465: 'kLightDirEditY', 466: 'kLightDirEditZ',
    471: 'kAccessoryCombo', 474: 'kMainComboGround', 475: 'kAttachBoneCombo',
    476: 'kAccessoryVisibleCheckbox', 477: 'kAccessoryAddBlendCheckbox',
    478: 'kAccPosXEdit', 479: 'kAccPosYEdit', 480: 'kAccPosZEdit',
    481: 'kAccRotXEdit', 482: 'kAccRotYEdit', 483: 'kAccRotZEdit',
    484: 'kAccScaleXEdit', 485: 'kAccScaleYEdit', 486: 'kAccessoryShadowCheckbox',
    490: 'kBoneSelectRadio', 491: 'kBoxSelectRadio', 492: 'kBoneMoveRadio',
    493: 'kBoneRotateRadio', 497: 'kBonePasteButton', 498: 'kBoneReversePasteButton',
    499: 'kPhysicsCheckbox', 500: 'kRegisterFrameButton',
    504: 'kMorphCombo0', 505: 'kMorphSlider0', 506: 'kMorphEdit0',
    509: 'kMorphCombo1', 510: 'kMorphSlider1', 511: 'kMorphEdit1',
    514: 'kMorphCombo2', 515: 'kMorphSlider2', 516: 'kMorphEdit2',
    519: 'kMorphCombo3', 520: 'kMorphSlider3', 521: 'kMorphEdit3',
    530: 'kPhysicsFrameCheckbox', 531: 'kCameraRefBoneCheckbox',
    534: 'kFrameVolumeSlider', 535: 'kFollowCameraCheckbox', 536: 'kModelEditToggle',
    543: 'kCameraDistanceButton',
    544: 'kReadoutPosXEdit', 545: 'kReadoutPosYEdit', 546: 'kReadoutPosZEdit',
    547: 'kReadoutRotXEdit', 548: 'kReadoutRotYEdit', 549: 'kReadoutRotZEdit',
    550: 'kReadoutDistEdit', 551: 'kInfoCheckbox', 552: 'kFrameVolumeCheckbox',
    554: 'kGotoFrameEdit', 556: 'kSelfShadowCheckbox', 557: 'kCoordAxisCheckbox',
    560: 'kSelfShadowRangeSlider', 561: 'kSelfShadowRangeEdit',
    562: 'kEditOffCheckbox', 563: 'kEditMode1Checkbox', 564: 'kEditMode2Checkbox',
    601: 'kBiasXEdit', 602: 'kBiasYEdit', 603: 'kBiasZEdit',
    605: 'kScaleRateEdit', 609: 'kAviStartFrameEdit', 610: 'kAviEndFrameEdit',
    611: 'kAviFpsEdit', 612: 'kAviWidthEdit', 613: 'kAviHeightEdit',
    614: 'kAviWaveCheckbox', 616: 'kShiftFramesEdit',
    618: 'kBlinkStartEdit', 619: 'kBlinkEndEdit',
    621: 'kScreenWidthEdit', 622: 'kScreenHeightEdit',
    625: 'kGroundShadowSlider', 626: 'kGroundShadowEdit',
    628: 'kOrderListBox', 629: 'kAccessoryNameEdit',
    630: 'kOrderUpButton', 631: 'kOrderDownButton', 632: 'kDialogOkButton',
    635: 'kAccessoryOrderEdit',
    637: 'kNumInputPosXEdit', 638: 'kNumInputPosYEdit', 639: 'kNumInputPosZEdit',
    640: 'kNumInputRotXEdit', 641: 'kNumInputRotYEdit', 642: 'kNumInputRotZEdit',
    644: 'kNumInputDistEdit', 646: 'kEdgeThicknessEdit', 647: 'kEdgeThicknessSlider',
    667: 'kModelNameEnEdit', 668: 'kModelCommentEnEdit', 669: 'kBoneNameCombo',
    670: 'kBoneNextButton', 671: 'kBonePrevButton', 672: 'kBoneNameEnEdit',
    673: 'kMorphNameCombo', 676: 'kMorphNameEnEdit', 677: 'kGroupNameCombo',
    680: 'kGroupNameEnEdit',
    686: 'kScaleFromEdit', 687: 'kScaleToEdit',
    688: 'kScaleBoneCheckbox', 689: 'kScaleMorphCheckbox', 690: 'kScaleDispIkCheckbox',
    709: 'kGravityAccelEdit', 710: 'kGravityDirXEdit', 711: 'kGravityDirYEdit',
    712: 'kGravityDirZEdit', 713: 'kGravityNoiseEdit',
    731: 'kGravityNoiseCheckbox', 741: 'kBodyACombo', 742: 'kBodyBCombo',
    806: 'kAvi3dVisionCheckbox', 807: 'kAviLeftHalfCheckbox',
}

OVERRIDES = [
    ('src/model/track_apply.cpp', 637, (0, 10 ** 9), 'kGravityDirXSlider'),
    ('src/model/track_apply.cpp', 638, (0, 10 ** 9), 'kGravityDirYSlider'),
    ('src/model/track_apply.cpp', 639, (0, 10 ** 9), 'kGravityDirZSlider'),
    ('src/window/command_file_menu.cpp', 628, (580, 730), 'kAviCodecCombo'),
    ('src/window/command_file_menu.cpp', 686, (735, 760), 'kCamMulPosXScaleEdit'),
    ('src/window/command_file_menu.cpp', 687, (735, 760), 'kCamMulPosXOffsetEdit'),
]

CALL = re.compile(
    r'\b(GetDlgItem|SendDlgItemMessage|CheckDlgButton|IsDlgButtonChecked)(A|W)?\s*\(')
LIT = re.compile(r'(0[xX][0-9A-Fa-f]+|\d+)[uUlL]*')


def comment_spans(text):
    spans = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            spans.append((i, j))
            i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            j = n if j < 0 else j
            spans.append((i, j))
            i = j
        elif c == '"':
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == '"':
                    break
                j += 1
            i = j + 1
        elif c == "'":
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == "'":
                    break
                j += 1
            i = j + 1
        else:
            i += 1
    return spans


def match_paren(text, open_pos):
    depth, i, n = 0, open_pos, len(text)
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
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def split_args(text, start, end):
    spans, depth, arg_start, i = [], 0, start, start
    while i < end:
        c = text[i]
        if c == '/' and i + 1 < end and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            i = end if j < 0 else j + 2
            continue
        if c == '/' and i + 1 < end and text[i + 1] == '/':
            j = text.find('\n', i)
            i = end if j < 0 else j
            continue
        if c in '"\'':
            q = c
            j = i + 1
            while j < end:
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
    spans.append((arg_start, end))
    return spans


def process(path):
    text = path.read_text(encoding='utf-8', errors='surrogateescape')
    cspans = comment_spans(text)

    def in_comment(pos):
        return any(a <= pos < b for a, b in cspans)

    edits = []
    stats = {}
    rel = path.as_posix()
    for m in CALL.finditer(text):
        if in_comment(m.start()):
            continue
        open_pos = m.end() - 1
        close = match_paren(text, open_pos)
        if close < 0:
            continue
        args = split_args(text, open_pos + 1, close)
        if len(args) < 2:
            continue
        s, e = args[1]
        stripped = re.sub(r'/\*.*?\*/', ' ', text[s:e], flags=re.S)
        stripped = re.sub(r'//[^\n]*', ' ', stripped).strip()
        lm = LIT.fullmatch(stripped)
        if not lm:
            continue
        val = int(stripped.rstrip('uUlL'), 0)
        if val not in MAP:
            continue
        line = text.count('\n', 0, s) + 1
        name = MAP[val]
        for f, v, (lo, hi), nm in OVERRIDES:
            if f == rel and v == val and lo <= line <= hi:
                name = nm
                break
        hm = LIT.search(text[s:e])
        tok_s, tok_e = s + hm.start(), s + hm.end()
        edits.append((tok_s, tok_e, name))
        stats.setdefault(name, []).append((rel, line, stripped))
    if DRY:
        return stats
    if not edits:
        return stats
    in_ns = 'namespace rinmotiondance' in text
    qual = 'panel::' if in_ns else 'rinmotiondance::panel::'
    for s, e, rep in sorted(edits, key=lambda t: -t[0]):
        text = text[:s] + qual + rep + text[e:]
    if '#include "rinmotiondance/panel_controls.hpp"' not in text:
        inc = '#include "rinmotiondance/panel_controls.hpp"\n'
        m = list(re.finditer(r'#include "rinmotiondance/[^"]+"\n', text))
        if m:
            text = text[:m[-1].end()] + inc + text[m[-1].end():]
        else:
            m = list(re.finditer(r'#include <[^\n]+>\n', text))
            assert m, rel
            text = text[:m[-1].end()] + inc + text[m[-1].end():]
    path.write_text(text, encoding='utf-8', errors='surrogateescape')
    return stats


def main():
    grand = {}
    files = 0
    for p in pathlib.Path('src').rglob('*.cpp'):
        s = p.as_posix()
        if 'mmdxshow' in s:
            continue
        stats = process(p)
        if stats:
            files += 1
            n = sum(len(v) for v in stats.values())
            if not DRY:
                print(f'{s}: {n}')
            for k, v in stats.items():
                grand.setdefault(k, []).extend(v)
    print(f'files={files} total={sum(len(v) for v in grand.values())}')
    for k in sorted(grand, key=lambda k: -len(grand[k])):
        print(f'  {k}: {len(grand[k])}')


if __name__ == '__main__':
    main()
