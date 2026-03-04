/**
 * @file YuaMQTT.h
 * @brief YuaMQTT - Yet Another UART MQTT Library
 *
 * MQTT v5.0 packet construction and parsing.
 * Transport-agnostic: builds/parses byte buffers only.
 * Designed for Arduino Uno (2KB RAM, 32KB flash).
 *
 * All construction functions return:
 *   - positive int = total packet length in bytes (on success)
 *   - negative int = error code (on failure)
 *     - -1 = NULL parameter
 *     - -2 = buffer overflow
 */

#ifndef YUAMQTT_H
#define YUAMQTT_H

#define YUAMQTT_VERSION "1.1.0"         /**< Library version string */
#define YUAMQTT_VERSION_MAJOR 1         /**< Major version number */
#define YUAMQTT_VERSION_MINOR 1         /**< Minor version number */
#define YUAMQTT_VERSION_PATCH 0         /**< Patch version number */

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name MQTT Packet Type Constants */
/** @{ */
#define MQTT_CONNECT    0x10
#define MQTT_CONNACK    0x20
#define MQTT_PUBLISH    0x30
#define MQTT_SUBSCRIBE  0x82
#define MQTT_SUBACK     0x90
#define MQTT_UNSUBSCRIBE 0xA2
#define MQTT_PINGREQ    0xC0
#define MQTT_PINGRESP   0xD0
#define MQTT_DISCONNECT 0xE0
/** @} */

#define MQTT_PROTOCOL_VERSION_5 0x05    /**< MQTT protocol version 5 */
#define MQTT_BUFFER_SIZE 256            /**< Preallocated buffer size in bytes */
#define MQTT_MAX_PROPERTIES 10          /**< Maximum properties per packet */

/** Default keep-alive in seconds (can be overridden before including this header) */
#ifndef MQTT_DEFAULT_KEEP_ALIVE
#define MQTT_DEFAULT_KEEP_ALIVE 15
#endif

/** @name MQTT V5 Property Data Types */
/** @{ */
#define MQTT_PROP_TYPE_BYTE         0
#define MQTT_PROP_TYPE_TWO_BYTE_INT 1
#define MQTT_PROP_TYPE_FOUR_BYTE_INT 2
#define MQTT_PROP_TYPE_UTF8_STRING  3
#define MQTT_PROP_TYPE_BINARY_DATA  4
#define MQTT_PROP_TYPE_VBI          5
#define MQTT_PROP_TYPE_UTF8_PAIR    6
/** @} */

/**
 * @brief MQTT V5 property structure.
 *
 * For fixed-size types (Byte, TwoByteInt, FourByteInt): store value in
 * @c value, set @c length to type size.
 * For variable-size types (UTF8String, BinaryData): @c value points to data,
 * @c length is data length.
 */
typedef struct {
    uint8_t property_id;    /**< MQTT V5 property identifier (e.g. 0x11 = Session Expiry) */
    uint16_t length;        /**< Value length in bytes */
    uint8_t *value;         /**< Pointer to value data (big-endian for integers) */
} mqtt_property;

/** Preallocated buffer for MQTT message construction (256 bytes) */
extern uint8_t preallocated_mqtt_buffer[MQTT_BUFFER_SIZE];

/**
 * @brief Encode a value as an MQTT Variable Byte Integer.
 * @param buf   Output buffer (must have room for up to 4 bytes)
 * @param value Value to encode (0 to 268,435,455)
 * @return Number of bytes written (1-4), or -1 on error
 * @see MQTT spec section 1.5.5
 */
int mqtt_v5_encode_vbi(uint8_t *buf, uint32_t value);

/**
 * @brief Decode an MQTT Variable Byte Integer.
 * @param buf   Input buffer containing VBI bytes
 * @param value Output: decoded value
 * @return Number of bytes consumed (1-4), or -1 on error
 */
int mqtt_v5_decode_vbi(const uint8_t *buf, uint32_t *value);

/**
 * @brief Construct an MQTT V5 CONNECT packet.
 * @param mqtt_message  Output buffer
 * @param client_id     Client identifier string
 * @param keep_alive    Keep-alive interval in seconds
 * @param properties    Array of MQTT V5 properties (or NULL)
 * @param property_count Number of properties
 * @return Packet length on success, negative error code on failure
 */
int mqtt_v5_connect_message(uint8_t *mqtt_message, const char *client_id, uint16_t keep_alive, mqtt_property *properties, uint8_t property_count);

/**
 * @brief Construct an MQTT V5 PUBLISH packet (QoS 0).
 * @param mqtt_message  Output buffer
 * @param topic         Topic string
 * @param message       Payload string
 * @param properties    Array of MQTT V5 properties (or NULL)
 * @param property_count Number of properties
 * @return Packet length on success, negative error code on failure
 */
int mqtt_v5_publish_message(uint8_t *mqtt_message, const char *topic, const char *message, mqtt_property *properties, uint8_t property_count);

/**
 * @brief Construct an MQTT V5 SUBSCRIBE packet.
 * @param mqtt_message  Output buffer
 * @param packet_id     Packet identifier (must be non-zero)
 * @param topic         Topic filter string
 * @param qos           Requested QoS level (0, 1, or 2)
 * @param properties    Array of MQTT V5 properties (or NULL)
 * @param property_count Number of properties
 * @return Packet length on success, negative error code on failure
 */
int mqtt_v5_subscribe_message(uint8_t *mqtt_message, uint16_t packet_id, const char *topic, uint8_t qos, mqtt_property *properties, uint8_t property_count);

/**
 * @brief Construct an MQTT V5 DISCONNECT packet (reason code 0x00).
 * @param mqtt_message Output buffer
 * @return Packet length (2) on success, negative error code on failure
 */
int mqtt_v5_disconnect_message(uint8_t *mqtt_message);

/**
 * @brief Construct an MQTT V5 PINGREQ packet.
 * @param mqtt_message Output buffer
 * @return Packet length (2) on success, negative error code on failure
 */
int mqtt_v5_pingreq_message(uint8_t *mqtt_message);

/**
 * @brief Parse an MQTT V5 CONNACK packet.
 * @param mqtt_message   Input buffer containing CONNACK
 * @param session_present Output: 1 if session is present, 0 otherwise
 * @param reason_code    Output: CONNACK reason code
 * @return 0 on success, -1 on wrong packet type, -3 if connection refused
 */
int mqtt_v5_parse_connack(const uint8_t *mqtt_message, uint8_t *session_present, uint8_t *reason_code);

/**
 * @brief Parse an MQTT V5 SUBACK packet.
 * @param mqtt_message Input buffer containing SUBACK
 * @param packet_id    Output: packet identifier from SUBACK
 * @param reason_code  Output: subscription reason code
 * @return 0 on success, -1 on wrong packet type
 */
int mqtt_v5_parse_suback_message(const uint8_t *mqtt_message, uint16_t *packet_id, uint8_t *reason_code);

/**
 * @brief Parse an incoming MQTT V5 PUBLISH packet.
 * @param mqtt_message   Input buffer containing PUBLISH
 * @param msg_len        Total length of the input buffer
 * @param topic          Output buffer for topic string
 * @param topic_buf_size Size of topic output buffer
 * @param payload        Output buffer for payload
 * @param payload_buf_size Size of payload output buffer
 * @param topic_len_out  Output: actual topic length
 * @param payload_len_out Output: actual payload length
 * @return 0 on success, -1 on wrong packet type
 */
int mqtt_v5_parse_publish(const uint8_t *mqtt_message, uint16_t msg_len, char *topic, uint16_t topic_buf_size, uint8_t *payload, uint16_t payload_buf_size, uint16_t *topic_len_out, uint16_t *payload_len_out);

/**
 * @brief Check if a message is a PINGRESP.
 * @param mqtt_message Input buffer
 * @return 1 if PINGRESP, 0 otherwise
 */
int mqtt_v5_is_pingresp(const uint8_t *mqtt_message);

#ifdef __cplusplus
}
#endif

#endif // YUAMQTT_H
