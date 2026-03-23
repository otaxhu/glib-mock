#include "greeter.h"
#include <windows.h>

#include <glib-2.0/glib.h>

void greet (FILE *file)
{
  static HMODULE stdio_ucrt;
  static size_t (*dyn_fwrite) (const void *buf, size_t size, size_t count, FILE *file);

  if (!stdio_ucrt)
    {
      /* See: https://gist.github.com/njsmith/08b1e52b65ea90427bfd */
      stdio_ucrt = LoadLibrary (L"api-ms-win-crt-stdio-l1-1-0.dll");
      if (!stdio_ucrt)
        g_error ("api-ms-win-crt-stdio-l1-1-0.dll couldn't be loaded");

      dyn_fwrite = GetProcAddress (stdio_ucrt, "fwrite");
      if (!dyn_fwrite)
        g_error ("fwrite couldn't be loaded");
    }

  dyn_fwrite ("Greetings!\n", 1, sizeof ("Greetings!\n") - 1, file);
}
