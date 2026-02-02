#pragma once

#include "tinytsumego2/state.h"

// Construct a Notcher based on a code sequence
// See: https://senseis.xmp.net/?NotcherCode
state notcher(const char *code);
