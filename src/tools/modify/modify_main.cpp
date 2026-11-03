/* PPSX33 Modify — per-game launcher (no Qt). Dark themed Win32 UI. */
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "msimg32.lib")

static std::string g_dir, g_game_exe = "game.exe", g_key;
static int g_api = 0, g_res = 0, g_tab = 0;
static HWND g_hwnd, g_log, g_tabs[3], g_pageApi, g_pageRes, g_pagePad;
static HWND g_cbApi, g_cbRes;
static HFONT g_font, g_fontBold;
static HBRUSH g_brBg, g_brPanel, g_brAccent;
static std::vector<std::pair<std::string,std::string>> g_keys;

static std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}
static std::wstring wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}
static std::string join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    char c = a.back();
    if (c == '\\' || c == '/') return a + b;
    return a + "\\" + b;
}

static void load_launch() {
    std::ifstream f(join(g_dir, "ppsx33_launch.json"));
    if (!f) return;
    std::stringstream ss; ss << f.rdbuf();
    std::string j = ss.str();
    auto pick = [&](const char* key) -> std::string {
        std::string k = std::string("\"") + key + "\"";
        size_t i = j.find(k); if (i == std::string::npos) return {};
        i = j.find(':', i); if (i == std::string::npos) return {};
        i = j.find('"', i); if (i == std::string::npos) return {};
        size_t e = j.find('"', i + 1); if (e == std::string::npos) return {};
        return j.substr(i + 1, e - i - 1);
    };
    auto g = pick("game"); if (!g.empty()) g_game_exe = g;
    auto k = pick("key"); if (!k.empty()) g_key = k;
    auto a = pick("api"); if (a == "OpenGL" || a == "opengl") g_api = 1;
}

static void load_keymap() {
    g_keys.clear();
    std::string path = join(g_dir, "keymap.json");
    if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        path = join(g_dir, "pad_map.json");
    }
    std::ifstream f(path);
    if (!f) {
        g_keys = {{"Cross","Z"},{"Circle","X"},{"Square","A"},{"Triangle","S"},
                  {"Start","V"},{"Select","Space"},{"DpadUp","Up"},{"DpadDown","Down"},
                  {"DpadLeft","Left"},{"DpadRight","Right"},{"L1","Q"},{"R1","E"}};
        return;
    }
    std::stringstream ss; ss << f.rdbuf();
    std::string j = ss.str();
    size_t i = 0;
    while ((i = j.find('"', i)) != std::string::npos) {
        size_t e = j.find('"', i + 1); if (e == std::string::npos) break;
        std::string key = j.substr(i + 1, e - i - 1);
        i = e + 1;
        size_t c = j.find(':', i); if (c == std::string::npos) break;
        size_t v0 = j.find('"', c); if (v0 == std::string::npos) break;
        size_t v1 = j.find('"', v0 + 1); if (v1 == std::string::npos) break;
        std::string val = j.substr(v0 + 1, v1 - v0 - 1);
        if (key != "api" && key != "game" && key != "key")
            g_keys.push_back({key, val});
        i = v1 + 1;
    }
}

static void save_prefs() {
    std::ofstream f(join(g_dir, "ppsx33_modify.json"));
    if (!f) return;
    const char* api = g_api ? "OpenGL" : "Vulkan";
    int h = (g_res == 2) ? 480 : (g_res == 1) ? 560 : 720;
    f << "{\n  \"api\": \"" << api << "\",\n  \"height\": " << h << ",\n  \"width\": 720\n}\n";
}

static void log_line(const char* s) {
    if (!g_log) return;
    std::wstring w = wide(std::string(s) + "\r\n");
    int n = GetWindowTextLengthW(g_log);
    SendMessageW(g_log, EM_SETSEL, n, n);
    SendMessageW(g_log, EM_REPLACESEL, FALSE, (LPARAM)w.c_str());
}

static void do_play() {
    save_prefs();
    std::string exe = join(g_dir, g_game_exe);
    std::string cmd = "\"" + exe + "\" -nogui";
    if (!g_key.empty()) { cmd += " -key="; cmd += g_key; }
    cmd += g_api ? " -api=opengl" : " -api=vulkan";
    if (g_res == 1) cmd += " -h=560";
    else if (g_res == 2) cmd += " -h=480";
    else cmd += " -h=720";
    log_line(cmd.c_str());
    STARTUPINFOA si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string mutable_cmd = cmd;
    if (!CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, g_dir.c_str(), &si, &pi)) {
        log_line("CreateProcess failed");
        return;
    }
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    log_line("launched");
}

static void show_tab(int t) {
    g_tab = t;
    ShowWindow(g_pageApi, t == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_pageRes, t == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_pagePad, t == 2 ? SW_SHOW : SW_HIDE);
    InvalidateRect(g_hwnd, nullptr, TRUE);
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        g_brBg = CreateSolidBrush(RGB(12, 16, 28));
        g_brPanel = CreateSolidBrush(RGB(18, 24, 40));
        g_brAccent = CreateSolidBrush(RGB(0, 200, 220));
        g_font = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontBold = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

        CreateWindowW(L"STATIC", L"PPSX33  MODIFY", WS_CHILD | WS_VISIBLE, 24, 16, 280, 24, h, 0, 0, 0);
        CreateWindowW(L"STATIC", L"Per-game launcher · dual API", WS_CHILD | WS_VISIBLE, 24, 40, 320, 18, h, 0, 0, 0);

        for (int i = 0; i < 3; ++i) {
            const wchar_t* lab[] = { L"Graphics", L"Quality", L"Controller" };
            g_tabs[i] = CreateWindowW(L"BUTTON", lab[i], WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                24 + i * 110, 72, 100, 32, h, (HMENU)(INT_PTR)(301 + i), 0, 0);
        }

        g_pageApi = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 24, 120, 420, 100, h, 0, 0, 0);
        CreateWindowW(L"STATIC", L"Present API (both built into runtime)", WS_CHILD | WS_VISIBLE, 0, 0, 360, 18, g_pageApi, 0, 0, 0);
        g_cbApi = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 28, 240, 120, g_pageApi, (HMENU)101, 0, 0);
        SendMessageW(g_cbApi, CB_ADDSTRING, 0, (LPARAM)L"Vulkan 1.2");
        SendMessageW(g_cbApi, CB_ADDSTRING, 0, (LPARAM)L"OpenGL 4.5");
        SendMessageW(g_cbApi, CB_SETCURSEL, g_api, 0);

        g_pageRes = CreateWindowW(L"STATIC", L"", WS_CHILD, 24, 120, 420, 100, h, 0, 0, 0);
        CreateWindowW(L"STATIC", L"Output height (width 720)", WS_CHILD | WS_VISIBLE, 0, 0, 300, 18, g_pageRes, 0, 0, 0);
        g_cbRes = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 28, 240, 120, g_pageRes, (HMENU)102, 0, 0);
        SendMessageW(g_cbRes, CB_ADDSTRING, 0, (LPARAM)L"720p");
        SendMessageW(g_cbRes, CB_ADDSTRING, 0, (LPARAM)L"560p");
        SendMessageW(g_cbRes, CB_ADDSTRING, 0, (LPARAM)L"480p");
        SendMessageW(g_cbRes, CB_SETCURSEL, g_res, 0);

        g_pagePad = CreateWindowW(L"STATIC", L"", WS_CHILD, 24, 120, 420, 160, h, 0, 0, 0);
        CreateWindowW(L"STATIC", L"Controller map (keymap.json)", WS_CHILD | WS_VISIBLE, 0, 0, 300, 18, g_pagePad, 0, 0, 0);
        HWND list = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
            0, 28, 300, 100, g_pagePad, (HMENU)103, 0, 0);
        load_keymap();
        for (auto& kv : g_keys) {
            std::wstring line = wide(kv.first + "  →  " + kv.second);
            SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)line.c_str());
        }
        CreateWindowW(L"BUTTON", L"Open keymap.json", WS_CHILD | WS_VISIBLE, 310, 28, 140, 28, g_pagePad, (HMENU)202, 0, 0);

        CreateWindowW(L"BUTTON", L"PLAY", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 24, 300, 140, 40, h, (HMENU)201, 0, 0);
        g_log = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            24, 360, 440, 100, h, 0, 0, 0);
        SendMessageW(g_log, WM_SETFONT, (WPARAM)g_font, TRUE);
        log_line("Modify ready. Select API / quality / controller, then PLAY.");
        if (!g_key.empty()) log_line(("Bound key: " + g_key.substr(0, 12) + "…").c_str());
        show_tab(0);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = (HDC)w;
        SetTextColor(dc, RGB(200, 230, 240));
        SetBkColor(dc, RGB(18, 24, 40));
        return (LRESULT)g_brPanel;
    }
    case WM_ERASEBKGND: {
        RECT rc; GetClientRect(h, &rc);
        FillRect((HDC)w, &rc, g_brBg);
        return 1;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)l;
        if (di->CtlType != ODT_BUTTON) break;
        HDC dc = di->hDC;
        RECT rc = di->rcItem;
        bool hot = (di->itemState & ODS_SELECTED) != 0;
        HBRUSH br = (di->CtlID == 201) ? g_brAccent : g_brPanel;
        FillRect(dc, &rc, br);
        FrameRect(dc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, (di->CtlID == 201) ? RGB(5, 20, 30) : RGB(180, 230, 240));
        wchar_t text[64]; GetWindowTextW(di->hwndItem, text, 64);
        DrawTextW(dc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id >= 301 && id <= 303) show_tab(id - 301);
        if (id == 101 && HIWORD(w) == CBN_SELCHANGE) g_api = (int)SendMessageW(g_cbApi, CB_GETCURSEL, 0, 0);
        if (id == 102 && HIWORD(w) == CBN_SELCHANGE) g_res = (int)SendMessageW(g_cbRes, CB_GETCURSEL, 0, 0);
        if (id == 201) do_play();
        if (id == 202) {
            std::string km = join(g_dir, "keymap.json");
            if (GetFileAttributesA(km.c_str()) == INVALID_FILE_ATTRIBUTES) {
                std::ofstream f(km);
                if (f) f << "{\n  \"Cross\": \"Z\", \"Circle\": \"X\", \"Square\": \"A\", \"Triangle\": \"S\",\n"
                           "  \"Start\": \"V\", \"Select\": \"Space\", \"DpadUp\": \"Up\", \"DpadDown\": \"Down\"\n}\n";
            }
            ShellExecuteA(nullptr, "open", km.c_str(), nullptr, g_dir.c_str(), SW_SHOWNORMAL);
            log_line("opened keymap.json");
        }
        return 0;
    }
    case WM_DESTROY:
        DeleteObject(g_brBg); DeleteObject(g_brPanel); DeleteObject(g_brAccent);
        DeleteObject(g_font); DeleteObject(g_fontBold);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int show) {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    g_dir = path;
    size_t s = g_dir.find_last_of("\\/");
    g_dir = (s == std::string::npos) ? std::string() : g_dir.substr(0, s + 1);
    load_launch();

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = L"PPSX33_Modify";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);
    g_hwnd = CreateWindowW(L"PPSX33_Modify", L"PPSX33 Modify",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 520, nullptr, nullptr, hi, nullptr);
    ShowWindow(g_hwnd, show);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
