#include "functional_camera.h"
#include <strsafe.h>
#include "infra.h"
#include "base.h"
#include <fstream>

using infra::Engine;
using namespace infra::structs;
extern std::ofstream g_LogWriter;

static CInfraCameraFreezeFrame* g_FreezeFrame;
static unsigned int g_ShouldSaveImage; // Set to 1 by our other thread when it's time to save the image, then the Direct3D thread does the actual saving.


static TCHAR* GetNextImagePath() {
	static TCHAR buf[MAX_PATH];
	DWORD dwAttrib;
	SYSTEMTIME systemTime;

	memset(buf, 0, sizeof(buf));

	StringCchCopy(buf, MAX_PATH, TEXT("DCIM\\"));

	// Make parent dir if not exists
	dwAttrib = GetFileAttributes(buf);
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
		CreateDirectory(buf, nullptr);
	}

	GetLocalTime(&systemTime);

	swprintf(
		buf, MAX_PATH, TEXT("DCIM\\%S_%d-%02d-%02d_%02d%02d%02d.jpg"),
		Engine()->get_map_name(), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond
	);

	return buf;
}

static void StretchAndSaveCameraImage(LPDIRECT3DDEVICE9 dev, IDirect3DTexture9* tex) {
	IDirect3DTexture9* pRenderTexture = nullptr;
	IDirect3DSurface9* pRenderSurface = nullptr;
	IDirect3DSurface9* pSrcSurface = nullptr;
	RECT rect;
	int screenWidth;
	int screenHeight;
	float aspectRatio;

#define CHECK_D3D_RESULT(func, msg) \
	if ((func) != D3D_OK) { \
		g_LogWriter << (msg) << std::endl; \
		g_LogWriter.flush(); \
		goto release; \
	}

	if (!GetClientRect(Base::Data::hWindow, &rect)) {
		g_LogWriter << "StretchAndSaveCameraImage(): GetClientRect() failed" << std::endl;
		goto release;
	}

	aspectRatio = static_cast<float>(rect.bottom - rect.top) / static_cast<float>(rect.right - rect.left);

	CHECK_D3D_RESULT(
		tex->GetSurfaceLevel(0, &pSrcSurface), "StretchAndSaveCameraImage(): Failed to get src texture surface level 0"
	)

	// The texture's always 1024px wide in memory as far as I can tell, so stretch the height to match as needed.
	CHECK_D3D_RESULT(
		dev->CreateTexture(1024, static_cast<UINT>(1024 * aspectRatio), 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRenderTexture, nullptr),
		"StretchAndSaveCameraImage(): Failed to CreateTexture()"
	)

	CHECK_D3D_RESULT(
		pRenderTexture->GetSurfaceLevel(0, &pRenderSurface), "StretchAndSaveCameraImage(): Failed to get dst texture surface level 0"
	)

	CHECK_D3D_RESULT(
		dev->StretchRect(pSrcSurface, NULL, pRenderSurface, NULL, D3DTEXF_NONE), "StretchAndSaveCameraImage(): StretchRect() failed"
	)

	CHECK_D3D_RESULT(
		D3DXSaveTextureToFile(GetNextImagePath(), D3DXIFF_JPG, pRenderTexture, NULL),
		"StretchAndSaveCameraImage(): D3DXSaveTextureToFile() failed"
	)

	g_LogWriter << "StretchAndSaveCameraImage(): Successfully saved image!" << std::endl;

release:
	if (pRenderSurface != nullptr) pRenderSurface->Release();
	if (pRenderTexture != nullptr) pRenderTexture->Release();
	if (pSrcSurface != nullptr) pSrcSurface->Release();
}

static void ExtractAndSaveCameraImage(LPDIRECT3DDEVICE9 pDevice, const CInfraCameraFreezeFrame* freezeFrame) {
	if (freezeFrame->m_pImage == nullptr) {
		return;
	}

	const char* map_name = Engine()->get_map_name();
	const CMatSystemTexture* tex = Engine()->MaterialSystem_GetTextureById(freezeFrame->m_pImage->m_nTextureId);

	if (tex == nullptr) {
		g_LogWriter << "tex was null in " << map_name << std::endl;
		return;
	}

	const CMaterial* mat = tex->m_pMaterial;

	if (mat == nullptr) {
		g_LogWriter << "mat was null in " << map_name << std::endl;
		return;
	}

	const CTexture* repTex = mat->m_representativeTexture;

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


	StretchAndSaveCameraImage(pDevice, (IDirect3DTexture9*)texHandles[0]->m_pTexture0);
}

void mod::functional_camera::OnTakePicture(CInfraCameraFreezeFrame *freezeFrame) {
	g_FreezeFrame = freezeFrame;

	// OnCommand actually gets run from a different thread than the DirectX thread, so we latch this
	// in order to then check from the DirectX thread and do the save.
	InterlockedCompareExchange(&g_ShouldSaveImage, 1, 0);
}

void mod::functional_camera::EndScene(LPDIRECT3DDEVICE9 pDevice) {
	if (InterlockedCompareExchange(&g_ShouldSaveImage, 0, 1)) {
		ExtractAndSaveCameraImage(pDevice, g_FreezeFrame);
	}
}