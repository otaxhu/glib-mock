#include "glib-mock.h"

#include <dlfcn.h>

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

  gpointer libfoo = dlopen (g_getenv ("LIB_FOO_PATH"), RTLD_LAZY);
  g_assert_nonnull (libfoo);

  gpointer cur_foo = dlsym (libfoo, "foo");
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
