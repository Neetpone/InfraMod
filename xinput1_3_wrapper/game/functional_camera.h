#pragma once
#include "infra.h"
#include "stdafx.h"

namespace mod {
	namespace functional_camera {
		void OnTakePicture(infra::structs::CInfraCameraFreezeFrame* freezeFrame);
		void EndScene(LPDIRECT3DDEVICE9 pDevice);
	}
}