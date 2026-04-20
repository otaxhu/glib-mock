#include "bar.h"

const char *
bar (void)
{
  return "Bar!";
}

/* This symbol is needed in order for the library to be linked always */
int bar_needed;
