#include "stdafx.h"
#include <base.h>
#include "overlay.h"
#include <font.hpp>
#include "Utils.h"
#include <fstream>
#include "inventory.h"

ImFont* g_font = NULL;
ImVec2 g_CounterWindowSize;
ImVec2 g_CounterWindowPos;

ImVec2 g_InventoryWindowSize;
ImVec2 g_InventoryWindowPos;

const ImVec4 g_TitleColor = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

static int DetermineFontSize(const RECT &rect) {
	int font_size = 12;
	int width = rect.right - rect.left;

	// Auto
	if (overlay::fontSize == 0) {
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
		case 3440:
			font_size = 18;
			break;
		case 3840:
			font_size = 30;
			break;
		}
	}
	else {
		font_size = overlay::fontSize;
	}

	return font_size;
}

static void ResizeImGui() {
	const int width = Base::Data::HACK_clientRect.right - Base::Data::HACK_clientRect.left;
	const int height = Base::Data::HACK_clientRect.bottom - Base::Data::HACK_clientRect.top;
	const ImVec2 charDimensions = ImGui::CalcTextSize("W");
	const ImVec2 mapNameDimensions = ImGui::CalcTextSize(overlay::title.value.c_str());
	const int counterWidth = std::max(static_cast<int>(mapNameDimensions.x), 28);
	const int flashlightDimensions = ImGui::CalcTextSize("Flashlight: 999").x;

	g_CounterWindowSize = ImVec2(counterWidth + 16, charDimensions.y * 7.2);
	g_CounterWindowPos = ImVec2(width - g_CounterWindowSize.x - 10, height - g_CounterWindowSize.y - 10);

	g_InventoryWindowSize = ImVec2(flashlightDimensions + 16, charDimensions.y * 3.8);
	g_InventoryWindowPos = ImVec2(10, 10);
}

static ImVec2 CalcMaxTextSize(const std::vector<std::string>& values) {
	float maxWidth = 0;
	float maxHeight = 0;

	for (const auto &value : values) {
		const ImVec2 size = ImGui::CalcTextSize(value.c_str());

		if (size.x > maxWidth) {
			maxWidth = size.x;
		}

		if (size.y > maxHeight) {
			maxHeight = size.y;
		}
	}

	return { maxWidth, maxHeight };
}

overlay::OverlayLine_t::OverlayLine_t(std::string name, std::string value, const ImVec4 &nameColor, const ImVec4 &valueColor) :
	name(std::move(name)), value(std::move(value)), nameColor(nameColor), valueColor(valueColor), blinksLeft(0), lastBlink(-1) {
}

bool overlay::shown = true;
overlay::OverlayLine_t overlay::title = overlay::OverlayLine_t();
std::vector<overlay::OverlayLine_t> overlay::lines = std::vector<overlay::OverlayLine_t>();
bool overlay::imGuiInitialized = false;
int overlay::fontSize = 0;

static void RenderCounters() {
	long long now;

	now = CurrentTimeMillis();

	// Measure the text size
	std::vector<std::string> lines;

	lines.resize(overlay::lines.size());
	std::transform(
		overlay::lines.begin(),
		overlay::lines.end(),
		lines.begin(),
		[](const overlay::OverlayLine_t &line) {
			return line.name + line.value;
		}
	);

	const ImVec2 maxSize = CalcMaxTextSize(lines);

	// Need to use the client rect as above but I'm lazy right now.
	// ImGui::SetNextWindowPos(ImVec2(10, 20));
	// ImGui::SetNextWindowSize(ImVec2(maxSize.x + 16, (lines.size() * maxSize.y) + 20));

	ImGui::SetNextWindowPos(g_CounterWindowPos, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(g_CounterWindowSize, ImGuiCond_FirstUseEver);

	ImGui::SetNextWindowBgAlpha(0.30f);

	ImGui::Begin("Infra counters", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse);

	ImGui::TextColored(g_TitleColor, overlay::title.value.c_str()); // map name

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
}

static void RenderInventory() {
	// Build the list of stuff to render
	std::vector<std::string> lines = std::vector<std::string>();

	if (mod::inventory::flashlightBatteriesCounter != nullptr) {
		lines.push_back(std::string("Flashlight: ") + std::to_string(*mod::inventory::flashlightBatteriesCounter));
	}

	if (mod::inventory::cameraBatteriesCounter != nullptr) {
		lines.push_back(std::string("Camera:     ") + std::to_string(*mod::inventory::cameraBatteriesCounter));
	}

	if (mod::inventory::osCoinsCounter != nullptr) {
		lines.push_back(std::string("OS Coins:   ") + std::to_string(static_cast<int>(*mod::inventory::osCoinsCounter)));
	}

	if (lines.empty()) {
		return;
	}

	const ImVec2 maxSize = CalcMaxTextSize(lines);

	ImGui::SetNextWindowPos(ImVec2(10, 20));
	ImGui::SetNextWindowSize(ImVec2(maxSize.x + 16, (lines.size() * maxSize.y) + 20));

	ImGui::SetNextWindowBgAlpha(0.30f);
	ImGui::Begin("Infra Inventory", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse);

	for (const auto& value : lines) {
		ImGui::TextColored(g_TitleColor, value.c_str());
	}

	ImGui::End();
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void overlay::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_KEYDOWN && wParam == Base::Data::Keys::ToggleMenu) {
		overlay::shown = !overlay::shown;
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
}
void overlay::Render(const HWND hWnd, const LPDIRECT3DDEVICE9 pDevice) {
	if (!overlay::imGuiInitialized) {
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
		io.IniFilename = nullptr;
		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX9_Init(pDevice);
		overlay::imGuiInitialized = true;
	}

	if (g_font == nullptr) {
		RECT rect;
		if (GetClientRect(Base::Data::hWindow, &rect)) {
			Base::Data::HACK_clientRect = rect;

			int fontSize = DetermineFontSize(rect);

			g_font =  ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF((const void*)DejaVuSansMono_compressed_data,
				DejaVuSansMono_compressed_size, fontSize);
		}
	}


	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::PushFont(g_font);

	ResizeImGui();

	RenderCounters();
	RenderInventory();

	ImGui::PopFont();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}