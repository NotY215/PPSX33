#ifdef _WIN32
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace {
constexpr wchar_t kClassName[] = L"PPSX33NativeControllerStudio";
constexpr COLORREF kBg = RGB(7, 13, 24);
constexpr COLORREF kPanel = RGB(16, 27, 43);
constexpr COLORREF kBorder = RGB(44, 67, 91);
constexpr COLORREF kText = RGB(226, 237, 248);
constexpr COLORREF kAccent = RGB(51, 190, 220);
constexpr UINT kSaveId = 9000;
struct Mapping { std::wstring name, property, key; RECT rect{}; };
std::vector<Mapping> g_map;
int g_capture = -1;
std::wstring g_status = L"Select a control, then press a key to remap it.";
HWND g_window = nullptr;

std::vector<Mapping> Defaults() {
    return {
        {L"Left stick · Left",L"LeftStickLeft",L"Left"},{L"Left stick · Down",L"LeftStickDown",L"Down"},
        {L"Left stick · Right",L"LeftStickRight",L"Right"},{L"Left stick · Up",L"LeftStickUp",L"Up"},
        {L"Right stick · Left",L"RightStickLeft",L"H"},{L"Right stick · Down",L"RightStickDown",L"J"},
        {L"Right stick · Right",L"RightStickRight",L"K"},{L"Right stick · Up",L"RightStickUp",L"U"},
        {L"Start",L"Start",L"V"},{L"Select",L"Select",L"Space"},{L"PS button",L"PsButton",L"Backspace"},
        {L"Square",L"Square",L"A"},{L"Cross",L"Cross",L"Z"},{L"Circle",L"Circle",L"X"},{L"Triangle",L"Triangle",L"S"},
        {L"D-pad · Left",L"DpadLeft",L"NumPad4"},{L"D-pad · Down",L"DpadDown",L"NumPad5"},
        {L"D-pad · Right",L"DpadRight",L"NumPad6"},{L"D-pad · Up",L"DpadUp",L"NumPad8"},
        {L"R1",L"R1",L"R"},{L"R2",L"R2",L"RControlKey"},{L"R3",L"R3",L"LControlKey"},
        {L"L1",L"L1",L"L"},{L"L2",L"L2",L"RShiftKey"},{L"L3",L"L3",L"LShiftKey"}
    };
}
std::wstring KeyName(UINT vk) {
    switch (vk) {
    case VK_LEFT:return L"Left"; case VK_RIGHT:return L"Right";
    case VK_UP:return L"Up"; case VK_DOWN:return L"Down";
    case VK_SPACE:return L"Space"; case VK_BACK:return L"Backspace";
    case VK_RETURN:return L"Enter"; case VK_ESCAPE:return L"Escape";
    case VK_TAB:return L"Tab"; case VK_SHIFT:return L"Shift";
    case VK_LSHIFT:return L"LShiftKey"; case VK_RSHIFT:return L"RShiftKey";
    case VK_CONTROL:return L"Control"; case VK_LCONTROL:return L"LControlKey";
    case VK_RCONTROL:return L"RControlKey"; case VK_MENU:return L"Alt";
    case VK_DELETE:return L"Delete"; case VK_INSERT:return L"Insert";
    case VK_HOME:return L"Home"; case VK_END:return L"End";
    case VK_PRIOR:return L"PageUp"; case VK_NEXT:return L"PageDown";
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return L"NumPad" + std::to_wstring(vk - VK_NUMPAD0);
    if (vk >= 'A' && vk <= 'Z') return std::wstring(1, wchar_t(vk));
    if (vk >= '0' && vk <= '9') return std::wstring(1, wchar_t(vk));
    if (vk >= VK_F1 && vk <= VK_F24) return L"F" + std::to_wstring(vk - VK_F1 + 1);
    return L"VK_" + std::to_wstring(vk);
}
std::wstring JsonEscape(const std::wstring& s) {
    std::wstring o;
    for (wchar_t c : s) { if (c == L'\\' || c == L'"') o += L'\\'; if (c >= 32) o += c; }
    return o;
}
void SaveMap() {
    wchar_t temp[MAX_PATH]{};
    GetModuleFileNameW(nullptr,temp,MAX_PATH);
    std::filesystem::path root = std::filesystem::path(temp).parent_path();
    std::filesystem::path out = root / L"pad_map.json";
    std::wofstream f(out);
    if (!f) { g_status = L"Could not write pad_map.json beside the application."; return; }
    f << L"{\n";
    for (size_t i=0;i<g_map.size();++i) {
        f << L"  \"" << JsonEscape(g_map[i].property) << L"\": \"" << JsonEscape(g_map[i].key) << L"\"";
        f << (i+1<g_map.size()?L",":L"") << L"\n";
    }
    f << L"}\n";
    g_status = L"Saved mapping to " + out.wstring();
}
void Text(HDC dc, const std::wstring& s, RECT r, COLORREF color, UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE) {
    SetTextColor(dc,color); SetBkMode(dc,TRANSPARENT); DrawTextW(dc,s.c_str(),-1,&r,flags);
}
void RoundPanel(HDC dc, RECT r, COLORREF fill, COLORREF edge) {
    HBRUSH b=CreateSolidBrush(fill); HPEN p=CreatePen(PS_SOLID,1,edge);
    auto ob=SelectObject(dc,b); auto op=SelectObject(dc,p);
    RoundRect(dc,r.left,r.top,r.right,r.bottom,10,10);
    SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(b); DeleteObject(p);
}
void Layout(int width) {
    const int margin=22, gap=10, col=(width-margin*2-gap)/2, rowH=39, startY=132;
    for (size_t i=0;i<g_map.size();++i) {
        int colIndex = i < 13 ? 0 : 1;
        int row = colIndex==0 ? int(i) : int(i)-13;
        int x = margin + colIndex*(col+gap), y=startY+row*47;
        g_map[i].rect={x,y,x+col,y+rowH};
    }
}
void Paint(HWND hwnd) {
    PAINTSTRUCT ps{}; HDC dc=BeginPaint(hwnd,&ps); RECT client{}; GetClientRect(hwnd,&client);
    HBRUSH bg=CreateSolidBrush(kBg); FillRect(dc,&client,bg); DeleteObject(bg);
    SetBkMode(dc,TRANSPARENT);
    RECT title{22,18,client.right-20,48}; Text(dc,L"PPSX33  /  CONTROLLER STUDIO",title,kText);
    RECT sub{22,48,client.right-20,72}; Text(dc,L"Native C++ mapping · DualShock-style layout · static theme",sub,RGB(144,166,190));
    RECT controller{22,78,client.right-22,119}; RoundPanel(dc,controller,kPanel,kBorder);
    Text(dc,L"FACE  Square  /  Cross  /  Circle  /  Triangle     •     SYSTEM  Start / Select / PS",RECT{34,82,client.right-32,114},kAccent);
    Layout(client.right);
    for (size_t i=0;i<g_map.size();++i) {
        bool selected = int(i)==g_capture;
        RoundPanel(dc,g_map[i].rect,selected?RGB(21,57,75):kPanel,selected?kAccent:kBorder);
        RECT label=g_map[i].rect; label.left+=11; label.right-=92;
        Text(dc,g_map[i].name,label,kText);
        RECT key=g_map[i].rect; key.left=key.right-88; key.right-=8;
        Text(dc,selected?L"PRESS KEY":g_map[i].key,key,selected?kAccent:kText,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
    }
    RECT status{22,client.bottom-55,client.right-150,client.bottom-20};
    Text(dc,g_status,status,RGB(157,190,209));
    RECT save{client.right-128,client.bottom-56,client.right-22,client.bottom-20};
    RoundPanel(dc,save,RGB(17,65,83),kAccent); Text(dc,L"SAVE MAP",save,kText,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    EndPaint(hwnd,&ps);
}
LRESULT CALLBACK Proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_SIZE: InvalidateRect(hwnd,nullptr,TRUE); return 0;
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_LBUTTONDOWN: {
        int x=GET_X_LPARAM(lp), y=GET_Y_LPARAM(lp); RECT c{}; GetClientRect(hwnd,&c);
        RECT save{c.right-128,c.bottom-56,c.right-22,c.bottom-20};
        if (PtInRect(&save,POINT{x,y})) { SaveMap(); InvalidateRect(hwnd,nullptr,TRUE); return 0; }
        Layout(c.right); g_capture=-1;
        for(size_t i=0;i<g_map.size();++i) if(PtInRect(&g_map[i].rect,POINT{x,y})) {g_capture=int(i);g_status=L"Press a keyboard key to assign: "+g_map[i].name;break;}
        InvalidateRect(hwnd,nullptr,TRUE); return 0;
    }
    case WM_KEYDOWN:
        if(g_capture>=0) {
            if(wp==VK_ESCAPE) {g_capture=-1;g_status=L"Mapping cancelled.";}
            else {g_map[g_capture].key=KeyName(UINT(wp));g_status=L"Mapped "+g_map[g_capture].name+L" → "+g_map[g_capture].key;g_capture=-1;}
            InvalidateRect(hwnd,nullptr,TRUE); return 0;
        }
        break;
    case WM_GETDLGCODE: return DLGC_WANTALLKEYS;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE, PWSTR,int show) {
    g_map=Defaults();
    WNDCLASSEXW wc{sizeof(wc)}; wc.lpfnWndProc=Proc; wc.hInstance=instance;
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=CreateSolidBrush(kBg); wc.lpszClassName=kClassName;
    if(!RegisterClassExW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return 1;
    g_window=CreateWindowExW(0,kClassName,L"PPSX33 Controller Studio",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT,CW_USEDEFAULT,900,1000,nullptr,nullptr,instance,nullptr);
    if(!g_window) return 2;
    ShowWindow(g_window,show); UpdateWindow(g_window); SetFocus(g_window);
    MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return int(msg.wParam);
}
#endif
