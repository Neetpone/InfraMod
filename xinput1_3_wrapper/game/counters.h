#pragma once
#include "stdafx.h"
#include "infra.h"
namespace mod {
	namespace counters {
		void InitMapStats();
		void StatSuccess(int event_type, int count, bool is_new);
	}
}