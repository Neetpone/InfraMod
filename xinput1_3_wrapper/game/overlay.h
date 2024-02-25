#pragma once
#include "stdafx.h"
#include <string>

namespace overlay {
	struct OverlayLine_t {
		std::string name;
		std::string value;
		ImVec4 nameColor;
		ImVec4 valueColor;

		// blink data
		int blinksLeft;
		long long lastBlink;

		OverlayLine_t() = default;
		OverlayLine_t(std::string name, std::string value, ImVec4 nameColor, ImVec4 valueColor);
	};

	const ImVec4 fontColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	const ImVec4 fontColorMax = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

	extern bool shown;
	extern OverlayLine_t title;
	extern std::vector<OverlayLine_t> lines;
	extern bool imGuiInitialized;

	void WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Render(HWND hWnd, LPDIRECT3DDEVICE9 pDevice);
}