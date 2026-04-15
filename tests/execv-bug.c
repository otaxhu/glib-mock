#include "glib-mock.h"

int
main (int argc, char **argv)
{
  /* Bug where if argv[0] is a program found only on PATH, then execv would error
   * out.
   */
  g_mock_init (&argc, &argv);

  return 0;
}
