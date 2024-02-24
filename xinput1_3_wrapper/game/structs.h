#pragma once
#include <stdafx.h>

namespace infra {
	// Various structures reverse-engineered from Infra / its version of the Source engine.
	namespace structs {
		// I made these with ReClass.NET
		class BitmapImage
		{
		public:
			char pad_0000[4]; //0x0000
			int32_t _upstairsPosX; //0x0004
			int32_t _upstairsPosY; //0x0008
			int32_t _upstairsSizeX; //0x000C
			int32_t _upstairsSizeY; //0x0010
			int32_t _color; //0x0014
			int32_t m_nTextureId; //0x0018
			int32_t m_clr; //0x001C
			char pad_0020[8]; //0x0020
			int32_t sizeX; //0x0028
			int32_t sizeY; //0x002C
			char pad_0030[4]; //0x0030
			bool b_useViewport; //0x0034
		}; //Size: 0x0035

		class CInfraCameraFreezeFrame
		{
		public:
			char pad_0000[80]; //0x0000
			void* _vpanel; //0x0050
			char* _panelName; //0x0054
			char pad_0058[260]; //0x0058
			BitmapImage* m_pImage; //0x015C
		}; //Size: 0x033C

		class CMaterialVar
		{
		public:
			char pad_0000[4]; //0x0000
			char* m_pStringVal; //0x0004
			int32_t m_intVal; //0x0008
			float m_VecVal[4]; //0x000C
			uint8_t m_Flags; //0x001C
			uint16_t m_Name; //0x001D
		}; //Size: 0x001F


		class Texture_t
		{
		public:
			uint8_t m_UTexWrap; //0x0000
			uint8_t m_VTexWrap; //0x0001
			uint8_t m_WTexWrap; //0x0002
			uint8_t m_MagFilter; //0x0003
			uint8_t m_MinFilter; //0x0004
			uint8_t m_MipFilter; //0x0005
			uint8_t m_NumLevels; //0x0006
			uint8_t m_SwitchNeeded; //0x0007
			uint8_t m_NumCopies; //0x0008
			uint8_t m_CurrentCopy; //0x0009
			char pad_000A[2]; //0x000A
			void* m_pTexture0; //0x000C
			void* m_pTexture1; //0x0010
			int32_t N000003E4; //0x0014
			int32_t m_CreationFlags; //0x0018
			uint16_t m_DebugName; //0x001C
			uint16_t m_TextureGroupName; //0x001E
			void* m_pTextureGroupCounterGlobal; //0x0020
			void* m_pTextureGroupCounterFrame; //0x0024
			int32_t m_SizeBytes; //0x0028
			int32_t m_SizeTexels; //0x002C
			int32_t m_LastBoundFrame; //0x0030
			int32_t m_nTimesBoundMax; //0x0034
			int32_t m_nTimesBoundThisFrame; //0x0038
			int16_t m_Width; //0x003C
			int16_t m_Height; //0x003E
			int16_t m_Depth; //0x0040
			uint16_t m_Flags; //0x0042
		}; //Size: 0x0044

		class CTexture
		{
		public:
			char pad_0000[4]; //0x0000
			float m_vecReflectivity[3]; //0x0004
			int16_t m_Name; //0x0010
			int16_t m_TextureGroupName; //0x0012
			uint32_t m_nFlags; //0x0014
			uint32_t m_nInternalFlags; //0x0018
			int32_t m_nRefCount; //0x001C
			int32_t m_ImageFormat; //0x0020
			uint16_t m_nMappingWidth; //0x0024
			uint16_t m_nMappingHeight; //0x0026
			uint16_t m_nMappingDepth; //0x0028
			uint16_t m_nActualWidth; //0x002A
			uint16_t m_nActualHeight; //0x002C
			uint16_t m_nActualDepth; //0x002E
			uint16_t m_nActualMipCount; //0x0030
			uint16_t m_nFrameCount; //0x0032
			uint16_t m_nOriginalRTWidth; //0x0034
			uint16_t m_nOriginalRTHeight; //0x0036
			int16_t m_nDesiredDimensionLimit; //0x0038
			int16_t m_nDesiredTempDimensionLimit; //0x003A
			int16_t m_nActualDimensionLimit; //0x003C
			uint16_t m_nMipSkipCount; //0x003E
			Texture_t** m_pTextureHandles; //0x0040
			void* m_pTempTextureHandles; //0x0044
			char* m_pLowResImage; //0x0048
			void* m_nOriginalRenderTargetType; //0x004C
		}; //Size: 0x0050

		class CMaterial
		{
		public:
			char pad_0000[44]; //0x0000
			void* m_pShader; //0x002C
			int16_t m_Name; //0x0030
			int16_t m_TextureGroupName; //0x0032
			int32_t m_RefCount; //0x0034
			uint16_t m_Flags; //0x0038
			uint8_t m_VarCount; //0x003A
			uint8_t m_ProxyCount; //0x003B
			CMaterialVar** m_pShaderParams; //0x003C
			char pad_0040[64]; //0x0040
			CTexture* m_representativeTexture; //0x0080
		}; //Size: 0x0084

		class CMatSystemTexture
		{
		public:
			float m_s0; //0x0000
			float m_t0; //0x0004
			float m_s1; //0x0008
			float m_t1; //0x000C
			int32_t m_crcFile; //0x0010
			CMaterial* m_pMaterial; //0x0014
			CTexture* m_pTexture; //0x0018
			int32_t m_Texture2; //0x001C
			int32_t m_iWide; //0x0020
			int32_t m_iTall; //0x0024
			int32_t m_iInputWide; //0x0028
			int32_t m_iInputTall; //0x002C
			int32_t m_ID; //0x0030
			int32_t m_Flags; //0x0034
			void* m_pRegen; //0x0038
		}; //Size: 0x003C
	}

	// Functions reverse-engineered from Infra / its version of the Source engine.
	namespace functions {
		typedef enum { GLOBAL_OFF = 0, GLOBAL_ON = 1, GLOBAL_DEAD = 2 } GLOBALESTATE;

		typedef int(__cdecl* GlobalEntity_AddEntity_t)(const char* pGlobalname, const char* pMapName, GLOBALESTATE state);
		typedef void(__cdecl* GlobalEntity_SetCounter_t)(int globalIndex, int counter);
		typedef int(__cdecl* GlobalEntity_GetCounter_t)(int globalIndex);
		typedef int(__cdecl* GlobalEntity_AddToCounter_t)(int globalIndex, int count);
		typedef GLOBALESTATE(__cdecl* GlobalEntity_GetState_t)(int globalIndex);
		typedef void(__cdecl* GlobalEntity_SetState_t)(int globalIndex, GLOBALESTATE state);
		typedef void* (__cdecl* GetPlayerByIndex_t)(int a1);

		typedef int(__thiscall* KeyValues__GetInt_t)(void* thiz, char* key, int defVal);
	}
}