#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/model.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {

void CurvePanelRepaint(MMDApp* app);

namespace {

struct Curve {
    int x1;
    int y1;
    int x2;
    int y2;
};

void DrawCurve(MMDApp* app, const Curve& curve) {  
    HDC dc = app->CurveDC();
    HPEN pen = CreatePen(PS_SOLID, 1, 0xFF0000u);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    int oldX = 0;
    int oldY = 126;
    for (int step = 1; step < 100; ++step) {
        const float t = static_cast<float>(step) / 100.0f;
        const float ax = static_cast<float>(curve.x1) * t;
        const float ay = static_cast<float>(curve.y1) * t;
        const float bx = static_cast<float>(curve.x1) +
                         static_cast<float>(curve.x2 - curve.x1) * t;
        const float by = static_cast<float>(curve.y1) +
                         static_cast<float>(curve.y2 - curve.y1) * t;
        const float cx = static_cast<float>(curve.x2) +
                         static_cast<float>(128 - curve.x2) * t;
        const float cy = static_cast<float>(curve.y2) +
                         static_cast<float>(128 - curve.y2) * t;
        const float dx = ax + (bx - ax) * t;
        const float dy = ay + (by - ay) * t;
        const float ex = bx + (cx - bx) * t;
        const float ey = by + (cy - by) * t;
        const int x = static_cast<int>(dx + (ex - dx) * t);
        const int y = 126 - static_cast<int>(dy + (ey - dy) * t);
        MoveToEx(dc, oldX, oldY, nullptr);
        LineTo(dc, x, y);
        oldX = x;
        oldY = y;
    }
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void DrawControlPoints(MMDApp* app, const Curve& curve) {  
    HDC dc = app->CurveDC();
    HPEN pen = CreatePen(PS_SOLID, 2, 0x000000FFu);
    HGDIOBJ oldPen = SelectObject(dc, pen);

    app->state.interpCurveControlCache[0] = static_cast<std::uint8_t>(curve.x1);
    app->state.interpCurveControlCache[1] =
        static_cast<std::uint8_t>(127 - curve.y1);
    app->state.interpCurveControlCache[2] = static_cast<std::uint8_t>(curve.x2);
    app->state.interpCurveControlCache[3] =
        static_cast<std::uint8_t>(127 - curve.y2);

    MoveToEx(dc, curve.x1 - 3, 124 - curve.y1, nullptr);
    LineTo(dc, curve.x1 + 3, 130 - curve.y1);
    MoveToEx(dc, curve.x1 + 3, 124 - curve.y1, nullptr);
    LineTo(dc, curve.x1 - 3, 130 - curve.y1);
    MoveToEx(dc, curve.x2 - 3, 124 - curve.y2, nullptr);
    LineTo(dc, curve.x2 + 3, 130 - curve.y2);
    MoveToEx(dc, curve.x2 + 3, 124 - curve.y2, nullptr);
    LineTo(dc, curve.x2 - 3, 130 - curve.y2);
    SelectObject(dc, oldPen);
    DeleteObject(pen);

    HPEN guide = CreatePen(PS_SOLID, 0, 0);
    SelectObject(dc, guide);
    MoveToEx(dc, 0, 127, nullptr);
    LineTo(dc, curve.x1, 127 - curve.y1);
    MoveToEx(dc, 127, 0, nullptr);
    LineTo(dc, curve.x2, 127 - curve.y2);
    SelectObject(dc, oldPen);
    DeleteObject(guide);
}

}  

void DragInterpolationControlPoint(MMDApp* app) {  
    const HWND window = static_cast<HWND>(app->Hwnd());
    HDC dc = app->CurveDC();
    HPEN pen = CreatePen(PS_SOLID, 1, 0x00FFFFFFu);              
    HBRUSH brush = CreateSolidBrush(0x00FFFFFFu);                
    SelectObject(dc, pen);
    SelectObject(dc, brush);
    Rectangle(dc, 0, 0, 128, 128);                               
    DeleteObject(pen);
    DeleteObject(brush);

    RECT client{};
    GetClientRect(window, &client);                              
    int x = app->MouseX() - 8;                                   
    if (x < 0) x = 0; else if (x > 127) x = 127;
    int y = client.bottom - app->MouseY() - 8;                   
    if (y < 0) y = 0; else if (y > 127) y = 127;

    Curve drawn{};  
    const int selected = static_cast<int>(SendMessageA(
        GetDlgItem(window, panel::kInterpCurveCombo), CB_GETCURSEL, 0, 0));

    if (app->state.optflag[0] != 0) {   
        unsigned char* records =
            reinterpret_cast<unsigned char*>(app->CameraKeys());
        if (records != nullptr) {
            for (int off = 0; off < 840000; off += 84) {
                if (records[off + 72] == 0)
                    continue;
                const std::uint8_t mode =
                    app->state.pendingTimelineSelectionRow;
                if (mode == 1) {
                    unsigned char* base;
                    if (selected >= 6) {                         
                        for (int c = 0; c < 6; ++c) {
                            records[off + 40 + c] =
                                static_cast<std::uint8_t>(x);
                            records[off + 46 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        base = records + off;
                    } else {                                     
                        records[off + selected + 40] =
                            static_cast<std::uint8_t>(x);
                        records[off + selected + 46] =
                            static_cast<std::uint8_t>(y);
                        base = records + off + selected;
                    }
                    drawn = {x, y,
                             static_cast<std::int8_t>(base[52]),
                             static_cast<std::int8_t>(base[58])};
                } else if (mode == 2) {
                    unsigned char* base;
                    if (selected >= 6) {                         
                        for (int c = 0; c < 6; ++c) {
                            records[off + 52 + c] =
                                static_cast<std::uint8_t>(x);
                            records[off + 58 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        base = records + off;
                    } else {                                     
                        records[off + selected + 52] =
                            static_cast<std::uint8_t>(x);
                        records[off + selected + 58] =
                            static_cast<std::uint8_t>(y);
                        base = records + off + selected;
                    }
                    drawn = {static_cast<std::int8_t>(base[40]),
                             static_cast<std::int8_t>(base[46]), x, y};
                }
            }
        }
    } else {                                                     
        unsigned char* model = app->SelectedModel();
        mdl::BoneKey* records = model != nullptr
            ? mdl::BoneKeys(model) : nullptr;
        if (records != nullptr) {
            for (int index = 0; index < static_cast<int>(mdl::kBoneKeyCapacity); ++index) {
                mdl::BoneKey& record = records[index];
                if (record.allocated == 0)
                    continue;
                const std::uint8_t mode =
                    app->state.pendingTimelineSelectionRow;
                if (mode == 1) {
                    int channel;
                    if (selected >= 4) {                         
                        for (int c = 0; c < 4; ++c) {
                            record.interpolation[c] =
                                static_cast<std::uint8_t>(x);
                            record.interpolation[4 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        channel = 0;
                    } else {                                     
                        record.interpolation[selected] =
                            static_cast<std::uint8_t>(x);
                        record.interpolation[4 + selected] =
                            static_cast<std::uint8_t>(y);
                        channel = selected;
                    }
                    drawn = {x, y,
                             static_cast<std::int8_t>(
                                 record.interpolation[8 + channel]),
                             static_cast<std::int8_t>(
                                 record.interpolation[12 + channel])};
                } else if (mode == 2) {
                    int channel;
                    if (selected >= 4) {                         
                        for (int c = 0; c < 4; ++c) {
                            record.interpolation[8 + c] =
                                static_cast<std::uint8_t>(x);
                            record.interpolation[12 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        channel = 0;
                    } else {                                     
                        record.interpolation[8 + selected] =
                            static_cast<std::uint8_t>(x);
                        record.interpolation[12 + selected] =
                            static_cast<std::uint8_t>(y);
                        channel = selected;
                    }
                    drawn = {
                        static_cast<std::int8_t>(
                            record.interpolation[channel]),
                        static_cast<std::int8_t>(
                            record.interpolation[4 + channel]), x, y};
                }
            }
        }
    }

    DrawCurve(app, drawn);                                       
    DrawControlPoints(app, drawn);                               
    GetClientRect(window, &client);                              
    RECT dirty{8, client.bottom - 135, 137, client.bottom - 6};
    InvalidateRect(window, &dirty, FALSE);                       
}

void SelectionReeval(MMDApp* app) {  
    CurvePanelRepaint(app);
}

}  
