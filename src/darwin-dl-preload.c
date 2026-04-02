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

/* Entrypoint written in asm for dlsym_preload */
__attribute__ ((naked))
static gpointer
dlsym_preload_asm (gpointer handle, const gchar *name)
{
  /* FIXME: Add aarch64 support */
  __asm__ (
    "cmpq $-1, %rdi\n\t"
    "jne _dlsym_preload\n\t" /* darwin BS :/, the symbols must be prefixed with underscore */

    /* handle is RTLD_NEXT, we need to return the real dlsym if name is the correct one.
     *
     * NOTE: We do that on dlsym_preload, for the sake of simpler assembly code.
     */

    /* Check if name is "dlsym" string */

    /* Prologue */

    "pushq %rbp\n\t"
    "movq %rsp, %rbp\n\t"
    "pushq %rdi\n\t"
    "pushq %rsi\n\t"

    "leaq .L_dlsym_str(%rip), %rdi\n\t"
    "call _g_strcmp0\n\t"

    "popq %rsi\n\t"
    "popq %rdi\n\t"
    "popq %rbp\n\t"

    "testl %eax, %eax\n\t"
    "jz _dlsym_preload\n\t" /* name It's "dlsym" */
    "jmp _dlsym\n\t"        /* name Isn't "dlsym" */

    ".section __TEXT,__cstring,cstring_literals\n\t"
    ".L_dlsym_str:\n\t"
    ".asciz \"dlsym\"\n\t"
    ".section __TEXT,__text,regular,pure_instructions\n\t"
  );
}

__attribute__ ((used))
static gpointer
dlsym_preload (gpointer handle, const gchar *name)
{
  /* We only mock functions from dlopen handles, everything else is bypassed to
   * dlsym (the real one), which should already been handled by the assembly
   * entrypoint above. */

  /* POSIX says that RTLD_DEFAULT handle returns the same no matter who called it.
   *
   * > The symbol lookup happens in the normal global scope; that is, a search for
   * > a symbol using this handle would find the same definition as a direct use of
   * > this symbol in the program code.
   *
   * https://pubs.opengroup.org/onlinepubs/009696799/functions/dlsym.html
   */
  if (handle == RTLD_DEFAULT)
    return dlsym (handle, name);

  if (handle == RTLD_NEXT)
    return (gpointer) &dlsym;

  /* It's a dlopen handle */

  gpointer real_func = dlsym (handle, name);
  if (!real_func)
    return NULL;

  _g_mock_dyn_promise_resolve (name, real_func);

  gpointer mock_func = _g_mock_entry_find_by_name (name);
  if (mock_func)
    return mock_func;

  return real_func;
}

/* See: https://gitlab.gnome.org/GNOME/glib/-/blob/798edf/glib/tests/getpwuid-preload.c#L44 */
__attribute__ ((used))
static struct
{
  const gpointer replacement;
  const gpointer replacee;
} _interpose_dlsym __attribute__ ((section ("__DATA,__interpose"))) = {
  (const gpointer) &dlsym_preload_asm,
  (const gpointer) &dlsym,
};
