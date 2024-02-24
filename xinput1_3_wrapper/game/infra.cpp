#include "stdafx.h"
#include <base.h>
#include <metadata.h>
#include <strsafe.h>
#include <fstream>
#include "structs.h"

using namespace infra::structs;
using namespace infra::functions;

namespace infra
{
	void* server_base;
	void* engine_base;
	void* client_base;

	void* vguimatsurface_base;
	void* materialsystem_base;
};

static std::ofstream g_LogWriter = std::ofstream();

#define HEX(X) std::hex << (X) << std::dec

bool g_server_spawned = false;

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam);
HWND GetProcessWindow();
bool GetD3D9Device(void** pTable, size_t Size);

/* Hooked functions begin */
typedef void(__thiscall* InitMapStats_t)(void*);
InitMapStats_t InitMapStats_orig;
void __fastcall InitMapStats(void* this_ptr);

typedef int(__thiscall* StatSuccess_t)(void* this_ptr, int event_type, int count, bool is_new);
StatSuccess_t StatSuccess_orig;
int __fastcall StatSuccess(void* this_ptr, int, int event_type, int count, bool is_new);

// This gets called when we set up the CInfraCameraFreezeFrame
typedef void*(__thiscall *CInfraCameraFreezeFrame__new_t)(void* this_ptr, int a2, void* p, int a4);
CInfraCameraFreezeFrame__new_t CInfraCameraFreezeFrame__new_orig;
void* __fastcall CInfraCameraFreezeFrame__new(void* this_ptr, int, int a2, void* p, int a4);

// I think this is actually CInfraCamera::OnCommand, but good enough.
// This gets called when we take a picture.
typedef int(__thiscall* CInfraCameraFreezeFrame__OnCommand_t)(void* thiz, void* lpKeyValues);
CInfraCameraFreezeFrame__OnCommand_t CInfraCameraFreezeFrame__OnCommand_orig;
int __fastcall CInfraCameraFreezeFrame__OnCommand(void* thiz, int, void* lpKeyValues);

typedef void(__thiscall* sub_16D3B0_t)(void* thiz);
sub_16D3B0_t sub_16D3B0_orig;
void __fastcall sub_16D3B0(void* thiz);

typedef void(__thiscall* sub_1ADDA0_t)(void* thiz);



// This is a hook of DirectX's SetTexture() function.
static SetTexture_t SetTexture_orig;
void __stdcall SetTexture(LPDIRECT3DDEVICE9 thiz, DWORD Stage, IDirect3DBaseTexture9* pTexture);

/* Hooked functions end */

// Handles to functions in the engine
static GlobalEntity_AddEntity_t GlobalEntity_AddEntity;
static GlobalEntity_SetCounter_t GlobalEntity_SetCounter;
static GlobalEntity_GetCounter_t GlobalEntity_GetCounter;
static GlobalEntity_AddToCounter_t GlobalEntity_AddToCounter;
static GlobalEntity_GetState_t GlobalEntity_GetState;
static GlobalEntity_SetState_t GlobalEntity_SetState;
static GetPlayerByIndex_t GetPlayerByIndex;
static KeyValues__GetInt_t KeyValues__GetInt;
static void* g_TextureDictionary;
static void* g_pShaderApi;

static IDirect3DDevice9* g_Dev;

nlohmann::json current_mapdata;
static long long g_LastCameraSnap = -1;
static IDirect3DTexture9* g_CameraTexture = NULL;
static CInfraCameraFreezeFrame* g_CameraFreezeFrame = nullptr;

static void StretchAndSaveCameraImage(LPDIRECT3DDEVICE9 dev, IDirect3DTexture9* tex, int index);

/*
 * State machine explanation:
 * Normally, we're in the state machine IDLE state. This is nothing special, just the "rest" state.
 * 
 * When we take a picture, and it's time for the camera to flash, we transition to TOOK_PICTURE state.
 * 
 * Once we're in TOOK_PICTURE, we wait for the next time the camera screen gets rendered, and that transitions us to RENDERED_ONCE.
 * 
 * In RENDERED_ONCE, this is where we get a bit silly. We monitor SetTexture() calls in Direct3D, and use heuristics to guess which texture is the
 * camera texture. Then, we save it, and go back into IDLE.
 */
enum CameraState {
	CS_IDLE,
	CS_TOOK_PICTURE,
	CS_RENDERED_ONCE
};

static LONG g_CameraState = CS_IDLE;
static int g_StageNumber;


static long long CurrentTimeMillis() {
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);

	return ((LONGLONG)ft.dwLowDateTime + ((LONGLONG)(ft.dwHighDateTime) << 32LL)) / 10000;
}

void* GetModuleAddress(const char* name) {
	void* addr;

	while (true) {
		addr = (void*)GetModuleHandleA(name);
		if (addr) {
			break;
		}

		Sleep(500);
	}

	return addr;
}

void Base::Hooks::hook_game_functions() {

	g_LogWriter.open("LOG.TXT", std::ios_base::out);

	g_LogWriter << "LOG BEGIN" << std::endl;

	infra::server_base = GetModuleAddress("server.dll");
	infra::engine_base = GetModuleAddress("engine.dll");
	infra::client_base = GetModuleAddress("client.dll");
	infra::vguimatsurface_base = GetModuleAddress("vguimatsurface.dll");
	infra::materialsystem_base = GetModuleAddress("MaterialSystem.dll");

	g_LogWriter << "vguimatsurface.dll is at " << std::hex << infra::vguimatsurface_base << std::dec << std::endl;

#define FHOOK(fname, libname, offset) \
	auto fname##_ptr = (void*)((uint32_t)infra::libname##_base + offset); \
	if (MH_CreateHook(fname##_ptr, &fname, reinterpret_cast<LPVOID *>(&fname##_orig)) != MH_OK) { \
		MessageBoxA(NULL, "Failed to make hook!", "Error!", MB_OK); \
		return; \
	} \
	if (MH_EnableHook(fname##_ptr) != MH_OK) { \
		MessageBoxA(NULL, "Failed to enable hook!", "Error!", MB_OK); \
	}

	FHOOK(InitMapStats, server, 0x297100);
	FHOOK(StatSuccess, server, 0x2974E0);
	FHOOK(CInfraCameraFreezeFrame__new, client, 0x1CCB60);
	FHOOK(CInfraCameraFreezeFrame__OnCommand, client, 0x1CCA30);
	FHOOK(sub_16D3B0, client, 0x16D3B0);

	GlobalEntity_AddEntity = (GlobalEntity_AddEntity_t)((uint32_t)infra::server_base + 0x158A10);
	GlobalEntity_SetCounter = (GlobalEntity_SetCounter_t)((uint32_t)infra::server_base + 0x158420);
	GlobalEntity_GetCounter = (GlobalEntity_GetCounter_t)((uint32_t)infra::server_base + 0x1585C0);
	GlobalEntity_AddToCounter = (GlobalEntity_AddToCounter_t)((uint32_t)infra::server_base + 0x158450);
	GlobalEntity_GetState = (GlobalEntity_GetState_t)((uint32_t)infra::server_base + 0x158590);
	GlobalEntity_SetState = (GlobalEntity_SetState_t)((uint32_t)infra::server_base + 0x1583F0);
	GetPlayerByIndex = (GetPlayerByIndex_t)((uint32_t)infra::client_base + 0x51DD0);
	KeyValues__GetInt = (KeyValues__GetInt_t)((uint32_t)infra::client_base + 0x3A0840);
	g_TextureDictionary = (void*)((uint32_t)infra::vguimatsurface_base + 0x1432D0);
	g_pShaderApi = (void*)((uint32_t)infra::materialsystem_base + 0x1194A8);


	try {
		std::ifstream f("mapdata.txt");
		auto data = nlohmann::json::parse(f);
		g_mapdata = data;
	}
	catch (std::exception e) {

	}
}

bool Base::Hooks::is_in_main_menu() {
	return (infra::engine_base) ?
		*(bool*)((uint32_t)infra::engine_base + 0x63FD86) : true;
}

bool Base::Hooks::loading_screen_visible() {
	return (infra::engine_base) ?
		*(bool*)((uint32_t)infra::engine_base + 0x5F9269) : true;
}

const char* get_map_name() {
	if (!infra::server_base) {
		return NULL;
	}

	uint32_t ptr = *(uint32_t*)((uint32_t)infra::server_base + 0x78BD00) + 0x3c;
	return *(char**)ptr;
}

const char* GetMapStatName(int event_type) {
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

std::string get_counter_name(const char* map_name, const char* stat_name) {
	return std::string(map_name) + "_counter_" + std::string(stat_name);
}

std::string format_stat(int value, int max_value) {
	return std::to_string(value) + " / " + std::to_string(max_value);
}

int get_max_value(int event_type, const char* map_name) {
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

void update_gui_table(int event_type, const char* map_name, int value) {
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

void init_counter(const char* map_name, int event_type) {
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

void exclude_inactive_photo_spot(std::string map_name, std::string spot_name, nlohmann::json& mapdata) {
	int index = GlobalEntity_AddEntity(spot_name.c_str(), map_name.c_str(), GLOBAL_OFF);

	if (GlobalEntity_GetState(index) == GLOBAL_ON) {
		int num = mapdata[map_name]["camera_targets"];

		if (num > 0) {
			--num;
		}

		mapdata[map_name]["camera_targets"] = num;
	}
}

auto exclude_inactive_photo_spots(std::string map_name, nlohmann::json mapdata) {
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
void __fastcall InitMapStats(void* this_ptr) {
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

int __fastcall StatSuccess(void* this_ptr, int, int event_type, int count, bool is_new) {
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

void DumpMatSystemTexture(const CMatSystemTexture* pTexture) {
	if (!pTexture) {
		g_LogWriter << "CMatSystemTexture@0x00 { nullptr }" << std::endl;
		return;
	}

	g_LogWriter << "CMatSystemTexture@" << pTexture << " {" << std::endl;
	g_LogWriter << "    m_s0: " << pTexture->m_s0 << std::endl;
	g_LogWriter << "    m_t0: " << pTexture->m_t0 << std::endl;
	g_LogWriter << "    m_s1: " << pTexture->m_s1 << std::endl;
	g_LogWriter << "    m_t1: " << pTexture->m_t1 << std::endl;
	g_LogWriter << "    m_crcFile: " << pTexture->m_crcFile << std::endl;
	g_LogWriter << "    m_pMaterial: " << pTexture->m_pMaterial << std::endl;
	g_LogWriter << "    m_pTexture: " << pTexture->m_pTexture << std::endl;
	g_LogWriter << "    m_Texture2: " << pTexture->m_Texture2 << std::endl;
	g_LogWriter << "    m_iWide: " << pTexture->m_iWide << std::endl;
	g_LogWriter << "    m_iTall: " << pTexture->m_iTall << std::endl;
	g_LogWriter << "    m_iInputWide: " << pTexture->m_iInputWide << std::endl;
	g_LogWriter << "    m_iInputTall: " << pTexture->m_iInputTall << std::endl;
	g_LogWriter << "    m_ID: " << pTexture->m_ID << std::endl;
	g_LogWriter << "    m_Flags: " << pTexture->m_Flags << std::endl;
	g_LogWriter << "    m_pRegen: " << pTexture->m_pRegen << std::endl;
	g_LogWriter << "}" << std::endl;
}

void DumpTexture(const Texture_t* pTexture) {
	if (!pTexture) {
		g_LogWriter << "Texture_t@0x00 { nullptr }" << std::endl;
		return;
	}

	g_LogWriter << "Texture_t@" << pTexture << " {" << std::endl;
	g_LogWriter << "    m_UTexWrap: " << static_cast<int>(pTexture->m_UTexWrap) << std::endl;
	g_LogWriter << "    m_VTexWrap: " << static_cast<int>(pTexture->m_VTexWrap) << std::endl;
	g_LogWriter << "    m_WTexWrap: " << static_cast<int>(pTexture->m_WTexWrap) << std::endl;
	g_LogWriter << "    m_MagFilter: " << static_cast<int>(pTexture->m_MagFilter) << std::endl;
	g_LogWriter << "    m_MinFilter: " << static_cast<int>(pTexture->m_MinFilter) << std::endl;
	g_LogWriter << "    m_MipFilter: " << static_cast<int>(pTexture->m_MipFilter) << std::endl;
	g_LogWriter << "    m_NumLevels: " << static_cast<int>(pTexture->m_NumLevels) << std::endl;
	g_LogWriter << "    m_SwitchNeeded: " << static_cast<int>(pTexture->m_SwitchNeeded) << std::endl;
	g_LogWriter << "    m_NumCopies: " << static_cast<int>(pTexture->m_NumCopies) << std::endl;
	g_LogWriter << "    m_CurrentCopy: " << static_cast<int>(pTexture->m_CurrentCopy) << std::endl;
	g_LogWriter << "    m_pTexture0: " << pTexture->m_pTexture0 << std::endl;
	g_LogWriter << "    m_pTexture1: " << pTexture->m_pTexture1 << std::endl;
	g_LogWriter << "    m_CreationFlags: " << pTexture->m_CreationFlags << std::endl;
	g_LogWriter << "    m_DebugName: " << pTexture->m_DebugName << std::endl;
	g_LogWriter << "    m_TextureGroupName: " << pTexture->m_TextureGroupName << std::endl;
	g_LogWriter << "    m_SizeBytes: " << pTexture->m_SizeBytes << std::endl;
	g_LogWriter << "    m_SizeTexels: " << pTexture->m_SizeTexels << std::endl;
	g_LogWriter << "    m_LastBoundFrame: " << pTexture->m_LastBoundFrame << std::endl;
	g_LogWriter << "    m_nTimesBoundMax: " << pTexture->m_nTimesBoundMax << std::endl;
	g_LogWriter << "    m_nTimesBoundThisFrame: " << pTexture->m_nTimesBoundThisFrame << std::endl;
	g_LogWriter << "    m_Width: " << pTexture->m_Width << std::endl;
	g_LogWriter << "    m_Height: " << pTexture->m_Height << std::endl;
	g_LogWriter << "    m_Depth: " << pTexture->m_Depth << std::endl;
	g_LogWriter << "    m_Flags: " << pTexture->m_Flags << std::endl;
	g_LogWriter << "}" << std::endl;
}

void SimpleDumpMatSystemTexture(const CMatSystemTexture* pTexture) {
	if (!pTexture) {
		g_LogWriter << "CMatSystemTexture@0x00 { nullptr }" << std::endl;
		return;
	}

	g_LogWriter << "CMatSystemTexture@" << pTexture << " {" << std::endl;
	if (pTexture->m_pMaterial) {
		g_LogWriter << "    m_pMaterial: CMaterial@" << HEX(pTexture->m_pMaterial) << " {" << std::endl;

		if (pTexture->m_pMaterial->m_representativeTexture) {
			g_LogWriter << "        m_representativeTexture: CTexture@" << HEX(pTexture->m_pMaterial->m_representativeTexture) << " {" << std::endl;
			if (pTexture->m_pMaterial->m_representativeTexture->m_pTextureHandles) {
				g_LogWriter << "            m_pTextureHandles[0]: " << pTexture->m_pMaterial->m_representativeTexture->m_pTextureHandles[0] << std::endl;
			}
			else {
				g_LogWriter << "            m_pTextureHandles: nullptr" << std::endl;
			}
			g_LogWriter << "        }" << std::endl;
		}
		else {
			g_LogWriter << "        m_representativeTexture: CTexture@0x00 { nullptr }" << std::endl;
		}

		g_LogWriter << "    }" << std::endl;
	}
	else {
		g_LogWriter << "    m_pMaterial: CMaterial@0x00 { nullptr }" << std::endl;
	}
	g_LogWriter << "}" << std::endl;

}

#define PTR_ADD(P, A) ((void *)(((uint32_t) (P)) + (A)))


CMatSystemTexture* GetTextureById(int id) {
	void* textureList = *((void**)PTR_ADD(g_TextureDictionary, 4));

	return reinterpret_cast<CMatSystemTexture*>(
		PTR_ADD(textureList, id << 6)
	);
}




// Purpose: Intercept the constructor of CInfraCameraFreezeFrame to capture a pointer to it.
void* __fastcall CInfraCameraFreezeFrame__new(void* this_ptr, int, int a2, void* p, int a4) {
	CInfraCameraFreezeFrame *freezeFrame = reinterpret_cast<CInfraCameraFreezeFrame *>(
		CInfraCameraFreezeFrame__new_orig(this_ptr, a2, p, a4)
	);

	if (true || g_CameraFreezeFrame == nullptr) {
		g_CameraFreezeFrame = freezeFrame;
		g_LogWriter << "Captured CInfraCameraFreezeFrame - sanity check: _panelName: " << freezeFrame->_panelName << " ptr: 0x" << std::hex << g_CameraFreezeFrame << std::dec << std::endl;
		g_LogWriter.flush();



	}

	return freezeFrame;
}

// Purpose: Determine when the CInfraCameraFreezeFrame receives a command.
int __fastcall CInfraCameraFreezeFrame__OnCommand(void* thiz, int, void* lpKeyValues) {
	int ret;
	CInfraCameraFreezeFrame* freezeFrame = g_CameraFreezeFrame;

	ret = CInfraCameraFreezeFrame__OnCommand_orig(thiz, lpKeyValues);

	// It's not a DoFlash command, don't care!
	if (KeyValues__GetInt(lpKeyValues, "DoFlash", 0) != 1) {
		return ret;
	}
	void** llL = (void **)PTR_ADD(g_TextureDictionary, 4);
	void* ll = *llL;
	g_LogWriter << "ll is at " << ll << std::endl;
	g_LogWriter << "g_TextureDictionary is at " << std::hex << g_TextureDictionary << std::dec << std::endl;
	if (freezeFrame->m_pImage != NULL) {
		int startId = freezeFrame->m_pImage->m_nTextureId;
		int endId = 1;

		g_LogWriter << "freezeFrame is " << HEX(freezeFrame) << std::endl;
		g_LogWriter << "thiz is " << HEX(thiz) << std::endl;
		g_LogWriter << "Texture ID is " << startId << std::endl;

		//for (int i = 0; i < startId; i++) {
		//	SimpleDumpMatSystemTexture(
		//		GetTextureById(i)
		//	);
		//}

		// I am like 17% sure the stupid texture handles are just random garbage numbers


		// Current issue: I'm getting the wrong texture pointer, it's just something random - but it IS valid memory.
		CMatSystemTexture* tex = GetTextureById(startId);

		Texture_t** texHandles = tex->m_pMaterial->m_representativeTexture->m_pTextureHandles;

		g_LogWriter << "CMatSystemTexture is at " << HEX(tex) << std::endl;
		g_LogWriter << "tex->m_pMaterial is at " << HEX(tex->m_pMaterial) << std::endl;
		g_LogWriter << "tex->m_pMaterial->m_representativeTexture is at " << HEX(tex->m_pMaterial->m_representativeTexture) << std::endl;
		//g_LogWriter << "potential handles: " << texHandles[0] << ", " << texHandles[1] << std::endl;

		DumpTexture(texHandles[0]);

		StretchAndSaveCameraImage(g_Dev, (IDirect3DTexture9 *)texHandles[0]->m_pTexture0, 0);

		for (int i = endId; i < startId; i++) {
		//	g_LogWriter << "Texture ID: " << i << std::endl;
		//	DumpMatSystemTexture(
		//		GetTextureById(i)
		//	);
		}
	}

	g_LogWriter << "Got DoFlash command" << std::endl;

	// Use cached texture if we have it, otherwise start the dance to get the texture handle out of DirectX :-)
	if (g_CameraTexture != NULL) {
		StretchAndSaveCameraImage(g_Dev, g_CameraTexture, 0);
		g_LogWriter << "Saved image using cached camera texture" << std::endl;
	}
	else {
		//InterlockedExchange(&g_CameraState, CS_TOOK_PICTURE);
		//g_CameraState = CS_TOOK_PICTURE;
		g_LogWriter << "Transitioning to CS_TOOK_PICTURE" << std::endl;
	}

	return ret;
}

static int g_SetTextureCounter;

// I don't even know what this subroutine it, it's probably CBitmapPanel::DoPaint but I'm not very confident.
void __fastcall sub_16D3B0(void* thiz) {
	if (thiz == g_CameraFreezeFrame && InterlockedCompareExchange(&g_CameraState, CS_RENDERED_ONCE, CS_TOOK_PICTURE) == CS_TOOK_PICTURE) {
		//g_CameraState = CS_RENDERED_ONCE;
		g_SetTextureCounter = 0;
		g_LogWriter << "Transitioning to CS_RENDERED_ONCE from thread " << GetCurrentThreadId() << std::endl;
	}

	sub_16D3B0_orig(thiz);
}

static TCHAR* GetNextImagePath(int index) {
	static TCHAR buf[MAX_PATH];
	DWORD dwAttrib;
	SYSTEMTIME systemTime;

	memset(buf, 0, sizeof(buf));
	
	StringCchCopy(buf, MAX_PATH, TEXT("DCIM\\"));
	
	// Make parent dir if not exists
	dwAttrib = GetFileAttributes(buf);
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
		CreateDirectory(buf, NULL);
	}

	GetLocalTime(&systemTime);

	swprintf(
		buf, MAX_PATH, TEXT("DCIM\\%S_%d-%02d-%02d_%02d%02d%02d__%d.jpg"),
		get_map_name(), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond,
		index
	);

	return buf;
}

static void StretchAndSaveCameraImage(LPDIRECT3DDEVICE9 dev, IDirect3DTexture9* tex, int index) {
	IDirect3DTexture9* pRenderTexture = nullptr;
	IDirect3DSurface9* pRenderSurface = nullptr;
	IDirect3DSurface9* pSrcSurface = nullptr;

#define CHECK_IT(func, msg) \
	if (func != D3D_OK) { \
		g_LogWriter << msg << std::endl; \
		g_LogWriter.flush(); \
		goto release; \
	}


	CHECK_IT(
		tex->GetSurfaceLevel(0, &pSrcSurface), "Failed to get src texture surface level 0"
	);

	CHECK_IT(
		// TODO: This needs to actually match the window aspect ratio.
		g_Dev->CreateTexture(1024, 576, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRenderTexture, nullptr),
		"Failed to CreateTexture()"
	);


	CHECK_IT(
		pRenderTexture->GetSurfaceLevel(0, &pRenderSurface), "Failed to GetSurfaceLevel(0)"
	);

	CHECK_IT(
		dev->StretchRect(pSrcSurface, NULL, pRenderSurface, NULL, D3DTEXF_NONE), "StretchRect() failed"
	);


	D3DXSaveTextureToFile(
		GetNextImagePath(index), D3DXIFF_JPG, pRenderTexture, NULL
	);

	g_LogWriter << "Saved image!" << std::endl;

release:
	if (pRenderSurface != nullptr) pRenderSurface->Release();
	if (pRenderTexture != nullptr) pRenderTexture->Release();
	if (pSrcSurface != nullptr) pSrcSurface->Release();
}

// Purpose: Hook Direct3D SetTexture() and use heuristics to capture the DirectX texture handle used for the camera screen.
void __stdcall SetTexture(LPDIRECT3DDEVICE9 thiz, DWORD Stage, IDirect3DBaseTexture9* pBaseTexture) {
	int dummy;
	int* addr;
	int i;
	D3DSURFACE_DESC surfaceDesc;
	IDirect3DTexture9* pTexture;

	SetTexture_orig(thiz, Stage, pBaseTexture);

	if (g_CameraState != CS_RENDERED_ONCE) {
		return;
	}

	if (pBaseTexture == nullptr) {
		return;
	}

	// Please for the love of h*ck never be on stage 0, this is the only heuristic I have, please dear Gods.
	if (Stage == 0) {
		//return;
	}

	//if (pBaseTexture->GetLevelCount() > 1) {
		//g_LogWriter << "Wrong level count" << std::endl;
	//	return;
	//}

	//if (!SUCCEEDED(pBaseTexture->QueryInterface(__uuidof(IDirect3DTexture9), (void**)&pTexture))) {
		//g_LogWriter << "No QueryInterface()" << std::endl;

//		return;
	//}

	pTexture = (IDirect3DTexture9*)pBaseTexture;

	//goto release;

	memset(&surfaceDesc, 0, sizeof(surfaceDesc));

	if (pTexture->GetLevelDesc(0, &surfaceDesc) != D3D_OK) {
		//g_LogWriter << "No GetLevelDesc()" << std::endl;

		return;
	}

	if (surfaceDesc.Width != 1024 || surfaceDesc.Height != 1024) {
		//g_LogWriter << "Wrong size" << std::endl;

		return;
	}

	// Future me: If you want to get to a known good state, load the first map and take a picture of the no fishing sign outside your car.
	// That should be loaded onto Stage 2.
	if (surfaceDesc.Format != D3DFMT_A8R8G8B8 || surfaceDesc.MultiSampleQuality != 0 || surfaceDesc.MultiSampleType != 0 || surfaceDesc.Type != D3DRTYPE_SURFACE || surfaceDesc.Usage != 1) {
		//g_LogWriter << "Wrong formats" << std::endl;

		return;
	}

	g_LogWriter << "SetTexture(): " << Stage << " " << pBaseTexture << " (counter: " << g_SetTextureCounter << ")" << std::endl;
	g_LogWriter << "Format: " << surfaceDesc.Format << ", MSQ: " << surfaceDesc.MultiSampleQuality << ", MST: " << surfaceDesc.MultiSampleType <<
		", Type: " << surfaceDesc.Type << ", Usage: " << surfaceDesc.Usage << ", thread: " << GetCurrentThreadId() << std::endl;
	g_LogWriter.flush();

	//if (g_SetTextureCounter > 5) {
		g_CameraState = CS_IDLE;
	//}

	// g_CameraTexture = pTexture;
	StretchAndSaveCameraImage(
		g_Dev, pTexture, g_SetTextureCounter
	);

	g_SetTextureCounter++;
}

bool Base::Hooks::Init() {
	infra::server_base = NULL;
	infra::engine_base = NULL;
	infra::client_base = NULL;

	if (MH_Initialize() != MH_OK) {
		MessageBoxA(NULL, "Failed to initialize MinHook!", "Error!", MB_OK);
		return false;
	}

	if (GetD3D9Device((void**)Data::pDeviceTable, D3DDEV9_LEN)) {
		// Sleep(3000); // give some time for the game engine to initialize its structures
		Data::pEndScene = Data::pDeviceTable[42];

		MH_CreateHook(Data::pEndScene, &Hooks::EndScene, reinterpret_cast<LPVOID *>(&Data::oEndScene));
		MH_EnableHook(Data::pEndScene);

		// SetTexture hook
		MH_CreateHook(Data::pDeviceTable[65], &SetTexture, reinterpret_cast<LPVOID*>(&SetTexture_orig));
		MH_EnableHook(Data::pDeviceTable[65]);

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

	// mem::in::detour_restore(Data::pEndScene, (mem::byte_t*)Data::oEndScene, Data::szEndScene);
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

bool GetD3D9Device(void** pTable, size_t Size) {
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

		g_Dev = dev;

		memcpy(pTable, *reinterpret_cast<void***>(dev), Size * sizeof(void*));
		break;
	}

	GetProcessWindow();
	return true;
}


