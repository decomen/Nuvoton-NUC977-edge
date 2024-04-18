/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_ASCII_PRIVATE_H
#define OBMODBUS_ASCII_PRIVATE_H

#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif

#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#endif

#define _OBMODBUS_ASCII_HEADER_LENGTH      _OBMODBUS_RTU_HEADER_LENGTH
#define _OBMODBUS_ASCII_PRESET_REQ_LENGTH  _OBMODBUS_RTU_PRESET_REQ_LENGTH
#define _OBMODBUS_ASCII_PRESET_RSP_LENGTH  _OBMODBUS_RTU_PRESET_RSP_LENGTH

#define _OBMODBUS_ASCII_CHECKSUM_LENGTH  1

uint8_t _obmodbus_ascii_char2hex(char c);
uint8_t _obmodbus_ascii_chars2hex(char a, char b);
int _obmodbus_ascii_chars2hexs(char *msg, int len);
char _obmodbus_ascii_hex2char(uint8_t b);
int _obmodbus_ascii_hexs2chars(const uint8_t *hexs, int hex_len, char *chars);
uint8_t _obmodbus_ascii_lrc(uint8_t *msg, int msg_length);

int _obmodbus_ascii_prepare_response_tid(const uint8_t *req, int *req_length);
int _obmodbus_ascii_send_msg_pre(uint8_t *req, int req_length);
ssize_t _obmodbus_ascii_send(obmodbus_t *ctx, const uint8_t *req, int req_length);
int _obmodbus_ascii_check_integrity(obmodbus_t *ctx, uint8_t *msg, const int msg_length);

#endif /* OBMODBUS_RTU_PRIVATE_H */
