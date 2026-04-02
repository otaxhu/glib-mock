#include "glib-mock.h"
#include "greeter.h"

#include <windows.h>

static struct
{
  gboolean must_mock;
} mock_state;

static gpointer (WINAPI *real_GetProcAddress) (HMODULE module, gchar *func_name);

static size_t (*real_my_fwrite) (const void *buf, size_t size, size_t count, FILE *file);
size_t my_fwrite (const void *buf, size_t size, size_t count, FILE *file) /* Mock */
{
  if (mock_state.must_mock)
    {
      real_my_fwrite ("Mocked!\n", 1, sizeof ("Mocked!\n"), file);

      return count;
    }

  return real_my_fwrite (buf, size, count, file);
}

static void
test_rt_dynamic_linked_other_module (void)
{
  /* Call greeter, which in turn should call the mocked GetProcAddress that returns
   * the my_fwrite mocked.
   */

  const gchar expected_contents[] =
    "Greetings!\n"
    "Mocked!\n";

  mock_state.must_mock = FALSE;

  FILE *tmp = tmpfile ();

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
  g_assert_true ((gpointer) real_GetProcAddress != (gpointer) GetProcAddress);

  const gchar *libmy_fwrite_utf8 = g_getenv ("LIB_MY_FWRITE_PATH");
  g_assert_nonnull (libmy_fwrite_utf8);
  WCHAR *libmy_fwrite_utf16 = g_utf8_to_utf16 (libmy_fwrite_utf8, -1, NULL, NULL, NULL);
  g_assert_nonnull (libmy_fwrite_utf16);
  HMODULE libmy_fwrite = LoadLibrary (libmy_fwrite_utf16);
  g_free (libmy_fwrite_utf16);
  g_assert_nonnull (libmy_fwrite);

  gpointer real_func = real_GetProcAddress (libmy_fwrite, "my_fwrite");
  g_assert_nonnull (real_func);

  gpointer mock_func = GetProcAddress (libmy_fwrite, "my_fwrite");
  g_assert_nonnull (mock_func);

  g_assert_true (real_func != mock_func);
}

int main (int argc, char **argv)
{
  mock_state.must_mock = FALSE;

  g_mock_init (&argc, &argv);
  g_mock_add (my_fwrite);
  g_mock_get_real ("my_fwrite", &real_my_fwrite);

  g_mock_get_real ("GetProcAddress", (gpointer *) &real_GetProcAddress);

  g_mock_commit ();

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/win32/rt-dynamic-linked", test_rt_dynamic_linked);
  g_test_add_func ("/win32/rt-dynamic-linked-other-module", test_rt_dynamic_linked_other_module);

  return g_test_run ();
}
