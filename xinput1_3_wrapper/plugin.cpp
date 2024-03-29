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
#include "plugin.h"

#include <cstring>
#include <iostream>

sdk::interface_reg_t* sdk::interface_reg_t::interface_regs = nullptr;

extern "C" __declspec(dllexport) void* CreateInterface(const char* name, int* return_code) {
	sdk::interface_reg_t* cur;

	for (cur = sdk::interface_reg_t::interface_regs; cur; cur = cur->next) {
		if (!std::strcmp(cur->name, name)) {
			if (return_code) {
				*return_code = 0;
			}
			return cur->create_fn();
		}
	}

	if (return_code) {
		*return_code = 1;
	}
	return nullptr;
}
