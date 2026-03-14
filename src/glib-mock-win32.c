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

#include "glib-mock.h"

#include <windows.h>
#include <psapi.h>

/* {{{ _ReturnAddress implementation copied from:
 *
 * https://github.com/dlfcn-win32/dlfcn-win32/blob/3e9bfa7/src/dlfcn.c#L60
 *
 * All the credits goes to the authors.
 */
#ifdef _MSC_VER
#if _MSC_VER >= 1000
/* https://docs.microsoft.com/en-us/cpp/intrinsics/returnaddress
 * When compiling in C++ mode, it is required to have C declaration for _ReturnAddress.
 */
#ifdef __cplusplus
extern "C" void *_ReturnAddress(void);
#endif
#pragma intrinsic(_ReturnAddress)
#else
/* On older version read return address from the value on stack pointer + 4 of
 * the caller. Caller stack pointer is stored in EBP register but only when
 * the EBP register is not optimized out. Usage of _alloca() function prevent
 * EBP register optimization. Read value of EBP + 4 via inline assembly. And
 * because inline assembly does not have a return value, put it into naked
 * function which does not have prologue and epilogue and preserve registers.
 * When compiling in C++ mode, it is required to have C declaration for _alloca.
 */
#ifdef __cplusplus
extern "C" void *__cdecl _alloca (size_t);
#endif
__declspec(naked) static void *_ReturnAddress (void) { __asm mov eax, [ebp+4] __asm ret }
#define _ReturnAddress() (_alloca (1), _ReturnAddress ())
#endif
#else
/* https://gcc.gnu.org/onlinedocs/gcc/Return-Address.html */
#ifndef _ReturnAddress
#define _ReturnAddress() (__builtin_extract_return_addr (__builtin_return_address (0)))
#endif
#endif

/* }}} */

/* We cannot inline because we require the real return address from the caller */
G_NO_INLINE void
_g_mock_add_win32(gpointer func, const gchar *func_name, gpointer *out_real)
{
  /* Nothing to do, user is allowed to pass NULL */
  if (!out_real)
    return;

  HMODULE exe_module, caller_module;

  /* Get the real implementation of the mock */

  exe_module = GetModuleHandle (NULL);
  g_assert (exe_module != NULL);

  g_assert (GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR) _ReturnAddress (),
                               &caller_module));

  g_assert (caller_module);

  DWORD modules_size, modules_size2;
  HANDLE process = GetCurrentProcess ();
  g_assert (process != NULL);

  g_assert (EnumProcessModules (process, NULL, 0, &modules_size));

  HMODULE *modules = (HMODULE *) g_malloc (modules_size);

  g_assert (EnumProcessModules (process, modules, modules_size, &modules_size2));
  g_assert (modules_size == modules_size2);

  for (int i = 0; i < modules_size / sizeof (HMODULE); i++)
    {
      HMODULE cur_module = modules[i];

      /* Mimick RTLD_NEXT dlsym functionality */
      if (cur_module == caller_module)
        continue;

      *out_real = (gpointer) GetProcAddress (cur_module, func_name);
      if (*out_real != NULL)
        {
          goto out;
        }
    }

  g_error ("No mock found for <%s> function", func_name);

out:
  g_free (modules);
}
