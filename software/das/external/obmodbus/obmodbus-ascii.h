/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_ASCII_H
#define OBMODBUS_ASCII_H

#include "obmodbus.h"

OBMODBUS_BEGIN_DECLS

#define OBMODBUS_ASCII_MAX_ADU_LENGTH     OBMODBUS_RTU_MAX_ADU_LENGTH

OBMODBUS_API obmodbus_t* obmodbus_new_ascii(const char *device, int baud, char parity,
                                    int data_bit, int stop_bit);

OBMODBUS_API int obmodbus_ascii_set_serial_mode(obmodbus_t *ctx, int mode);
OBMODBUS_API int obmodbus_ascii_get_serial_mode(obmodbus_t *ctx);

OBMODBUS_API int obmodbus_ascii_set_rts(obmodbus_t *ctx, int mode);
OBMODBUS_API int obmodbus_ascii_get_rts(obmodbus_t *ctx);

OBMODBUS_API int obmodbus_ascii_set_custom_rts(obmodbus_t *ctx, void (*set_rts) (obmodbus_t *ctx, int on));

OBMODBUS_API int obmodbus_ascii_set_rts_delay(obmodbus_t *ctx, int us);
OBMODBUS_API int obmodbus_ascii_get_rts_delay(obmodbus_t *ctx);

OBMODBUS_END_DECLS

#endif /* OBMODBUS_RTU_H */
