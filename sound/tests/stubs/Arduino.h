#pragma once
#include <cstddef>
#include <cstdint>
#define PROGMEM
inline uint16_t pgm_read_word(const int16_t* p) { return static_cast<uint16_t>(*p); }
