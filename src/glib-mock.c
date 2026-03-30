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

#include "config.h"

#include "glib-mock.h"

#include "glib-mock-priv.h"

#if defined(__linux__)
#include <link.h>
#include <elf.h>
#include <sys/mman.h>
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

#if defined(G_PLATFORM_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

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

#define G_WIN32_API_FAILED(func_name) \
  g_error ("Unexpected error from Win32 API during <" \
           # func_name \
           "> call: '%ld' error code", \
           GetLastError ())

#define G_WIN32_API_FAILED_NO_CODE(func_name) \
  g_error ("Unexpected error from Win32 API during <" \
           # func_name \
           "> call")

#if defined(G_PLATFORM_WIN32)
#define G_WARN_MOCK_UNAPPLIED(func_name) \
  g_warning ("Mock <%s> not found in any of the loaded IATs. If using LoadLibrary() " \
             "+ GetProcAddress(), safely ignore this warning: the mock will be returned when " \
             "GetProcAddress() is called with the correct name.", \
             func_name)
#else
#define G_WARN_MOCK_UNAPPLIED(func_name) \
  g_warning ("Mock <%s> couldn't be applied, you may not see it working correctly", \
             func_name)
#endif

static void
mock_dyn_promise_element_clear (GMockDynPromise *promise)
{
  g_free (promise->func_name);
}

static gboolean committed = FALSE;

gboolean
g_mock_is_committed (void)
{
  return committed;
}

void
g_mock_init (int *argc, char ***argv)
{
  g_return_if_fail (argc != NULL && *argc > 0);
  g_return_if_fail (argv != NULL);

#if defined(__APPLE__) || defined(__linux__)
  gboolean needs_reexec = FALSE;
#endif

#if defined(__APPLE__)
  /* Check flat namespace */

  const gchar *flat_namespace = g_getenv ("DYLD_FORCE_FLAT_NAMESPACE");

  if (flat_namespace == NULL || flat_namespace[0] != '1')
    {
      needs_reexec = TRUE;
      g_setenv ("DYLD_FORCE_FLAT_NAMESPACE", "1", TRUE);
    }
#endif

#if defined(__APPLE__)
#define PRELOAD_ENV "DYLD_INSERT_LIBRARIES"
#elif defined(__linux__)
#define PRELOAD_ENV "LD_PRELOAD"
#endif

#if defined(__APPLE__) || defined(__linux__)
  const gchar *preload_libs = g_getenv (PRELOAD_ENV);
  gchar *new_preload_libs = NULL;

  if (preload_libs)
    {
      /* Check if preload_libs contains G_MOCK_PRELOAD_PATH at prefix */

      gboolean needs_prepend_lib = FALSE;

      if (strncmp (preload_libs,
                   G_MOCK_DL_PRELOAD_PATH,
                   sizeof (G_MOCK_DL_PRELOAD_PATH) - 1) != 0)
        {
          needs_prepend_lib = TRUE;
        }
      else
        {
          gchar last_char = preload_libs[sizeof (G_MOCK_DL_PRELOAD_PATH) - 1];
          if (last_char != ':' && last_char != '\0')
            needs_prepend_lib = TRUE;
        }

      if (needs_prepend_lib)
        {
          needs_reexec = TRUE;

          size_t new_size = strlen (preload_libs) + sizeof (G_MOCK_DL_PRELOAD_PATH) + 1;

          new_preload_libs = (gchar *) g_malloc (new_size);

          memcpy (new_preload_libs,
                  G_MOCK_DL_PRELOAD_PATH,
                  sizeof (G_MOCK_DL_PRELOAD_PATH) - 1);
          new_preload_libs[sizeof (G_MOCK_DL_PRELOAD_PATH) - 1] = ':';
          memcpy (new_preload_libs + sizeof (G_MOCK_DL_PRELOAD_PATH),
                  preload_libs,
                  new_size - sizeof (G_MOCK_DL_PRELOAD_PATH));
        }
    }
  else
    {
      needs_reexec = TRUE;
      new_preload_libs = G_MOCK_DL_PRELOAD_PATH;
    }

  if (new_preload_libs)
    {
      g_setenv (PRELOAD_ENV, new_preload_libs, TRUE);
      if (preload_libs)
        g_free (new_preload_libs);
    }
#endif

#if defined(__linux__)
  /* Check LD_BIND_NOW */

  const gchar *bind_now = g_getenv ("LD_BIND_NOW");

  if (bind_now == NULL || bind_now[0] != '1')
    {
      needs_reexec = TRUE;
      g_setenv ("LD_BIND_NOW", "1", TRUE);
    }
#endif

#if defined(__APPLE__) || defined(__linux__)
  if (needs_reexec)
    {
      execv ((*argv)[0], *argv);

      g_error ("Couldn't re-exec the program");
    }
#endif
}

void
g_mock_add_full (gpointer func, const gchar *func_name)
{
  if G_UNLIKELY (g_mock_is_committed ())
    g_error ("Unexpected call to g_mock_add after g_mock_commit has been called");

  g_return_if_fail (func != NULL);
  g_return_if_fail (func_name != NULL && func_name[0] != '\0');

  if G_UNLIKELY (!_g_mock_entries)
    _g_mock_entries = g_array_sized_new (FALSE, FALSE, sizeof (GMockEntry), 1);

  GMockEntry entry = {
    .func = func,
    .func_name = g_strdup (func_name),
  };

  g_array_append_val (_g_mock_entries, entry);
}

#if defined(G_PLATFORM_WIN32)
static gpointer (WINAPI *real_GetProcAddress) (HMODULE module, gchar *func_name);
static gpointer WINAPI
mock_GetProcAddress (HMODULE module, gchar *func_name)
{
  gpointer real_func = real_GetProcAddress (module, func_name);
  if (!real_func || G_UNLIKELY (!_g_mock_entries))
    return real_func;

  for (guint i = 0; i < _g_mock_entries->len; i++)
    {
      GMockEntry *entry = &g_array_index (_g_mock_entries, GMockEntry, i);

      if (g_strcmp0 (entry->func_name, func_name) == 0)
        return entry->func;
    }

  return real_func;
}
#endif

void
g_mock_commit (void)
{
  if G_UNLIKELY (g_mock_is_committed ())
    return;

#if defined(G_PLATFORM_WIN32)
  if G_LIKELY (_g_mock_entries)
    {
      g_mock_add_full (mock_GetProcAddress, "GetProcAddress");
      g_mock_get_real ("GetProcAddress", (gpointer *) &real_GetProcAddress);
    }
#endif

  committed = TRUE;

#if defined(G_PLATFORM_WIN32)
  if G_UNLIKELY (!_g_mock_entries)
    return;

  DWORD modules_size, modules_size2;
  HANDLE process = GetCurrentProcess ();
  if (!process)
    G_WIN32_API_FAILED (GetCurrentProcess);

  if (!EnumProcessModules (process, NULL, 0, &modules_size))
    G_WIN32_API_FAILED (EnumProcessModules);

  HMODULE *modules = (HMODULE *) g_malloc (modules_size);

  if (!EnumProcessModules (process, modules, modules_size, &modules_size2))
    G_WIN32_API_FAILED (EnumProcessModules);

  if (modules_size != modules_size2)
    g_error ("EnumProcessModules returned different sizes of modules on the second call");

  for (int i = 0; i < modules_size / sizeof (HMODULE); i++)
    {
      guintptr base_addr = (guintptr) modules[i];

      IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *) base_addr;

      IMAGE_NT_HEADERS *nt_headers = (IMAGE_NT_HEADERS *) (base_addr + dos_header->e_lfanew);
      IMAGE_DATA_DIRECTORY import_dir = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
      if (import_dir.VirtualAddress == 0)
        continue;

      IMAGE_IMPORT_DESCRIPTOR *import_desc = (IMAGE_IMPORT_DESCRIPTOR *) (base_addr + import_dir.VirtualAddress);

      while (import_desc->Name)
        {
          IMAGE_THUNK_DATA *name_thunk = (IMAGE_THUNK_DATA *) (base_addr + import_desc->OriginalFirstThunk);
          IMAGE_THUNK_DATA *addr_thunk = (IMAGE_THUNK_DATA *) (base_addr + import_desc->FirstThunk);

          while (name_thunk->u1.AddressOfData)
            {
              /* We can't hook into ordinal imports */
              if (!(name_thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG))
                {
                  IMAGE_IMPORT_BY_NAME *import_by_name = (IMAGE_IMPORT_BY_NAME *) (base_addr + name_thunk->u1.AddressOfData);
                  const gchar *imp_name = (const gchar *) import_by_name->Name;

                  for (guint i = 0; i < _g_mock_entries->len; i++)
                    {
                      GMockEntry *entry = &g_array_index (_g_mock_entries, GMockEntry, i);

                      if (g_strcmp0 (imp_name, entry->func_name) == 0)
                        {
                          DWORD old_protect;
                          if (!VirtualProtect (&addr_thunk->u1.Function, sizeof (gpointer), PAGE_READWRITE, &old_protect))
                            G_WIN32_API_FAILED (VirtualProtect);

                          addr_thunk->u1.Function = (ULONG_PTR) entry->func;
                          VirtualProtect (&addr_thunk->u1.Function, sizeof (gpointer), old_protect, &old_protect);

                          entry->applied = TRUE;
                        }
                    }
                }
              name_thunk++;
              addr_thunk++;
            }
          import_desc++;
        }
    }

  /* Check mocks were applied */

  for (guint i = 0; i < _g_mock_entries->len; i++)
    {
      GMockEntry *entry = &g_array_index (_g_mock_entries, GMockEntry, i);

      if (!entry->applied)
        G_WARN_MOCK_UNAPPLIED (entry->func_name);
    }

  /* Leak _g_mock_entries, due to it being used by mock_GetProcAddress
   * and mock_dlsym.
   */
  /* g_clear_pointer (&_g_mock_entries, g_array_unref); */
#elif defined(__linux__)
  _g_mock_patch_got_linux ();
#endif
}

G_GNUC_NORETURN
static void
real_not_found_dynamic (void)
{
  /* This function prototype is compatible with both cdecl and stdcall
   * call-convs, since it will never return to the caller when called.
   *
   * I "think" it's compatible with almost every call-conv, but take it FWIW.
   */
#if defined(G_PLATFORM_WIN32)
  g_error ("Attempted to call a function whose real implementation could not be resolved. "
           "Check the testing target's calls to LoadLibrary() to see if they are working correctly.");
#else
  g_error ("Attempted to call a function whose real implementation could not be resolved. "
           "Check the testing target's calls to dlopen() to see if they are working correctly.");
#endif
}

#if defined(G_OS_WIN32)
static
#endif
void
_g_mock_create_dyn_promise (const gchar *func_name, gpointer *out_real)
{
  *out_real = real_not_found_dynamic;

  if G_UNLIKELY (!_g_mock_dyn_promises)
    {
      _g_mock_dyn_promises = g_array_sized_new (FALSE, FALSE, sizeof (GMockDynPromise), 1);
      g_array_set_clear_func (_g_mock_dyn_promises,
                              (GDestroyNotify) mock_dyn_promise_element_clear);
    }

  GMockDynPromise new_promise = {
    .func_name = g_strdup (func_name),
    .user_out_real = out_real,
  };
  g_array_append_val (_g_mock_dyn_promises, new_promise);
}

#if defined(G_OS_WIN32)
G_NO_INLINE
#endif
void
(g_mock_get_real) (const gchar *func_name, gpointer *out_real)
{
  if G_UNLIKELY (g_mock_is_committed ())
    g_error ("Unexpected call to g_mock_get_real after g_mock_commit has been called");

  g_return_if_fail (func_name != NULL && func_name[0] != '\0');
  g_return_if_fail (out_real != NULL);

#if defined(G_OS_UNIX)
  g_error ("You must call g_mock_get_real macro instead of this function");
#elif defined(G_OS_WIN32)

  HMODULE caller_module;

  /* Get the real implementation of the mock */

  if (!GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (gpointer) _ReturnAddress (),
                          &caller_module))
    G_WIN32_API_FAILED (GetModuleHandleEx);

  DWORD modules_size, modules_size2;
  HANDLE process = GetCurrentProcess ();
  if (!process)
    G_WIN32_API_FAILED (GetCurrentProcess);

  if (!EnumProcessModules (process, NULL, 0, &modules_size))
    G_WIN32_API_FAILED (EnumProcessModules);

  HMODULE *modules = (HMODULE *) g_malloc (modules_size);

  if (!EnumProcessModules (process, modules, modules_size, &modules_size2))
    G_WIN32_API_FAILED (EnumProcessModules);

  if (modules_size != modules_size2)
    g_error ("EnumProcessModules returned different sizes of modules on the second call");

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

  /* Assume the function will be loaded at runtime later.
   *
   * Save a promise object.
   */
  _g_mock_create_dyn_promise (func_name, out_real);

out:
  g_free (modules);
#endif
}
