// mqtt_v5.h
/******************************************
* MQTT v5.0 Library Header File
* This file introduces MQTT 5.0 compatibility features including properties and protocol versioning.
******************************************/

#ifndef MQTT_V5_H_INCLUDED
#define MQTT_V5_H_INCLUDED

#include <string.h>
#include <stdint.h>
#include <stdio.h>

// Define constants for MQTT message types and lengths
#define MQTT_CONNECT 0x10
#define MQTT_PUBLISH 0x30
#define MQTT_DISCONNECT 0xE0
#define MQTT_PROTOCOL_VERSION_5 0x05
#define MQTT_BUFFER_SIZE 256
#define MQTT_MAX_PROPERTIES 10

// Structure for MQTT properties
typedef struct {
    uint8_t property_id;
    uint16_t length;
    uint8_t *value;
} mqtt_property;

// Function prototypes
int mqtt_connect_message(uint8_t *, const char *, mqtt_property *, uint8_t);
int mqtt_publish_message(uint8_t *, const char *, const char *, mqtt_property *, uint8_t);
int mqtt_disconnect_message(uint8_t *);

#endif // MQTT_V5_H_INCLUDED
