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

#if defined(__linux__)
#define _GNU_SOURCE
#define __USE_GNU
#include <link.h>
#include <elf.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

GArray *_g_mock_entries;
GArray *_g_mock_dyn_promises;

void
_g_mock_dyn_promise_resolve (const gchar *func_name, gpointer real_func)
{
  g_return_if_fail (func_name != NULL);
  g_return_if_fail (real_func != NULL);

  GMockDynPromise promise = {
    .func_name = (gchar *) func_name,
  };
  guint out_index;

  if (_g_mock_dyn_promises &&
      g_array_binary_search (_g_mock_dyn_promises,
                             &promise,
                             (GCompareFunc) _g_mock_dyn_promises_sort_func,
                             &out_index))
    {
      /* Find first occurance */
      for (; out_index > 0; out_index--)
        {
          GMockDynPromise *prev = &g_array_index (_g_mock_dyn_promises, GMockDynPromise, out_index - 1);
          if (_g_mock_dyn_promises_sort_func (prev, &promise) != 0)
            break;
        }

      /* Iterate from first occurance onwards, up until it finds a different element. */
      while (out_index < _g_mock_dyn_promises->len)
        {
          GMockDynPromise *found = &g_array_index (_g_mock_dyn_promises, GMockDynPromise, out_index);
          if (_g_mock_dyn_promises_sort_func (found, &promise) != 0)
            break;

          *found->user_out_real = real_func;
          g_array_remove_index (_g_mock_dyn_promises, out_index);
        }

      if (_g_mock_dyn_promises->len == 0)
        {
          g_clear_pointer (&_g_mock_dyn_promises, g_array_unref);
        }
    }
}

int
_g_mock_entries_sort_func (const GMockEntry *a, const GMockEntry *b)
{
  return g_strcmp0 (a->func_name, b->func_name);
}

int
_g_mock_dyn_promises_sort_func (const GMockDynPromise *a, const GMockDynPromise *b)
{
  return g_strcmp0 (a->func_name, b->func_name);
}

gpointer
_g_mock_entry_find_by_name (const gchar *func_name)
{
  g_return_val_if_fail (func_name != NULL, NULL);

  GMockEntry entry = {
    .func_name = (gchar *) func_name,
  };
  guint out_index;

  if (G_LIKELY (_g_mock_entries) &&
      g_array_binary_search (_g_mock_entries,
                             &entry,
                             (GCompareFunc) _g_mock_entries_sort_func,
                             &out_index))
    {
      return (&g_array_index (_g_mock_entries, GMockEntry, out_index))->func;
    }

  return NULL;
}

#if defined(__linux__)
gpointer (*_g_mock_real_dlsym) (gpointer handle, const gchar *name);

/* Entrypoint written in asm for mock_dlsym. */
__attribute__((naked)) static gpointer
mock_dlsym_asm (gpointer handle, const gchar *name)
{
  __asm__ (
      "cmpq $-1, %rdi\n\t"
      "jne mock_dlsym\n\t"
      "movq _g_mock_real_dlsym@GOTPCREL(%rip), %rax\n\t"
      "jmp *(%rax)\n\t"
  );
}

__attribute__((used)) static gpointer
mock_dlsym (gpointer handle, const gchar *name)
{
  /* We only mock functions from dlopen handles, everything else is bypassed to
   * real_dlsym, which should already been handled by the assembly entrypoint above. */

  /* POSIX says that RTLD_DEFAULT handle returns the same no matter who called it.
   *
   * > The symbol lookup happens in the normal global scope; that is, a search for
   * > a symbol using this handle would find the same definition as a direct use of
   * > this symbol in the program code.
   *
   * https://pubs.opengroup.org/onlinepubs/009696799/functions/dlsym.html
   */
  if (handle == RTLD_DEFAULT)
    return _g_mock_real_dlsym (handle, name);

  /* It must've already been handled by mock_dlsym_asm */
  g_assert (handle != RTLD_NEXT);

  /* It's a dlopen handle */

  gpointer real_func = _g_mock_real_dlsym (handle, name);
  if (!real_func)
    return NULL;

  _g_mock_dyn_promise_resolve (name, real_func);

  gpointer mock_func = _g_mock_entry_find_by_name (name);
  if (mock_func)
    return mock_func;

  return real_func;
}

static int
phdr_patch_cb (struct dl_phdr_info *info, size_t size, gpointer data)
{
  Elf64_Dyn *dyn = NULL;
  Elf64_Rela *rela = NULL;
  Elf64_Sym *symtab = NULL;
  gchar *strtab = NULL;
  size_t plt_rel_sz = 0;

  for (size_t i = 0; i < info->dlpi_phnum; i++)
    {
      if (info->dlpi_phdr[i].p_type == PT_DYNAMIC)
        {
          dyn = (Elf64_Dyn *) (info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
          break;
        }
    }

  if (!dyn)
    return 0;

  for (; dyn->d_tag != DT_NULL; dyn++)
    {
      switch (dyn->d_tag)
        {
        case DT_JMPREL:
          {
            rela = (Elf64_Rela *) (dyn->d_un.d_ptr);
            break;
          }
        case DT_SYMTAB:
          {
            symtab = (Elf64_Sym *) (dyn->d_un.d_ptr);
            break;
          }
        case DT_STRTAB:
          {
            strtab = (gchar *) (dyn->d_un.d_ptr);
            break;
          }
        case DT_PLTRELSZ:
          {
            plt_rel_sz = dyn->d_un.d_val;
            break;
          }
        }
    }

  if (!rela || !symtab || !strtab)
    return 0;

  size_t n_relocs = plt_rel_sz / sizeof (Elf64_Rela);
  for (size_t i = 0; i < n_relocs; i++)
    {
      size_t sym_idx = ELF64_R_SYM (rela[i].r_info);
      gchar *symbol_name = strtab + symtab[sym_idx].st_name;

      static int page_size;

      if (page_size == 0)
        {
          page_size = sysconf (_SC_PAGESIZE);
          if (page_size <= 0)
            g_error ("Couldn't get the page size");
        }

      gpointer *got = (gpointer *) (info->dlpi_addr + rela[i].r_offset);

      if (g_strcmp0 (symbol_name, "dlsym") == 0)
        {
          /* dlsym patching occurs here! */

          if (mprotect ((gpointer) (((guintptr) got) & ~(page_size - 1)),
                        page_size,
                        PROT_READ | PROT_WRITE))
            {
              g_error ("Failed to hook dlsym");
            }

          *got = mock_dlsym_asm;

          /* FIXME: Recover the original mprotect flags */
          /*
          mprotect ((gpointer) (((guintptr) got) & ~(page_size - 1)),
                    page_size,
                    PROT_READ);
          */
        }
    }

  return 0;
}

void
_g_mock_patch_got_linux (void)
{
  dl_iterate_phdr (phdr_patch_cb, NULL);
}
#endif
