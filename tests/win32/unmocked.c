#include "glib-mock.h"

#include <stdio.h>

#include <windows.h>

static size_t (*real_my_fwrite) (const gpointer buf, size_t size, size_t count, FILE *file);

static void
test_unmocked (void)
{
  g_assert_nonnull (real_my_fwrite);

  const gchar *my_fwrite_path_utf8 = g_getenv ("LIB_MY_FWRITE_PATH");
  g_assert_nonnull (my_fwrite_path_utf8);

  WCHAR *my_fwrite_path_utf16 = g_utf8_to_utf16 (my_fwrite_path_utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (my_fwrite_path_utf16);

  HMODULE libmy_fwrite = LoadLibrary (my_fwrite_path_utf16);
  g_free (my_fwrite_path_utf16);
  g_assert_nonnull (libmy_fwrite);

  gpointer mock_my_fwrite = GetProcAddress (libmy_fwrite, "my_fwrite");
  g_assert_nonnull (mock_my_fwrite);

  g_assert_true (mock_my_fwrite == real_my_fwrite);

  g_assert_true (FreeLibrary (libmy_fwrite) != FALSE);
}

int
main (int argc, char **argv)
{
  g_mock_init (&argc, &argv);

  g_mock_get_real ("my_fwrite", &real_my_fwrite);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unmocked", test_unmocked);

  return g_test_run ();
}
