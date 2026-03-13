/*
 * Copyright (C) 2026 Oscar Pernia <oscarperniamoreno@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib-2.0/glib.h>

#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

G_BEGIN_DECLS

#if defined(__linux__) || defined(__APPLE__)

#define g_mock_add(func_name) ((void)(func_name)) /* No-op */

#define g_mock_add_with_real(func_name, out_real) \
  ((void)(*(out_real) = dlsym (RTLD_NEXT, # func_name)))

/* TODO: Win32 mocking using IAT hooking */

#else
#message "This platform doesn't support mocks"
#endif

G_END_DECLS
