#pragma once

// Helpers for zigbidouille.yaml (Homey integration, two pets, quiet hours).
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

// Is local time `now` inside [start, end)? Windows may cross midnight
// ("22:00" -> "07:00"). An invalid clock or window means "not inside": quiet
// hours must never be the reason a litter box stops cleaning for good.
inline bool in_window(const esphome::ESPTime &now, const std::string &start, const std::string &end) {
  if (!now.is_valid()) return false;
  const int s = parse_hhmm(start), e = parse_hhmm(end);
  if (s < 0 || e < 0 || s == e) return false;
  const int t = now.hour * 60 + now.minute;
  return s < e ? (t >= s && t < e) : (t >= s || t < e);
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
