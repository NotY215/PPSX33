/* PPSX33 Modify — per-game launcher (keymapping, resolution, API). No OpenGL warning popups. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

static std::string g_dir;
static std::string g_game_exe = "game.exe";
static std::string g_key;
static int g_api = 0; /* 0 Vulkan 1.2, 1 OpenGL 4.5 */
static int g_res = 0; /* 0=720p 1=560p 2=480p */
static HWND g_hwndApi, g_hwndRes, g_hwndLog;

static std::string join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    char c = a.back();
    if (c == '\\' || c == '/') return a + b;
    return a + "\\" + b;
}

static void load_launch_json() {
    std::string path = join(g_dir, "ppsx33_launch.json");
    std::ifstream f(path);
    if (!f) return;
    std::stringstream ss; ss << f.rdbuf();
    std::string j = ss.str();
    auto pick = [&](const char* key) -> std::string {
        std::string k = std::string("\"") + key + "\"";
        size_t i = j.find(k);
        if (i == std::string::npos) return {};
        i = j.find(':', i);
        if (i == std::string::npos) return {};
        i = j.find('"', i);
        if (i == std::string::npos) return {};
        size_t e = j.find('"', i + 1);
        if (e == std::string::npos) return {};
        return j.substr(i + 1, e - i - 1);
    };
    auto g = pick("game");
    if (!g.empty()) g_game_exe = g;
    auto k = pick("key");
    if (!k.empty()) g_key = k;
    auto a = pick("api");
    if (a == "OpenGL" || a == "opengl") g_api = 1;
}

static void save_prefs() {
    std::string path = join(g_dir, "ppsx33_modify.json");
    std::ofstream f(path);
    if (!f) return;
    const char* api = g_api ? "OpenGL" : "Vulkan";
    int h = (g_res == 2) ? 480 : (g_res == 1) ? 560 : 720;
    f << "{\n  \"api\": \"" << api << "\",\n  \"height\": " << h << ",\n  \"width\": 720\n}\n";
}

static void log_line(const char* s) {
    if (!g_hwndLog) return;
    int n = GetWindowTextLengthA(g_hwndLog);
    SendMessageA(g_hwndLog, EM_SETSEL, n, n);
    SendMessageA(g_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)s);
    SendMessageA(g_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
}

static void do_play() {
    save_prefs();
    std::string exe = join(g_dir, g_game_exe);
    std::string cmd = "\"" + exe + "\" -nogui";
    if (!g_key.empty()) {
        cmd += " -key=";
        cmd += g_key;
    }
    cmd += g_api ? " -api=opengl" : " -api=vulkan";
    if (g_res == 1) cmd += " -h=560";
    else if (g_res == 2) cmd += " -h=480";
    else cmd += " -h=720";

    log_line(cmd.c_str());
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string mutable_cmd = cmd;
    if (!CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, g_dir.c_str(), &si, &pi)) {
        log_line("CreateProcess failed");
        return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    log_line("launched");
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "PPSX33 Modify", WS_CHILD | WS_VISIBLE, 16, 12, 300, 20, h, 0, 0, 0);
        CreateWindowA("STATIC", "API (both built)", WS_CHILD | WS_VISIBLE, 16, 44, 160, 18, h, 0, 0, 0);
        g_hwndApi = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 16, 64, 200, 120, h, (HMENU)101, 0, 0);
        SendMessageA(g_hwndApi, CB_ADDSTRING, 0, (LPARAM)"Vulkan 1.2");
        SendMessageA(g_hwndApi, CB_ADDSTRING, 0, (LPARAM)"OpenGL 4.5");
        SendMessageA(g_hwndApi, CB_SETCURSEL, g_api, 0);
        CreateWindowA("STATIC", "Quality", WS_CHILD | WS_VISIBLE, 16, 100, 120, 18, h, 0, 0, 0);
        g_hwndRes = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 16, 120, 200, 120, h, (HMENU)102, 0, 0);
        SendMessageA(g_hwndRes, CB_ADDSTRING, 0, (LPARAM)"720p");
        SendMessageA(g_hwndRes, CB_ADDSTRING, 0, (LPARAM)"560p");
        SendMessageA(g_hwndRes, CB_ADDSTRING, 0, (LPARAM)"480p");
        SendMessageA(g_hwndRes, CB_SETCURSEL, g_res, 0);
        CreateWindowA("BUTTON", "Play", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 16, 160, 100, 28, h, (HMENU)201, 0, 0);
        CreateWindowA("BUTTON", "Open keymap.json", WS_CHILD | WS_VISIBLE, 130, 160, 140, 28, h, (HMENU)202, 0, 0);
        CreateWindowA("STATIC", "Keymapping file: keymap.json (edit or Open)", WS_CHILD | WS_VISIBLE, 16, 200, 360, 18, h, 0, 0, 0);
        g_hwndLog = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
            16, 230, 400, 120, h, 0, 0, 0);
        log_line("Modify ready — select API/quality, then Play.");
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == 101 && HIWORD(w) == CBN_SELCHANGE)
            g_api = (int)SendMessageA(g_hwndApi, CB_GETCURSEL, 0, 0);
        if (LOWORD(w) == 102 && HIWORD(w) == CBN_SELCHANGE)
            g_res = (int)SendMessageA(g_hwndRes, CB_GETCURSEL, 0, 0);
        if (LOWORD(w) == 201)
            do_play();
        if (LOWORD(w) == 202) {
            std::string km = join(g_dir, "keymap.json");
            if (GetFileAttributesA(km.c_str()) == INVALID_FILE_ATTRIBUTES) {
                std::ofstream f(km);
                if (f) f << "{\n  \"Cross\": \"Z\", \"Circle\": \"X\", \"Square\": \"A\", \"Triangle\": \"S\",\n"                          "  \"Start\": \"V\", \"Select\": \"Space\", \"DpadUp\": \"Up\", \"DpadDown\": \"Down\"\n}\n";
            }
            ShellExecuteA(nullptr, "open", km.c_str(), nullptr, g_dir.c_str(), SW_SHOWNORMAL);
            log_line("opened keymap.json");
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int show) {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    g_dir = path;
    size_t s = g_dir.find_last_of("\\/");
    g_dir = (s == std::string::npos) ? std::string() : g_dir.substr(0, s + 1);
    load_launch_json();

    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = "PPSX33_Modify";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("PPSX33_Modify", "PPSX33 Modify",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 460, 420, nullptr, nullptr, hi, nullptr);
    ShowWindow(hwnd, show);
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}
