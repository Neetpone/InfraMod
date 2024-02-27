#include "inventory.h"

#include "infra.h"
#include "shlwapi.h"

using infra::Engine;
using infra::CMathCounter;

float *mod::inventory::osCoinsCounter = nullptr;

void mod::inventory::MapLoaded(const char *name) {
	osCoinsCounter = nullptr;

	if (StrStrA(name, "tenements") != nullptr) {
		CMathCounter* mathCounter = reinterpret_cast<CMathCounter*>(
			Engine()->CGlobalEntityList__FindEntityByName(nullptr, "Opensewer_coins_counter")
		);

		if (mathCounter != nullptr) {
			osCoinsCounter = &mathCounter->m_CounterValue;
		}
	}
}
