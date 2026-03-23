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

#if defined(__APPLE__)
#include <unistd.h>
#endif

#if defined(G_PLATFORM_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

#if defined(G_OS_UNIX) && !defined(G_PLATFORM_WIN32)
#include <dlfcn.h>
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

void
g_mock_init (int *argc, char ***argv)
{
  g_return_if_fail (argc != NULL && *argc > 0);
  g_return_if_fail (argv != NULL);

#if defined(__APPLE__)
  const gchar *flat_namespace = g_getenv ("DYLD_FORCE_FLAT_NAMESPACE");

  /* Early return if the program is already in flat namespace */
  if (flat_namespace != NULL)
    return;

  g_setenv ("DYLD_FORCE_FLAT_NAMESPACE", "1", TRUE);

  execv ((*argv)[0], *argv);

  g_error ("Couldn't re-exec the program");
#endif
}

#if defined(G_PLATFORM_WIN32)
typedef struct
{
  gpointer func;
  const gchar *func_name;
  gboolean applied;
} GMockEntry;

static GArray *mock_entries = NULL;
#endif

static gboolean committed = FALSE;

void
g_mock_add_full (gpointer func, const gchar *func_name)
{
  if G_UNLIKELY (committed)
    g_error ("Unexpected call to g_mock_add after g_mock_commit has been called");

  g_return_if_fail (func != NULL);
  g_return_if_fail (func_name != NULL && func_name[0] != '\0');

#if defined(G_PLATFORM_WIN32)
  if G_UNLIKELY (!mock_entries)
    mock_entries = g_array_sized_new (FALSE, FALSE, sizeof (GMockEntry), 1);

  GMockEntry entry = {
    .func = func,
    .func_name = g_strdup (func_name),
  };

  g_array_append_val (mock_entries, entry);
#elif defined(G_OS_UNIX)
  /* Check directly in this function if the mock was succesfully applied.
   *
   * Same logic as in Win32 g_mock_commit, but without storing anything to an array
   * (optimization.)
   */

  gpointer sym = (gpointer) dlsym (RTLD_DEFAULT, func_name);
  if (!sym)
    g_error ("dlsym(RTLD_DEFAULT) failed: dlerror returned: %s", dlerror ());
  else if (sym != func)
    G_WARN_MOCK_UNAPPLIED (func_name);
#endif
}

#if defined(G_PLATFORM_WIN32)
static gpointer (WINAPI *real_GetProcAddress) (HMODULE module, gchar *func_name);
static gpointer WINAPI
mock_GetProcAddress (HMODULE module, gchar *func_name)
{
  gpointer real_func = real_GetProcAddress (module, func_name);
  if (!real_func)
    return NULL;

  for (guint i = 0; i < mock_entries->len; i++)
    {
      GMockEntry *entry = &g_array_index (mock_entries, GMockEntry, i);

      if (g_strcmp0 (entry->func_name, func_name) == 0)
        return entry->func;
    }

  return real_func;
}
#endif

void
g_mock_commit (void)
{
  if G_UNLIKELY (committed)
    return;

#if defined(G_PLATFORM_WIN32)
  if G_LIKELY (mock_entries)
    {
      g_mock_add_full (mock_GetProcAddress, "GetProcAddress");
      g_mock_get_real_full ((gpointer) mock_GetProcAddress, "GetProcAddress", (gpointer *) &real_GetProcAddress);
    }
#endif

  committed = TRUE;

#if defined(G_PLATFORM_WIN32)
  if G_UNLIKELY (!mock_entries)
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

                  for (guint i = 0; i < mock_entries->len; i++)
                    {
                      GMockEntry *entry = &g_array_index (mock_entries, GMockEntry, i);

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

  for (guint i = 0; i < mock_entries->len; i++)
    {
      GMockEntry *entry = &g_array_index (mock_entries, GMockEntry, i);

      if (!entry->applied)
        G_WARN_MOCK_UNAPPLIED (entry->func_name);
    }

  /* Leak mock_entries, due to it being used by mock_GetProcAddress */
  /* g_clear_pointer (&mock_entries, g_array_unref); */
#endif
}

#if defined(G_OS_WIN32)
G_NO_INLINE
#endif
void
g_mock_get_real_full (gpointer func,
                      const gchar *func_name,
                      gpointer *out_real)
{
  if G_UNLIKELY (committed)
    g_error ("Unexpected call to g_mock_get_real after g_mock_commit has been called");

  g_return_if_fail (func != NULL);
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

  g_error ("No real implementation found for <%s> mock function", func_name);

out:
  g_free (modules);
#endif
}
