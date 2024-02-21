#pragma once
#include <stdafx.h>

namespace infra {
	// Various structures reverse-engineered from Infra / its version of the Source engine.
	namespace structs {
		// I made these with ReClass
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