#include "stdafx.h"
#include <base.h>
#include <strsafe.h>
#include <fstream>
#include "overlay.h"
#include "SimpleIni.h"
#include "infra.h"

#include "counters.h"

using namespace infra::structs;
using namespace infra::functions;

static std::ofstream g_LogWriter = std::ofstream();

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
static void* g_TextureDictionary;
static void* g_pShaderApi;
static IDirect3DDevice9* g_Dev;
static CInfraCameraFreezeFrame* g_FreezeFrame;
static unsigned int g_ShouldSaveImage; // Set to 1 by our other thread when it's time to save the image, then the Direct3D thread does the actual saving.


#define PTR_ADD(P, A) ((void *)(((uint32_t) (P)) + (A)))


static void StretchAndSaveCameraImage(LPDIRECT3DDEVICE9 dev, IDirect3DTexture9* tex);

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

static infra::InfraEngine* g_Engine;

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

	// TODO: Maybe make the hooker print out an error message if the hook fails.
#define HOOKER(BASE) \
	template <typename T> \
	void InfraEngine::hook_##BASE(const int32_t offset, LPVOID pDetour, T** pOriginal) { \
		void* pTarget = PTR_ADD(this->BASE##_base, offset); \
		if (MH_CreateHook(pTarget, pDetour, reinterpret_cast<void **>(pOriginal)) == MH_OK) { \
			MH_EnableHook(pTarget); \
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

	InfraEngine* Engine() {
		return g_Engine;
	}
}



void Base::Hooks::hook_game_functions() {
	g_LogWriter.open("LOG.TXT", std::ios_base::out);

	g_LogWriter << "LOG BEGIN" << std::endl;

	// Set this up here so that hopefully our structures are all ready.
	g_Engine = new infra::InfraEngine();

	g_Engine->hook_server(0x297100, &InitMapStats, &InitMapStats_orig);
	g_Engine->hook_server(0x2974E0, &StatSuccess, &StatSuccess_orig);
	g_Engine->hook_client(0x1CCA30, &CInfraCameraFreezeFrame__OnCommand, &CInfraCameraFreezeFrame__OnCommand_orig);

	GetPlayerByIndex = reinterpret_cast<GetPlayerByIndex_t>(g_Engine->get_client_ptr(0x51DD0));
	KeyValues__GetInt = reinterpret_cast<KeyValues__GetInt_t>(g_Engine->get_client_ptr(0x3A0840));

	g_TextureDictionary = reinterpret_cast<void*>(g_Engine->get_vguimatsurface_ptr(0x1432D0));
	g_pShaderApi = reinterpret_cast<void*>(g_Engine->get_materialsystem_ptr(0x1194A8));
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


CMatSystemTexture* GetTextureById(int id) {
	void* textureList = *((void**)PTR_ADD(g_TextureDictionary, 4));

	return reinterpret_cast<CMatSystemTexture*>(
		PTR_ADD(textureList, id << 6)
	);
}

// Purpose: Determine when the CInfraCameraFreezeFrame receives a command.
int __fastcall CInfraCameraFreezeFrame__OnCommand(CInfraCameraFreezeFrame* thiz, int, void* lpKeyValues) {
	int ret;

	ret = CInfraCameraFreezeFrame__OnCommand_orig(thiz, lpKeyValues);

	// It's not a DoFlash command, don't care!
	if (KeyValues__GetInt(lpKeyValues, "DoFlash", 0) != 1) {
		return ret;
	}

	g_FreezeFrame = thiz;

	// OnCommand actually gets run from a different thread than the DirectX thread, so we latch this in
	// in order to then check from the DirectX thread and do the save.
	InterlockedCompareExchange(&g_ShouldSaveImage, 1, 0);
	return ret;
}


static TCHAR* GetNextImagePath() {
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
		buf, MAX_PATH, TEXT("DCIM\\%S_%d-%02d-%02d_%02d%02d%02d.jpg"),
		g_Engine->get_map_name(), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond
	);

	return buf;
}

static void StretchAndSaveCameraImage(LPDIRECT3DDEVICE9 dev, IDirect3DTexture9* tex) {
	IDirect3DTexture9* pRenderTexture = nullptr;
	IDirect3DSurface9* pRenderSurface = nullptr;
	IDirect3DSurface9* pSrcSurface = nullptr;

#define CHECK_D3D_RESULT(func, msg) \
	if ((func) != D3D_OK) { \
		g_LogWriter << (msg) << std::endl; \
		g_LogWriter.flush(); \
		goto release; \
	}

	CHECK_D3D_RESULT(
		tex->GetSurfaceLevel(0, &pSrcSurface), "Failed to get src texture surface level 0"
	)

	CHECK_D3D_RESULT(
		// TODO: This needs to actually match the window aspect ratio.
		g_Dev->CreateTexture(1024, 576, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRenderTexture, nullptr),
		"Failed to CreateTexture()"
	)


	CHECK_D3D_RESULT(
		pRenderTexture->GetSurfaceLevel(0, &pRenderSurface), "Failed to GetSurfaceLevel(0)"
	)

	CHECK_D3D_RESULT(
		dev->StretchRect(pSrcSurface, NULL, pRenderSurface, NULL, D3DTEXF_NONE), "StretchRect() failed"
	)


	CHECK_D3D_RESULT(
		D3DXSaveTextureToFile(GetNextImagePath(), D3DXIFF_JPG, pRenderTexture, NULL),
		"D3DXSaveTextureToFile() failed"
	)

	g_LogWriter << "Saved image!" << std::endl;

release:
	if (pRenderSurface != nullptr) pRenderSurface->Release();
	if (pRenderTexture != nullptr) pRenderTexture->Release();
	if (pSrcSurface != nullptr) pSrcSurface->Release();
}

static void ExtractAndSaveCameraImage(CInfraCameraFreezeFrame *freezeFrame) {
	if (freezeFrame->m_pImage != NULL) {
		const char* map_name = g_Engine->get_map_name();
		CMatSystemTexture* tex = GetTextureById(freezeFrame->m_pImage->m_nTextureId);

		if (tex == nullptr) {
			g_LogWriter << "tex was null in " << map_name << std::endl;
			return;
		}

		CMaterial* mat = tex->m_pMaterial;

		if (mat == nullptr) {
			g_LogWriter << "mat was null in " << map_name << std::endl;
			return;
		}

		CTexture* repTex = mat->m_representativeTexture;

		if (repTex == nullptr) {
			g_LogWriter << "repTex was null in " << map_name << std::endl;
			return;
		}

		Texture_t** texHandles = tex->m_pMaterial->m_representativeTexture->m_pTextureHandles;

		if (texHandles == nullptr) {
			g_LogWriter << "texHandles was null in " << map_name << std::endl;
			return;
		}

		if (texHandles[0] == nullptr) {
			g_LogWriter << "texHandles[0] was null in " << map_name << std::endl;
			return;
		}


		StretchAndSaveCameraImage(g_Dev, (IDirect3DTexture9*)texHandles[0]->m_pTexture0);
	}
}

HRESULT __stdcall EndScene(LPDIRECT3DDEVICE9 pDevice) {
	if (InterlockedCompareExchange(&g_ShouldSaveImage, 0, 1)) {
		ExtractAndSaveCameraImage(g_FreezeFrame);
	}

	if (overlay::shown && !g_Engine->is_in_main_menu() && !g_Engine->loading_screen_visible()) {
		overlay::Render(Base::Data::hWindow, pDevice);
	}

	return EndScene_orig(pDevice);
}

bool Base::Hooks::Init() {
	if (MH_Initialize() != MH_OK) {
		MessageBoxA(NULL, "Failed to initialize MinHook!", "Error!", MB_OK);
		return false;
	}

	if (GetD3D9Device((void**)Data::pDeviceTable, D3DDEV9_LEN)) {
		void* pEndScene = Data::pDeviceTable[42];
\
		// MH_CreateHook(Data::pEndScene, &Hooks::EndScene, reinterpret_cast<LPVOID *>(&Data::oEndScene));
		// MH_EnableHook(Data::pEndScene);
		MH_CreateHook(pEndScene, &EndScene, reinterpret_cast<LPVOID*>(&EndScene_orig));
		MH_EnableHook(pEndScene);

		Data::oWndProc  = (WndProc_t)SetWindowLongPtr(Data::hWindow, WNDPROC_INDEX, (LONG_PTR)Hooks::WndProc);
		hook_game_functions();	

		return true;
	}

	return false;
}

bool Base::Hooks::Shutdown() {
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

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
	DWORD wndProcId = 0;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE;

	Base::Data::hWindow = handle;
	return FALSE;
}

HWND GetProcessWindow() {
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


