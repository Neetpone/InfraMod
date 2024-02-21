#include "stdafx.h"
#include <base.h>

ImVec4 g_font_color = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
ImVec4 g_font_color_max = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

//Data
HMODULE           Base::Data::hModule    = (HMODULE)NULL;
void*             Base::Data::pDeviceTable[D3DDEV9_LEN];
LPDIRECT3DDEVICE9 Base::Data::pDxDevice9 = (LPDIRECT3DDEVICE9)NULL;
HWND              Base::Data::hWindow    = (HWND)NULL;
mem::voidptr_t    Base::Data::pEndScene  = (mem::voidptr_t)NULL;
EndScene_t        Base::Data::oEndScene  = (EndScene_t)NULL;
WndProc_t         Base::Data::oWndProc   = (WndProc_t)NULL;
#if defined(MEM_86)
mem::size_t       Base::Data::szEndScene = 7;
#elif defined(MEM_64)
mem::size_t       Base::Data::szEndScene = 15;
#endif
UINT              Base::Data::WmKeys[0xFF];
bool              Base::Data::Detached   = false;
bool              Base::Data::ToDetach   = false;
bool              Base::Data::ShowOverlay = true;
bool              Base::Data::InitImGui  = false;

std::vector<std::string> Base::Data::lines;
std::vector<ImVec4> Base::Data::lines_colors;
std::vector<bool> Base::Data::line_blink;
std::vector<std::string> Base::Data::lines2;
std::vector<ImVec4> Base::Data::lines2_colors;

bool Base::Init()
{
	Base::Data::lines.resize(10);
	Base::Data::lines_colors.resize(10);
	Base::Data::line_blink.resize(10);
	Base::Data::lines2.resize(10);
	Base::Data::lines2_colors.resize(10);
	Hooks::Init();
	return true;
}

bool Base::Shutdown()
{
	Hooks::Shutdown();
	return true;
}

bool Base::Detach()
{
	Base::Shutdown();
	CreateThread(nullptr, 0, ExitThread, Data::hModule, 0, nullptr);
	return true;
}
