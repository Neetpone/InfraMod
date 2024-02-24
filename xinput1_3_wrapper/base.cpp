#include "stdafx.h"
#include <base.h>

ImVec4 g_font_color = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
ImVec4 g_font_color_max = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

//Data
HMODULE           Base::Data::hModule    = (HMODULE)NULL;
void*             Base::Data::pDeviceTable[D3DDEV9_LEN];
LPDIRECT3DDEVICE9 Base::Data::pDxDevice9 = (LPDIRECT3DDEVICE9)NULL;
HWND              Base::Data::hWindow    = (HWND)NULL;
void *    Base::Data::pEndScene  =NULL;
EndScene_t        Base::Data::oEndScene  = (EndScene_t)NULL;
WndProc_t         Base::Data::oWndProc   = (WndProc_t)NULL;
UINT              Base::Data::WmKeys[0xFF];
bool              Base::Data::Detached   = false;
bool              Base::Data::ToDetach   = false;
bool              Base::Data::ShowOverlay = true;
bool              Base::Data::InitImGui  = false;
bool Base::Data::Inited = false;
int Base::Data::takeImage = 0;

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
	return true;
}
