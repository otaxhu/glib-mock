#include "glib-mock.h"
#include <errno.h>
#include "greeter.h"

static struct {
  gboolean must_fail;
  gboolean must_mock;
} mock_state;

static size_t (*real_fwrite) (const void *buf, size_t size, size_t count, FILE *file);
size_t fwrite (const void *buf, size_t size, size_t count, FILE *file) /* Mock */
{
  if (mock_state.must_fail)
    {
      errno = EIO;
      return 0;
    }

  if (mock_state.must_mock)
    {
      real_fwrite ("Mocked!\n", 1, sizeof ("Mocked!\n") - 1, file);

      return count;
    }

  return real_fwrite (buf, size, count, file);
}

void
test_wrap_function (void)
{
  const gchar expected_contents[] =
    "Greetings!\n"
    "" /* failed */
    "Mocked!\n";

  FILE *tmp = tmpfile ();

  mock_state.must_fail = FALSE;
  mock_state.must_mock = FALSE;

  greet (tmp);

  mock_state.must_fail = TRUE;
  errno = 0;

  greet (tmp);

  g_assert_true (errno != 0);

  mock_state.must_fail = FALSE;
  mock_state.must_mock = TRUE;

  greet (tmp);

  mock_state.must_mock = FALSE;

  gchar read_buf[sizeof (expected_contents)];

  rewind (tmp);
  size_t n_read = fread ((void *) &read_buf, 1, sizeof (read_buf), tmp);

  g_assert_true (n_read == sizeof (read_buf) - 1);

  read_buf[sizeof (expected_contents) - 1] = '\0';

  g_assert_true (strncmp (read_buf, expected_contents, sizeof (read_buf)) == 0);

  fclose (tmp);
}

int
main (int argc, char **argv)
{
  /* Be cautious about the mock_state variable, if you are mocking
   * a syscall, you must have a mechanism to bypass the mock in order to prevent
   * breaking any other function, a gboolean named .must_mock as in this test example.
   */

  mock_state.must_mock = FALSE;
  mock_state.must_fail = FALSE;

  g_mock_add (fwrite);
  g_mock_get_real (fwrite, &real_fwrite);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/wrap-function", test_wrap_function);

  return g_test_run ();
}
