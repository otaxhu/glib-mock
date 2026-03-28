/* This represents a "genuine" `fwrite` implementation, to be used by other tests as a library.
 *
 * We don't use libc's fwrite because it's usually already linked at process load-time.
 * By putting this in a separate library, we guarantee the symbol `my_fwrite` IS NOT
 * present during the initial `g_mock_commit` call.
 * This allows us to verify that glib-mock correctly interposes symbols from
 * libraries loaded LATE at runtime via LoadLibrary/dlopen.
 *
 * This file is not a test in itself.
 */

#include <stdio.h>

size_t
my_fwrite (const void *buf, size_t size, size_t count, FILE *file)
{
  return fwrite (buf, size, count, file);
}
