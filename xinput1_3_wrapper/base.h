#pragma once
#ifndef BASE_H
#define BASE_H

#if defined(MEM_86)
#define WNDPROC_INDEX GWL_WNDPROC
#elif defined(MEM_64)
#define WNDPROC_INDEX GWLP_WNDPROC
#endif

#define WNDPROC_INDEX GWL_WNDPROC


#define D3DDEV9_LEN 119

#include "stdafx.h"

typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
typedef LRESULT(CALLBACK*  WndProc_t) (HWND, UINT, WPARAM, LPARAM);
typedef HRESULT(__stdcall* SetTexture_t)(LPDIRECT3DDEVICE9, DWORD, IDirect3DBaseTexture9*);

extern ImVec4 g_font_color;
extern ImVec4 g_font_color_max;

namespace Base
{
	bool Init();
	bool Shutdown();
	bool Detach();

	namespace Data
	{
		extern HMODULE           hModule;
		extern LPDIRECT3DDEVICE9 pDxDevice9;
		extern void*             pDeviceTable[D3DDEV9_LEN];
		extern HWND              hWindow;
		extern void*    pEndScene;
		extern EndScene_t        oEndScene;
		extern WndProc_t         oWndProc;
		extern UINT              WmKeys[0xFF];
		extern bool              Detached;
		extern bool              ToDetach;
		extern bool              InitImGui;
		extern bool              ShowOverlay;
		extern bool Inited;


		namespace Keys
		{
			const UINT ToggleMenu = VK_INSERT;
		}

		extern std::vector<std::string> lines;
		extern std::vector<ImVec4> lines_colors;
		extern std::vector<bool> line_blink;
		extern std::vector<std::string> lines2;
		extern std::vector<ImVec4> lines2_colors;
		extern int takeImage;
	}

	namespace Hooks
	{
		bool Init();
		bool Shutdown();
		HRESULT __stdcall EndScene(LPDIRECT3DDEVICE9 pDevice);
		LRESULT CALLBACK  WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void hook_game_functions();
		bool is_in_main_menu();
		bool loading_screen_visible();
	}
}

#endif