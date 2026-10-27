#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#define API extern "C" __declspec(dllexport)
using MappingChanged = void (__stdcall*)(const wchar_t*);
namespace {
constexpr wchar_t ClassName[] = L"PPSX33ControllerChild";
constexpr COLORREF Bg=RGB(7,13,24), Panel=RGB(16,27,43), Edge=RGB(44,67,91), Text=RGB(226,237,248), Muted=RGB(144,166,190), Accent=RGB(51,190,220);
struct Item { const wchar_t* label; const wchar_t* property; std::wstring key; RECT rect{}; };
std::vector<Item> items;
HWND child=nullptr;
MappingChanged changed=nullptr;
int capture=-1;
std::wstring status=L"Click a binding, then press a key. Escape cancels.";
int width=800,height=600;
const wchar_t* labels[] = {
 L"Left stick / Left",L"Left stick / Down",L"Left stick / Right",L"Left stick / Up",
 L"Right stick / Left",L"Right stick / Down",L"Right stick / Right",L"Right stick / Up",
 L"Start",L"Select",L"PS button",L"Square",L"Cross",L"Circle",L"Triangle",
 L"D-pad / Left",L"D-pad / Down",L"D-pad / Right",L"D-pad / Up",L"R1",L"R2",L"R3",L"L1",L"L2",L"L3"
};
const wchar_t* props[] = {
 L"LeftStickLeft",L"LeftStickDown",L"LeftStickRight",L"LeftStickUp",
 L"RightStickLeft",L"RightStickDown",L"RightStickRight",L"RightStickUp",
 L"Start",L"Select",L"PsButton",L"Square",L"Cross",L"Circle",L"Triangle",
 L"DpadLeft",L"DpadDown",L"DpadRight",L"DpadUp",L"R1",L"R2",L"R3",L"L1",L"L2",L"L3"
};
const wchar_t* defaults[] = {
 L"Left",L"Down",L"Right",L"Up",L"H",L"J",L"K",L"U",L"V",L"Space",L"Backspace",
 L"A",L"Z",L"X",L"S",L"NumPad4",L"NumPad5",L"NumPad6",L"NumPad8",
 L"R",L"RControlKey",L"LControlKey",L"L",L"RShiftKey",L"LShiftKey"
};
std::wstring EscapeJson(const std::wstring& s) {
 std::wstring out; for(wchar_t c:s){if(c==L'\\'||c==L'"')out+=L'\\';if(c>=32)out+=c;} return out;
}
std::wstring ToJson() {
 std::wstring s=L"{";
 for(size_t i=0;i<items.size();++i){if(i)s+=L",";s+=L"\"";s+=items[i].property;s+=L"\":\"";s+=EscapeJson(items[i].key);s+=L"\"";}
 return s+L"}";
}
void Notify(){if(changed){auto j=ToJson();changed(j.c_str());}}
std::wstring KeyName(UINT vk) {
 switch(vk){
 case VK_LEFT:return L"Left";case VK_RIGHT:return L"Right";case VK_UP:return L"Up";case VK_DOWN:return L"Down";
 case VK_SPACE:return L"Space";case VK_BACK:return L"Backspace";case VK_RETURN:return L"Enter";case VK_ESCAPE:return L"Escape";
 case VK_TAB:return L"Tab";case VK_SHIFT:return L"ShiftKey";case VK_LSHIFT:return L"LShiftKey";case VK_RSHIFT:return L"RShiftKey";
 case VK_CONTROL:return L"ControlKey";case VK_LCONTROL:return L"LControlKey";case VK_RCONTROL:return L"RControlKey";
 case VK_MENU:return L"Menu";case VK_DELETE:return L"Delete";case VK_INSERT:return L"Insert";case VK_HOME:return L"Home";case VK_END:return L"End";
 case VK_PRIOR:return L"PageUp";case VK_NEXT:return L"PageDown";
 }
 if(vk>=VK_NUMPAD0&&vk<=VK_NUMPAD9)return L"NumPad"+std::to_wstring(vk-VK_NUMPAD0);
 if(vk>='A'&&vk<='Z')return std::wstring(1,wchar_t(vk));
 if(vk>='0'&&vk<='9')return std::wstring(1,wchar_t(vk));
 if(vk>=VK_F1&&vk<=VK_F24)return L"F"+std::to_wstring(vk-VK_F1+1);
 return L"VK_"+std::to_wstring(vk);
}
void Panel(HDC dc,RECT r,COLORREF fill,COLORREF edge){
 HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,1,edge);auto ob=SelectObject(dc,b);auto op=SelectObject(dc,p);
 RoundRect(dc,r.left,r.top,r.right,r.bottom,8,8);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
}
void DrawTextAt(HDC dc,const std::wstring& s,RECT r,COLORREF color,UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE){
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,s.c_str(),-1,&r,flags);
}
void Layout(){
 const int margin=18,gap=10,usable=std::max(300,width-margin*2-gap),col=usable/2,rowH=37;
 for(size_t i=0;i<items.size();++i){int c=i<13?0:1,row=c==0?int(i):int(i)-13;int x=margin+c*(col+gap),y=116+row*43;items[i].rect={x,y,x+col,y+rowH};}
}
void Paint(HWND hwnd){
 PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT c{};GetClientRect(hwnd,&c);HBRUSH b=CreateSolidBrush(Bg);FillRect(dc,&c,b);DeleteObject(b);
 DrawTextAt(dc,L"PPSX33  /  CONTROLLER STUDIO",RECT{18,14,c.right-18,42},Text);
 DrawTextAt(dc,L"NATIVE C++ MAPPING  ·  STATIC THEME  ·  25 CONTROLS",RECT{18,43,c.right-18,65},Muted);
 Panel(dc,RECT{18,74,c.right-18,105},Panel,Edge);
 DrawTextAt(dc,L"DUALSHOCK-STYLE LAYOUT      FACE BUTTONS  /  D-PAD  /  STICKS  /  SYSTEM",RECT{28,76,c.right-28,103},Accent);
 Layout();
 for(size_t i=0;i<items.size();++i){
  bool sel=int(i)==capture;Panel(dc,items[i].rect,sel?RGB(21,57,75):Panel,sel?Accent:Edge);
  RECT l=items[i].rect;l.left+=10;l.right-=104;DrawTextAt(dc,items[i].label,l,Text);
  RECT k=items[i].rect;k.left=k.right-96;k.right-=8;DrawTextAt(dc,sel?L"PRESS KEY":items[i].key,k,sel?Accent:Text,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 }
 RECT foot{18,c.bottom-45,c.right-145,c.bottom-12};DrawTextAt(dc,status,foot,Muted);
 RECT save{c.right-128,c.bottom-47,c.right-18,c.bottom-12};Panel(dc,save,RGB(17,65,83),Accent);DrawTextAt(dc,L"SAVE MAP",save,Text,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 EndPaint(hwnd,&ps);
}
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_SIZE:width=LOWORD(lp);height=HIWORD(lp);InvalidateRect(hwnd,nullptr,TRUE);return 0;
 case WM_PAINT:Paint(hwnd);return 0;
 case WM_LBUTTONDOWN:{
  int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);RECT c{};GetClientRect(hwnd,&c);
  RECT save{c.right-128,c.bottom-47,c.right-18,c.bottom-12};
  if(PtInRect(&save,POINT{x,y})){Notify();status=L"Map sent to PPSX33 settings and project output.";InvalidateRect(hwnd,nullptr,TRUE);return 0;}
  Layout();capture=-1;
  for(size_t i=0;i<items.size();++i)if(PtInRect(&items[i].rect,POINT{x,y})){capture=int(i);status=L"Press a keyboard key for the selected control. Escape cancels.";break;}
  SetFocus(hwnd);InvalidateRect(hwnd,nullptr,TRUE);return 0;
 }
 case WM_KEYDOWN:
  if(capture>=0){if(wp==VK_ESCAPE){capture=-1;status=L"Rebinding cancelled.";}else{items[capture].key=KeyName(UINT(wp));status=L"Binding updated. Click SAVE MAP to apply it.";capture=-1;}InvalidateRect(hwnd,nullptr,TRUE);return 0;}
  return 0;
 case WM_GETDLGCODE:return DLGC_WANTALLKEYS;
 case WM_ERASEBKGND:return 1;
 }
 return DefWindowProcW(hwnd,msg,wp,lp);
}
void EnsureItems(){if(!items.empty())return;for(int i=0;i<25;++i)items.push_back({labels[i],props[i],defaults[i],{}});}
std::wstring FindJsonString(const wchar_t* json,const wchar_t* property){
 if(!json)return {};std::wstring token=L"\"";token+=property;token+=L"\"";std::wstring s(json);auto p=s.find(token);if(p==std::wstring::npos)return {};
 p=s.find(L':',p+token.size());if(p==std::wstring::npos)return {};p=s.find(L'"',p+1);if(p==std::wstring::npos)return {};
 std::wstring out;for(++p;p<s.size();++p){if(s[p]==L'"')break;if(s[p]==L'\\'&&p+1<s.size())++p;out+=s[p];}return out;
}
}
API HWND __stdcall PPSX33_ControllerCreate(HWND parent,MappingChanged callback,const wchar_t* initialJson){
 EnsureItems();changed=callback;
 if(initialJson){for(auto& item:items){auto value=FindJsonString(initialJson,item.property);if(!value.empty())item.key=value;}}
 WNDCLASSEXW wc{sizeof(wc)};wc.lpfnWndProc=WndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=CreateSolidBrush(Bg);wc.lpszClassName=ClassName;
 if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return nullptr;
 RECT r{};GetClientRect(parent,&r);width=r.right;height=r.bottom;
 child=CreateWindowExW(0,ClassName,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,0,0,width,height,parent,nullptr,GetModuleHandleW(nullptr),nullptr);
 if(child){SetFocus(child);InvalidateRect(child,nullptr,TRUE);}return child;
}
API void __stdcall PPSX33_ControllerResize(HWND hwnd,int w,int h){if(hwnd){width=w;height=h;SetWindowPos(hwnd,nullptr,0,0,w,h,SWP_NOZORDER|SWP_NOACTIVATE);InvalidateRect(hwnd,nullptr,TRUE);}}
API void __stdcall PPSX33_ControllerSetMap(const wchar_t* json){EnsureItems();for(auto& item:items){auto value=FindJsonString(json,item.property);if(!value.empty())item.key=value;}if(child)InvalidateRect(child,nullptr,TRUE);}
API void __stdcall PPSX33_ControllerDestroy(HWND hwnd){if(hwnd&&IsWindow(hwnd))DestroyWindow(hwnd);if(child==hwnd)child=nullptr;}
