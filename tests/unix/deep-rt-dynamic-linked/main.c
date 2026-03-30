/* This test is similar to unix/rt-dynamic-linked. While the former links against
 * 'greeter' at compile-time, this test doesn't, and introduces a "loader" layer.
 *
 * The executable only knows about "libgreeter" shared object path that can be opened with
 * dlopen. This shared object in turn calls `dlopen()`/`dlsym()` to find `my_fwrite`.
 *
 * This validates that our dlsym implementation can intercept symbols even when
 * the call site (libgreeter) is loaded dynamically at runtime, meaning it's not
 * present initially in the executable.
 */

#include "glib-mock.h"
#include "greeter.h"

#include <dlfcn.h>

static struct
{
  gboolean must_mock;
} mock_state;

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
test_deep_rt_dynamic_linked_greeter (void)
{
  gpointer libgreeter = dlopen (g_getenv ("LIB_GREETER_PATH"), RTLD_LAZY);
  g_assert_nonnull (libgreeter);

  greet_proto greet = dlsym (libgreeter, "greet");

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
test_deep_rt_dynamic_linked (void)
{
  gpointer libgreeter = dlopen (g_getenv ("LIB_GREETER_PATH"), RTLD_LAZY);
  g_assert_nonnull (libgreeter);

  greet_proto greet = dlsym (libgreeter, "greet");
  g_assert_nonnull (greet);

  g_assert_true (dlclose (libgreeter) == 0);
}

int main (int argc, char **argv)
{
  mock_state.must_mock = FALSE;

  g_mock_init (&argc, &argv);

  g_mock_add (my_fwrite);
  g_mock_get_real ("my_fwrite", &real_my_fwrite);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unix/deep-rt-dynamic-linked", test_deep_rt_dynamic_linked);
  g_test_add_func ("/unix/deep-rt-dynamic-linked-greeter", test_deep_rt_dynamic_linked_greeter);

  return g_test_run ();
}
