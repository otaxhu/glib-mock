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

#include "glib-mock-priv.h"

#include <dlfcn.h>

static gpointer (*real_dlopen) (const gchar *path, int flags);

gpointer
dlopen (const gchar *path, int flags)
{
  if (!real_dlopen)
    {
      real_dlopen = dlsym (RTLD_NEXT, "dlopen");
      if (!real_dlopen)
        g_error ("Couldn't get the real dlopen");
    }
#if defined(RTLD_NOLOAD)
  /* (optimization) Don't patch if the shared object it's already loaded,
    * because we already patched it and all of its dependencies recursively.
    */
  gpointer real_so = real_dlopen (path, flags | RTLD_NOLOAD);
  if (real_so)
    return real_so;
  real_so = real_dlopen (path, flags & ~RTLD_NOLOAD);
#else
  gpointer real_so = real_dlopen (path, flags);
#endif
  if (!real_so)
    return NULL;

  /* FIXME: Very expensive operation, mainly on programs that import a lot of shared objects. */
  _g_mock_patch_got_linux ();

  return real_so;
}
