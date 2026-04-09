#include <glib.h>

#include <stdio.h>
#include <dlfcn.h>

void
greet (FILE *file)
{
  static size_t (*dyn_my_fwrite) (const void *buf, size_t size, size_t count, FILE *file);
  static gpointer libmy_fwrite;

  if (!dyn_my_fwrite)
    {
      libmy_fwrite = dlopen (g_getenv ("LIB_MY_FWRITE_PATH"), RTLD_LAZY);
      g_assert_nonnull (libmy_fwrite);
      dyn_my_fwrite = dlsym (libmy_fwrite, "my_fwrite");
      g_assert_nonnull (dyn_my_fwrite);
    }

  dyn_my_fwrite ("Greetings!\n", 1, sizeof ("Greetings!\n") - 1, file);
}
