#pragma once
#include "stdafx.h"

namespace mod {
	namespace inventory {
		extern float *osCoinsCounter;
		extern int* flashlightBatteriesCounter;
		extern int* cameraBatteriesCounter;

		void MapLoaded(const char *name);
	}
}