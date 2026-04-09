#include "glib-mock.h"

#include <stdio.h>
#include <dlfcn.h>

static size_t (*real_my_fwrite) (const gpointer buf, size_t size, size_t count, FILE *file);

static void
test_unmocked (void)
{
  g_assert_nonnull (real_my_fwrite);

  gpointer mock_my_fwrite = dlsym (RTLD_DEFAULT, "my_fwrite");
  g_assert_null (mock_my_fwrite);

  gpointer libmy_fwrite = dlopen (g_getenv ("LIB_MY_FWRITE_PATH"), RTLD_LAZY);
  g_assert_nonnull (libmy_fwrite);

  mock_my_fwrite = dlsym (libmy_fwrite, "my_fwrite");
  g_assert_nonnull (mock_my_fwrite);

  g_assert_true (real_my_fwrite == mock_my_fwrite);

  g_assert_true (dlclose (libmy_fwrite) == 0);
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
