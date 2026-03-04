// YuaMQTT.h
/******************************************
* YuaMQTT - Yet Another UART MQTT Library
* MQTT v5.0 packet construction and parsing
*
* Transport-agnostic: builds/parses byte buffers only.
* Designed for Arduino Uno (2KB RAM, 32KB flash).
*
* All construction functions return:
*   positive int = total packet length in bytes (on success)
*   negative int = error code (on failure)
*     -1 = NULL parameter
*     -2 = buffer overflow
******************************************/

#ifndef YUAMQTT_H
#define YUAMQTT_H

#define YUAMQTT_VERSION "1.1.0"
#define YUAMQTT_VERSION_MAJOR 1
#define YUAMQTT_VERSION_MINOR 1
#define YUAMQTT_VERSION_PATCH 0

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MQTT packet type constants
#define MQTT_CONNECT    0x10
#define MQTT_CONNACK    0x20
#define MQTT_PUBLISH    0x30
#define MQTT_SUBSCRIBE  0x82
#define MQTT_SUBACK     0x90
#define MQTT_UNSUBSCRIBE 0xA2
#define MQTT_PINGREQ    0xC0
#define MQTT_PINGRESP   0xD0
#define MQTT_DISCONNECT 0xE0

#define MQTT_PROTOCOL_VERSION_5 0x05
#define MQTT_BUFFER_SIZE 256
#define MQTT_MAX_PROPERTIES 10

// Default keep-alive in seconds (can be overridden before including this header)
#ifndef MQTT_DEFAULT_KEEP_ALIVE
#define MQTT_DEFAULT_KEEP_ALIVE 15
#endif

// MQTT V5 property data types
#define MQTT_PROP_TYPE_BYTE         0
#define MQTT_PROP_TYPE_TWO_BYTE_INT 1
#define MQTT_PROP_TYPE_FOUR_BYTE_INT 2
#define MQTT_PROP_TYPE_UTF8_STRING  3
#define MQTT_PROP_TYPE_BINARY_DATA  4
#define MQTT_PROP_TYPE_VBI          5
#define MQTT_PROP_TYPE_UTF8_PAIR    6

// MQTT Property structure
// For fixed-size types (Byte, TwoByteInt, FourByteInt): store value in `value`, set `length` to type size
// For variable-size types (UTF8String, BinaryData): `value` points to data, `length` is data length
typedef struct {
    uint8_t property_id;
    uint16_t length;
    uint8_t *value;
} mqtt_property;

// Preallocated buffer for MQTT message construction
extern uint8_t preallocated_mqtt_buffer[MQTT_BUFFER_SIZE];

// Variable Byte Integer encoding/decoding (MQTT spec section 1.5.5)
int mqtt_v5_encode_vbi(uint8_t *buf, uint32_t value);
int mqtt_v5_decode_vbi(const uint8_t *buf, uint32_t *value);

// Packet construction functions - return packet length on success, negative on error
int mqtt_v5_connect_message(uint8_t *mqtt_message, const char *client_id, uint16_t keep_alive, mqtt_property *properties, uint8_t property_count);
int mqtt_v5_publish_message(uint8_t *mqtt_message, const char *topic, const char *message, mqtt_property *properties, uint8_t property_count);
int mqtt_v5_subscribe_message(uint8_t *mqtt_message, uint16_t packet_id, const char *topic, uint8_t qos, mqtt_property *properties, uint8_t property_count);
int mqtt_v5_disconnect_message(uint8_t *mqtt_message);
int mqtt_v5_pingreq_message(uint8_t *mqtt_message);

// Packet parsing functions
int mqtt_v5_parse_connack(const uint8_t *mqtt_message, uint8_t *session_present, uint8_t *reason_code);
int mqtt_v5_parse_suback_message(const uint8_t *mqtt_message, uint16_t *packet_id, uint8_t *reason_code);
int mqtt_v5_parse_publish(const uint8_t *mqtt_message, uint16_t msg_len, char *topic, uint16_t topic_buf_size, uint8_t *payload, uint16_t payload_buf_size, uint16_t *topic_len_out, uint16_t *payload_len_out);
int mqtt_v5_is_pingresp(const uint8_t *mqtt_message);

#ifdef __cplusplus
}
#endif

#endif // YUAMQTT_H
