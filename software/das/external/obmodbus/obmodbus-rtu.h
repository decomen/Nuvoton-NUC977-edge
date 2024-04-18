/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_RTU_H
#define OBMODBUS_RTU_H

#include "obmodbus.h"

OBMODBUS_BEGIN_DECLS

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5
 * RS232 / RS485 ADU = 253 bytes + slave (1 byte) + CRC (2 bytes) = 256 bytes
 */
#define OBMODBUS_RTU_MAX_ADU_LENGTH  256

OBMODBUS_API obmodbus_t* obmodbus_new_rtu(const char *device, int baud, char parity,
                                    int data_bit, int stop_bit);

#define OBMODBUS_RS232 0
#define OBMODBUS_RS485 1

OBMODBUS_API int obmodbus_rtu_set_serial_mode(obmodbus_t *ctx, int mode);
OBMODBUS_API int obmodbus_rtu_get_serial_mode(obmodbus_t *ctx);

#define OBMODBUS_RTS_NONE   0
#define OBMODBUS_RTS_UP     1
#define OBMODBUS_RTS_DOWN   2

OBMODBUS_API int obmodbus_rtu_set_rts(obmodbus_t *ctx, int mode);
OBMODBUS_API int obmodbus_rtu_get_rts(obmodbus_t *ctx);

OBMODBUS_API int obmodbus_rtu_set_custom_rts(obmodbus_t *ctx, void (*set_rts) (obmodbus_t *ctx, int on));

OBMODBUS_API int obmodbus_rtu_set_rts_delay(obmodbus_t *ctx, int us);
OBMODBUS_API int obmodbus_rtu_get_rts_delay(obmodbus_t *ctx);

OBMODBUS_END_DECLS

#endif /* OBMODBUS_RTU_H */
