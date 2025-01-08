// mqtt_v5.h
/******************************************
* MQTT v5.0 Library Header File
* This file introduces MQTT 5.0 compatibility features including properties and protocol versioning.
* Extended for SUBSCRIBE and SUBACK support.
******************************************/

#ifndef MQTT_V5_H_INCLUDED
#define MQTT_V5_H_INCLUDED

#include <string.h>
#include <stdint.h>
#include <stdio.h>

// Define constants for MQTT message types
#define MQTT_CONNECT 0x10
#define MQTT_PUBLISH 0x30
#define MQTT_SUBSCRIBE 0x82
#define MQTT_SUBACK 0x90
#define MQTT_DISCONNECT 0xE0
#define MQTT_PROTOCOL_VERSION_5 0x05
#define MQTT_BUFFER_SIZE 256
#define MQTT_MAX_PROPERTIES 10

// MQTT Property structure
typedef struct {
    uint8_t property_id;
    uint16_t length;
    uint8_t *value;
} mqtt_property;

// Function prototypes
int mqtt_v5_connect_message(uint8_t *, const char *, mqtt_property *, uint8_t);
int mqtt_v5_publish_message(uint8_t *, const char *, const char *, mqtt_property *, uint8_t);
int mqtt_v5_subscribe_message(uint8_t *, const char *, uint8_t, mqtt_property *, uint8_t);
int mqtt_v5_parse_suback_message(const uint8_t *, uint8_t *, uint8_t *);
int mqtt_v5_disconnect_message(uint8_t *);

#endif // MQTT_V5_H_INCLUDED
