/* This represents a "genuine" implementation of a function that just returns the string "Bar!",
 * to be used by tests as a library.
 *
 * This file is not a test in itself.
 */

const char *
bar (void)
{
  return "Bar!";
}

/* This symbol is needed in order for the library to be linked always */
int bar_needed;
