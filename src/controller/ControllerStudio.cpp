// PPSX33 native DualShock-style key mapper (Win32 child window for WinForms host)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define API extern "C" __declspec(dllexport)

using MappingChanged = void(__stdcall*)(const wchar_t*);

namespace {

constexpr wchar_t kClassName[] = L"PPSX33ControllerChild";

constexpr COLORREF kBg      = RGB(7, 13, 24);
constexpr COLORREF kPanel   = RGB(16, 27, 43);
constexpr COLORREF kEdge    = RGB(44, 67, 91);
constexpr COLORREF kText    = RGB(226, 237, 248);
constexpr COLORREF kMuted   = RGB(144, 166, 190);
constexpr COLORREF kAccent  = RGB(51, 190, 220);
constexpr COLORREF kAccent2 = RGB(17, 65, 83);
constexpr COLORREF kCapture = RGB(40, 90, 110);

struct Item {
    const wchar_t* label;
    const wchar_t* property;
    std::wstring key;
    RECT rect{};
};

std::vector<Item> g_items;
HWND g_child = nullptr;
MappingChanged g_changed = nullptr;
int g_capture = -1;
std::wstring g_status = L"Click a binding, then press a key. Escape cancels.";
int g_width = 800;
int g_height = 600;

// Labels / JSON property names match src/gui/KeyMap.cs
const wchar_t* kLabels[] = {
    L"Left stick · Left",  L"Left stick · Down",  L"Left stick · Right", L"Left stick · Up",
    L"Right stick · Left", L"Right stick · Down", L"Right stick · Right", L"Right stick · Up",
    L"Start", L"Select", L"PS button",
    L"Square", L"Cross", L"Circle", L"Triangle",
    L"D-pad · Left", L"D-pad · Down", L"D-pad · Right", L"D-pad · Up",
    L"R1", L"R2", L"R3", L"L1", L"L2", L"L3"
};
const wchar_t* kProps[] = {
    L"LeftStickLeft", L"LeftStickDown", L"LeftStickRight", L"LeftStickUp",
    L"RightStickLeft", L"RightStickDown", L"RightStickRight", L"RightStickUp",
    L"Start", L"Select", L"PsButton",
    L"Square", L"Cross", L"Circle", L"Triangle",
    L"DpadLeft", L"DpadDown", L"DpadRight", L"DpadUp",
    L"R1", L"R2", L"R3", L"L1", L"L2", L"L3"
};
const wchar_t* kDefaults[] = {
    L"Left", L"Down", L"Right", L"Up",
    L"H", L"J", L"K", L"U",
    L"V", L"Space", L"Backspace",
    L"A", L"Z", L"X", L"S",
    L"NumPad4", L"NumPad5", L"NumPad6", L"NumPad8",
    L"R", L"RControlKey", L"LControlKey", L"L", L"RShiftKey", L"LShiftKey"
};
constexpr int kItemCount = 25;

std::wstring EscapeJson(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.size() + 4);
    for (wchar_t c : s) {
        if (c == L'\\' || c == L'"') out += L'\\';
        if (c >= 32) out += c;
    }
    return out;
}

std::wstring ToJson() {
    std::wstring s = L"{";
    for (size_t i = 0; i < g_items.size(); ++i) {
        if (i) s += L",";
        s += L"\"";
        s += g_items[i].property;
        s += L"\":\"";
        s += EscapeJson(g_items[i].key);
        s += L"\"";
    }
    s += L"}";
    return s;
}

void Notify() {
    if (g_changed) {
        auto j = ToJson();
        g_changed(j.c_str());
    }
}

std::wstring KeyName(UINT vk) {
    switch (vk) {
    case VK_LEFT: return L"Left";
    case VK_RIGHT: return L"Right";
    case VK_UP: return L"Up";
    case VK_DOWN: return L"Down";
    case VK_SPACE: return L"Space";
    case VK_BACK: return L"Backspace";
    case VK_RETURN: return L"Enter";
    case VK_ESCAPE: return L"Escape";
    case VK_TAB: return L"Tab";
    case VK_SHIFT: return L"ShiftKey";
    case VK_LSHIFT: return L"LShiftKey";
    case VK_RSHIFT: return L"RShiftKey";
    case VK_CONTROL: return L"ControlKey";
    case VK_LCONTROL: return L"LControlKey";
    case VK_RCONTROL: return L"RControlKey";
    case VK_MENU: return L"Menu";
    case VK_LMENU: return L"LMenu";
    case VK_RMENU: return L"RMenu";
    case VK_DELETE: return L"Delete";
    case VK_INSERT: return L"Insert";
    case VK_HOME: return L"Home";
    case VK_END: return L"End";
    case VK_PRIOR: return L"PageUp";
    case VK_NEXT: return L"PageDown";
    default: break;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
        return L"NumPad" + std::to_wstring(vk - VK_NUMPAD0);
    if (vk >= 'A' && vk <= 'Z')
        return std::wstring(1, static_cast<wchar_t>(vk));
    if (vk >= '0' && vk <= '9')
        return std::wstring(1, static_cast<wchar_t>(vk));
    if (vk >= VK_F1 && vk <= VK_F24)
        return L"F" + std::to_wstring(vk - VK_F1 + 1);
    return L"VK_" + std::to_wstring(vk);
}

void DrawRoundPanel(HDC dc, RECT r, COLORREF fill, COLORREF edge) {
    HBRUSH b = CreateSolidBrush(fill);
    HPEN p = CreatePen(PS_SOLID, 1, edge);
    HGDIOBJ ob = SelectObject(dc, b);
    HGDIOBJ op = SelectObject(dc, p);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, 10, 10);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(b);
    DeleteObject(p);
}

void DrawTextAt(HDC dc, const std::wstring& s, RECT r, COLORREF color, UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, s.c_str(), -1, &r, flags);
}

void Layout() {
    // Two-column list under a header band; action buttons on the bottom bar.
    const int margin = 16;
    const int headerH = 56;
    const int footerH = 56;
    const int cols = 2;
    const int gap = 8;
    const int rowH = 34;
    int contentTop = headerH + 12;
    int contentBottom = g_height - footerH - 8;
    int contentH = (std::max)(40, contentBottom - contentTop);
    int colW = (std::max)(120, (g_width - margin * 2 - gap) / cols);

    for (int i = 0; i < static_cast<int>(g_items.size()); ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = margin + col * (colW + gap);
        int y = contentTop + row * (rowH + gap);
        if (y + rowH > contentBottom) {
            g_items[i].rect = RECT{0, 0, 0, 0};
            continue;
        }
        g_items[i].rect = RECT{x, y, x + colW, y + rowH};
    }
    (void)contentH;
}

void EnsureItems() {
    if (!g_items.empty()) return;
    g_items.reserve(kItemCount);
    for (int i = 0; i < kItemCount; ++i)
        g_items.push_back(Item{kLabels[i], kProps[i], kDefaults[i], {}});
}

std::wstring FindJsonString(const wchar_t* json, const wchar_t* property) {
    if (!json || !property) return {};
    std::wstring token = L"\"";
    token += property;
    token += L"\"";
    std::wstring s(json);
    auto p = s.find(token);
    if (p == std::wstring::npos) return {};
    p = s.find(L':', p + token.size());
    if (p == std::wstring::npos) return {};
    p = s.find(L'"', p + 1);
    if (p == std::wstring::npos) return {};
    std::wstring out;
    for (++p; p < s.size(); ++p) {
        if (s[p] == L'"') break;
        if (s[p] == L'\\' && p + 1 < s.size()) ++p;
        out += s[p];
    }
    return out;
}

void Paint(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);

    HBRUSH bg = CreateSolidBrush(kBg);
    FillRect(dc, &client, bg);
    DeleteObject(bg);

    // Header
    RECT header{12, 10, client.right - 12, 58};
    DrawRoundPanel(dc, header, kPanel, kEdge);
    RECT titleR = header;
    titleR.left += 16;
    DrawTextAt(dc, L"PPSX33  ·  DualShock key mapping", titleR, kAccent, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT sub = header;
    sub.left = titleR.left + 280;
    DrawTextAt(dc, L"Native C++ studio  ·  same bindings as pad_map.json", sub, kMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    Layout();
    HFONT font = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT fontSmall = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, font);

    for (size_t i = 0; i < g_items.size(); ++i) {
        RECT r = g_items[i].rect;
        if (r.right <= r.left) continue;
        COLORREF fill = (static_cast<int>(i) == g_capture) ? kCapture : kPanel;
        DrawRoundPanel(dc, r, fill, (static_cast<int>(i) == g_capture) ? kAccent : kEdge);

        RECT lab = r;
        lab.left += 12;
        lab.right = r.left + (r.right - r.left) * 3 / 5;
        SelectObject(dc, fontSmall);
        DrawTextAt(dc, g_items[i].label, lab, kMuted);

        RECT key = r;
        key.left = lab.right;
        key.right -= 12;
        SelectObject(dc, font);
        DrawTextAt(dc, g_items[i].key, key, kText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    // Footer buttons
    RECT bar{12, client.bottom - 52, client.right - 12, client.bottom - 12};
    DrawRoundPanel(dc, bar, kPanel, kEdge);

    RECT statusR = bar;
    statusR.left += 14;
    statusR.right = bar.right - 430;
    SelectObject(dc, fontSmall);
    DrawTextAt(dc, g_status, statusR, kMuted);

    auto btn = [&](RECT rc, const wchar_t* label, COLORREF fill) {
        DrawRoundPanel(dc, rc, fill, kAccent);
        SelectObject(dc, font);
        DrawTextAt(dc, label, rc, kText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    };

    RECT rcSave{client.right - 108, client.bottom - 47, client.right - 18, client.bottom - 12};
    RECT rcExport{client.right - 208, client.bottom - 47, client.right - 118, client.bottom - 12};
    RECT rcImport{client.right - 308, client.bottom - 47, client.right - 218, client.bottom - 12};
    RECT rcReset{client.right - 414, client.bottom - 47, client.right - 318, client.bottom - 12};

    btn(rcReset, L"RESET", kPanel);
    btn(rcImport, L"IMPORT", kPanel);
    btn(rcExport, L"EXPORT", kPanel);
    btn(rcSave, L"SAVE", kAccent2);

    SelectObject(dc, oldFont);
    DeleteObject(font);
    DeleteObject(fontSmall);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        g_width = LOWORD(lp);
        g_height = HIWORD(lp);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lp);
        int y = GET_Y_LPARAM(lp);
        RECT c{};
        GetClientRect(hwnd, &c);
        POINT pt{x, y};

        RECT rcSave{c.right - 108, c.bottom - 47, c.right - 18, c.bottom - 12};
        RECT rcExport{c.right - 208, c.bottom - 47, c.right - 118, c.bottom - 12};
        RECT rcImport{c.right - 308, c.bottom - 47, c.right - 218, c.bottom - 12};
        RECT rcReset{c.right - 414, c.bottom - 47, c.right - 318, c.bottom - 12};

        if (PtInRect(&rcSave, pt)) {
            Notify();
            g_status = L"Map sent to PPSX33 settings and project output.";
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        if (PtInRect(&rcReset, pt)) {
            for (size_t i = 0; i < g_items.size(); ++i)
                g_items[i].key = kDefaults[i];
            g_capture = -1;
            g_status = L"Default bindings restored. Click SAVE to apply.";
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        if (PtInRect(&rcExport, pt)) {
            wchar_t path[MAX_PATH] = L"ppsx33-key-map.json";
            OPENFILENAMEW ofn{sizeof(ofn)};
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"JSON key maps (*.json)\0*.json\0All files (*.*)\0*.*\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt = L"json";
            if (GetSaveFileNameW(&ofn)) {
                std::wofstream out(path);
                out << ToJson();
                g_status = L"Exported key map JSON.";
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        if (PtInRect(&rcImport, pt)) {
            wchar_t path[MAX_PATH]{};
            OPENFILENAMEW ofn{sizeof(ofn)};
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"JSON key maps (*.json)\0*.json\0All files (*.*)\0*.*\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&ofn)) {
                std::wifstream in(path);
                std::wstringstream buffer;
                buffer << in.rdbuf();
                std::wstring json = buffer.str();
                for (auto& item : g_items) {
                    auto value = FindJsonString(json.c_str(), item.property);
                    if (!value.empty()) item.key = value;
                }
                g_status = L"Imported key map. Click SAVE to apply.";
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }

        Layout();
        g_capture = -1;
        for (size_t i = 0; i < g_items.size(); ++i) {
            if (PtInRect(&g_items[i].rect, pt)) {
                g_capture = static_cast<int>(i);
                g_status = L"Press a keyboard key for the selected control. Escape cancels.";
                break;
            }
        }
        SetFocus(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_KEYDOWN:
        if (g_capture >= 0) {
            if (wp == VK_ESCAPE) {
                g_capture = -1;
                g_status = L"Rebinding cancelled.";
            } else {
                g_items[static_cast<size_t>(g_capture)].key = KeyName(static_cast<UINT>(wp));
                g_status = L"Binding updated. Click SAVE to apply it.";
                g_capture = -1;
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        return 0;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_ERASEBKGND:
        return 1;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

API HWND __stdcall PPSX33_ControllerCreate(HWND parent, MappingChanged callback, const wchar_t* initialJson) {
    EnsureItems();
    g_changed = callback;
    if (initialJson) {
        for (auto& item : g_items) {
            auto value = FindJsonString(initialJson, item.property);
            if (!value.empty()) item.key = value;
        }
    }

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(kBg);
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return nullptr;

    RECT r{};
    GetClientRect(parent, &r);
    g_width = r.right;
    g_height = r.bottom;

    g_child = CreateWindowExW(
        0, kClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
        0, 0, g_width, g_height,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (g_child) {
        SetFocus(g_child);
        InvalidateRect(g_child, nullptr, TRUE);
    }
    return g_child;
}

API void __stdcall PPSX33_ControllerResize(HWND hwnd, int w, int h) {
    if (!hwnd) return;
    g_width = w;
    g_height = h;
    SetWindowPos(hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(hwnd, nullptr, TRUE);
}

API void __stdcall PPSX33_ControllerSetMap(const wchar_t* json) {
    EnsureItems();
    if (json) {
        for (auto& item : g_items) {
            auto value = FindJsonString(json, item.property);
            if (!value.empty()) item.key = value;
        }
    }
    if (g_child) InvalidateRect(g_child, nullptr, TRUE);
}

API void __stdcall PPSX33_ControllerDestroy(HWND hwnd) {
    if (hwnd && IsWindow(hwnd)) DestroyWindow(hwnd);
    if (g_child == hwnd) g_child = nullptr;
}
