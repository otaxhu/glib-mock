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

#if defined(G_OS_UNIX)
#define __USE_GNU
#define _GNU_SOURCE
#include <dlfcn.h>
#endif

G_BEGIN_DECLS

#if !defined(G_PLATFORM_WIN32) && !defined(G_OS_UNIX)
#error "This platform doesn't support mocks"
#endif

void
g_mock_init (int *argc, char ***argv);

void
g_mock_add_full (gpointer func, const gchar *func_name);

#define g_mock_add(func_name) \
  g_mock_add_full ((gpointer) (func_name), G_STRINGIFY (func_name))

void
g_mock_commit (void);

gboolean
g_mock_is_committed (void);

#if defined(G_OS_WIN32)
G_NO_INLINE
#endif
void
g_mock_get_real (const gchar *func_name, gpointer *out_real);

#if defined(G_OS_UNIX)

void
_g_mock_create_dyn_promise (const gchar *func_name, gpointer *out_real);

#define g_mock_get_real(func_name, out_real) \
  G_STMT_START \
  { \
    if (g_mock_is_committed ()) \
      g_error ("Unexpected call to g_mock_get_real after g_mock_commit has been called"); \
    gpointer *_out_real = (gpointer *) (out_real); \
    const gchar *_func_name = (const gchar *) (func_name); \
    if (_out_real) \
      { \
        *_out_real = dlsym (RTLD_NEXT, _func_name); \
        if (!*_out_real) \
          _g_mock_create_dyn_promise (_func_name, _out_real); \
      } \
  } \
  G_STMT_END

#endif

G_END_DECLS
