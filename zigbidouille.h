#pragma once

// Helpers for zigbidouille.yaml (Homey integration, two pets, K3 deodorizer).
// Kept out of the YAML lambdas so they can be read — and changed — in one place.

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "esphome/core/time.h"

namespace zigbidouille {

// "HH:MM" -> minutes since midnight, or -1 if it is not a valid time.
inline int parse_hhmm(const std::string &s) {
  int h = -1, m = -1;
  if (std::sscanf(s.c_str(), "%d:%d", &h, &m) != 2) return -1;
  if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return h * 60 + m;
}

// Is local time `now` (to the minute) one of the "HH:MM,HH:MM,..." times in `list`?
// Used for the periodic deodorizing schedule. Malformed entries are ignored.
inline bool time_in_list(const esphome::ESPTime &now, const std::string &list) {
  if (!now.is_valid()) return false;
  const int t = now.hour * 60 + now.minute;
  size_t pos = 0;
  while (pos <= list.size()) {
    size_t comma = list.find(',', pos);
    if (comma == std::string::npos) comma = list.size();
    std::string item = list.substr(pos, comma - pos);
    item.erase(0, item.find_first_not_of(' '));
    if (!item.empty() && parse_hhmm(item) == t) return true;
    pos = comma + 1;
  }
  return false;
}

// "fafcfd..." -> bytes. Stops at the first non-hex pair.
inline std::vector<uint8_t> from_hex(const std::string &hex) {
  std::vector<uint8_t> out;
  for (size_t i = 0; i + 1 < hex.size(); i += 2) {
    unsigned v;
    if (std::sscanf(hex.substr(i, 2).c_str(), "%2x", &v) != 1) break;
    out.push_back(static_cast<uint8_t>(v));
  }
  return out;
}

// Median of the weight samples taken while a pet was inside: robust against
// the jump-in and jump-out spikes that a plain average or maximum would keep.
inline float median(std::vector<float> v) {
  if (v.empty()) return NAN;
  std::sort(v.begin(), v.end());
  const size_t n = v.size();
  return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.f;
}

// Which pet a weight belongs to: 1, 2, or 0 when it fits neither range.
inline int classify(float w, float min1, float max1, float min2, float max2) {
  if (w >= min1 && w < max1) return 1;
  if (w >= min2 && w < max2) return 2;
  return 0;
}

}  // namespace zigbidouille
