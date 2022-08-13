#pragma once

#include <stdint.h>

extern void xxtea_encode(uint32_t *restrict v, unsigned n, uint32_t const key[restrict 4]);
extern void xxtea_decode(uint32_t *restrict v, unsigned n, uint32_t const key[restrict 4]);
