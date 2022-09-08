/**
 * @file lib/random/xxtea.h
 *
 * @brief XXTEA block cipher
 *
 * An implementation of the XXTEA block cipher
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdint.h>

extern void xxtea_encode(uint32_t *restrict v, unsigned n, uint32_t const key[restrict 4]);
extern void xxtea_decode(uint32_t *restrict v, unsigned n, uint32_t const key[restrict 4]);
