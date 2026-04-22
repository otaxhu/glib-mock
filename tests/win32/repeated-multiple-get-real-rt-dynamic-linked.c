#include "glib-mock.h"

#include <windows.h>

static const char * (*real_foo1) (void);
static const char * (*real_foo2) (void);

static void
test_repeated_multiple_get_real_rt_dynamic_linked (void)
{
  g_assert_nonnull (real_foo1);
  g_assert_nonnull (real_foo2);

  /* Both must be equal to the "not found" function which is returned when g_mock_get_real
   * fails to get a function.
   */
  g_assert_true (real_foo1 == real_foo2);

  gpointer prev_real_foo1 = real_foo1;
  gpointer prev_real_foo2 = real_foo2;

  const char *libfoo_path_utf8 = g_getenv ("LIB_FOO_PATH");
  g_assert_nonnull (libfoo_path_utf8);

  WCHAR *libfoo_path_utf16 = g_utf8_to_utf16 (libfoo_path_utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (libfoo_path_utf8);

  HMODULE libfoo = LoadLibrary (libfoo_path_utf16);
  g_free (libfoo_path_utf16);
  g_assert_nonnull (libfoo);

  gpointer cur_foo = GetProcAddress (libfoo, "foo");
  g_assert_nonnull (cur_foo);

  g_assert_true (prev_real_foo1 != real_foo1);
  g_assert_true (prev_real_foo2 != real_foo2);
  g_assert_true (real_foo1 == cur_foo);
  g_assert_true (real_foo2 == cur_foo);
}

int
main (int argc, char **argv)
{
  g_mock_init (&argc, &argv);

  g_mock_get_real ("foo", &real_foo1);
  g_mock_get_real ("foo", &real_foo2);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/repeated-multiple-get-real-rt-dynamic-linked",
                   test_repeated_multiple_get_real_rt_dynamic_linked);

  return g_test_run ();
}
