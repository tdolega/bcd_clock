#pragma once
#include "../config/globals.hpp"
#include <ctype.h>
#include <string.h>

/**
 * @brief Checks if a specific period of time has elapsed.
 */
inline bool elapsed_ms(uint32_t now_ms, uint32_t last_ms, uint32_t period_ms) {
  return (uint32_t)(now_ms - last_ms) >= period_ms;
}

/**
 * @brief Checks if a due timestamp has been reached.
 */
inline bool due_ms(uint32_t now_ms, uint32_t due_ts_ms) {
  return (int32_t)(now_ms - due_ts_ms) >= 0;
}

/**
 * @brief Trims whitespaces from ends and converts string to lowercase in place.
 */
inline void trim_and_lowercase(char* buffer) {
  if(buffer == NULL) return;

  size_t len = strlen(buffer);
  size_t start = 0;

  while(start < len && isspace((unsigned char)buffer[start])) start++;
  while(len > start && isspace((unsigned char)buffer[len - 1])) len--;

  size_t out = 0;
  for(size_t i = start; i < len; i++) {
    buffer[out++] = (char)tolower((unsigned char)buffer[i]);
  }
  buffer[out] = '\0';
}
