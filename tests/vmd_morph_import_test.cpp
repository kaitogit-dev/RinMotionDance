#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

int main() {
    namespace mdl = rinmotiondance::mdl;
    auto model = std::make_unique<mdl::ModelRecord>();
    auto keys = std::make_unique<mdl::MorphKey[]>(mdl::kMorphKeyCapacity);
    mdl::MorphRecord morphs[4]{};

    std::strcpy(morphs[0].name, "base");
    std::strcpy(morphs[1].name, "blink");
    std::strcpy(morphs[2].name, "smile");
    std::strcpy(morphs[2].nameEn, "smile_en");
    std::strcpy(morphs[3].name, "long_morph_name_123");

    model->morphCount = 4;
    model->morphs = morphs;
    model->morphKeys = keys.get();
    model->searchCursor = 4;

    auto import = [&](const char* name, unsigned frame, float value,
                      unsigned offset = 0) {
        unsigned char record[40]{};
        std::memcpy(record, name, std::min(std::strlen(name), size_t(15)));
        std::memcpy(record + 32, &frame, sizeof(frame));
        std::memcpy(record + 36, &value, sizeof(value));
        return rinmotiondance::RegisterMorphKeyFromRecord(
            reinterpret_cast<unsigned char*>(model.get()), record, offset);
    };
    bool ok = true;
    const auto check = [&](bool condition, const char* description) {
        if (!condition) {
            std::fprintf(stderr, "%s\n", description);
            ok = false;
        }
    };
    check(import("blink", 12, 0.8f), "blink import failed");
    check(keys[1].next == 4 && keys[4].frame == 12 &&
          std::fabs(keys[4].value - 0.8f) < 0.00001f &&
          keys[4].allocated == 1, "blink keys missing");

    check(import("blink", 24, 0.2f), "second blink import failed");
    check(keys[4].next == 5 && keys[5].previous == 4 &&
          keys[5].frame == 24 && keys[5].allocated == 1,
          "blink key chain missing");

    check(import("smile", 9, 0.7f), "smile import failed");
    check(keys[2].next == 6 && keys[6].frame == 9 &&
          std::fabs(keys[6].value - 0.7f) < 0.00001f,
          "second morph missing");

    check(import("smile_en", 20, 0.3f), "English alias import failed");
    check(keys[6].next == 7 && keys[7].frame == 20,
          "English alias key missing");

    check(import("long_morph_name_123", 2, 0.5f, 3),
          "truncated morph name import failed");
    check(keys[3].next == 8 && keys[8].frame == 5 &&
          std::fabs(keys[8].value - 0.5f) < 0.00001f,
          "truncated name or frame offset broken");

    const std::uint32_t maxFrame = model->maxFrame;
    check(import("not_in_model", 36, 0.9f), "unknown morph import failed");
    check(model->maxFrame == maxFrame && keys[9].allocated == 0,
          "unknown morph registered");
    return ok ? 0 : 1;
}
