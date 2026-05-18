#pragma once
#include "../config/globals.hpp"
#include <ctype.h>
#include <string>
#include <algorithm>

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
 * @brief Trims whitespaces from ends and converts C++ string to lowercase in place.
 */
inline void trim_and_lowercase(std::string& str) {
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
  str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}
