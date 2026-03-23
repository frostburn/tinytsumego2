#pragma once

#include "tinytsumego2/state.h"

/**
 * @file shape.h
 * @brief Helpers for constructing predefined tsumego shapes.
 */

/**
 * @brief Construct a notcher shape from a Sensei's Library code.
 *
 * See https://senseis.xmp.net/?NotcherCode for the encoding format.
 *
 * @param code Null-terminated notcher code string.
 * @return Parsed state for the encoded shape.
 */
state notcher(const char *code);
