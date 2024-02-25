#include "stdafx.h"
#include <base.h>
#include <strsafe.h>
#include <fstream>
#include "overlay.h"
#include "SimpleIni.h"
#include "infra.h"

#include "counters.h"
#include "functional_camera.h"

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
static KeyValues__GetInt_t KeyValues__GetInt;

// Various state variables
static infra::InfraEngine* g_Engine;

#define PTR_ADD(P, A) ((void *)(((uint32_t) (P)) + (A)))


static void* GetModuleAddress(const char* name) {
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
	}

	InfraEngine::~InfraEngine() {
		for (void* const &hook : this->enabledHooks) {
			MH_DisableHook(hook);
			MH_RemoveHook(hook);
		}
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
	void* InfraEngine::get_##BASE##_ptr(const int32_t offset) { \
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

	int InfraEngine::GlobalEntity_AddEntity(const char* pGlobalname, const char* pMapName, functions::GLOBALESTATE state) const {
		return this->pGlobalEntityAddEntity(pGlobalname, pMapName, state);
	}

	void InfraEngine::GlobalEntity_SetCounter(int globalIndex, int counter) const {
		this->pGlobalEntitySetCounter(globalIndex, counter);
	}

	int InfraEngine::GlobalEntity_GetCounter(int globalIndex) const {
		return this->pGlobalEntityGetCounter(globalIndex);
	}

	int InfraEngine::GlobalEntity_AddToCounter(int globalIndex, int count) const {
		return this->pGlobalEntityAddToCounter(globalIndex, count);
	}

	int InfraEngine::GlobalEntity_GetState(int globalIndex) const {
		return this->pGlobalEntityGetState(globalIndex);
	}

	void InfraEngine::GlobalEntity_SetState(int globalIndex, functions::GLOBALESTATE state) const {
		return this->pGlobalEntitySetState(globalIndex, state);
	}

	CMatSystemTexture* InfraEngine::MaterialSystem_GetTextureById(int id) {
		void* textureDictionary = this->get_vguimatsurface_ptr(0x1432D0);
		void* textureList = *((void**)PTR_ADD(textureDictionary, 4));

		return static_cast<CMatSystemTexture*>(PTR_ADD(textureList, id << 6));
	}


	InfraEngine* Engine() {
		return g_Engine;
	}
}


static void hook_game_functions() {
	g_LogWriter.open("LOG.TXT", std::ios_base::out);

	g_LogWriter << "LOG BEGIN" << std::endl;

	// Set this up here so that hopefully our structures are all ready.
	g_Engine = new infra::InfraEngine();

	g_Engine->hook_server(0x297100, &InitMapStats, &InitMapStats_orig);
	g_Engine->hook_server(0x2974E0, &StatSuccess, &StatSuccess_orig);
	g_Engine->hook_client(0x1CCA30, &CInfraCameraFreezeFrame__OnCommand, &CInfraCameraFreezeFrame__OnCommand_orig);

	GetPlayerByIndex = reinterpret_cast<GetPlayerByIndex_t>(g_Engine->get_client_ptr(0x51DD0));
	KeyValues__GetInt = reinterpret_cast<KeyValues__GetInt_t>(g_Engine->get_client_ptr(0x3A0840));
}

void __fastcall InitMapStats(void* this_ptr) {
	InitMapStats_orig(this_ptr);

	mod::counters::InitMapStats();
}

int __fastcall StatSuccess(void* this_ptr, int, int event_type, int count, bool is_new) {
	int r = StatSuccess_orig(this_ptr, event_type, count, is_new);

	mod::counters::StatSuccess(event_type, count, is_new);
	return r;
}


// Purpose: Determine when the CInfraCameraFreezeFrame receives a command.
int __fastcall CInfraCameraFreezeFrame__OnCommand(CInfraCameraFreezeFrame* thiz, int, void* lpKeyValues) {
	int ret;

	ret = CInfraCameraFreezeFrame__OnCommand_orig(thiz, lpKeyValues);

	// It's not a DoFlash command, don't care!
	if (KeyValues__GetInt(lpKeyValues, "DoFlash", 0) != 1) {
		return ret;
	}

	mod::functional_camera::OnTakePicture(thiz);

	return ret;
}

HRESULT __stdcall EndScene(const LPDIRECT3DDEVICE9 pDevice) {
	mod::functional_camera::EndScene(pDevice);

	if (overlay::shown && !g_Engine->is_in_main_menu() && !g_Engine->loading_screen_visible()) {
		overlay::Render(Base::Data::hWindow, pDevice);
	}

	return EndScene_orig(pDevice);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	overlay::WndProc(hWnd, uMsg, wParam, lParam);

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
