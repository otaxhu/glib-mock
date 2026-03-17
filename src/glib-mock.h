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
#include <dlfcn.h>
#endif

G_BEGIN_DECLS

#if defined(G_PLATFORM_WIN32)

void
_g_mock_add_win32 (gpointer func, const gchar *func_name);

#define g_mock_add(func_name) \
  _g_mock_add_win32 ((func_name), # func_name)

G_NO_INLINE void
g_mock_commit (void);

#elif defined(G_OS_UNIX)

#define g_mock_add(func_name) ((void)(func_name)) /* No-op */

#define g_mock_commit() G_STMT_START {} G_STMT_END /* No-op */

#endif

#if defined(G_OS_UNIX)

void
_g_mock_abort (const gchar *msg, ...);

#define g_mock_get_real(func_name, out_real) \
  G_STMT_START \
  { \
    (void)(func_name); \
    gpointer *_out_real = (gpointer *)(out_real); \
    if (_out_real) \
      { \
        (void) dlerror (); /* Clear last error */ \
        *_out_real = dlsym (RTLD_NEXT, # func_name); \
        const gchar *error = dlerror (); \
        if (!*_out_real) \
          _g_mock_abort ("GLib-Mock: No real implementation found for <" \
                         # func_name \
                         "> mock function: dlerror returned: %s", \
                         error); \
      } \
  } \
  G_STMT_END

#elif defined(G_OS_WIN32)

G_NO_INLINE void
_g_mock_get_real_win32 (gpointer func, const gchar *func_name, gpointer *out_real);

#define g_mock_get_real(func_name, out_real) \
  _g_mock_get_real_win32 ((gpointer)(func_name), # func_name, (gpointer *)(out_real))

#else
#pragma message "This platform doesn't support mocks"
#endif

G_END_DECLS
