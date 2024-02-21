#include "stdafx.h"
#include "Logger.h"
#include "Utils.h"
#include "base.h"

static const char * legal_notice = {
	"\nBased of code x360ce - XBOX 360 Controller emulator\n"
    "https://code.google.com/p/x360ce/\n\n"
    "Copyright (C) 2014 Vicent Ahuir\n\n"
    "This program is free software you can redistribute it and/or modify it under\n"
    "the terms of the GNU Lesser General Public License as published by the Free\n"
    "Software Foundation, either version 3 of the License, or any later version.\n\n"
};

DWORD WINAPI MainThread(LPVOID lpThreadParameter)
{
	Base::Data::hModule = (HMODULE)lpThreadParameter;
	Base::Init();
	return TRUE;
}

DWORD WINAPI ExitThread(LPVOID lpThreadParameter)
{
	if (!Base::Data::Detached)
	{
		Base::Data::Detached = true;
		FreeLibraryAndExitThread(Base::Data::hModule, TRUE);
	}
	return TRUE;
}

extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
			CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
            break;

        case DLL_PROCESS_DETACH:
			if (!Base::Data::Detached)
				CreateThread(nullptr, 0, ExitThread, hModule, 0, nullptr);
            break;
    }

    return TRUE;
}