/*******************************************************************************
 * Copyright (c) 2009, 2018 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs, Allan Stockdill-Mander - SSL updates
 *    Ian Craggs - MQTT 3.1.1 support
 *    Ian Craggs - SNI support
 *    Ian Craggs - MQTT 5.0 support
 *******************************************************************************/

#if !defined(MQTTPROTOCOLOUT_H)
#define MQTTPROTOCOLOUT_H

#include "LinkedList.h"
#include "MQTTPacket.h"
#include "Clients.h"
#include "Log.h"
#include "Messages.h"
#include "MQTTProtocol.h"
#include "MQTTProtocolClient.h"

#define DEFAULT_PORT 1883

size_t MQTTProtocol_addressPort(const char* uri, int* port, const char **topic);
void MQTTProtocol_reconnect(const char* ip_address, Clients* client);
#if defined(OPENSSL)
int MQTTProtocol_connect(const char* ip_address, const char *intfc_name, Clients* acClients, int ssl, int websocket, int MQTTVersion,
		MQTTProperties* connectProperties, MQTTProperties* willProperties);
#else
int MQTTProtocol_connect(const char* ip_address, const char *intfc_name, Clients* acClients, int websocket, int MQTTVersion,
		MQTTProperties* connectProperties, MQTTProperties* willProperties);
#endif
int MQTTProtocol_handlePingresps(void* pack, int sock);
int MQTTProtocol_subscribe(Clients* client, List* topics, List* qoss, int msgID,
		MQTTSubscribe_options* opts, MQTTProperties* props);
int MQTTProtocol_handleSubacks(void* pack, int sock);
int MQTTProtocol_unsubscribe(Clients* client, List* topics, int msgID, MQTTProperties* props);
int MQTTProtocol_handleUnsubacks(void* pack, int sock);

#endif