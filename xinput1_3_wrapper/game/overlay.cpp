#include "stdafx.h"
#include <base.h>
#include <font.hpp>

bool EndSceneInit = false;
ImFont* g_font = NULL;
ImVec2 g_window_size;
ImVec2 g_window_pos;

static int ResizeImGui() {
	RECT rect;
	int font_size = 12;

	if (GetClientRect(Base::Data::hWindow, &rect)) {
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		switch (width) {
		case 1176:
			font_size = 6;
			break;
		case 1280:
		case 1360:
		case 1366:
			font_size = 7;
			break;
		case 1600:
			font_size = 9;
			break;
		case 1768:
			font_size = 10;
			break;
		case 1920:
			font_size = 12;
			break;
		case 2560:
			font_size = 18;
			break;
		case 3840:
			font_size = 30;
			break;
		}

		g_window_size = ImVec2(width * 0.1, height * 0.1);
		g_window_pos = ImVec2(width - g_window_size.x - 10, height - g_window_size.y - 10);
	}

	return font_size;
}

HRESULT __stdcall Base::Hooks::EndScene(LPDIRECT3DDEVICE9 pDevice)
{
	Data::pDxDevice9 = pDevice;
	if (!Data::InitImGui)
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
		io.IniFilename = NULL;
		ImGui_ImplWin32_Init(Data::hWindow);
		ImGui_ImplDX9_Init(pDevice);
		Data::InitImGui = true;

		int font_size = ResizeImGui();
	

		g_font = io.Fonts->AddFontFromMemoryCompressedTTF((const void*)DejaVuSansMono_compressed_data,
			DejaVuSansMono_compressed_size, font_size);
	}
	
	if (!Data::InitImGui) {
		return Data::oEndScene(pDevice);
	}

	ResizeImGui();

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (Data::ShowOverlay &&
		!Base::Hooks::is_in_main_menu() &&
		!Base::Hooks::loading_screen_visible())
	{
		ImGui::SetNextWindowPos(g_window_pos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(g_window_size, ImGuiCond_FirstUseEver);

		ImGui::PushFont(g_font);
		ImGui::SetNextWindowBgAlpha(0.30f);

		ImGui::Begin("Infra counters", 0, 
			ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoMove | 
			ImGuiWindowFlags_NoCollapse | 
			ImGuiWindowFlags_NoSavedSettings | 
			ImGuiWindowFlags_NoTitleBar | 
			ImGuiWindowFlags_NoScrollbar | 
			ImGuiWindowFlags_NoScrollWithMouse);

		ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.40f, 1.00f), Data::lines[0].c_str()); // map name

		
		ImGui::TextColored(Data::lines_colors[1], Data::lines[1].c_str());
		ImGui::SameLine();
		ImGui::TextColored(Data::lines2_colors[1], Data::lines2[1].c_str());
		
		ImGui::TextColored(Data::lines_colors[2], Data::lines[2].c_str());
		ImGui::SameLine();
		ImGui::TextColored(Data::lines2_colors[2], Data::lines2[2].c_str());

		ImGui::TextColored(Data::lines_colors[3], Data::lines[3].c_str());
		ImGui::SameLine();
		ImGui::TextColored(Data::lines2_colors[3], Data::lines2[3].c_str());

		ImGui::TextColored(Data::lines_colors[4], Data::lines[4].c_str());
		ImGui::SameLine();
		ImGui::TextColored(Data::lines2_colors[4], Data::lines2[4].c_str());

		ImGui::TextColored(Data::lines_colors[5], Data::lines[5].c_str());
		ImGui::SameLine();
		ImGui::TextColored(Data::lines2_colors[5], Data::lines2[5].c_str());

		ImGui::End();

		ImGui::PopFont();
	}

	//ImGui::ShowDemoWindow();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	if(Data::ToDetach)
		Base::Detach();

	return Data::oEndScene(pDevice);
}