#include <glib-2.0/glib.h>

#include <stdio.h>
#include <windows.h>

void
greet (FILE *file)
{
  static size_t (*dyn_my_fwrite) (const void *buf, size_t size, size_t count, FILE *file);
  static HMODULE libmy_fwrite;

  if (!dyn_my_fwrite)
    {
      const gchar *libmy_fwrite_path_utf8 = g_getenv ("LIB_MY_FWRITE_PATH");
      g_assert_nonnull (libmy_fwrite_path_utf8);
      WCHAR *libmy_fwrite_path_utf16 = g_utf8_to_utf16 (libmy_fwrite_path_utf8, -1, NULL, NULL, NULL);
      g_assert_nonnull (libmy_fwrite_path_utf16);
      libmy_fwrite = LoadLibrary (libmy_fwrite_path_utf16);
      g_assert_nonnull (libmy_fwrite);

      dyn_my_fwrite = (gpointer) GetProcAddress (libmy_fwrite, "my_fwrite");
      g_assert_nonnull (dyn_my_fwrite);
    }

  dyn_my_fwrite ("Greetings!\n", 1, sizeof ("Greetings!\n") - 1, file);
}
