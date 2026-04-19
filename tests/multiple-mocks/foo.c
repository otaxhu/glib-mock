#include "foo.h"

const char *
foo (void)
{
  return "Foo!";
}

/* This symbol is needed in order for the library to be linked always */
int foo_needed;
