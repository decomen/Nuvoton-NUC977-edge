/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_UDP_PRIVATE_H
#define OBMODBUS_UDP_PRIVATE_H

int _obmodbus_udp_connect(obmodbus_t *ctx);
int _obmodbus_udp_flush(obmodbus_t *ctx);
ssize_t _obmodbus_udp_send(obmodbus_t *ctx, const uint8_t *req, int req_length);
ssize_t _obmodbus_udp_recv(obmodbus_t *ctx, uint8_t *rsp, int rsp_length);

#endif /* OBMODBUS_TCP_PRIVATE_H */
