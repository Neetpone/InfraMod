#include "mod.h"
#include "stdafx.h"
#include "Utils.h"
#include "base.h"
#include <Windows.h>

mod g_Mod;

expose_single_interface_globalvar(mod, i_server_plugin_callbacks, interfaceversion_iserverplugincallbacks, g_Mod);


static HMODULE GetCurrentModule() {
	HMODULE hModule = NULL;

	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCTSTR)GetCurrentModule,
		&hModule
	);

	return hModule;
}

mod::mod() {
}

bool mod::load(sdk::create_interface_fn interface_factory, sdk::create_interface_fn game_server_factory) {


	//AllocConsole();

	//freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(0)), "conin$", "r", static_cast<_iobuf*>(__acrt_iob_func(0)));
	//freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(1)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(1)));
	//freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(2)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(2)));
	
	return true;
}


void mod::unload() {
	// Probably want to do Base::Data::Detached = true;
	Base::Hooks::Shutdown();
}

void mod::pause() {}
void mod::un_pause() {}
const char* mod::get_plugin_description() {
	return "floorb_infra_mod";
}

void mod::level_init(char const* map_name) {
	if (!Base::Data::Inited) {
		Base::Data::hModule = GetCurrentModule();
		Base::Init();
		Base::Data::Inited = true;
	}
}

void mod::server_activate(void* edict_list, int edict_count, int client_max) {}
void mod::game_frame(bool simulating) {}
void mod::level_shutdown() {}
void mod::client_fully_connect(void* edict) {}
void mod::client_active(void* entity) {}
void mod::client_disconnect(void* entity) {}
void mod::client_put_in_server(void* entity, char const* playername) {}
void mod::set_command_client(int index) {}
void mod::client_settings_changed(void* edict) {}
int mod::client_connect(bool* allow_connect, void* entity, const char* name, const char* address, char* reject, int maxrejectlen) {
	return 0;
}
int mod::client_command(void* entity, const void*& args) {
	return 0;
}
int mod::network_id_validated(const char* user_name, const char* network_id) {
	return 0;
}
void mod::on_query_cvar_value_finished(int cookie, void* player_entity, int status, const char* cvar_name, const char* cvar_value) {}
void mod::on_edict_allocated(void* edict) {}
void mod::on_edict_freed(const void* edict) {}