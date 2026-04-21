/* This represents a "genuine" implementation of a function that just returns the string "Foo!",
 * to be used by tests as a library.
 *
 * This file is not a test in itself.
 */

const char *
foo (void)
{
  return "Foo!";
}

/* This symbol is needed in order for the library to be linked always */
int foo_needed;
