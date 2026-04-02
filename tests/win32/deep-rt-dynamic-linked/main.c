#include "glib-mock.h"
#include "greeter.h"

#include <windows.h>

static struct
{
  gboolean must_mock;
} mock_state;

static size_t (*real_my_fwrite) (const gpointer buf, size_t size, size_t count, FILE *file);
size_t
my_fwrite (const gpointer buf, size_t size, size_t count, FILE *file) /* Mock */
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
  const gchar *libgreeter_path_utf8 = g_getenv ("LIB_GREETER_PATH");
  g_assert_nonnull (libgreeter_path_utf8);
  WCHAR *libgreeter_path_utf16 = g_utf8_to_utf16 (libgreeter_path_utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (libgreeter_path_utf16);

  HMODULE libgreeter = LoadLibrary (libgreeter_path_utf16);
  g_free (libgreeter_path_utf16);
  g_assert_nonnull (libgreeter);

  greet_proto greet = (gpointer) GetProcAddress (libgreeter, "greet");
  g_assert_nonnull (greet);

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
  const gchar *libgreeter_path_utf8 = g_getenv ("LIB_GREETER_PATH");
  g_assert_nonnull (libgreeter_path_utf8);
  WCHAR *libgreeter_path_utf16 = g_utf8_to_utf16 (libgreeter_path_utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (libgreeter_path_utf16);
  HMODULE libgreeter = LoadLibrary (libgreeter_path_utf16);
  g_free (libgreeter_path_utf16);
  g_assert_nonnull (libgreeter);

  gpointer greet = (gpointer) GetProcAddress (libgreeter, "greet");
  g_assert_nonnull (greet);

  g_assert_true (FreeLibrary (libgreeter) != FALSE);
}

int
main (int argc, char **argv)
{
  mock_state.must_mock = FALSE;

  g_mock_init (&argc, &argv);

  g_mock_add (my_fwrite);
  g_mock_get_real ("my_fwrite", &real_my_fwrite);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/win32/deep-rt-dynamic-linked", test_deep_rt_dynamic_linked);
  g_test_add_func ("/win32/deep-rt-dynamic-linked-greeter", test_deep_rt_dynamic_linked_greeter);

  return g_test_run ();
}
