#include "greeter.h"

void
greet (FILE *file)
{
  fwrite ("Greetings!\n", 1, sizeof ("Greetings!\n") - 1, file);
}
