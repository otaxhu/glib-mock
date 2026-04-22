/* This tests if the framework can get multiple non-available at load-time functions,
 * and resolve it succesfully after they get loaded by LoadLibrary/GetProcAddress.
 */

#include "glib-mock.h"

#include <windows.h>

static const char (*real_foo) (void);
static const char (*real_bar) (void);

static void
test_multiple_get_real_rt_dynamic (void)
{
  g_assert_nonnull (real_foo);
  g_assert_nonnull (real_bar);

  /* Both must be equal to the "not found" function which is returned when g_mock_get_real
   * fails to get a function.
   */
  g_assert_true (real_foo == real_bar);

  gpointer prev_real = real_foo;

  /* Load libfoo */

  const char *libfoo_path_utf8 = g_getenv ("LIB_FOO_PATH");
  g_assert_nonnull (libfoo_path_utf8);

  WCHAR *libfoo_path_utf16 = g_utf8_to_utf16 (libfoo_path_utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (libfoo_path_utf16);

  HMODULE libfoo = LoadLibrary (libfoo_path_utf16);
  g_free (libfoo_path_utf16);
  g_assert_nonnull (libfoo);

  gpointer cur_foo = GetProcAddress (libfoo, "foo");
  g_assert_nonnull (cur_foo);

  g_assert_true (cur_foo == (gpointer) real_foo);
  g_assert_true (prev_real != cur_foo);

  /* Load libbar */

  const char *libbar_path_utf8 = g_getenv ("LIB_BAR_PATH");
  g_assert_nonnull (libbar_path_utf8);

  WCHAR *libbar_path_utf16 = g_utf8_to_utf16 (libbar_path_utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (libbar_path_utf16);

  HMODULE libbar = LoadLibrary (libbar_path_utf16);
  g_free (libbar_path_utf16);
  g_assert_nonnull (libbar);

  gpointer cur_bar = GetProcAddress (libbar, "bar");
  g_assert_nonnull (cur_bar);

  g_assert_true (cur_bar == (gpointer) real_bar);
  g_assert_true (prev_real != cur_bar);
}

int
main (int argc, char **argv)
{
  g_mock_init (&argc, &argv);

  g_mock_get_real ("foo", &real_foo);
  g_mock_get_real ("bar", &real_bar);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/multiple-get-real-rt-dynamic", test_multiple_get_real_rt_dynamic);

  return g_test_run ();
}
