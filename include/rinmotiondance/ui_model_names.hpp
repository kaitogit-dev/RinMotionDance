#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <algorithm>
#include <cstring>

namespace rinmotiondance {

inline std::string ModelDisplayName(const char* nativeName,
                                    const char* englishName) {
    if (englishName != nullptr && englishName[0] != '\0') {
        bool readable = true;
        for (const unsigned char* p =
                reinterpret_cast<const unsigned char*>(englishName);
             *p != 0; ++p) {
            if (*p < 0x20 || *p >= 0x7F) {
                readable = false;
                break;
            }
        }
        if (readable)
            return englishName;
    }
    if (nativeName == nullptr || nativeName[0] == '\0')
        return englishName != nullptr ? englishName : "";

    const int count = MultiByteToWideChar(932, 0, nativeName, -1,
                                           nullptr, 0);
    if (count <= 1)
        return nativeName;
    std::wstring name(static_cast<std::size_t>(count), L'\0');
    if (!MultiByteToWideChar(932, 0, nativeName, -1, &name[0], count))
        return nativeName;
    name.resize(static_cast<std::size_t>(count - 1));
    const std::pair<const wchar_t*, const char*> dictionary[] = {
        {L"\u5de6\u3064\u307e\u5148\uff29\uff2b", "Left toe IK"},
        {L"\u53f3\u3064\u307e\u5148\uff29\uff2b", "Right toe IK"},
        {L"\u30bb\u30f3\u30bf\u30fc", "Center"},
        {L"\u5168\u3066\u306e\u89aa", "Mother bone"},
        {L"\u30b0\u30eb\u30fc\u30d6", "Groove"},
        {L"\u4e0a\u534a\u8eab2", "Upper body 2"},
        {L"\u5de6\u3064\u307e\u5148", "Left toe"},
        {L"\u53f3\u3064\u307e\u5148", "Right toe"},
        {L"\u5de6\u8db3\uff29\uff2b", "Left leg IK"},
        {L"\u53f3\u8db3\uff29\uff2b", "Right leg IK"},
        {L"\u5de6\u8db3IK", "Left leg IK"},
        {L"\u53f3\u8db3IK", "Right leg IK"},
        {L"\u307e\u3070\u305f\u304d", "Blink"},
        {L"\u30a6\u30a3\u30f3\u30af", "Wink"},
        {L"\u3073\u3063\u304f\u308a", "Surprised"},
        {L"\u4e0a\u534a\u8eab", "Upper body"},
        {L"\u4e0b\u534a\u8eab", "Lower body"},
        {L"\u5de6\u3072\u3058", "Left elbow"},
        {L"\u53f3\u3072\u3058", "Right elbow"},
        {L"\u5de6\u624b\u9996", "Left wrist"},
        {L"\u53f3\u624b\u9996", "Right wrist"},
        {L"\u5de6\u89aa\u6307", "Left thumb"},
        {L"\u53f3\u89aa\u6307", "Right thumb"},
        {L"\u5de6\u4eba\u6307", "Left index"},
        {L"\u53f3\u4eba\u6307", "Right index"},
        {L"\u5de6\u4e2d\u6307", "Left middle finger"},
        {L"\u53f3\u4e2d\u6307", "Right middle finger"},
        {L"\u5de6\u85ac\u6307", "Left ring finger"},
        {L"\u53f3\u85ac\u6307", "Right ring finger"},
        {L"\u5de6\u5c0f\u6307", "Left pinky"},
        {L"\u53f3\u5c0f\u6307", "Right pinky"},
        {L"\u5de6\u3072\u3056", "Left knee"},
        {L"\u53f3\u3072\u3056", "Right knee"},
        {L"\u5de6\u8db3\u9996", "Left ankle"},
        {L"\u53f3\u8db3\u9996", "Right ankle"},
        {L"\u305d\u306e\u4ed6", "Other"},
        {L"\u306b\u3053\u308a", "Grin"},
        {L"\u60b2\u3057\u307f", "Sad"},
        {L"\u771f\u9762\u76ee", "Serious"},
        {L"\u5de6\u76ee", "Left eye"},
        {L"\u53f3\u76ee", "Right eye"},
        {L"\u4e21\u76ee", "Eyes"},
        {L"\u5de6\u80a9", "Left shoulder"},
        {L"\u53f3\u80a9", "Right shoulder"},
        {L"\u5de6\u8155", "Left arm"},
        {L"\u53f3\u8155", "Right arm"},
        {L"\u5de6\u8db3", "Left leg"},
        {L"\u53f3\u8db3", "Right leg"},
        {L"\u8868\u60c5", "Expressions"},
        {L"\u5168\u8eab", "Whole body"},
        {L"\u7b11\u3044", "Smile"},
        {L"\u6012\u308a", "Angry"},
        {L"\u56f0\u308b", "Troubled"},
        {L"\u7167\u308c", "Blush"},
        {L"\u8170", "Waist"},
        {L"\u9996", "Neck"},
        {L"\u982d", "Head"},
        {L"\u9aea", "Hair"},
        {L"\u9854", "Face"},
        {L"\u76ee", "Eyes"},
        {L"\u53e3", "Mouth"},
        {L"\u7709", "Brows"},
        {L"\u6369", "Twist"},
        {L"\u3042", "A"},
        {L"\u3044", "I"},
        {L"\u3046", "U"},
        {L"\u3048", "E"},
        {L"\u304a", "O"},
        {L"\u5de6", "Left "},
        {L"\u53f3", "Right "},
    };
    for (const auto& entry : dictionary) {
        if (name == entry.first)
            return entry.second;
    }
    for (const auto& entry : dictionary) {
        if (name.compare(0, wcslen(entry.first), entry.first) == 0) {
            const wchar_t* rest = name.c_str() + wcslen(entry.first);
            if (*rest == L'\0')
                return entry.second;
            const int bytes = WideCharToMultiByte(932, 0, rest, -1,
                                                  nullptr, 0, nullptr, nullptr);
            if (bytes > 1) {
                std::string suffix(static_cast<std::size_t>(bytes), '\0');
                WideCharToMultiByte(932, 0, rest, -1,
                                    &suffix[0], bytes, nullptr, nullptr);
                suffix.resize(static_cast<std::size_t>(bytes - 1));
                return std::string(entry.second) + " " + suffix;
            }
        }
    }
    return nativeName;
}

}
