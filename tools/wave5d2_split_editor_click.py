import re
import sys

PATH = r"src\window\ui_editor_click.cpp"

raw = open(PATH, "rb").read().decode("utf-8")
eol = "\r\n" if "\r\n" in raw else "\n"
lines = raw.split(eol)
assert len(lines) == 1235, len(lines)  


def sl(a, b):
    """1-based inclusive slice."""
    return lines[a - 1 : b]


def dedent4(chunk):
    out = []
    for ln in chunk:
        if ln.startswith("    "):
            out.append(ln[4:])
        else:
            assert ln.strip() == "", repr(ln)
            out.append(ln)
    return out


GOTO_TAIL = re.compile(r"^(\s*)goto L_tail;(\s*)(//.*)?$")


def untail(chunk):
    out = []
    n = 0
    for ln in chunk:
        m = GOTO_TAIL.match(ln)
        if m:
            out.append("%sreturn;%s%s" % (m.group(1), m.group(2), m.group(3) or ""))
            n += 1
        else:
            out.append(ln)
    return out, n


out = []
out += sl(1, 21)

out += [
    "//",
    "// Every path funnels into the shared tail (dirty flag 0xA0189, click anchor",
    "// 0xA018C/0xA0190) and the selection-count re-evaluation (0xA03EC region,",
    "// 0x40 dwords, per-model flag counting + qsort + 0x49D410 feed).",
    "//",
    "// Layout: HandleLButtonDown keeps the S1 gates and dispatches to one",
    "// file-local sub-function per click region; every original `goto L_tail`",
    "// exit became a plain return, and the L_rowtype forward jump inside the",
    "// name column is kept verbatim:",
    "//   HandleLButtonDown_MarginColumnToggle   S2    0x446B01-0x446B9C",
    "//   HandleLButtonDown_NameColumnHit        S3+S4 0x446BA1-0x44738F",
    "//   HandleLButtonDown_TimelineAreaHit      S5-S6 0x447390-0x448EEB",
    "//   HandleLButtonDown_FrameStripHit        S7    0x44A468-0x44A850",
    "//   HandleLButtonDown_NameRowRectHit       S7b   0x44A855-0x44A97D",
    "//",
    "// Reference: ../translated/MikuMikuDance/fcn_00446a70.cpp",
    "//   NOTE: the translated file is a register-tracking re-render that deviates",
    "//   in places (coordinate mapping, message ids, helper arg lists); this port",
    "//   follows the IDA decompilation of MikuMikuDance.exe 0x446A70.",
    "// =========================================================================//",
]

out += sl(32, 477)

out += [
    "",
    "// ---- HandleLButtonDown region helpers ------------------------------------",
    "",
    "// One S5a display-mode cell toggle - the 15-line branch body that the four",
    "// per-band blocks (0x4473DA-0x4479BB: rigid/joint/IK/morph) repeat",
    "// verbatim.  selected != 0: shift clears it, a plain click only marks the",
    "// stats dirty; selected == 0: a plain click first clears all selection",
    "// bitmaps, a shift-click sets it without the clear.",
    "static void ToggleTimelineCell(MMDApp* app, bool shiftActive,",
    "                               std::uint8_t& selected) {",
    "    if (selected != 0) {",
    "        if (shiftActive)",
    "            selected = 0;",
    "        else",
    "            app->TimelineSelectionChanged() = 1;",
    "    } else if (!shiftActive) {",
    "        ClearSelectionBitmaps(app);",
    "        selected = 1;",
    "    } else {",
    "        selected = 1;",
    "    }",
    "}",
    "",
    "// S5a band table: the four structurally identical band toggles differ only",
    "// in {y-range, row-hit table, track array}, so they collapse into the",
    "// bands[] loop in HandleLButtonDown_TimelineAreaHit.  The y-ranges are",
    "// disjoint and contiguous ([0xA0, 0xD8) in 14px steps), so at most one",
    "// entry matches - the same one-block-executes invariant as the original",
    "// if-chain.  The fifth band (accessory 2D grid, y >= 0xD8) has its own",
    "// index math and stays separate.",
    "struct TimelineBandHit {",
    "    int yMin;",
    "    int yMax;",
    "    const std::int32_t* rowHits;              // state.rowHitBand0..3",
    "    std::uint8_t& (*keySelected)(MMDApp*, int);",
    "};",
    "",
    "static std::uint8_t& CameraKeySelected(MMDApp* app, int i) {",
    "    return app->CameraKeys()[i].selected;",
    "}",
    "static std::uint8_t& LightKeySelected(MMDApp* app, int i) {",
    "    return app->LightKeys()[i].selected;",
    "}",
    "static std::uint8_t& ShadowKeySelected(MMDApp* app, int i) {",
    "    return app->ShadowKeys()[i].selected;",
    "}",
    "static std::uint8_t& GravityKeySelected(MMDApp* app, int i) {",
    "    return app->GravityKeys()[i].selected;",
    "}",
    "",
    "// S6 stage-toggle helpers.  The three edit-mode stages (display/morph/bone",
    "// keys, loc_447A4D-0x448EEB) repeat two branch shapes over three key",
    "// families:",
    "//   type > 0 / type == -10 : the allocated-toggle below (6 copies in the",
    "//                            original, differing only in key family/index)",
    "//   type == -1 / type == -2: the timeline-range rows below (4 copies,",
    "//                            morph/bone stages only)",
    "// Both range rows address [type*sizeof(Key), +1] in the key array; -1",
    "// marks the range apply-enabled under shift (else just stats-dirty), -2",
    "// always sets the range and clears the model flags unless shift is held.",
    "// NOTE: unlike the original, the non-shift -1 row evaluates the key-array",
    "// pointer (a pure read of the active-slot table) before branching -",
    "// behaviour is identical.",
    "template <typename KeyT>",
    "static void ToggleModelKeyAllocated(MMDApp* app, KeyT& key,",
    "                                    bool shiftActive) {",
    "    if (key.allocated != 0) {",
    "        if (shiftActive)",
    "            key.allocated = 0;",
    "        else",
    "            app->TimelineSelectionChanged() = 1;",
    "    } else if (shiftActive) {",
    "        key.allocated = 1;",
    "    } else {",
    "        ClearActiveModelFlags(app);",
    "        key.allocated = 1;",
    "    }",
    "}",
    "",
    "template <typename KeyT>",
    "static void StageRangeRow(MMDApp* app, std::int32_t type, KeyT* keys,",
    "                          bool shiftActive) {",
    "    if (type == -1) {                          // loc_4483B8 / loc_448AB8",
    "        if (shiftActive) {",
    "            SetTimelineSelectionRange(app, type * sizeof(*keys), keys,",
    "                                      type * sizeof(*keys) + 1,",
    "                                      reinterpret_cast<unsigned char*>(keys) + 1);",
    "            app->TimelineRangeApplyEnabled() = 1;",
    "        } else {",
    "            app->TimelineSelectionChanged() = 1;",
    "        }",
    "    } else {                                   // type == -2, loc_4483F1/0x448AF1",
    "        SetTimelineSelectionRange(app, type * sizeof(*keys), keys,",
    "                                  type * sizeof(*keys) + 1,",
    "                                  reinterpret_cast<unsigned char*>(keys) + 1);",
    "        if (!shiftActive)",
    "            ClearActiveModelFlags(app);",
    "    }",
    "}",
]

s2, n2 = untail(sl(517, 529))
assert n2 == 2, n2
out += [
    "",
    "// ---- S2 (VA 0x446B01-0x446B9C): margin column (X in (4,20)) ---------------",
    "// Row-type display toggle, edit mode only; entered with the row gates",
    "// already checked.  Row index = (y-0xA0)/14 (magic 0x92492493, sar 3); row",
    "// type byte at model+0x2DC0 indexes a 0x65-stride table (model+0x26D0)",
    "// whose flag at +0x64 is toggled.",
    "static void HandleLButtonDown_MarginColumnToggle(MMDApp* app,",
    "                                                 std::int32_t y) {",
]
out += dedent4(s2)
out += ["}"]

s3a = dedent4(sl(538, 554))
s3c, n3 = untail(sl(584, 770))
assert n3 == 6, n3  
s3c = dedent4(s3c)
out += [
    "",
    "// ---- S3+S4 (VA 0x446BA1-0x44738F): name column (X in (20,95]) --------------",
    "// Bone/morph row selection; entered with the row gates already checked.",
    "// Edit mode selects the clicked row (row table model+0x2E88, special morph",
    "// rows encoded as -1-idx, -999 = \"bottom\" morph row) and dispatches the",
    "// row-type handlers (model+0x2DC0 byte); display mode goes to the S4",
    "// sub-branch below.",
    "static void HandleLButtonDown_NameColumnHit(MMDApp* app, HWND hwnd,",
    "                                            std::int32_t y) {",
]
out += s3a
out += [
    "        // loc_447210: column-band toggles (band selected by this+8) - four",
    "        // isomorphic {y-range, global-track} blocks merged into the table",
    "        // below; the y-ranges are disjoint/contiguous ([0xA0, 0xD8) in 14px",
    "        // steps), so at most one entry matches, exactly like the original",
    "        // if-chain.",
    "        static const struct {",
    "            int yMin, yMax;",
    "            GlobalTimelineTrack track;",
    "        } kColumnBands[4] = {",
    "            {0xA0, 0xAE, GlobalTimelineTrack::Camera},      // bone band",
    "            {0xAE, 0xBC, GlobalTimelineTrack::Light},       // morph band",
    "            {0xBC, 0xCA, GlobalTimelineTrack::SelfShadow},  // IK band",
    "            {0xCA, 0xD8, GlobalTimelineTrack::Gravity},     // rigid band",
    "        };",
    "        for (const auto& band : kColumnBands) {",
    "            if (y >= band.yMin && y < band.yMax) {",
    "                app->GlobalTrackSelected(band.track) =",
    "                    app->GlobalTrackSelected(band.track) == 0 ? 1 : 0;",
    "                PostLanguageSweep(app);",
    "                return;",
    "            }",
    "        }",
]
out += s3c
out += ["}"]

s5a_open = dedent4(sl(780, 786))       
s5a_shift = dedent4(sl(792, 792))      
s5a_joint2d, nj = untail(sl(861, 884))
assert nj == 0, nj
s5a_joint2d = dedent4(s5a_joint2d)
s5a_stats, ns = untail(sl(890, 891))
assert ns == 1, ns
s5a_stats = dedent4(s5a_stats)
s6_comment = dedent4(sl(894, 898))
out += [
    "",
    "// ---- S5-S6 (VA 0x447390-0x448EEB): main area (X in (95, sidebar-18)) --------",
    "// Timeline cell hits, entered with the row gates already checked.  Display",
    "// mode (S5a) toggles keys in the five timeline bands; edit mode (S6) runs",
    "// the three-stage row-type chain.  Both converge on SelectionStats",
    "// (loc_448EEB).",
    "static void HandleLButtonDown_TimelineAreaHit(MMDApp* app, HWND hwnd,",
    "                                              std::int32_t x,",
    "                                              std::int32_t y) {",
    "    if (app->PlaybackActive() != 0)",
    "        return;",
    "    app->TimelineSelectionChanged() = 0;",
    "    if (app->state.optflag[0] != 0) {  // 760 (0x2F8)",
]
out += s5a_open
out += [
    "        std::int32_t bandIdx = -1;  // var_194 / hWnd var / var_198 / wParam var",
    "        std::int32_t joint2dIdx = -1;                        // var_18C",
]
out += s5a_shift
out += [
    "        // four isomorphic band toggles (0x4473DA-0x4479BB), table-driven:",
    "        // rigid/joint/IK/morph differ only in {y-range, row-hit table,",
    "        // track array}.  At most one band matches; the shared bandIdx latch",
    "        // therefore equals the original four latches all staying -1.",
    "        const TimelineBandHit bands[4] = {",
    "            {0xA0, 0xAE, app->state.rowHitBand0, CameraKeySelected},  // rigid",
    "            {0xAE, 0xBC, app->state.rowHitBand1, LightKeySelected},   // joint",
    "            {0xBC, 0xCA, app->state.rowHitBand2, ShadowKeySelected},  // IK",
    "            {0xCA, 0xD8, app->state.rowHitBand3, GravityKeySelected}, // morph",
    "        };",
    "        for (const TimelineBandHit& band : bands) {",
    "            if (y < band.yMin || y >= band.yMax)",
    "                continue;",
    "            bandIdx = band.rowHits[rowD];",
    "            if (bandIdx >= 0)",
    "                ToggleTimelineCell(app, shiftActive,",
    "                                   band.keySelected(app, bandIdx));",
    "        }",
]
out += s5a_joint2d
out += [
    "        // loc_447990: no band hit at all -> wholesale bitmap clear",
    "        if (bandIdx == -1 && joint2dIdx == -1 && !shiftActive) {",
    "            ClearSelectionBitmaps(app);",
    "        }",
]
out += s5a_stats
out += ["    }"]
out += s6_comment
out += [
    "    else {",
    "        if (ActiveModel(app) == nullptr)",
    "            return;",
    "        const bool shiftActive = app->ShiftModifierActive();",
    "        const std::int32_t row2d = Div13(x - 0x64) * 0xC8 + Div14(y - 0xA0);",
    "        // stage 1 (loc_447A4D): IK-type flags, model+0x26E8 0x1C-stride",
    "        // records with flag at +0x14",
    "        const std::int32_t ikType = app->state.rowHitIk[row2d];",
    "        if (ikType > 0) {",
    "            ToggleModelKeyAllocated(app,",
    "                rinmotiondance::mdl::DisplayKeys(ActiveModel(app))[ikType],",
    "                shiftActive);",
    "        } else if (ikType == -10) {                 // loc_447CD9 (row 0)",
    "            ToggleModelKeyAllocated(app,",
    "                rinmotiondance::mdl::DisplayKeys(ActiveModel(app))[0],",
    "                shiftActive);",
    "        }",
    "        // stage 2 (loc_447F03): morph-type flags, model+0x26E4",
    "        // 0x14-stride records with flag at +0x10",
    "        const std::int32_t morphType = app->state.rowHitMorph[row2d];",
    "        if (morphType > 0) {",
    "            ToggleModelKeyAllocated(app,",
    "                rinmotiondance::mdl::MorphKeys(ActiveModel(app))[morphType],",
    "                shiftActive);",
    "        } else if (morphType == -10) {              // loc_448189 (row 0)",
    "            ToggleModelKeyAllocated(app,",
    "                rinmotiondance::mdl::MorphKeys(ActiveModel(app))[0],",
    "                shiftActive);",
    "        } else if (morphType == -1) {               // loc_4483B8",
    "            StageRangeRow(app, morphType,",
    "                          rinmotiondance::mdl::MorphKeys(ActiveModel(app)),",
    "                          shiftActive);",
    "        } else if (morphType == -2) {               // loc_4483F1",
    "            StageRangeRow(app, morphType,",
    "                          rinmotiondance::mdl::MorphKeys(ActiveModel(app)),",
    "                          shiftActive);",
    "        }",
    "        // stage 3 (loc_4485FB): bone-type flags, model+0x26E0 0x3C flag",
    "        // stride with flag at +0x38",
    "        const std::int32_t boneType = app->state.rowHitBone[row2d];",
    "        if (boneType > 0) {",
    "            ToggleModelKeyAllocated(app,",
    "                rinmotiondance::mdl::BoneKeys(ActiveModel(app))[boneType],",
    "                shiftActive);",
    "        } else if (boneType == -10) {               // loc_448889 (row 0)",
    "            ToggleModelKeyAllocated(app,",
    "                rinmotiondance::mdl::BoneKeys(ActiveModel(app))[0],",
    "                shiftActive);",
    "        } else if (boneType == -1) {                // loc_448AB8",
    "            StageRangeRow(app, boneType,",
    "                          rinmotiondance::mdl::BoneKeys(ActiveModel(app)),",
    "                          shiftActive);",
    "        } else if (boneType == -2) {                // loc_448AF1",
    "            StageRangeRow(app, boneType,",
    "                          rinmotiondance::mdl::BoneKeys(ActiveModel(app)),",
    "                          shiftActive);",
    "        }",
    "        // loc_448CFB: none of the three stages hit -> wholesale",
    "        // active-model flag clear (unless mode == 3)",
    "        if (ikType == 0 && morphType == 0 && boneType == 0 && !shiftActive)",
    "            ClearActiveModelFlags(app);",
    "        SelectionStats(app, x, y, hwnd);           // loc_448EEB",
    "        return;",
    "    }",
    "}",
]

s7, n7 = untail(sl(1053, 1186))
assert n7 == 2, n7  
s7 = dedent4(s7)
out += [
    "",
    "// ---- S7 (VA 0x44A468-0x44A850): frame header strip (y in (144,160)) ---------",
    "// Reached when the main-area gate fails; the strip gate in the original is",
    "// (x < sidebar-0x12) && y in (0x90, 0xA0) && x > 0x64.  Snapshots edited",
    "// bones, seeks every model to the clicked frame, then repaints and resyncs",
    "// physics/audio.",
    "static void HandleLButtonDown_FrameStripHit(MMDApp* app, HWND hwnd,",
    "                                            std::int32_t x, std::int32_t y,",
    "                                            std::int32_t sidebar) {",
]
out += s7
out += ["}"]

s7b, n7b = untail(sl(1194, 1226))
assert n7b == 4, n7b  
s7b = dedent4(s7b)
s7b = [ln.replace("rc.bottom", "bottom") for ln in s7b]
out += [
    "",
    "// ---- S7b (VA 0x44A855-0x44A97D): bone/morph name-row rects ------------------",
    "// Bone rect: X in (9DA05+3, 9DA05+13), Y in (9DA06+bottom-0x8C,",
    "// 9DA06+bottom-0x82), gated on byte 9DA04; keyframe insert (0x4A1510)",
    "// + row marker 0x9DA09 = 1.  Morph rect: same with 9DA07/9DA08, marker 2.",
    "// A miss in both rects falls through to the caller's tail (loc_44A97E).",
    "static void HandleLButtonDown_NameRowRectHit(MMDApp* app, std::int32_t x,",
    "                                             std::int32_t y,",
    "                                             std::int32_t bottom) {",
]
out += s7b
out += ["}"]

out += [
    "",
    "void HandleLButtonDown(MMDApp* app) {",
    "    // ---- S1: guard 0xA0274 + client rect + early-out gate ------------",
    "    // (0x446A70-0x446B00) region dispatch follows; each branch returns",
    "    // straight to the shared tail (loc_44A980).",
    "    if (app->FullscreenMode() != 0)",
    "        return;                                              // loc_44A980",
    "",
    "    const HWND hwnd = static_cast<HWND>(app->state.hwnd);  // 657080",
    "    RECT rc;",
    "    GetClientRect(hwnd, &rc);",
    "    const std::int32_t y = app->MouseY();",
    "    const std::int32_t x = app->MouseX();",
    "    const std::int32_t sidebar = app->SidebarWidth();",
    "",
    "    // early-out: click in the left panel header strip - this+0xC8 = 1,",
    "    // this+0xA442C = 0, then shared tail.  Gates (eval order as in the",
    "    // original): y <= bottom-0x9E(158) && x <= sidebar+6 &&",
    "    // [0xA0D38]==0 && sidebar <= x.",
    "    if (y <= rc.bottom - 0x9E && x <= sidebar + 6 &&",
    "        app->FloatingWindow() == nullptr && sidebar <= x) {",
    "        app->SidebarResizeDragging() = 1;",
    "        app->WindowLayoutReady() = 0;",
    "        return;",
    "    }",
    "    // row gates used by every region below (0x446B01-0x446B3A):",
    "    //   var_1A0 = y < bottom - 0xF8 (248) ; var_190 = y > 0xA0 (160)",
    "    const bool rowGateY = y > 0xA0 && y < rc.bottom - 0xF8;",
    "",
    "    // ---- S2: margin column (X in (4,20)): row-type display toggle",
    "    if (rowGateY && x > 4 && x < 0x14) {",
    "        HandleLButtonDown_MarginColumnToggle(app, y);",
    "        return;",
    "    }",
    "",
    "    // ---- S3: name column (X in (20,95]): bone/morph row selection",
    "    if (rowGateY && x > 0x14 && x <= 0x5F) {",
    "        HandleLButtonDown_NameColumnHit(app, hwnd, y);",
    "        return;",
    "    }",
    "",
    "    // ---- S5: main area (X in (95, sidebar-18) && row gates) --------------",
    "    // (0x447390-0x447A4C) gate fail falls into S7 (name-row rects).",
    "    if (rowGateY && x > 0x5F && x < sidebar - 0x12) {",
    "        HandleLButtonDown_TimelineAreaHit(app, hwnd, x, y);",
    "        return;",
    "    }",
    "",
    "    // ---- S7: header strip (y in (144,160))",
    "    if (x < sidebar - 0x12 && y > 0x90 && y < 0xA0 && x > 0x64) {",
    "        HandleLButtonDown_FrameStripHit(app, hwnd, x, y, sidebar);",
    "        return;",
    "    }",
    "",
    "    // ---- S7b: bone/morph name-row rects",
    "    HandleLButtonDown_NameRowRectHit(app, x, y, rc.bottom);",
    "}",
    "",
    "}  // namespace rinmotiondance",
    "",
]

text = eol.join(out)

tail_stmts = [ln for ln in text.split(eol) if ln.lstrip().startswith("goto L_tail;")]
assert not tail_stmts, tail_stmts[:3]
rowtype_stmts = [ln for ln in text.split(eol)
                 if ln.lstrip().startswith(("goto L_rowtype;", "L_rowtype:;"))]
assert len(rowtype_stmts) == 3, rowtype_stmts
assert text.count("static void HandleLButtonDown_") == 5
assert text.count("void HandleLButtonDown(MMDApp* app) {") == 1
assert text.count("{") == text.count("}")
assert lines[537].rstrip().endswith("// 760 (0x2F8)"), lines[537]
assert lines[785].lstrip().startswith("const std::int32_t rowD"), lines[785]
assert lines[889].lstrip().startswith("SelectionStats"), lines[889]
assert lines[1184].lstrip().startswith("app->PhysicsResetPending"), lines[1184]
open(PATH, "wb").write(text.encode("utf-8"))
print("ok: lines", len(out), "| braces", text.count("{"))
