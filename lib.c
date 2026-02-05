#include <stdlib.h>
#include <stdio.h>
#include "api/tinytsumego2.h"

void version (char *out) {
  sprintf(out, "%s.%s.%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}
