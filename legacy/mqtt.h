/******************************************
* this code was last updatesd by Edwin Kestler on 7/1/2024 
* this code is a header file for mqtt.cpp
* updates include adding the mqtt_disconnect_message function
* Key updates:
* - Added static buffer for memory reuse in mqtt.cpp
* - Defined constants for protocol and message types
* #define MQTT_BUFFER_SIZE 256 to define the buffer size
* #define MQTT_CONNECT 0x10 to define the connect message type
* #define MQTT_PUBLISH 0x30 to define the publish message type
* #define MQTT_DISCONNECT 0xE0 to define the disconnect message type
* #define MQTT_PROTOCOL_VERSION 3 to define the protocol version
*****************************************
 */

#ifndef MQTT_H_INCLUDED
#define MQTT_H_INCLUDED

#include <string.h>
#include <stdint.h>
#include <stdio.h>

// Define constants for MQTT message types and lengths
#define MQTT_CONNECT 0x10
#define MQTT_PUBLISH 0x30
#define MQTT_DISCONNECT 0xE0
#define MQTT_PROTOCOL_VERSION 3
#define MQTT_BUFFER_SIZE 256

// Preallocated buffer to reduce memory fragmentation
extern uint8_t preallocated_mqtt_buffer[MQTT_BUFFER_SIZE];

// Function prototypes
int mqtt_connect_message(uint8_t *, const char *);
int mqtt_publish_message(uint8_t *, const char *, const char *);
int mqtt_disconnect_message(uint8_t *);

#endif // MQTT_H_INCLUDED
