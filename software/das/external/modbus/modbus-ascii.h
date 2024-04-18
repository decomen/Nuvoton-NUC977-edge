/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MODBUS_ASCII_H
#define MODBUS_ASCII_H

#include "modbus.h"

MODBUS_BEGIN_DECLS

#define MODBUS_ASCII_MAX_ADU_LENGTH     MODBUS_RTU_MAX_ADU_LENGTH

MODBUS_API modbus_t* modbus_new_ascii(const char *device, int baud, char parity,
                                    int data_bit, int stop_bit);

MODBUS_API int modbus_ascii_set_serial_mode(modbus_t *ctx, int mode);
MODBUS_API int modbus_ascii_get_serial_mode(modbus_t *ctx);

MODBUS_API int modbus_ascii_set_rts(modbus_t *ctx, int mode);
MODBUS_API int modbus_ascii_get_rts(modbus_t *ctx);

MODBUS_API int modbus_ascii_set_custom_rts(modbus_t *ctx, void (*set_rts) (modbus_t *ctx, int on));

MODBUS_API int modbus_ascii_set_rts_delay(modbus_t *ctx, int us);
MODBUS_API int modbus_ascii_get_rts_delay(modbus_t *ctx);

MODBUS_END_DECLS

#endif /* MODBUS_RTU_H */
