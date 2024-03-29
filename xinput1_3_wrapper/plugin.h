/*
 * MIT License
 *
 * Copyright (c) 2023 Zyntex
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://github.com/hero622/photon/
 */
#pragma once

#define interfaceversion_iserverplugincallbacks "ISERVERPLUGINCALLBACKS002"

namespace sdk {
	typedef void* (*create_interface_fn)(const char* name, int* return_code);
	typedef void* (*instantiate_interface_fn)();

	struct interface_reg_t {
		instantiate_interface_fn create_fn;
		const char* name;
		interface_reg_t* next;
		static interface_reg_t* interface_regs;

		interface_reg_t(instantiate_interface_fn fn, const char* name)
			: name(name) {
			create_fn = fn;
			next = interface_regs;
			interface_regs = this;
		}
	};

	class i_server_plugin_callbacks {
	public:
		virtual bool load(create_interface_fn interface_factory, create_interface_fn game_server_factory) = 0;
		virtual void unload() = 0;
		virtual void pause() = 0;
		virtual void un_pause() = 0;
		virtual const char* get_plugin_description() = 0;
		virtual void level_init(char const* map_name) = 0;
		virtual void server_activate(void* edict_list, int edict_count, int client_max) = 0;
		virtual void game_frame(bool simulating) = 0;
		virtual void level_shutdown() = 0;
		virtual void client_fully_connect(void* edict) = 0;
		virtual void client_active(void* entity) = 0;
		virtual void client_disconnect(void* entity) = 0;
		virtual void client_put_in_server(void* entity, char const* playername) = 0;
		virtual void set_command_client(int index) = 0;
		virtual void client_settings_changed(void* edict) = 0;
		virtual int client_connect(bool* allow_connect, void* entity, const char* name, const char* address, char* reject, int maxrejectlen) = 0;
		virtual int client_command(void* entity, const void*& args) = 0;
		virtual int network_id_validated(const char* user_name, const char* network_id) = 0;
		virtual void on_query_cvar_value_finished(int cookie, void* player_entity, int status, const char* cvar_name, const char* cvar_value) = 0;
		virtual void on_edict_allocated(void* edict) = 0;
		virtual void on_edict_freed(const void* edict) = 0;
	};

	struct c_plugin {
		char name[128];
		bool disable;
		i_server_plugin_callbacks* plugin;
		int plugin_interface_version;
		void* plugin_module;
	};
}  // namespace sdk

#define expose_single_interface_globalvar( class_name, interface_name, version_name, global_var_name ) \
	static void *__create##class_name##interface_name##_interface( ) {                                    \
		return static_cast<sdk::interface_name *>( &global_var_name );                                       \
	}                                                                                                     \
	static sdk::interface_reg_t __g_create##class_name##interface_name##_reg( __create##class_name##interface_name##_interface, version_name );