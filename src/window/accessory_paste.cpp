#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "rinmotiondance/accessory_layout.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {
namespace {

template <typename T>
T& At(unsigned char* p, std::size_t offset) {
    return *reinterpret_cast<T*>(p + offset);
}
template <typename T>
const T& At(const unsigned char* p, std::size_t offset) {
    return *reinterpret_cast<const T*>(p + offset);
}

const char kJpOverflow[] =
    "\x93\x6f\x98\x5e\x83\x7c\x83\x43\x83\x93\x83\x67"
    "\x90\x94\x82\xaa%d\x8c\xc2\x82\xf0\x89\x7a\x82\xa6"
    "\x82\xdc\x82\xb5\x82\xbd\n\x82\xb1\x82\xea\x88\xc8"
    "\x8f\xe3\x82\xcc\x93\x6f\x98\x5e\x82\xcd\x8d\x73"
    "\x82\xa6\x82\xdc\x82\xb9\x82\xf1\n\x81\x75\xcc\xda"
    "\xb0\xd1\x95\xd2\x8f\x57\x81\x76\x82\xcc\x81\x75"
    "\x95\x73\x97\x70\xcc\xda\xb0\xd1\x8d\xed\x8f\x9c"
    "\x81\x76\x82\xf0\x8e\xc0\x8d\x73\x82\xb5\x82\xc4"
    "\x89\xba\x82\xb3\x82\xa2";
const char kJpTitle[] = "\xcc\xda\xb0\xd1\x93\x6f\x98\x5e";

std::uint32_t FindFreeAccSlot(MMDApp* app, mdl::AccessoryKey* table) {
    std::uint32_t freeIndex = 1;
    while (table[freeIndex].frame != 0) {
        ++freeIndex;
        if (freeIndex >= 10000) {
            char message[0x100];
            if (app->state.englishUI != 0) {
                sprintf_s(message, sizeof(message),
                          "You cannot regist over %dpoint.\n"
                          "Please execute 'delete unused frame'", 10000);
                MessageBoxA(app->state.hwnd, message,
                            "register frame", 0);
            } else {
                sprintf_s(message, sizeof(message), kJpOverflow, 10000);
                MessageBoxA(app->state.hwnd, message,
                            kJpTitle, 0);
            }
            return 0;
        }
    }
    return freeIndex;
}

void FillAccKeyRecord(mdl::AccessoryKey& dst,
                      const mdl::AccessoryClipboardKey& src,
                      std::uint32_t frame) {
    dst.frame = frame;
    std::memcpy(&dst.visible, &src.visible, 4);
    dst.shadowEnabled = src.shadowEnabled;
    dst.parentModel = src.parentModel;
    dst.parentBone = src.parentBone;
    std::memcpy(dst.position, src.position, sizeof(dst.position));
    std::memcpy(dst.rotation, src.rotation, sizeof(dst.rotation));
    dst.scale = src.scale;
    dst.opacity = src.opacity;
    dst.selected = 1;
    std::memset(dst.reservedSelection, 0, sizeof(dst.reservedSelection));
}

float UnsignedFrameDelta(std::uint32_t a, std::uint32_t b) {
    const std::int32_t raw =
        static_cast<std::int32_t>(a - b);
    float f = static_cast<float>(raw);
    if (raw < 0) f += 4294967296.0f;
    return f;
}

}  

void* ConstructArrayElements(void* block, std::uint32_t elementSize,
                             std::uint32_t count, void* ctor) {
    const auto fn = reinterpret_cast<void (*)(void*)>(ctor);
    unsigned char* p = static_cast<unsigned char*>(block);
    for (std::int32_t i = static_cast<std::int32_t>(count) - 1; i >= 0;
         --i) {
        fn(p + static_cast<std::size_t>(i) * elementSize);
    }
    return block;
}

void ApplyAccessoryTrack(MMDApp* app, int slot) {  
    if (app == nullptr || slot < 0 || slot >= 255)
        return;
    const std::uint32_t cur =
        static_cast<std::uint32_t>(app->CurrentFrame());
    mdl::AccessoryKey* keys = app->AccessoryKeys(slot);
    auto* obj = app->AccessorySlot(slot);
    if (keys == nullptr || obj == nullptr)
        return;

    std::uint32_t index = 0;
    bool terminal = false;
    if (keys[0].frame < cur) {
        for (;;) {
            const std::uint32_t next = keys[index].next;
            if (next == 0) {
                terminal = true;  
                break;
            }
            index = next;
            if (keys[index].frame >= cur)
                break;
        }
    }

    const auto pushState = [&](const mdl::AccessoryKey& rec,
                               bool withFloats) {
        obj->visible = rec.visible;
        obj->shadowEnabled = rec.shadowEnabled;
        obj->parentModel = rec.parentModel;
        obj->parentBone = rec.parentBone;
        std::memcpy(obj->rotation, rec.rotation, sizeof obj->rotation);
        std::memcpy(obj->position, rec.position, sizeof obj->position);
        if (withFloats) {
            obj->scale = rec.scale;
            obj->opacity = rec.opacity;
        }
    };

    if (terminal) {
        pushState(keys[index], true);
        return;
    }

    mdl::AccessoryKey& rec = keys[index];
    if (rec.frame != cur) {
        const mdl::AccessoryKey& prec = keys[rec.previous];
        const float t =
            UnsignedFrameDelta(cur, prec.frame) /
            UnsignedFrameDelta(rec.frame, prec.frame);
        obj->scale =
            prec.scale + t * (rec.scale - prec.scale);
        obj->opacity =
            prec.opacity + t * (rec.opacity - prec.opacity);
        pushState(prec, false);
        return;
    }

    pushState(rec, true);  
}

int PasteAccessoryKeyRecord(MMDApp* app, void* recData, int useSelectedSlot) {
    const auto& src =
        *static_cast<const mdl::AccessoryClipboardKey*>(recData);
    const std::uint8_t slot =
        useSelectedSlot != 0 ? app->SelectedAccessorySlot() : src.slot;

    if (app->AccessorySlot(slot) == nullptr) {
        return 1;
    }

    mdl::AccessoryKey* table = app->AccessoryKeys(slot);
    const std::uint32_t frame =
        static_cast<std::uint32_t>(app->CurrentFrame()) + src.frameOffset;

    std::uint32_t index = 0;
    bool offEnd = false;
    if (table[0].frame < frame) {
        for (;;) {
            const std::uint32_t next = table[index].next;
            if (next == 0) {
                offEnd = true;  
                break;
            }
            index = next;
            if (table[index].frame >= frame)
                break;
        }
    }

    mdl::AccessoryKey& current = table[index];

    if (!offEnd && current.frame == frame) {
        FillAccKeyRecord(current, src, frame);
        return 1;
    }

    const std::uint32_t freeIndex = FindFreeAccSlot(app, table);
    if (freeIndex == 0) return 0;  

    mdl::AccessoryKey& added = table[freeIndex];
    if (offEnd) {
        current.next = freeIndex;
        added.previous = index;
    } else {
        const std::uint32_t prev = current.previous;
        table[prev].next = freeIndex;
        added.previous = prev;
        current.previous = freeIndex;
        added.next = index;
    }
    FillAccKeyRecord(added, src, frame);

    if (frame > app->state.lastRegisteredFrame) {
        app->state.lastRegisteredFrame = frame;
    }
    return 1;
}

}  
