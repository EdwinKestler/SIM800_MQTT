/******************************************
* YuaMQTT.cpp
* MQTT 5.0 packet construction and parsing.
*
* All construction functions return:
*   positive int = total packet length (success)
*   negative int = error code (failure)
******************************************/

#include "YuaMQTT.h"

// Preallocated buffer to minimize dynamic allocations
uint8_t preallocated_mqtt_buffer[MQTT_BUFFER_SIZE];

/******************************************
* Variable Byte Integer encoding (MQTT spec 1.5.5)
* Returns number of bytes written (1-4), or -1 on error.
******************************************/
int mqtt_v5_encode_vbi(uint8_t *buf, uint32_t value) {
    if (!buf) return -1;
    if (value > 268435455UL) return -1; // Max VBI value

    int count = 0;
    do {
        uint8_t encoded_byte = value % 128;
        value = value / 128;
        if (value > 0) {
            encoded_byte |= 0x80; // Set continuation bit
        }
        buf[count++] = encoded_byte;
    } while (value > 0);

    return count;
}

/******************************************
* Variable Byte Integer decoding
* Returns number of bytes consumed (1-4), or -1 on error.
* Decoded value is written to *value.
******************************************/
int mqtt_v5_decode_vbi(const uint8_t *buf, uint32_t *value) {
    if (!buf || !value) return -1;

    uint32_t result = 0;
    uint32_t multiplier = 1;
    int count = 0;

    do {
        if (count >= 4) return -1; // Malformed VBI
        uint8_t encoded_byte = buf[count];
        result += (encoded_byte & 0x7F) * multiplier;
        multiplier *= 128;
        count++;
        if ((encoded_byte & 0x80) == 0) break;
    } while (1);

    *value = result;
    return count;
}

/******************************************
* Property type lookup per MQTT V5 spec.
* Returns the data type for a given property ID,
* or -1 if unknown.
******************************************/
static int get_property_type(uint8_t property_id) {
    switch (property_id) {
        // Byte properties
        case 0x01: // Payload Format Indicator
        case 0x17: // Request Problem Information
        case 0x19: // Request Response Information
        case 0x24: // Maximum QoS
        case 0x25: // Retain Available
        case 0x28: // Wildcard Subscription Available
        case 0x29: // Subscription Identifier Available
        case 0x2A: // Shared Subscription Available
            return MQTT_PROP_TYPE_BYTE;

        // Two Byte Integer properties
        case 0x13: // Server Keep Alive
        case 0x21: // Receive Maximum
        case 0x22: // Topic Alias Maximum
        case 0x23: // Topic Alias
            return MQTT_PROP_TYPE_TWO_BYTE_INT;

        // Four Byte Integer properties
        case 0x02: // Message Expiry Interval
        case 0x11: // Session Expiry Interval
        case 0x18: // Will Delay Interval
        case 0x27: // Maximum Packet Size
            return MQTT_PROP_TYPE_FOUR_BYTE_INT;

        // UTF-8 Encoded String properties
        case 0x03: // Content Type
        case 0x08: // Response Topic
        case 0x12: // Assigned Client Identifier
        case 0x15: // Authentication Method
        case 0x1A: // Response Information
        case 0x1C: // Server Reference
        case 0x1F: // Reason String
            return MQTT_PROP_TYPE_UTF8_STRING;

        // Binary Data properties
        case 0x09: // Correlation Data
        case 0x16: // Authentication Data
            return MQTT_PROP_TYPE_BINARY_DATA;

        // Variable Byte Integer properties
        case 0x0B: // Subscription Identifier
            return MQTT_PROP_TYPE_VBI;

        // UTF-8 String Pair
        case 0x26: // User Property
            return MQTT_PROP_TYPE_UTF8_PAIR;

        default:
            return -1;
    }
}

/******************************************
* Encode a single property into the buffer.
* Returns bytes written, or -1 on error.
*
* Encoding per spec:
*   Byte:           [id][1 byte value]
*   Two Byte Int:   [id][2 bytes big-endian]
*   Four Byte Int:  [id][4 bytes big-endian]
*   UTF-8 String:   [id][2-byte length][string bytes]
*   Binary Data:    [id][2-byte length][data bytes]
*   VBI:            [id][VBI-encoded value]
*   UTF-8 Pair:     [id][2-byte len][key][2-byte len][val] (simplified: treat as binary)
******************************************/
static int encode_property(uint8_t *buf, const mqtt_property *prop) {
    if (!buf || !prop || !prop->value) return -1;

    int type = get_property_type(prop->property_id);
    if (type < 0) return -1;

    uint16_t index = 0;
    buf[index++] = prop->property_id;

    switch (type) {
        case MQTT_PROP_TYPE_BYTE:
            buf[index++] = prop->value[0];
            break;

        case MQTT_PROP_TYPE_TWO_BYTE_INT:
            buf[index++] = prop->value[0];
            buf[index++] = prop->value[1];
            break;

        case MQTT_PROP_TYPE_FOUR_BYTE_INT:
            buf[index++] = prop->value[0];
            buf[index++] = prop->value[1];
            buf[index++] = prop->value[2];
            buf[index++] = prop->value[3];
            break;

        case MQTT_PROP_TYPE_UTF8_STRING:
        case MQTT_PROP_TYPE_BINARY_DATA:
        case MQTT_PROP_TYPE_UTF8_PAIR:
            // Length-prefixed: [2-byte length MSB,LSB][data]
            buf[index++] = (prop->length >> 8) & 0xFF;
            buf[index++] = prop->length & 0xFF;
            memcpy(&buf[index], prop->value, prop->length);
            index += prop->length;
            break;

        case MQTT_PROP_TYPE_VBI: {
            // Caller stores the integer value in the value buffer (up to 4 bytes, big-endian)
            uint32_t vbi_val = 0;
            for (uint16_t i = 0; i < prop->length && i < 4; i++) {
                vbi_val = (vbi_val << 8) | prop->value[i];
            }
            int vbi_len = mqtt_v5_encode_vbi(&buf[index], vbi_val);
            if (vbi_len < 0) return -1;
            index += vbi_len;
            break;
        }

        default:
            return -1;
    }

    return index;
}

/******************************************
* Calculate the encoded size of a single property (without writing).
* Returns size in bytes, or -1 on error.
******************************************/
static int property_encoded_size(const mqtt_property *prop) {
    if (!prop || !prop->value) return -1;

    int type = get_property_type(prop->property_id);
    if (type < 0) return -1;

    switch (type) {
        case MQTT_PROP_TYPE_BYTE:
            return 1 + 1; // id + 1 byte
        case MQTT_PROP_TYPE_TWO_BYTE_INT:
            return 1 + 2; // id + 2 bytes
        case MQTT_PROP_TYPE_FOUR_BYTE_INT:
            return 1 + 4; // id + 4 bytes
        case MQTT_PROP_TYPE_UTF8_STRING:
        case MQTT_PROP_TYPE_BINARY_DATA:
        case MQTT_PROP_TYPE_UTF8_PAIR:
            return 1 + 2 + prop->length; // id + length prefix + data
        case MQTT_PROP_TYPE_VBI: {
            // Calculate VBI encoded size
            uint32_t vbi_val = 0;
            for (uint16_t i = 0; i < prop->length && i < 4; i++) {
                vbi_val = (vbi_val << 8) | prop->value[i];
            }
            uint8_t tmp[4];
            int vbi_len = mqtt_v5_encode_vbi(tmp, vbi_val);
            if (vbi_len < 0) return -1;
            return 1 + vbi_len; // id + VBI bytes
        }
        default:
            return -1;
    }
}

/******************************************
* Calculate total encoded size of all properties (data only, no length prefix).
* Returns total bytes, or -1 on error.
******************************************/
static int calculate_properties_size(const mqtt_property *properties, uint8_t count) {
    int total = 0;
    for (uint8_t i = 0; i < count; i++) {
        int size = property_encoded_size(&properties[i]);
        if (size < 0) return -1;
        total += size;
    }
    return total;
}

/******************************************
* Encode all properties into buffer (properties data only, no length prefix).
* Returns bytes written, or -1 on error.
******************************************/
static int encode_all_properties(uint8_t *buf, const mqtt_property *properties, uint8_t count) {
    int total = 0;
    for (uint8_t i = 0; i < count; i++) {
        int written = encode_property(&buf[total], &properties[i]);
        if (written < 0) return -1;
        total += written;
    }
    return total;
}

/******************************************
* MQTT V5 CONNECT message construction
*
* Packet structure:
*   Fixed header: [0x10][Remaining Length VBI]
*   Variable header:
*     [0x00][0x04]MQTT       - Protocol Name
*     [0x05]                  - Protocol Version 5
*     [0x02]                  - Connect Flags (Clean Start)
*     [keep_alive MSB][LSB]   - Keep Alive (seconds)
*     [Properties Length VBI] - Properties length
*     [Properties...]         - Property data
*   Payload:
*     [0x00][client_id_len]   - Client ID length
*     [client_id bytes]       - Client ID
*
* Returns: total packet length on success, negative on error
******************************************/
int mqtt_v5_connect_message(uint8_t *mqtt_message, const char *client_id, uint16_t keep_alive, mqtt_property *properties, uint8_t property_count) {
    if (!mqtt_message || !client_id) return -1;

    uint16_t client_id_length = strlen(client_id);

    // Calculate properties data size
    int props_data_size = 0;
    if (property_count > 0 && properties) {
        props_data_size = calculate_properties_size(properties, property_count);
        if (props_data_size < 0) return -1;
    }

    // Calculate properties length field size (VBI)
    uint8_t props_len_vbi[4];
    int props_len_vbi_size = mqtt_v5_encode_vbi(props_len_vbi, (uint32_t)props_data_size);
    if (props_len_vbi_size < 0) return -1;

    // Variable header: protocol name(6) + version(1) + flags(1) + keepalive(2) = 10
    // + properties length VBI + properties data
    // Payload: client ID length(2) + client ID
    uint32_t remaining_length = 10 + props_len_vbi_size + props_data_size + 2 + client_id_length;

    // Encode remaining length as VBI
    uint8_t rem_len_vbi[4];
    int rem_len_vbi_size = mqtt_v5_encode_vbi(rem_len_vbi, remaining_length);
    if (rem_len_vbi_size < 0) return -1;

    uint16_t total_size = 1 + rem_len_vbi_size + remaining_length;
    if (total_size > MQTT_BUFFER_SIZE) return -2;

    uint16_t index = 0;

    // Fixed header
    mqtt_message[index++] = MQTT_CONNECT;
    memcpy(&mqtt_message[index], rem_len_vbi, rem_len_vbi_size);
    index += rem_len_vbi_size;

    // Protocol Name
    mqtt_message[index++] = 0x00;
    mqtt_message[index++] = 0x04;
    memcpy(&mqtt_message[index], "MQTT", 4);
    index += 4;

    // Protocol Version
    mqtt_message[index++] = MQTT_PROTOCOL_VERSION_5;

    // Connect Flags (Clean Start = 1)
    mqtt_message[index++] = 0x02;

    // Keep Alive
    mqtt_message[index++] = (keep_alive >> 8) & 0xFF;
    mqtt_message[index++] = keep_alive & 0xFF;

    // Properties Length (VBI)
    memcpy(&mqtt_message[index], props_len_vbi, props_len_vbi_size);
    index += props_len_vbi_size;

    // Properties data
    if (props_data_size > 0 && properties) {
        int written = encode_all_properties(&mqtt_message[index], properties, property_count);
        if (written < 0) return -1;
        index += written;
    }

    // Payload: Client ID
    mqtt_message[index++] = (client_id_length >> 8) & 0xFF;
    mqtt_message[index++] = client_id_length & 0xFF;
    memcpy(&mqtt_message[index], client_id, client_id_length);
    index += client_id_length;

    return (int)index;
}

/******************************************
* MQTT V5 PUBLISH message construction (QoS 0)
*
* Packet structure:
*   Fixed header: [0x30][Remaining Length VBI]
*   Variable header:
*     [topic_len MSB][topic_len LSB][topic]
*     [Properties Length VBI][Properties...]
*   Payload:
*     [message bytes]
*
* Returns: total packet length on success, negative on error
******************************************/
int mqtt_v5_publish_message(uint8_t *mqtt_message, const char *topic, const char *message, mqtt_property *properties, uint8_t property_count) {
    if (!mqtt_message || !topic || !message) return -1;

    uint16_t topic_length = strlen(topic);
    uint16_t message_length = strlen(message);

    // Calculate properties data size
    int props_data_size = 0;
    if (property_count > 0 && properties) {
        props_data_size = calculate_properties_size(properties, property_count);
        if (props_data_size < 0) return -1;
    }

    // Properties length VBI
    uint8_t props_len_vbi[4];
    int props_len_vbi_size = mqtt_v5_encode_vbi(props_len_vbi, (uint32_t)props_data_size);
    if (props_len_vbi_size < 0) return -1;

    // Remaining length = topic length field(2) + topic + props len VBI + props data + payload
    uint32_t remaining_length = 2 + topic_length + props_len_vbi_size + props_data_size + message_length;

    // Remaining length VBI
    uint8_t rem_len_vbi[4];
    int rem_len_vbi_size = mqtt_v5_encode_vbi(rem_len_vbi, remaining_length);
    if (rem_len_vbi_size < 0) return -1;

    uint16_t total_size = 1 + rem_len_vbi_size + remaining_length;
    if (total_size > MQTT_BUFFER_SIZE) return -2;

    uint16_t index = 0;

    // Fixed header
    mqtt_message[index++] = MQTT_PUBLISH;
    memcpy(&mqtt_message[index], rem_len_vbi, rem_len_vbi_size);
    index += rem_len_vbi_size;

    // Topic
    mqtt_message[index++] = (topic_length >> 8) & 0xFF;
    mqtt_message[index++] = topic_length & 0xFF;
    memcpy(&mqtt_message[index], topic, topic_length);
    index += topic_length;

    // Properties Length (VBI)
    memcpy(&mqtt_message[index], props_len_vbi, props_len_vbi_size);
    index += props_len_vbi_size;

    // Properties data
    if (props_data_size > 0 && properties) {
        int written = encode_all_properties(&mqtt_message[index], properties, property_count);
        if (written < 0) return -1;
        index += written;
    }

    // Payload
    memcpy(&mqtt_message[index], message, message_length);
    index += message_length;

    return (int)index;
}

/******************************************
* MQTT V5 SUBSCRIBE message construction
*
* Packet structure:
*   Fixed header: [0x82][Remaining Length VBI]
*   Variable header:
*     [Packet ID MSB][Packet ID LSB]
*     [Properties Length VBI][Properties...]
*   Payload:
*     [topic_len MSB][topic_len LSB][topic][QoS]
*
* Returns: total packet length on success, negative on error
******************************************/
int mqtt_v5_subscribe_message(uint8_t *mqtt_message, uint16_t packet_id, const char *topic, uint8_t qos, mqtt_property *properties, uint8_t property_count) {
    if (!mqtt_message || !topic) return -1;

    uint16_t topic_length = strlen(topic);

    // Calculate properties data size
    int props_data_size = 0;
    if (property_count > 0 && properties) {
        props_data_size = calculate_properties_size(properties, property_count);
        if (props_data_size < 0) return -1;
    }

    // Properties length VBI
    uint8_t props_len_vbi[4];
    int props_len_vbi_size = mqtt_v5_encode_vbi(props_len_vbi, (uint32_t)props_data_size);
    if (props_len_vbi_size < 0) return -1;

    // Remaining length = packet ID(2) + props len VBI + props data + topic len field(2) + topic + QoS(1)
    uint32_t remaining_length = 2 + props_len_vbi_size + props_data_size + 2 + topic_length + 1;

    // Remaining length VBI
    uint8_t rem_len_vbi[4];
    int rem_len_vbi_size = mqtt_v5_encode_vbi(rem_len_vbi, remaining_length);
    if (rem_len_vbi_size < 0) return -1;

    uint16_t total_size = 1 + rem_len_vbi_size + remaining_length;
    if (total_size > MQTT_BUFFER_SIZE) return -2;

    uint16_t index = 0;

    // Fixed header
    mqtt_message[index++] = MQTT_SUBSCRIBE;
    memcpy(&mqtt_message[index], rem_len_vbi, rem_len_vbi_size);
    index += rem_len_vbi_size;

    // Packet Identifier
    mqtt_message[index++] = (packet_id >> 8) & 0xFF;
    mqtt_message[index++] = packet_id & 0xFF;

    // Properties Length (VBI)
    memcpy(&mqtt_message[index], props_len_vbi, props_len_vbi_size);
    index += props_len_vbi_size;

    // Properties data
    if (props_data_size > 0 && properties) {
        int written = encode_all_properties(&mqtt_message[index], properties, property_count);
        if (written < 0) return -1;
        index += written;
    }

    // Payload: Topic Filter + Subscription Options
    mqtt_message[index++] = (topic_length >> 8) & 0xFF;
    mqtt_message[index++] = topic_length & 0xFF;
    memcpy(&mqtt_message[index], topic, topic_length);
    index += topic_length;

    mqtt_message[index++] = qos; // Subscription Options (QoS)

    return (int)index;
}

/******************************************
* Parse SUBACK message
*
* SUBACK structure:
*   [0x90][Remaining Length VBI]
*   [Packet ID MSB][Packet ID LSB]
*   [Properties Length VBI][Properties...]
*   [Reason Code]
*
* Returns: 0 on success, negative on error
******************************************/
int mqtt_v5_parse_suback_message(const uint8_t *mqtt_message, uint16_t *packet_id, uint8_t *reason_code) {
    if (!mqtt_message || !packet_id || !reason_code) return -1;

    if (mqtt_message[0] != MQTT_SUBACK) return -2;

    uint16_t index = 1;

    // Decode remaining length (VBI)
    uint32_t remaining_length;
    int vbi_len = mqtt_v5_decode_vbi(&mqtt_message[index], &remaining_length);
    if (vbi_len < 0) return -1;
    index += vbi_len;

    // Packet Identifier (2 bytes)
    *packet_id = ((uint16_t)mqtt_message[index] << 8) | mqtt_message[index + 1];
    index += 2;

    // Properties Length (VBI) - skip properties
    uint32_t props_length;
    vbi_len = mqtt_v5_decode_vbi(&mqtt_message[index], &props_length);
    if (vbi_len < 0) return -1;
    index += vbi_len;
    index += props_length; // Skip property data

    // Reason Code
    *reason_code = mqtt_message[index];

    return 0;
}

/******************************************
* MQTT V5 DISCONNECT message construction
*
* Minimal disconnect: [0xE0][0x00]
* Returns: total packet length (2) on success, negative on error
******************************************/
int mqtt_v5_disconnect_message(uint8_t *mqtt_message) {
    if (!mqtt_message) return -1;

    mqtt_message[0] = MQTT_DISCONNECT;
    mqtt_message[1] = 0x00;

    return 2;
}

/******************************************
* MQTT V5 PINGREQ message construction
*
* PINGREQ: [0xC0][0x00]
* Must be sent within the keep-alive interval to prevent
* the broker from disconnecting the client.
*
* Returns: total packet length (2) on success, negative on error
******************************************/
int mqtt_v5_pingreq_message(uint8_t *mqtt_message) {
    if (!mqtt_message) return -1;

    mqtt_message[0] = MQTT_PINGREQ;
    mqtt_message[1] = 0x00;

    return 2;
}

/******************************************
* Parse CONNACK message
*
* CONNACK structure:
*   [0x20][Remaining Length VBI]
*   [Connect Acknowledge Flags] (bit 0 = Session Present)
*   [Reason Code]
*   [Properties Length VBI][Properties...]
*
* Returns: 0 on success, negative on error
*   -1 = NULL parameter
*   -2 = not a CONNACK packet
*   -3 = connection refused (reason_code will contain the code)
******************************************/
int mqtt_v5_parse_connack(const uint8_t *mqtt_message, uint8_t *session_present, uint8_t *reason_code) {
    if (!mqtt_message || !session_present || !reason_code) return -1;

    if (mqtt_message[0] != MQTT_CONNACK) return -2;

    uint16_t index = 1;

    // Decode remaining length (VBI)
    uint32_t remaining_length;
    int vbi_len = mqtt_v5_decode_vbi(&mqtt_message[index], &remaining_length);
    if (vbi_len < 0) return -1;
    index += vbi_len;

    // Connect Acknowledge Flags (1 byte)
    *session_present = mqtt_message[index] & 0x01;
    index++;

    // Reason Code (1 byte)
    *reason_code = mqtt_message[index];

    // reason_code 0x00 = Success, anything else = refused
    if (*reason_code != 0x00) return -3;

    return 0;
}

/******************************************
* Parse incoming PUBLISH message (QoS 0)
*
* PUBLISH structure (QoS 0):
*   [0x30 | flags][Remaining Length VBI]
*   [Topic Length MSB][Topic Length LSB][Topic]
*   [Properties Length VBI][Properties...]
*   [Payload]
*
* Extracts topic and payload into caller-provided buffers.
* topic_len_out and payload_len_out receive actual lengths.
*
* Returns: 0 on success, negative on error
*   -1 = NULL parameter
*   -2 = not a PUBLISH packet
*   -3 = topic buffer too small
*   -4 = payload buffer too small
******************************************/
int mqtt_v5_parse_publish(const uint8_t *mqtt_message, uint16_t msg_len,
                          char *topic, uint16_t topic_buf_size,
                          uint8_t *payload, uint16_t payload_buf_size,
                          uint16_t *topic_len_out, uint16_t *payload_len_out) {
    if (!mqtt_message || !topic || !payload || !topic_len_out || !payload_len_out) return -1;

    // Check packet type (high nibble of byte 0, mask out flags)
    if ((mqtt_message[0] & 0xF0) != MQTT_PUBLISH) return -2;

    uint16_t index = 1;

    // Decode remaining length (VBI)
    uint32_t remaining_length;
    int vbi_len = mqtt_v5_decode_vbi(&mqtt_message[index], &remaining_length);
    if (vbi_len < 0) return -1;
    index += vbi_len;

    uint16_t payload_start_offset = index; // Start of variable header

    // Topic Length (2 bytes)
    uint16_t topic_length = ((uint16_t)mqtt_message[index] << 8) | mqtt_message[index + 1];
    index += 2;

    // Check QoS from fixed header flags (bits 1-2)
    uint8_t qos = (mqtt_message[0] >> 1) & 0x03;

    // Topic
    if (topic_length >= topic_buf_size) return -3;
    memcpy(topic, &mqtt_message[index], topic_length);
    topic[topic_length] = '\0';
    *topic_len_out = topic_length;
    index += topic_length;

    // Packet Identifier (only present for QoS 1 and 2)
    if (qos > 0) {
        index += 2; // Skip packet identifier
    }

    // Properties Length (VBI) - skip properties
    uint32_t props_length;
    vbi_len = mqtt_v5_decode_vbi(&mqtt_message[index], &props_length);
    if (vbi_len < 0) return -1;
    index += vbi_len;
    index += props_length;

    // Payload = everything remaining
    uint16_t payload_length = remaining_length - (index - payload_start_offset);
    if (payload_length >= payload_buf_size) return -4;
    memcpy(payload, &mqtt_message[index], payload_length);
    payload[payload_length] = '\0';
    *payload_len_out = payload_length;

    return 0;
}

/******************************************
* Check if a message is a PINGRESP
*
* PINGRESP: [0xD0][0x00]
*
* Returns: 1 if PINGRESP, 0 otherwise
******************************************/
int mqtt_v5_is_pingresp(const uint8_t *mqtt_message) {
    if (!mqtt_message) return 0;
    return (mqtt_message[0] == MQTT_PINGRESP && mqtt_message[1] == 0x00) ? 1 : 0;
}
