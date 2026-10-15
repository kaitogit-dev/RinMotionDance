import subprocess, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OLD = subprocess.run(['git', 'show', 'HEAD:src/io/pmm_save.cpp'],
                     cwd=ROOT, capture_output=True, text=True,
                     encoding='utf-8').stdout
OLDV1 = subprocess.run(['git', 'show', 'HEAD:src/io/pmm_load_v1.cpp'],
                       cwd=ROOT, capture_output=True, text=True,
                       encoding='utf-8').stdout
OLDV2 = subprocess.run(['git', 'show', 'HEAD:src/io/pmm_load_v2.cpp'],
                       cwd=ROOT, capture_output=True, text=True,
                       encoding='utf-8').stdout
NEW = open(os.path.join(ROOT, 'src/io/pmm_save.cpp'), encoding='utf-8').read()

def slice_between(text, start, end):
    i = text.index(start)
    j = text.index(end, i)
    return text[i:j]

def dedent(block):
    out = []
    for line in block.splitlines():
        if line.startswith('    '):
            out.append(line[4:])
        else:
            out.append(line)
    return '\n'.join(out)

def brace_body(src, from_idx):
    """Return (body_without_braces, end_index) of the brace-balanced block
    starting at the first '{' at/after from_idx (string literals with
    braces do not occur in the extracted reader bodies)."""
    start = src.index('{', from_idx)
    depth = 0
    k = start
    while True:
        if src[k] == '{':
            depth += 1
        elif src[k] == '}':
            depth -= 1
            if depth == 0:
                break
        k += 1
    return src[start + 1:k], k

def lambda_to_fn(src, header_start, param_type, new_name):
    """Cut a `const auto name = [&](T key) { ... };` lambda into a plain
    function body, renaming to new_name(int fd, T& key)."""
    i = src.index(header_start)
    inner, _ = brace_body(src, i)
    return ('static void %s(int fd, %s& key) {%s\n}\n'
            % (new_name, param_type, inner))

def plain_fn_to_fn(src, signature, new_name):
    """Cut `void name(int fd, T& key) { ... }` (brace-matched) and rename."""
    i = src.index(signature)
    inner, _ = brace_body(src, i)
    params = signature[:signature.rindex(')') + 1]  
    params = params[params.index('('):]              
    return ('static void %s%s {%s\n}\n' % (new_name, params, inner))

blocks = {}
blocks['old_cam_rec0'] = dedent(slice_between(
    OLD, '    W(fd, cam + 0x00, 4);', '    {  // sparse scan over camera keys'))
blocks['old_cam_sparse'] = dedent(slice_between(
    OLD, '    {  // sparse scan over camera keys 1..9999 (0x54 stride)',
    '    // camera misc (0x41CE85..0x41CF3E)'))
blocks['old_light_rec0'] = dedent(slice_between(
    OLD, '    W(fd, light + 0x00, 4);', '    {  // sparse scan over light keys'))
blocks['old_light_sparse'] = dedent(slice_between(
    OLD, '    {  // sparse scan over light keys 1..9999 (0x28 stride)',
    '    // light misc (0x41D207..0x41D28F)'))
blocks['old_sel_rec0'] = dedent(slice_between(
    OLD, '    W(fd, sel + 0x00, 4);', '    {  // sparse scan over selection keys'))
blocks['old_sel_sparse'] = dedent(slice_between(
    OLD, '    {  // sparse scan over selection keys 1..9999 (0x24 stride)',
    '    {\n        const unsigned char b = s->state.selfShadowCfgOrUint32'))
blocks['old_shadow_rec0'] = dedent(slice_between(
    OLD, '    W(fd, shadow + 0x00, 4);',
    '    {  // sparse scan over self-shadow keys'))
blocks['old_shadow_sparse'] = dedent(slice_between(
    OLD, '    {  // sparse scan over self-shadow keys 1..9999 (0x18 stride)',
    '    // ---- 10. config2'))

blocks['new_writers'] = slice_between(
    NEW, 'void WritePmmCameraKey(int fd, const mdl::CameraKey& key) {',
    '// Shift-JIS texts from the original .rdata')

blocks['new_cam_sparse'] = dedent(slice_between(
    NEW, '    {  // sparse scan over camera keys 1..9999 (sizeof(mdl::CameraKey) stride)',
    '    // camera misc (0x41CE85..0x41CF3E)'))
blocks['new_light_sparse'] = dedent(slice_between(
    NEW, '    {  // sparse scan over light keys 1..9999 (sizeof(mdl::LightKey) stride)',
    '    // light misc (0x41D207..0x41D28F)'))
blocks['new_sel_sparse'] = dedent(slice_between(
    NEW, '    {  // sparse scan over selection keys 1..9999',
    '    {\n        const unsigned char b = s->state.selfShadowEnabled'))
blocks['new_shadow_sparse'] = dedent(slice_between(
    NEW, '    {  // sparse scan over self-shadow keys 1..9999',
    '    // ---- 10. config2'))

old_readers = []
old_readers.append(plain_fn_to_fn(
    OLDV1, 'void ReadV1BoneKey(int fd, mdl::BoneKey& key) {',
    'old_v1_ReadV1BoneKey'))
old_readers.append(plain_fn_to_fn(
    OLDV1, 'void ReadV1MorphKey(int fd, mdl::MorphKey& key) {',
    'old_v1_ReadV1MorphKey'))
old_readers.append(lambda_to_fn(
    OLDV1, 'const auto readCamRecord = [&](mdl::CameraKey& key) {',
    'mdl::CameraKey', 'old_v1_readCamRecord'))
old_readers.append(lambda_to_fn(
    OLDV1, 'const auto readLightRecord = [&](mdl::LightKey& key) {',
    'mdl::LightKey', 'old_v1_readLightRecord'))
old_readers.append(lambda_to_fn(
    OLDV1, 'const auto readAccRecord = [&](mdl::AccessoryKey& key) {',
    'mdl::AccessoryKey', 'old_v1_readAccRecord'))
old_readers.append(lambda_to_fn(
    OLDV1, 'const auto readSelRecord =',
    'mdl::SelfShadowKey', 'old_v1_readSelRecord'))
old_readers.append(plain_fn_to_fn(
    OLDV2, 'void ReadPmmBoneKey(int fd, mdl::BoneKey& key) {',
    'old_v2_ReadPmmBoneKey'))
old_readers.append(plain_fn_to_fn(
    OLDV2, 'void ReadPmmMorphKey(int fd, mdl::MorphKey& key) {',
    'old_v2_ReadPmmMorphKey'))
old_readers.append(lambda_to_fn(
    OLDV2, 'const auto readCamRecord = [&](mdl::CameraKey& key) {',
    'mdl::CameraKey', 'old_v2_readCamRecord'))
old_readers.append(lambda_to_fn(
    OLDV2, 'const auto readLightRecord = [&](mdl::LightKey& key) {',
    'mdl::LightKey', 'old_v2_readLightRecord'))
old_readers.append(lambda_to_fn(
    OLDV2, 'const auto readAccRecord = [&](mdl::AccessoryKey& key) {',
    'mdl::AccessoryKey', 'old_v2_readAccRecord'))
old_readers.append(lambda_to_fn(
    OLDV2, 'const auto readGravityRecord = [&](mdl::GravityKey& key) {',
    'mdl::GravityKey', 'old_v2_readGravityRecord'))
old_readers.append(lambda_to_fn(
    OLDV2, 'const auto readShadowRecord = [&](mdl::SelfShadowKey& key) {',
    'mdl::SelfShadowKey', 'old_v2_readShadowRecord'))

blocks['old_readers'] = '\n'.join(old_readers)

harness = r'''// GENERATED by tools/wave4a_make_verify.py - Wave4-A behavior-fidelity
// harness.  OLD blocks = git HEAD pmm_save.cpp raw-offset writes + git HEAD
// loader readers; NEW blocks = working-tree typed writers + shared readers.
// Do not edit by hand.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <vector>
#include <random>
#include <string>

#include "rinmotiondance/global_key_layout.hpp"
#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/model.hpp"

// Memory-backed _read shim: routes the shared header's Rd (and therefore all
// ReadPmm* readers) onto g_in for stream-equality tests.  Production builds
// are unaffected - this define exists only inside the harness TU.
static std::vector<unsigned char>* g_in = nullptr;
static std::size_t g_in_pos = 0;
static inline int verify_read(int, void* buf, unsigned int count) {
    if (g_in == nullptr) return 0;
    if (g_in_pos >= g_in->size()) return 0;
    if (g_in_pos + count > g_in->size())
        count = static_cast<unsigned int>(g_in->size() - g_in_pos);
    std::memcpy(buf, g_in->data() + g_in_pos, count);
    g_in_pos += count;
    return static_cast<int>(count);
}
#define _read verify_read
#include "pmm_io_common.hpp"
#undef _read

using namespace rinmotiondance;
using namespace rinmotiondance::pmm_io;

// ---- sink-backed W --------------------------------------------------------
static std::vector<unsigned char> g_out;
static int fd = 0;
static void W(int, const void* buf, unsigned int count) {
    const unsigned char* p = static_cast<const unsigned char*>(buf);
    g_out.insert(g_out.end(), p, p + count);
}
static std::vector<unsigned char> old_cam_rec0(const unsigned char* cam) {
    g_out.clear();
__OLD_CAM_REC0__
    return g_out;
}
static std::vector<unsigned char> old_cam_sparse(const unsigned char* cam) {
    g_out.clear();
__OLD_CAM_SPARSE__
    return g_out;
}
static std::vector<unsigned char> old_light_rec0(const unsigned char* light) {
    g_out.clear();
__OLD_LIGHT_REC0__
    return g_out;
}
static std::vector<unsigned char> old_light_sparse(const unsigned char* light) {
    g_out.clear();
__OLD_LIGHT_SPARSE__
    return g_out;
}
static std::vector<unsigned char> old_sel_rec0(const unsigned char* sel) {
    g_out.clear();
__OLD_SEL_REC0__
    return g_out;
}
static std::vector<unsigned char> old_sel_sparse(const unsigned char* sel) {
    g_out.clear();
__OLD_SEL_SPARSE__
    return g_out;
}
static std::vector<unsigned char> old_shadow_rec0(const unsigned char* shadow) {
    g_out.clear();
__OLD_SHADOW_REC0__
    return g_out;
}
static std::vector<unsigned char> old_shadow_sparse(const unsigned char* shadow) {
    g_out.clear();
__OLD_SHADOW_SPARSE__
    return g_out;
}

// ---- OLD loader readers (verbatim from git HEAD loader bodies) ------------
__OLD_READERS__

// ---- NEW typed writers + count template (verbatim from working tree) ------
namespace {
__NEW_WRITERS__
}

// NEW call-site shapes (record 0 + sparse), wrapped with the same locals the
// SaveSceneFile body provides.
static std::vector<unsigned char> new_cam(const mdl::CameraKey* cam) {
    g_out.clear();
    WritePmmCameraKey(fd, cam[0]);
__NEW_CAM_SPARSE__
    return g_out;
}
static std::vector<unsigned char> new_light(const mdl::LightKey* light) {
    g_out.clear();
    WritePmmLightKey(fd, light[0]);
__NEW_LIGHT_SPARSE__
    return g_out;
}
static std::vector<unsigned char> new_sel(const mdl::GravityKey* sel) {
    g_out.clear();
    WritePmmGravityKey(fd, sel[0]);
__NEW_SEL_SPARSE__
    return g_out;
}
static std::vector<unsigned char> new_shadow(const mdl::SelfShadowKey* shadow) {
    g_out.clear();
    WritePmmSelfShadowKey(fd, shadow[0]);
__NEW_SHADOW_SPARSE__
    return g_out;
}

// ---- comparison driver ----------------------------------------------------
static int g_failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { ++g_failures; \
        std::printf("FAIL %s\n", msg); } \
} while (0)

static void report(const char* what, const std::vector<unsigned char>& a,
                   const std::vector<unsigned char>& b) {
    if (a == b) return;
    ++g_failures;
    std::printf("FAIL %s: %zu vs %zu bytes\n", what, a.size(), b.size());
    for (std::size_t i = 0; i < a.size() && i < b.size(); ++i)
        if (a[i] != b[i]) {
            std::printf("  first diff @%zu: old=%02X new=%02X\n", i, a[i],
                        b[i]);
            break;
        }
}

template <typename TKey, typename OldRec0, typename OldSparse, typename NewAll>
void compare_track(const char* name, unsigned seed, OldRec0 oldRec0,
                   OldSparse oldSparse, NewAll newAll) {
    std::mt19937 rng(seed);
    for (int pattern = 0; pattern < 4; ++pattern) {
        std::vector<TKey> track(10000);
        std::vector<unsigned char> raw(
            reinterpret_cast<unsigned char*>(track.data()),
            reinterpret_cast<unsigned char*>(track.data()) + track.size()
                * sizeof(TKey));
        for (auto& b : raw) b = static_cast<unsigned char>(rng() & 0xFF);
        // occupancy patterns: 0 all-zero frames, 1 dense-random, 2 sparse,
        // 3 mostly-first-half
        for (std::size_t i = 0; i < track.size(); ++i) {
            switch (pattern) {
                case 0: track[i].frame = 0; break;
                case 1: track[i].frame = rng(); break;
                case 2: track[i].frame = (rng() % 1000) == 0 ? rng() : 0;
                        break;
                case 3: track[i].frame =
                    i < 5000 ? (rng() % 3 == 0 ? rng() : 0) : 0; break;
            }
        }
        const unsigned char* base =
            reinterpret_cast<const unsigned char*>(track.data());
        std::vector<unsigned char> oldBytes = oldRec0(base);
        std::vector<unsigned char> sp = oldSparse(base);
        oldBytes.insert(oldBytes.end(), sp.begin(), sp.end());
        std::vector<unsigned char> newBytes = newAll(track.data());
        report((std::string(name) + " pattern" +
                std::to_string(pattern)).c_str(), oldBytes, newBytes);
    }
}

// ---- reader equivalence: old (HEAD) vs new (shared header) -----------------
template <typename Key, typename OldFn, typename NewFn>
void compare_reader(const char* name, std::size_t expectBytes, OldFn oldFn,
                    NewFn newFn) {
    const std::size_t n = expectBytes + 16;   // tail must stay unread
    std::vector<unsigned char> s(n);
    for (std::size_t k = 0; k < n; ++k)
        s[k] = static_cast<unsigned char>((k * 7 + 3) & 0xFF);
    Key a, b;
    std::memset(&a, 0xAA, sizeof a);
    std::memset(&b, 0xAA, sizeof b);
    g_in = &s; g_in_pos = 0;
    oldFn(0, a);
    const std::size_t usedOld = g_in_pos;
    g_in_pos = 0;
    newFn(0, b);
    const std::size_t usedNew = g_in_pos;
    CHECK(usedOld == expectBytes,
          (std::string(name) + ": old consumed wrong count").c_str());
    CHECK(usedNew == expectBytes,
          (std::string(name) + ": new consumed wrong count").c_str());
    CHECK(std::memcmp(&a, &b, sizeof(Key)) == 0,
          (std::string(name) + ": record bytes diverge (incl. padding)").c_str());
    g_in = nullptr;
}

// write -> read round trip through the NEW writer + NEW reader (proves the
// typed writer stream matches the shared reader field-for-field).
static void clamp_flags(mdl::CameraKey& k);
static void clamp_flags(mdl::LightKey& k);
static void clamp_flags(mdl::GravityKey& k);
static void clamp_flags(mdl::SelfShadowKey& k);
template <typename Key, typename WriteFn, typename ReadFn>
void round_trip(const char* name, unsigned seed, WriteFn wf, ReadFn rf) {
    std::mt19937 rng(seed);
    Key src, dst;
    std::vector<unsigned char> raw(sizeof(Key));
    for (auto& b : raw) b = static_cast<unsigned char>(rng() & 0xFF);
    std::memcpy(&src, raw.data(), sizeof(Key));
    // clamp single-byte flag fields to 0/1 like real data
    clamp_flags(src);
    g_out.clear();
    wf(fd, src);
    const std::vector<unsigned char> first = g_out;
    g_in = &g_out; g_in_pos = 0;
    std::memset(&dst, 0xAA, sizeof dst);
    rf(0, dst);
    CHECK(g_in_pos == first.size(),
          (std::string(name) + ": round-trip consumed != produced").c_str());
    g_out.clear();
    wf(fd, dst);
    CHECK(g_out == first,
          (std::string(name) + ": round-trip stream differs").c_str());
    g_in = nullptr;
}

static void clamp_flags(mdl::CameraKey& k) {
    k.perspective = k.perspective ? 1 : 0;
    k.selected = k.selected ? 1 : 0;
}
static void clamp_flags(mdl::LightKey& k) { k.selected = k.selected ? 1 : 0; }
static void clamp_flags(mdl::GravityKey& k) {
    k.noiseEnabled = k.noiseEnabled ? 1 : 0;
    k.selected = k.selected ? 1 : 0;
}
static void clamp_flags(mdl::SelfShadowKey& k) {
    k.selected = k.selected ? 1 : 0;
}

int main() {
    std::printf("sizeof: CameraKey=%zu LightKey=%zu GravityKey=%zu "
                "SelfShadowKey=%zu BoneKey=%zu MorphKey=%zu AccessoryKey=%zu\n",
                sizeof(mdl::CameraKey), sizeof(mdl::LightKey),
                sizeof(mdl::GravityKey), sizeof(mdl::SelfShadowKey),
                sizeof(mdl::BoneKey), sizeof(mdl::MorphKey),
                sizeof(mdl::AccessoryKey));

    for (unsigned seed = 1; seed <= 8; ++seed) {
        compare_track<mdl::CameraKey>(
            "camera", seed, old_cam_rec0, old_cam_sparse,
            [](const mdl::CameraKey* t) { return new_cam(t); });
        compare_track<mdl::LightKey>(
            "light", seed, old_light_rec0, old_light_sparse,
            [](const mdl::LightKey* t) { return new_light(t); });
        compare_track<mdl::GravityKey>(
            "selection", seed, old_sel_rec0, old_sel_sparse,
            [](const mdl::GravityKey* t) { return new_sel(t); });
        compare_track<mdl::SelfShadowKey>(
            "self-shadow", seed, old_shadow_rec0, old_shadow_sparse,
            [](const mdl::SelfShadowKey* t) { return new_shadow(t); });
    }
    std::printf(g_failures == 0
        ? "SAVE SIDE: old raw-offset streams == new typed streams "
          "(byte-identical, 8 seeds x 4 occupancy patterns x 4 tracks)\n"
        : "SAVE SIDE: FAILURES ABOVE\n");

    // ---- LOAD SIDE: old readers (HEAD loader bodies) vs shared readers ----
    // stream sizes: bone v1 57 / v2 58; morph 17; cam v1 72 / v2 80;
    // light 37; shadow 18; gravity 34; accessory 55
    compare_reader<mdl::BoneKey>("bone v1", 57, old_v1_ReadV1BoneKey,
        [](int f, mdl::BoneKey& k) {
            ReadPmmBoneKey<PmmStream::V1>(f, k); });
    compare_reader<mdl::BoneKey>("bone v2", 58, old_v2_ReadPmmBoneKey,
        [](int f, mdl::BoneKey& k) {
            ReadPmmBoneKey<PmmStream::V2>(f, k); });
    compare_reader<mdl::MorphKey>("morph v1", 17, old_v1_ReadV1MorphKey,
        [](int f, mdl::MorphKey& k) { ReadPmmMorphKey(f, k); });
    compare_reader<mdl::MorphKey>("morph v2", 17, old_v2_ReadPmmMorphKey,
        [](int f, mdl::MorphKey& k) { ReadPmmMorphKey(f, k); });
    compare_reader<mdl::CameraKey>("camera v1", 70, old_v1_readCamRecord,
        [](int f, mdl::CameraKey& k) {
            ReadPmmCameraKey<PmmStream::V1>(f, k); });
    compare_reader<mdl::CameraKey>("camera v2", 78, old_v2_readCamRecord,
        [](int f, mdl::CameraKey& k) {
            ReadPmmCameraKey<PmmStream::V2>(f, k); });
    compare_reader<mdl::LightKey>("light v1", 37, old_v1_readLightRecord,
        [](int f, mdl::LightKey& k) { ReadPmmLightKey(f, k); });
    compare_reader<mdl::LightKey>("light v2", 37, old_v2_readLightRecord,
        [](int f, mdl::LightKey& k) { ReadPmmLightKey(f, k); });
    compare_reader<mdl::AccessoryKey>("accessory v1", 51,
        old_v1_readAccRecord,
        [](int f, mdl::AccessoryKey& k) { ReadPmmAccessoryKey(f, k); });
    compare_reader<mdl::AccessoryKey>("accessory v2", 51,
        old_v2_readAccRecord,
        [](int f, mdl::AccessoryKey& k) { ReadPmmAccessoryKey(f, k); });
    compare_reader<mdl::SelfShadowKey>("self-shadow v1", 18,
        old_v1_readSelRecord,
        [](int f, mdl::SelfShadowKey& k) { ReadPmmSelfShadowKey(f, k); });
    compare_reader<mdl::SelfShadowKey>("self-shadow v2", 18,
        old_v2_readShadowRecord,
        [](int f, mdl::SelfShadowKey& k) { ReadPmmSelfShadowKey(f, k); });
    compare_reader<mdl::GravityKey>("gravity v2", 34, old_v2_readGravityRecord,
        [](int f, mdl::GravityKey& k) { ReadPmmGravityKey(f, k); });
    std::printf(g_failures == 0
        ? "LOAD SIDE: old reader records == shared reader records "
          "(identical bytes consumed + identical record bytes)\n"
        : "LOAD SIDE: FAILURES ABOVE\n");

    // ---- v1/v2 divergence points of the parameterized readers -------------
    {
        std::vector<unsigned char> s(128);
        for (std::size_t k = 0; k < s.size(); ++k) s[k] =
            static_cast<unsigned char>((k * 7 + 3) & 0xFF);
        mdl::BoneKey bk;
        std::memset(&bk, 0xAA, sizeof bk);
        g_in = &s; g_in_pos = 0;
        ReadPmmBoneKey<PmmStream::V1>(0, bk);
        CHECK(g_in_pos == 57, "v1 bone reader must consume 57 bytes");
        CHECK(bk.physicsDisabled == 0xAA,
              "v1 bone reader must NOT touch physicsDisabled");
        g_in_pos = 0;
        std::memset(&bk, 0xAA, sizeof bk);
        ReadPmmBoneKey<PmmStream::V2>(0, bk);
        CHECK(g_in_pos == 58, "v2 bone reader must consume 58 bytes");
        CHECK(bk.physicsDisabled == s[57],
              "v2 bone reader physicsDisabled mapping (raw byte)");
        mdl::CameraKey ck;
        std::memset(&ck, 0xAA, sizeof ck);
        g_in_pos = 0;
        ReadPmmCameraKey<PmmStream::V1>(0, ck);
        CHECK(g_in_pos == 70, "v1 camera reader must consume 70 bytes");
        CHECK(ck.parentModel == -1431655766 && ck.parentBone == -1431655766,
              "v1 camera reader must NOT touch parentModel/parentBone");
        g_in_pos = 0;
        std::memset(&ck, 0xAA, sizeof ck);
        ReadPmmCameraKey<PmmStream::V2>(0, ck);
        CHECK(g_in_pos == 78, "v2 camera reader must consume 78 bytes");
        int m, b2;
        std::memcpy(&m, &s[0x28], 4);
        std::memcpy(&b2, &s[0x2C], 4);
        CHECK(ck.parentModel == m && ck.parentBone == b2,
              "v2 camera reader parent mapping");
        g_in = nullptr;
    }
    std::printf(g_failures == 0
        ? "DIVERGENCE POINTS: v1/v2 template specializations behave as "
          "documented\n" : "DIVERGENCE POINTS: FAILURES ABOVE\n");

    // ---- typed writer <-> shared reader round trips ------------------------
    for (unsigned seed = 100; seed <= 116; ++seed) {
        round_trip<mdl::CameraKey>("camera rt", seed,
            [](int f, const mdl::CameraKey& k) {
                WritePmmCameraKey(f, k); },
            [](int f, mdl::CameraKey& k) {
                ReadPmmCameraKey<PmmStream::V2>(f, k); });
        round_trip<mdl::LightKey>("light rt", seed,
            [](int f, const mdl::LightKey& k) { WritePmmLightKey(f, k); },
            [](int f, mdl::LightKey& k) { ReadPmmLightKey(f, k); });
        round_trip<mdl::GravityKey>("gravity rt", seed,
            [](int f, const mdl::GravityKey& k) {
                WritePmmGravityKey(f, k); },
            [](int f, mdl::GravityKey& k) { ReadPmmGravityKey(f, k); });
        round_trip<mdl::SelfShadowKey>("shadow rt", seed,
            [](int f, const mdl::SelfShadowKey& k) {
                WritePmmSelfShadowKey(f, k); },
            [](int f, mdl::SelfShadowKey& k) {
                ReadPmmSelfShadowKey(f, k); });
    }
    std::printf(g_failures == 0
        ? "ROUND TRIPS: typed writer streams re-read to identical records "
          "(17 seeds x 4 tracks)\n" : "ROUND TRIPS: FAILURES ABOVE\n");

    std::printf(g_failures == 0 ? "ALL CHECKS PASSED\n" : "CHECKS FAILED\n");
    return g_failures == 0 ? 0 : 1;
}
'''

harness = harness.replace('__OLD_CAM_REC0__', blocks['old_cam_rec0'])
harness = harness.replace('__OLD_CAM_SPARSE__', blocks['old_cam_sparse'])
harness = harness.replace('__OLD_LIGHT_REC0__', blocks['old_light_rec0'])
harness = harness.replace('__OLD_LIGHT_SPARSE__', blocks['old_light_sparse'])
harness = harness.replace('__OLD_SEL_REC0__', blocks['old_sel_rec0'])
harness = harness.replace('__OLD_SEL_SPARSE__', blocks['old_sel_sparse'])
harness = harness.replace('__OLD_SHADOW_REC0__', blocks['old_shadow_rec0'])
harness = harness.replace('__OLD_SHADOW_SPARSE__', blocks['old_shadow_sparse'])
harness = harness.replace('__NEW_WRITERS__', blocks['new_writers'])
harness = harness.replace('__NEW_CAM_SPARSE__', blocks['new_cam_sparse'])
harness = harness.replace('__NEW_LIGHT_SPARSE__', blocks['new_light_sparse'])
harness = harness.replace('__NEW_SEL_SPARSE__', blocks['new_sel_sparse'])
harness = harness.replace('__NEW_SHADOW_SPARSE__', blocks['new_shadow_sparse'])
harness = harness.replace('__OLD_READERS__', blocks['old_readers'])

os.makedirs(os.path.join(ROOT, 'build', 'wave4a'), exist_ok=True)
out = os.path.join(ROOT, 'build', 'wave4a', 'wave4a_verify.cpp')
open(out, 'w', encoding='utf-8', newline='\n').write(harness)
print('wrote', out, len(harness), 'bytes')
