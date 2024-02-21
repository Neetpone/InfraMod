#include "stdafx.h"
#include <base.h>
#include <metadata.h>

namespace infra
{
	mem::voidptr_t server_base;
	mem::voidptr_t engine_base;
};

typedef enum { GLOBAL_OFF = 0, GLOBAL_ON = 1, GLOBAL_DEAD = 2 } GLOBALESTATE;

bool g_server_spawned = false;

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam);
HWND GetProcessWindow();
bool GetD3D9Device(void** pTable, size_t Size);

typedef void(__thiscall* InitMapStats_t)(void*);
InitMapStats_t InitMapStats_orig;
void __fastcall InitMapStats(void* this_ptr);

typedef int(__thiscall* StatSuccess_t)(void* this_ptr, int event_type, int count, bool is_new);
StatSuccess_t StatSuccess_orig;
int __fastcall StatSuccess(void* this_ptr, int, int event_type, int count, bool is_new);

typedef int(__cdecl* GlobalEntity_AddEntity_t)(const char* pGlobalname, const char* pMapName, GLOBALESTATE state);
GlobalEntity_AddEntity_t GlobalEntity_AddEntity;

typedef void(__cdecl* GlobalEntity_SetCounter_t)(int globalIndex, int counter);
GlobalEntity_SetCounter_t GlobalEntity_SetCounter;

typedef int(__cdecl* GlobalEntity_GetCounter_t)(int globalIndex);
GlobalEntity_GetCounter_t GlobalEntity_GetCounter;

typedef int(__cdecl* GlobalEntity_AddToCounter_t)(int globalIndex, int count);
GlobalEntity_AddToCounter_t GlobalEntity_AddToCounter;

typedef GLOBALESTATE(__cdecl* GlobalEntity_GetState_t)(int globalIndex);
GlobalEntity_GetState_t GlobalEntity_GetState;

typedef void(__cdecl* GlobalEntity_SetState_t)(int globalIndex, GLOBALESTATE state);
GlobalEntity_SetState_t GlobalEntity_SetState;

nlohmann::json current_mapdata;

void Base::Hooks::hook_game_functions()
{
	while (true) {
		Sleep(500);

		DWORD base = (DWORD)GetModuleHandleA("server.dll");

		if (!base) {
			continue;
		}

		infra::server_base = (mem::voidptr_t)base;

		base = (DWORD)GetModuleHandleA("engine.dll");

		if (!base) {
			continue;
		}

		infra::engine_base = (mem::voidptr_t)base;
		break;
	}

#define FHOOK(fname, libname, offset) \
	auto fname##_ptr = (mem::voidptr_t)((mem::uint32_t)infra::libname##_base + offset); \
	fname##_orig = (fname##_t)mem::in::detour_trampoline(fname##_ptr, (mem::voidptr_t)fname, 9) 

	FHOOK(InitMapStats, server, 0x297100);
	FHOOK(StatSuccess, server, 0x2974E0);

	GlobalEntity_AddEntity = (GlobalEntity_AddEntity_t)((mem::uint32_t)infra::server_base + 0x158A10);
	GlobalEntity_SetCounter = (GlobalEntity_SetCounter_t)((mem::uint32_t)infra::server_base + 0x158420);
	GlobalEntity_GetCounter = (GlobalEntity_GetCounter_t)((mem::uint32_t)infra::server_base + 0x1585C0);
	GlobalEntity_AddToCounter = (GlobalEntity_AddToCounter_t)((mem::uint32_t)infra::server_base + 0x158450);
	GlobalEntity_GetState = (GlobalEntity_GetState_t)((mem::uint32_t)infra::server_base + 0x158590);
	GlobalEntity_SetState = (GlobalEntity_SetState_t)((mem::uint32_t)infra::server_base + 0x1583F0);

	try {
		std::ifstream f("mapdata.txt");
		auto data = nlohmann::json::parse(f);
		g_mapdata = data;
	}
	catch (std::exception e) {

	}
}

bool Base::Hooks::is_in_main_menu()
{
	return (infra::engine_base) ?
		*(bool*)((mem::uint32_t)infra::engine_base + 0x63FD86) : true;
}

bool Base::Hooks::loading_screen_visible()
{
	return (infra::engine_base) ?
		*(bool*)((mem::uint32_t)infra::engine_base + 0x5F9269) : true;
}

const char* get_map_name()
{
	if (!infra::server_base) {
		return NULL;
	}

	mem::uint32_t ptr = *(mem::uint32_t*)((mem::uint32_t)infra::server_base + 0x78BD00) + 0x3c;
	return *(char**)ptr;
}

const char* GetMapStatName(int event_type)
{
	const char* result = NULL;

	switch (event_type)
	{
	case 0:
		result = "successful_photos";
		break;
	case 1:
		result = "corruption_uncovered";
		break;
	case 2:
		result = "spots_repaired";
		break;
	case 3:
		result = "mistakes_made";
		break;
	case 4:
		result = "geocaches_found";
		break;
	case 5:
		result = "water_flow_meters_repaired";
		break;
	default:
		break;
	}
	return result;
}

std::string get_counter_name(const char* map_name, const char* stat_name)
{
	return std::string(map_name) + "_counter_" + std::string(stat_name);
}

std::string format_stat(int value, int max_value)
{
	return std::to_string(value) + " / " + std::to_string(max_value);
}

int get_max_value(int event_type, const char* map_name)
{
	const char* v = "";

	switch (event_type)
	{
	case 0:
		v = "camera_targets";
		break;
	case 1:
		v = "corruption_targets";
		break;
	case 2:
		v = "repair_targets";
		break;
	case 3:
		v = "mistake_targets";
		break;
	case 4:
		v = "geocaches";
		break;
	case 5:
		v = "water_flow_meter_targets";
		break;
	default:
		break;
	}

	std::string s = map_name;
	transform(s.begin(), s.end(), s.begin(), ::tolower);
	return current_mapdata[s][v];
}

void update_gui_table(int event_type, const char* map_name, int value)
{
	int max_value = 0;
	
	try {
		max_value = get_max_value(event_type, map_name);
	}
	catch (std::exception e) {

	}
	
	ImVec4 color = (value == max_value) ? g_font_color_max : g_font_color;

	switch (event_type)
	{
	case 0:
		Base::Data::lines[1] = "Defects:     ";
		Base::Data::lines2[1] = format_stat(value, max_value);
		Base::Data::lines2_colors[1] = color;
		break;
	case 1:
		Base::Data::lines[2] = "Corruption:  ";
		Base::Data::lines2[2] = format_stat(value, max_value);
		Base::Data::lines2_colors[2] = color;
		break;
	case 2:
		Base::Data::lines[3] = "Repairs:     ";
		Base::Data::lines2[3] = format_stat(value, max_value);
		Base::Data::lines2_colors[3] = color;
		break;
	case 3:
		// "mistakes_made";
		break;
	case 4:
		Base::Data::lines[4] = "Geocaches:   ";
		Base::Data::lines2[4] = format_stat(value, max_value);
		Base::Data::lines2_colors[4] = color;
		break;
	case 5:
		Base::Data::lines[5] = "Flow meters: ";
		Base::Data::lines2[5] = format_stat(value, max_value);
		Base::Data::lines2_colors[5] = color;
		break;
	default:
		break;
	}
}

void init_counter(const char* map_name, int event_type)
{
	auto name = get_counter_name(map_name, GetMapStatName(event_type));
	int index = GlobalEntity_AddEntity(name.c_str(), map_name, GLOBAL_OFF);
	int value = 0;

	if (GlobalEntity_GetState(index) == GLOBAL_OFF) {
		GlobalEntity_SetCounter(index, 0);
		GlobalEntity_SetState(index, GLOBAL_ON);
	}
	else {
		value = GlobalEntity_GetCounter(index);
	}

	update_gui_table(event_type, map_name, value);
}

void exclude_inactive_photo_spot(std::string map_name, std::string spot_name, nlohmann::json& mapdata)
{
	int index = GlobalEntity_AddEntity(spot_name.c_str(), map_name.c_str(), GLOBAL_OFF);

	if (GlobalEntity_GetState(index) == GLOBAL_ON) {
		int num = mapdata[map_name]["camera_targets"];

		if (num > 0) {
			--num;
		}

		mapdata[map_name]["camera_targets"] = num;
	}
}

auto exclude_inactive_photo_spots(std::string map_name, nlohmann::json mapdata)
{
	if (map_name == "infra_c2_m2_reserve2") {
		exclude_inactive_photo_spot(map_name, "infra_reserve1_dam_picture_taken", mapdata);
	}
	else if (map_name == "infra_c3_m2_tunnel2") {
		exclude_inactive_photo_spot(map_name, "infra_tunnel_elevator_picture_taken", mapdata);
		exclude_inactive_photo_spot(map_name, "infra_tunnel_cracks_picture_taken", mapdata);
	}
	else if (map_name == "infra_c5_m1_watertreatment") {
		exclude_inactive_photo_spot(map_name, "infra_watertreatment_steam_picture_taken", mapdata);
	}

	return mapdata;
}

/*
	This called once before a map is loaded 
*/
void __fastcall InitMapStats(void* this_ptr)
{
	InitMapStats_orig(this_ptr);

	if (Base::Hooks::is_in_main_menu()) {
		return;
	}

	for (int i = 1; i < 6; ++i) {
		Base::Data::lines_colors[i] = g_font_color;
		Base::Data::line_blink[i] = false;
	}

	auto map_name = get_map_name();
	auto& s = Base::Data::lines[0];
	s = map_name;
	transform(s.begin(), s.end(), s.begin(), ::tolower);

	current_mapdata = exclude_inactive_photo_spots(s, g_mapdata);
	
	for (int i = 0; i < 6; ++i) {
		init_counter(map_name, i);
	}
}

DWORD WINAPI blink_line(LPVOID lpThreadParameter)
{
	int line_num = (int)lpThreadParameter;

	if (Base::Data::line_blink[line_num]) {
		return TRUE;
	}

	Base::Data::line_blink[line_num] = true;

	for (int i = 0; i < 5; ++i) {
		Base::Data::lines_colors[line_num] = g_font_color_max;
		Sleep(500);
		Base::Data::lines_colors[line_num] = g_font_color;
		Sleep(500);
	}

	Base::Data::line_blink[line_num] = false;
	return TRUE;
}

int __fastcall StatSuccess(void* this_ptr, int, int event_type, int count, bool is_new)
{
	int r = StatSuccess_orig(this_ptr, event_type, count, is_new);

	if (!is_new || count == 0) {
		return r;
	}

	auto stat_name = GetMapStatName(event_type);

	if (!stat_name) {
		return r;
	}

	auto map_name = get_map_name();
	auto name = get_counter_name(map_name, stat_name);

	int index = GlobalEntity_AddEntity(name.c_str(), map_name, GLOBAL_OFF);
	int value = GlobalEntity_AddToCounter(index, 1);

	update_gui_table(event_type, map_name, value);

	int line_idx = 0;

	switch (event_type)
	{
		case 0: line_idx = 1; break;
		case 1:	line_idx = 2; break;
		case 2:	line_idx = 3; break;
		case 4:	line_idx = 4; break;
		case 5:	line_idx = 5; break;
		default: break;
	}

	CreateThread(nullptr, 0, blink_line, (LPVOID)line_idx, 0, nullptr);
	return r;
}

bool Base::Hooks::Init()
{
	infra::server_base = NULL;
	infra::engine_base = NULL;

	if (GetD3D9Device((void**)Data::pDeviceTable, D3DDEV9_LEN))
	{
		Sleep(3000); // give some time for the game engine to initialize its structures
		Data::pEndScene = Data::pDeviceTable[42];
		Data::oEndScene = (EndScene_t)mem::in::detour_trampoline((mem::voidptr_t)Data::pEndScene, (mem::voidptr_t)Hooks::EndScene, Data::szEndScene);
		Data::oWndProc  = (WndProc_t)SetWindowLongPtr(Data::hWindow, WNDPROC_INDEX, (LONG_PTR)Hooks::WndProc);
		hook_game_functions();	
		return true;
	}

	return false;
}

bool Base::Hooks::Shutdown()
{
	if (Data::InitImGui)
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	mem::in::detour_restore(Data::pEndScene, (mem::byte_t*)Data::oEndScene, Data::szEndScene);
	SetWindowLongPtr(Data::hWindow, WNDPROC_INDEX, (LONG_PTR)Data::oWndProc);
	return true;
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId = 0;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE;

	Base::Data::hWindow = handle;
	return FALSE;
}

HWND GetProcessWindow()
{
	Base::Data::hWindow = (HWND)NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return Base::Data::hWindow;
}

bool GetD3D9Device(void** pTable, size_t Size)
{
	while (true) {
		Sleep(500);

		DWORD base = (DWORD)GetModuleHandleA("shaderapidx9.dll");

		if (!base) {
			continue;
		}

		IDirect3DDevice9* dev = *reinterpret_cast<IDirect3DDevice9**>(base + 0x17E70C);

		if (!dev) {
			continue;
		}

		memcpy(pTable, *reinterpret_cast<void***>(dev), Size * sizeof(void*));
		break;
	}

	GetProcessWindow();
	return true;
}
