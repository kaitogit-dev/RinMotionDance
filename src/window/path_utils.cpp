#include <cwchar>

#include "rinmotiondance/ported_funcs.hpp"

namespace rinmotiondance {

wchar_t* ExtractDirFromPath(wchar_t* destination, const wchar_t* fullPath) {
    wchar_t* out = destination;  
    wcscpy_s(out, 1000, fullPath);

    if (*out == L'\0')
        return out;

    wchar_t* p = out;
    while (*p++)
        ;
    --p;                       
    wchar_t* last = p - 1;     
    if (*last == L'\\') {
        *last = L'\0';         
        return out;
    }
    while (last >= out) {
        if (*last == L'\\') {
            *last = L'\0';     
            return out;
        }
        --last;
    }
    *out = L'\0';              
    return out;
}

}  
