#include "stdafx.h"
#include <base.h>
#include <strsafe.h>
#include <fstream>
#include "overlay.h"
#include "SimpleIni.h"
#include "infra.h"

#include "counters.h"
#include "functional_camera.h"
#include "inventory.h"

using namespace infra::structs;
using namespace infra::functions;

// Not static because other kids use it...
std::ofstream g_LogWriter = std::ofstream();

#define HEX(X) std::hex << (X) << std::dec

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

// I think this is actually CInfraCamera::OnCommand, but good enough.
// This gets called when we take a picture.
typedef int(__thiscall* CInfraCameraFreezeFrame__OnCommand_t)(void* thiz, void* lpKeyValues);
CInfraCameraFreezeFrame__OnCommand_t CInfraCameraFreezeFrame__OnCommand_orig;
int __fastcall CInfraCameraFreezeFrame__OnCommand(CInfraCameraFreezeFrame* thiz, int, void* lpKeyValues);


// DirectX EndScene()
HRESULT __stdcall EndScene(LPDIRECT3DDEVICE9 pDevice);
HRESULT (__stdcall *EndScene_orig)(LPDIRECT3DDEVICE9 pDevice);
/* Hooked functions end */

// Handles to functions in the engine
static GetPlayerByIndex_t GetPlayerByIndex;

// Various state variables
static bool g_SuccessCountersEnabled = true;
static bool g_FunctionalCameraEnabled = true;
static infra::InfraEngine* g_Engine;

static void* GetModuleAddress(const char* name) {
	void* addr;

	while (true) {
		addr = static_cast<void*>(GetModuleHandleA(name));
		if (addr) {
			break;
		}

		Sleep(500);
	}

	return addr;
}

namespace infra {
	InfraEngine::InfraEngine() {
		this->engine_base = GetModuleAddress("engine.dll");
		this->client_base = GetModuleAddress("client.dll");
		this->server_base = GetModuleAddress("server.dll");
		this->materialsystem_base = GetModuleAddress("MaterialSystem.dll");
		this->vguimatsurface_base = GetModuleAddress("vguimatsurface.dll");

		this->pGlobalEntityAddEntity = reinterpret_cast<GlobalEntity_AddEntity_t>(this->get_server_ptr(0x158A10));
		this->pGlobalEntitySetCounter = reinterpret_cast<GlobalEntity_SetCounter_t>(this->get_server_ptr(0x158420));
		this->pGlobalEntityGetCounter = reinterpret_cast<GlobalEntity_GetCounter_t>(this->get_server_ptr(0x1585C0));
		this->pGlobalEntityAddToCounter = reinterpret_cast<GlobalEntity_AddToCounter_t>(this->get_server_ptr(0x158450));
		this->pGlobalEntityGetState = reinterpret_cast<GlobalEntity_GetState_t>(this->get_server_ptr(0x158590));
		this->pGlobalEntitySetState = reinterpret_cast<GlobalEntity_SetState_t>(this->get_server_ptr(0x1583F0));

		this->pKeyValuesGetInt = reinterpret_cast<KeyValues__GetInt_t>(this->get_client_ptr(0x3A0840));
		// For some reason, this yields an "Access violation executing location" error - maybe this was true one day, but now it isn't?
		this->pFindEntityByName = reinterpret_cast<CGlobalEntityList__FindEntityByName_t>(this->get_server_ptr(0x10ED60));
	}

	InfraEngine::~InfraEngine() {
		for (void* const &hook : this->enabledHooks) {
			MH_DisableHook(hook);
			MH_RemoveHook(hook);
		}

		this->enabledHooks.clear();
	}

	// TODO: Maybe make the hooker print out an error message if the hook fails.
#define HOOKER(BASE) \
	template <typename T> \
	void InfraEngine::hook_##BASE(const int32_t offset, LPVOID pDetour, T** pOriginal) { \
		void* pTarget = PTR_ADD(this->BASE##_base, offset); \
		if (MH_CreateHook(pTarget, pDetour, reinterpret_cast<void **>(pOriginal)) == MH_OK) { \
			if (MH_EnableHook(pTarget) == MH_OK) { \
				this->enabledHooks.push_back(pTarget); \
			} \
		} \
	}
#define GETTER(BASE) \
	void* InfraEngine::get_##BASE##_ptr(const int32_t offset) const { \
		return PTR_ADD(this->BASE##_base, offset); \
	}

	HOOKER(client)
	HOOKER(server)

	GETTER(client)
	GETTER(server)
	GETTER(engine)
	GETTER(vguimatsurface)
	GETTER(materialsystem)

#undef HOOKER
#undef GETTER

	bool InfraEngine::is_in_main_menu() {
		return this->engine_base
			&& *(static_cast<bool*>(PTR_ADD(this->engine_base, 0x63FD86)));
	}

	bool InfraEngine::loading_screen_visible() {
		return this->engine_base
			&& *(static_cast<bool*>(PTR_ADD(this->engine_base, 0x5F9269)));
	}

	const char* InfraEngine::get_map_name() {
		if (!this->server_base) {
			return nullptr;
		}

		// 		uint32_t ptr = *(uint32_t*)((uint32_t)this->server_base + 0x78BD00) + 0x3c;

		void* ptr = *(static_cast<void**>(PTR_ADD(this->server_base, 0x78BD00)));

		return *(static_cast<char**>(PTR_ADD(ptr, 0x3C)));
	}

	int InfraEngine::GlobalEntity_AddEntity(const char* pGlobalname, const char* pMapName, const GLOBALESTATE state) const {
		return this->pGlobalEntityAddEntity(pGlobalname, pMapName, state);
	}

	void InfraEngine::GlobalEntity_SetCounter(const int globalIndex, const int counter) const {
		this->pGlobalEntitySetCounter(globalIndex, counter);
	}

	int InfraEngine::GlobalEntity_GetCounter(const int globalIndex) const {
		return this->pGlobalEntityGetCounter(globalIndex);
	}

	int InfraEngine::GlobalEntity_AddToCounter(const int globalIndex, const int count) const {
		return this->pGlobalEntityAddToCounter(globalIndex, count);
	}

	int InfraEngine::GlobalEntity_GetState(const int globalIndex) const {
		return this->pGlobalEntityGetState(globalIndex);
	}

	void InfraEngine::GlobalEntity_SetState(const int globalIndex, const GLOBALESTATE state) const {
		return this->pGlobalEntitySetState(globalIndex, state);
	}

	CMatSystemTexture* InfraEngine::MaterialSystem_GetTextureById(const int id) const {
		void* textureDictionary = this->get_vguimatsurface_ptr(0x1432D0);
		void* textureList = *((void**)PTR_ADD(textureDictionary, 4));

		return static_cast<CMatSystemTexture*>(PTR_ADD(textureList, id << 6));
	}

	CVGui *InfraEngine::VGui() const {
		// This is the g_Vgui in VGuiMaterialSurface, it was the best way I could find to get a ptr to it.
		return *static_cast<CVGui**>(PTR_ADD(this->vguimatsurface_base, 0x14E1DC));
	}

	CHud *InfraEngine::Hud() const {
		return static_cast<CHud*>(PTR_ADD(this->client_base, 0x6BB028));
	}

	int InfraEngine::KeyValues__GetInt(void* lpKeyValues, const char* name, const int defaultValue) const {
		return this->pKeyValuesGetInt(lpKeyValues, name, defaultValue);
	}

	CGlobalEntityList* InfraEngine::server_entity_list() const {
		return static_cast<CGlobalEntityList *>(this->get_server_ptr(0x725178));
	}

	CBaseEntity* InfraEngine::CGlobalEntityList__FindEntityByName(CBaseEntity* pStartEntity, const char* szName,
	                                                 CBaseEntity* pSearchingEntity, CBaseEntity* pActivator,
	                                                 CBaseEntity* pCaller, void* pFilter) const {
		return this->pFindEntityByName(this->server_entity_list(), pStartEntity, szName, pSearchingEntity, pActivator, pCaller, pFilter);
	}


	InfraEngine* Engine() {
		return g_Engine;
	}
}


static void load_config() {
	CSimpleIniA config;

	if (config.LoadFile("floorb_infra_mod.ini") == SI_OK) {
		g_SuccessCountersEnabled = config.GetBoolValue("features", "success_counters", true);
		g_FunctionalCameraEnabled = config.GetBoolValue("features", "functional_camera", true);
		overlay::fontSize = config.GetLongValue("overlay", "font_size", 0);
	} else {
		config.SetBoolValue("features", "success_counters", true);
		config.SetBoolValue("features", "functional_camera", true);
		config.SetLongValue("overlay", "font_size", 0);
		config.SaveFile("floorb_infra_mod.ini");
	}
}

static void hook_game_functions() {
	g_LogWriter.open("floorb_infra_mod.log", std::ios_base::out);
	g_LogWriter << "LOG BEGIN" << std::endl;

	load_config();

	// Set this up here so that hopefully our structures are all ready.
	g_Engine = new infra::InfraEngine();

	g_Engine->hook_server(0x297100, &InitMapStats, &InitMapStats_orig);
	g_Engine->hook_server(0x2974E0, &StatSuccess, &StatSuccess_orig);
	g_Engine->hook_client(0x1CCA30, &CInfraCameraFreezeFrame__OnCommand, &CInfraCameraFreezeFrame__OnCommand_orig);

	GetPlayerByIndex = reinterpret_cast<GetPlayerByIndex_t>(g_Engine->get_client_ptr(0x51DD0));
}

static int(__thiscall* Weapon_Equip_orig)(void*, void*);

static int __fastcall Weapon_Equip_hook(void *thiz, int, void *wep) {
	return Weapon_Equip_orig(thiz, wep);
}

void __fastcall InitMapStats(void* this_ptr) {
	InitMapStats_orig(this_ptr);

	if (g_SuccessCountersEnabled) {
		mod::counters::InitMapStats();
	}

	mod::inventory::MapLoaded(g_Engine->get_map_name());
}

int __fastcall StatSuccess(void* this_ptr, int, const int event_type, const int count, const bool is_new) {
	const int r = StatSuccess_orig(this_ptr, event_type, count, is_new);

	if (g_SuccessCountersEnabled) {
		mod::counters::StatSuccess(event_type, count, is_new);
	}
	return r;
}


// Purpose: Determine when the CInfraCameraFreezeFrame receives a command.
int __fastcall CInfraCameraFreezeFrame__OnCommand(CInfraCameraFreezeFrame* thiz, int, void* lpKeyValues) {
	int ret;

	ret = CInfraCameraFreezeFrame__OnCommand_orig(thiz, lpKeyValues);

	// It's not a DoFlash command, don't care!
	if (g_Engine->KeyValues__GetInt(lpKeyValues, "DoFlash", 0) != 1) {
		return ret;
	}

	if (g_FunctionalCameraEnabled) {
		mod::functional_camera::OnTakePicture(thiz);
	}

	return ret;
}

HRESULT __stdcall EndScene(const LPDIRECT3DDEVICE9 pDevice) {
	if (g_FunctionalCameraEnabled) {
		mod::functional_camera::EndScene(pDevice);
	}

	if (g_SuccessCountersEnabled && overlay::shown && !g_Engine->is_in_main_menu() && !g_Engine->loading_screen_visible()) {
		overlay::Render(Base::Data::hWindow, pDevice);
	}

	return EndScene_orig(pDevice);
}

LRESULT CALLBACK WndProc(const HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam) {
	if (g_SuccessCountersEnabled) {
		overlay::WndProc(hWnd, uMsg, wParam, lParam);
	}

	if (uMsg == WM_KEYDOWN && wParam == VK_DELETE) {
		void* globalBills = g_Engine->CGlobalEntityList__FindEntityByName(nullptr, "env_global_bills");

		g_LogWriter << "GlobalBills addr: " << globalBills << std::endl;

		CINFRA_Player* pPlayer = reinterpret_cast<CINFRA_Player*>(
			g_Engine->CGlobalEntityList__FindEntityByName(nullptr, "!player")
		);
		const CGlobalEntityList* pEntityList = g_Engine->server_entity_list();
		g_LogWriter << "Player Entity " << pPlayer << std::endl;
		g_LogWriter << "Entity List " << pEntityList << std::endl;

		/*
		for (int i = 0; i < NUM_ENT_ENTRIES; i++) {
			CEntInfo info = pEntityList->m_EntPtrArray[i];

			g_LogWriter << "Entity " << i << " class: " << info.m_iClassName << ", name: " << info.m_iName << std::endl;
		}*/

		g_LogWriter << "First weapon " << pEntityList->LookupEntity(&(pPlayer->m_Weapon0)) << std::endl;
		g_LogWriter << "Second weapon " << pEntityList->LookupEntity(&(pPlayer->m_Weapon1)) << std::endl;
		g_LogWriter << "Third weapon " << pEntityList->LookupEntity(&(pPlayer->m_Weapon2)) << std::endl;
	}

	return CallWindowProc(Base::Data::oWndProc, hWnd, uMsg, wParam, lParam);
}

bool Base::Hooks::Init() {
	if (MH_Initialize() != MH_OK) {
		MessageBoxA(nullptr, "Failed to initialize MinHook!", "Error!", MB_OK);
		return false;
	}

	if (GetD3D9Device((void**)Data::pDeviceTable, D3DDEV9_LEN)) {
		void* pEndScene = Data::pDeviceTable[42];

		MH_CreateHook(pEndScene, &EndScene, reinterpret_cast<LPVOID*>(&EndScene_orig));
		MH_EnableHook(pEndScene);

		Data::oWndProc  = (WndProc_t)SetWindowLongPtr(Data::hWindow, WNDPROC_INDEX, (LONG_PTR)WndProc);
		hook_game_functions();	

		return true;
	}

	return false;
}

bool Base::Hooks::Shutdown() {
	void* pEndScene = Data::pDeviceTable[42];

	if (overlay::imGuiInitialized) {
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	delete g_Engine;

	MH_DisableHook(pEndScene);
	MH_RemoveHook(pEndScene);

	SetWindowLongPtr(Data::hWindow, WNDPROC_INDEX, (LONG_PTR)Data::oWndProc);

	MH_Uninitialize();
	return true;
}

BOOL CALLBACK EnumWindowsCallback(const HWND handle, LPARAM lParam) {
	DWORD wndProcId = 0;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE;

	Base::Data::hWindow = handle;
	return FALSE;
}

HWND GetProcessWindow() {
	Base::Data::hWindow = (HWND)nullptr;
	EnumWindows(EnumWindowsCallback, NULL);
	return Base::Data::hWindow;
}

bool GetD3D9Device(void** pTable, const size_t Size) {
	while (true) {
		void *base = GetModuleAddress("shaderapidx9.dll");

		IDirect3DDevice9* dev = *reinterpret_cast<IDirect3DDevice9**>(PTR_ADD(base, 0x17E70C));

		if (!dev) {
			Sleep(500);
			continue;
		}

		memcpy(pTable, *reinterpret_cast<void***>(dev), Size * sizeof(void*));
		break;
	}

	GetProcessWindow();
	return true;
}
