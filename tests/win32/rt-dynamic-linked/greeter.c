#include "greeter.h"
#include <windows.h>

#include <glib-2.0/glib.h>

void greet (FILE *file)
{
  static HMODULE my_fwrite_handle;
  static size_t (*dyn_fwrite) (const void *buf, size_t size, size_t count, FILE *file);

  if (!my_fwrite_handle)
    {
      gunichar2 *my_fwrite_path = g_utf8_to_utf16 (g_getenv ("LIB_MY_FWRITE_PATH"),
                                                   -1, NULL, NULL, NULL);
      g_assert_nonnull (my_fwrite_path);
      my_fwrite_handle = LoadLibrary (my_fwrite_path);
      g_free (my_fwrite_path);
      g_assert_nonnull (my_fwrite_handle);

      dyn_fwrite = (gpointer) GetProcAddress (my_fwrite_handle, "my_fwrite");
      g_assert_nonnull (dyn_fwrite);
    }

  dyn_fwrite ("Greetings!\n", 1, sizeof ("Greetings!\n") - 1, file);
}
