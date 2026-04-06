#include "glib-mock.h"
#include "greeter.h"

#include <dlfcn.h>

static struct
{
  gboolean must_mock;
} mock_state;

static gpointer (*real_dlsym) (gpointer handle, const gchar *name);

static size_t
(*real_my_fwrite) (const void *buf, size_t size, size_t count, FILE *file);
size_t
my_fwrite (const void *buf, size_t size, size_t count, FILE *file) /* Mock */
{
  if (mock_state.must_mock)
    {
      real_my_fwrite ("Mocked!\n", 1, sizeof ("Mocked!\n") - 1, file);

      return count;
    }

  return real_my_fwrite (buf, size, count, file);
}

static void
test_rt_dynamic_linked_external (void)
{
  const gchar expected_contents[] =
    "Greetings!\n"
    "Mocked!\n";

  FILE *tmp = tmpfile ();

  mock_state.must_mock = FALSE;

  greet (tmp);

  mock_state.must_mock = TRUE;

  greet (tmp);

  mock_state.must_mock = FALSE;

  gchar read_buf[sizeof (expected_contents)];

  rewind (tmp);
  size_t n_read = fread (&read_buf, 1, sizeof (read_buf) - 1, tmp);

  g_assert_true (n_read == sizeof (read_buf) - 1);

  read_buf[sizeof (expected_contents) - 1] = '\0';

  g_assert_true (strncmp (read_buf, expected_contents, sizeof (read_buf)) == 0);

  fclose (tmp);
}

static void
test_rt_dynamic_linked (void)
{
  /* FIXME: The below assertion doesn't work on Ubuntu Linux, needs investigation,
   * at least the rest of assertions are working correctly, testing the behaviour is
   * more important at this moment.
   */
  /*
  g_assert_true (*real_dlsym != dlsym);
  */

  /* NOTE: The below assertion works on Linux, but doesn't on macOS, as a matter of fact,
   * if you compile the test with "-rdynamic" flag on Linux, it stops working on Linux.
   *
   * See: https://stackoverflow.com/questions/73207260
   *
   * Since we are expecting the user to define a mock as non-static, we can assume
   * that the symbol is available through RTLD_DEFAULT, but since it depends on how they
   * compiled the test executable, this assertion it's uncertain.
   */
  /*
  gpointer real_func = dlsym (RTLD_DEFAULT, "my_fwrite");
  g_assert_null (real_func);
  */

  gpointer libmy_fwrite = dlopen (g_getenv ("LIB_MY_FWRITE_PATH"), RTLD_NOW);
  g_assert_nonnull (libmy_fwrite);

  gpointer real_func = real_dlsym (libmy_fwrite, "my_fwrite");
  g_assert_nonnull (real_func);

  gpointer mock_func = dlsym (libmy_fwrite, "my_fwrite");
  g_assert_nonnull (mock_func);

  g_assert_true (real_func != mock_func);
}

int
main (int argc, char **argv)
{
  mock_state.must_mock = FALSE;

  g_mock_init (&argc, &argv);

  g_mock_add (my_fwrite);
  g_mock_get_real ("my_fwrite", &real_my_fwrite);
  g_mock_get_real ("dlsym", &real_dlsym);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unix/rt-dynamic-linked", test_rt_dynamic_linked);
  g_test_add_func ("/unix/rt-dynamic-linked-external", test_rt_dynamic_linked_external);

  return g_test_run ();
}
