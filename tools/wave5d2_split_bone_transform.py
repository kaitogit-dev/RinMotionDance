import re

PATH = r"src\model\bone_transform.cpp"

raw = open(PATH, "rb").read().decode("utf-8")
eol = "\r\n" if "\r\n" in raw else "\n"
lines = raw.split(eol)
assert len(lines) == 1228, len(lines)  

assert "phase A" in lines[585], lines[585]
assert lines[586].lstrip().startswith("for (int i = 0; i < boneCount"), lines[586]
assert "phase B" in lines[618], lines[618]
assert lines[619].lstrip().startswith("for (int i = 0; i < boneCount"), lines[619]
assert "phase C" in lines[702], lines[702]
assert lines[703].lstrip().startswith("for (int i = 0; i < boneCount"), lines[703]
assert "phase D" in lines[784], lines[784]
assert lines[785].lstrip().startswith("for (int ci = 0"), lines[785]
assert "phase E" in lines[1135], lines[1135]
assert lines[1136].lstrip().startswith("for (int i = 0; i < boneCount"), lines[1136]
assert "typedChannelPass = [&]" in lines[577], lines[577]
assert lines[1224].rstrip() == "}", lines[1224]
assert lines[1226].rstrip() == "}  // namespace rinmotiondance", lines[1226]


def sl(a, b):
    return lines[a - 1 : b]


def dedent4(chunk):
    out = []
    for ln in chunk:
        if ln.startswith("    "):
            out.append(ln[4:])
        elif ln.startswith("#") or ln.strip() == "":
            out.append(ln)  
        else:
            raise AssertionError(repr(ln))
    return out


def gates(chunk):
    out = []
    nc = 0
    ng = 0
    for ln in chunk:
        if "typedChannelPass(bone)" in ln:
            ln = ln.replace("!typedChannelPass(bone)", "!ChannelPass(bone, afterPhysics)")
            nc += 1
        elif "typedChannelPass(target)" in ln:
            ln = ln.replace("!typedChannelPass(target)",
                            "!ChannelPass(target, afterPhysics)")
            nc += 1
        elif "typedCopyGate(bone)" in ln:
            ln = ln.replace("typedCopyGate(bone)", "CopyGate(bone, physicsMode)")
            ng += 1
        out.append(ln)
    return out, nc, ng


A, ca, ga = gates(sl(587, 617))
B, cb, gb = gates(sl(620, 701))
C, cc, gc = gates(sl(704, 783))
D, cd, gd = gates(sl(786, 1134))
E, ce, ge = gates(sl(1137, 1224))
assert (ca, cb, cc, cd, ce) == (1, 1, 1, 1, 1), (ca, cb, cc, cd, ce)
assert (ga, gb, gc, gd, ge) == (0, 0, 1, 0, 1), (ga, gb, gc, gd, ge)

out = []
out += sl(1, 47)
out += [
    "//",
    "// Layout: the five passes live in file-local functions at the original",
    "// phase boundaries; BoneFrameTransform computes the shared locals and",
    "// calls them in order (the two gate lambdas of the original body became",
    "// ChannelPass/CopyGate with the captured parameters explicit):",
    "//   BoneTransform_PhysicsSweptLocals  A 0x493A85",
    "//   BoneTransform_StandardLocals      B 0x493E71",
    "//   BoneTransform_WorldPass           C 0x49455F",
    "//   BoneTransform_CcdIk               D 0x494A56",
    "//   BoneTransform_PostIkPass          E 0x496881",
]
out += sl(48, 555)  
out += [
    "",
    "// ---- BoneFrameTransform phase passes --------------------------------------",
    "",
    "// Channel router (BoneFrameTransform a2): 0 -> bones WITHOUT flag 0x1000",
    "// (before physics), 1 -> bones WITH flag 0x1000 (after physics).",
    "bool ChannelPass(const mdl::BoneRecord& bone, unsigned char afterPhysics) {",
    "    const bool has = (bone.flags & mdl::kBoneFlagAfterPhysics) != 0;",
    "    return afterPhysics ? has : !has;",
    "}",
    "",
    "// +244 copy gate (LABEL_83): ((physicsMode==1 || (physicsMode>=2 &&",
    "// bone+493==0)) & bone+492) == 0.",
    "bool CopyGate(const mdl::BoneRecord& bone, int physicsMode) {",
    "    return ((physicsMode == 1 || (physicsMode >= 2 && bone.physicsDisabled == 0)) & bone.hasRigidBody) == 0;",
    "}",
    "",
    "// phase A (0x493A85): local matrix for physics-swept bones (+596 flag",
    "// set): inherit 0x100 from bone+488's +116, own quat +376, 0x200",
    "// translation inherit -> bone+116 = T(-rest308) * R * T(off364) *",
    "// T(rest308).",
    "void BoneTransform_PhysicsSweptLocals(D3& d3, mdl::BoneRecord* boneRecords,",
    "                                      int boneCount,",
    "                                      unsigned char afterPhysics,",
    "                                      int layer) {",
]
out += dedent4(A)
out += [
    "}",
    "",
    "// phase B (0x493E71): local matrix for the rest: type 4 resets scale",
    "// +348..360, type 9 axis-angle from tail(+460) quat x rate(int)(+488)",
    "// /100, type 5 quat = parent(+488)quat * own, 0x100 inherit; after the",
    "// sandwich, 0x200 bones add the inherited translation to +364.",
    "void BoneTransform_StandardLocals(D3& d3, mdl::BoneRecord* boneRecords,",
    "                                  int boneCount,",
    "                                  unsigned char afterPhysics,",
    "                                  int layer) {",
]
out += dedent4(B)
out += [
    "}",
    "",
    "// phase C (0x49455F): world pass: external parent (bone+600 ->",
    "// model+314596 table {slot@+12, bone@+16}) chains the OTHER model's",
    "// bone+244; else +52 = +116 * parent(+48)+52; +180 = parent world",
    "// rotation-only mirror; root keeps +52 = +116 and +180 = identity.",
    "// +244 copy gate per CopyGate.",
    "void BoneTransform_WorldPass(D3& d3, mdl::BoneRecord* boneRecords,",
    "                             int boneCount, mdl::BoneOrderEntry* extTable,",
    "                             unsigned char* const* modelSlots,",
    "                             unsigned char afterPhysics, int layer,",
    "                             int physicsMode) {",
]
out += dedent4(C)
out += [
    "}",
    "",
    "// phase D (0x494A56): CCD IK per 24-byte chain at model+9920 {+0",
    "// effector, +4 root, +8 links, +12 child*, +16 loops, +18 enabled,",
    "// +20 angle}; PMX limit planes, PMD axis projection and the knee hinge",
    "// flip per the file header.  The floating-point sequence is ported",
    "// verbatim (x87 fidelity notes included) - do not reorder.",
    "void BoneTransform_CcdIk(D3& d3, unsigned char* m,",
    "                         mdl::BoneRecord* boneRecords, mdl::IkChain* iks,",
    "                         int ikCount, unsigned char afterPhysics,",
    "                         int layer, int physicsMode, bool pmx2) {",
]
out += dedent4(D)
out += [
    "}",
    "",
    "// phase E (0x496881): post-IK pass for +596 bones (type != 4): type 9/5",
    "// take the source quat from tail(+460)/parent(+488) using +348 when",
    "// that source bone is type 4 (IK-touched), rebuild +116 and world +52.",
    "void BoneTransform_PostIkPass(D3& d3, mdl::BoneRecord* boneRecords,",
    "                              int boneCount, unsigned char afterPhysics,",
    "                              int layer, int physicsMode) {",
]
out += dedent4(E)
out += [
    "}",
]
out += sl(556, 564)  
out += [
    "",
    "void BoneFrameTransform(unsigned char* m, unsigned char afterPhysics,",
    "                        int layer, unsigned char* const* modelSlots,",
    "                        int physicsMode) {                             // 0x493A60",
    "    D3 d3;",
    "    mdl::ModelRecord& model = *mdl::Mdl(m);",
    "    const int boneCount = static_cast<int>(model.boneCount);",
    "    mdl::BoneRecord* boneRecords = model.boneTable;",
    "    mdl::IkChain* iks = mdl::IkChains(m);",
    "    const int ikCount = static_cast<int>(model.ikChainCount);",
    "    const bool pmx2 = model.physicsMode == 2;",
    "    mdl::BoneOrderEntry* extTable = mdl::BoneOrder(m);",
    "",
    "    // five sequential passes at the original phase boundaries (header)",
    "    BoneTransform_PhysicsSweptLocals(d3, boneRecords, boneCount,",
    "                                     afterPhysics,",
    "                                     layer);                         // A 0x493A85",
    "    BoneTransform_StandardLocals(d3, boneRecords, boneCount,",
    "                                 afterPhysics,",
    "                                 layer);                             // B 0x493E71",
    "    BoneTransform_WorldPass(d3, boneRecords, boneCount, extTable,",
    "                            modelSlots, afterPhysics, layer,",
    "                            physicsMode);                            // C 0x49455F",
    "    BoneTransform_CcdIk(d3, m, boneRecords, iks, ikCount, afterPhysics,",
    "                        layer, physicsMode,",
    "                        pmx2);                                        // D 0x494A56",
    "    BoneTransform_PostIkPass(d3, boneRecords, boneCount, afterPhysics,",
    "                              layer,",
    "                              physicsMode);                          // E 0x496881",
    "}",
]
out += sl(1226, 1227)  

text = eol.join(out)
assert text.count("typedChannelPass") == 0
assert text.count("typedCopyGate") == 0
assert text.count("ChannelPass(bone, afterPhysics)") == 4
assert text.count("ChannelPass(target, afterPhysics)") == 1
assert text.count("CopyGate(bone, physicsMode)") == 2
assert text.count("void BoneTransform_") == 5
assert text.count("{") == text.count("}")
assert "WriteIkPrecheck(m, ci, iter, li, d1, d2, diff2, target," in text
open(PATH, "wb").write(text.encode("utf-8"))
print("ok: lines", len(out), "| braces", text.count("{"))
