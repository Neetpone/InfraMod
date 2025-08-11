#include "inventory.h"

#include "infra.h"
#include "shlwapi.h"

using infra::Engine;
using infra::CMathCounter;
using infra::CHud;
using infra::CHudElement;
using infra::CFlashlightAmmo;
using infra::CCameraAmmo;
using infra::CINFRA_Player;

float *mod::inventory::osCoinsCounter = nullptr;
int* mod::inventory::flashlightBatteriesCounter = nullptr;
int* mod::inventory::cameraBatteriesCounter = nullptr;

static bool FindCameraAndFlashlightCounters(int**ppFlashlightCount, int**ppCameraBatteriesCount) {
	CINFRA_Player* player = reinterpret_cast<CINFRA_Player*>(
		Engine()->CGlobalEntityList__FindEntityByName(nullptr, "!player")
	);

	if (player == nullptr) {
		return false;
	}

	*ppFlashlightCount = &(player->m_nFlashlightBatteries);
	*ppCameraBatteriesCount = &(player->m_nCameraBatteries);

	return false;
}

void mod::inventory::MapLoaded(const char *name) {
	osCoinsCounter = nullptr;
	flashlightBatteriesCounter = nullptr;
	cameraBatteriesCounter = nullptr;

	FindCameraAndFlashlightCounters(&flashlightBatteriesCounter, &cameraBatteriesCounter);

	if (StrStrA(name, "tenements") != nullptr) {
		CMathCounter* mathCounter = reinterpret_cast<CMathCounter*>(
			Engine()->CGlobalEntityList__FindEntityByName(nullptr, "Opensewer_coins_counter")
		);

		if (mathCounter != nullptr) {
			osCoinsCounter = &mathCounter->m_CounterValue;
		}
	}
}
