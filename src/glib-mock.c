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

#if defined(__APPLE__)

#include "glib-mock.h"
#include <unistd.h>
#include <mach-o/dyld.h>

#endif

#if defined(__APPLE__)

void
g_mock_init (int *argc, char ***argv)
{
  const gchar *flat_namespace = g_getenv ("DYLD_FORCE_FLAT_NAMESPACE");

  /* Early return if the program is already in flat namespace */
  if (flat_namespace != NULL)
    return;

  g_setenv ("DYLD_FORCE_FLAT_NAMESPACE", "1", TRUE);

  execv ((*argv)[0], *argv);

  g_error ("Couldn't re-exec the program");
}

#endif
