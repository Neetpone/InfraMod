#include "stdafx.h"
#include <base.h>
#include "overlay.h"
#include <font.hpp>
#include "Utils.h"
#include <fstream>

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

overlay::OverlayLine_t::OverlayLine_t(std::string name, std::string value, ImVec4 nameColor, ImVec4 valueColor) :
	name(name), value(value), nameColor(nameColor), valueColor(valueColor), blinksLeft(0), lastBlink(-1) {
}

bool overlay::shown = true;
overlay::OverlayLine_t overlay::title = overlay::OverlayLine_t();
std::vector<overlay::OverlayLine_t> overlay::lines = std::vector<overlay::OverlayLine_t>();
bool overlay::imGuiInitialized = false;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void overlay::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (uMsg == WM_KEYDOWN ){
		switch (wParam) {
		case Base::Data::Keys::ToggleMenu:
			overlay::shown = !overlay::shown;
			break;
		}
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
}

void overlay::Render(HWND hWnd, LPDIRECT3DDEVICE9 pDevice) {
	long long now;

	if (!overlay::imGuiInitialized) {
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
		io.IniFilename = NULL;
		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX9_Init(pDevice);
		overlay::imGuiInitialized = true;

		int font_size = ResizeImGui();
	

		g_font = io.Fonts->AddFontFromMemoryCompressedTTF((const void*)DejaVuSansMono_compressed_data,
			DejaVuSansMono_compressed_size, font_size);
	}

	ResizeImGui();
	now = CurrentTimeMillis();

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

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

		ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.40f, 1.00f), overlay::title.value.c_str()); // map name

		for (int i = 0; i < overlay::lines.size(); i++) {
			overlay::OverlayLine_t& line = overlay::lines[i];
			ImVec4 color;

			// Handle blinking the line if necessary
			if (line.blinksLeft > 0) {
				color = ((line.blinksLeft % 2) == 0) ? overlay::fontColorMax : overlay::fontColor;

				if ((now - line.lastBlink) >= 500) {
					line.blinksLeft--;
					line.lastBlink = now;
				}
			}
			else {
				color = line.nameColor;
			}

			ImGui::TextColored(color, line.name.c_str());
			ImGui::SameLine();
			ImGui::TextColored(line.valueColor, line.value.c_str());
		}

		ImGui::End();
		ImGui::PopFont();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}