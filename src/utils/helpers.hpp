#pragma once

#include "../config/globals.hpp"

/**
 * @brief Checks if a specific period of time has elapsed.
 */
inline bool elapsed_ms(uint32_t now_ms, uint32_t last_ms, uint32_t period_ms) {
  return (uint32_t)(now_ms - last_ms) >= period_ms;
}
