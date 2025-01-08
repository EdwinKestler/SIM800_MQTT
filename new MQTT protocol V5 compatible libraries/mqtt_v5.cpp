/******************************************
* mqtt_v5.cpp
* This source file implements MQTT 5.0 compatibility.
******************************************/

#include "mqtt_v5.h"

// MQTT CONNECT message construction
int mqtt_v5_connect_message(uint8_t *mqtt_message, const char *client_id, mqtt_property *properties, uint8_t property_count) {
    if (!mqtt_message || !client_id) {
        fprintf(stderr, "Error: NULL parameter provided.\n");
        return -1;
    }

    uint8_t client_id_length = strlen(client_id);
    uint16_t remaining_length = 14 + client_id_length;

    // Calculate properties length
    uint16_t properties_length = 0;
    for (uint8_t i = 0; i < property_count; i++) {
        properties_length += 1 + 2 + properties[i].length; // ID + Length + Value
    }
    remaining_length += 2 + properties_length; // Properties length field

    if (remaining_length > MQTT_BUFFER_SIZE) {
        fprintf(stderr, "Error: Buffer size exceeded in mqtt_v5_connect_message.\n");
        return -2;
    }

    mqtt_message[0] = MQTT_CONNECT;
    mqtt_message[1] = remaining_length;

    mqtt_message[2] = 0;
    mqtt_message[3] = 4; // Protocol name length
    memcpy(&mqtt_message[4], "MQTT", 4);
    mqtt_message[8] = MQTT_PROTOCOL_VERSION_5;
    mqtt_message[9] = 2; // Connection flags
    mqtt_message[10] = 0;
    mqtt_message[11] = 15; // Keep-alive

    mqtt_message[12] = properties_length >> 8;
    mqtt_message[13] = properties_length & 0xFF;

    uint16_t index = 14;
    for (uint8_t i = 0; i < property_count; i++) {
        mqtt_message[index++] = properties[i].property_id;
        mqtt_message[index++] = (properties[i].length >> 8) & 0xFF;
        mqtt_message[index++] = properties[i].length & 0xFF;
        memcpy(&mqtt_message[index], properties[i].value, properties[i].length);
        index += properties[i].length;
    }

    mqtt_message[index++] = 0;
    mqtt_message[index++] = client_id_length;

    memcpy(&mqtt_message[index], client_id, client_id_length);
    return 0;
}

// MQTT PUBLISH message construction
int mqtt_v5_publish_message(uint8_t *mqtt_message, const char *topic, const char *message, mqtt_property *properties, uint8_t property_count) {
    if (!mqtt_message || !topic || !message) {
        fprintf(stderr, "Error: NULL parameter provided.\n");
        return -1;
    }

    uint8_t topic_length = strlen(topic);
    uint8_t message_length = strlen(message);
    uint16_t remaining_length = 2 + topic_length + message_length;

    // Calculate properties length
    uint16_t properties_length = 0;
    for (uint8_t i = 0; i < property_count; i++) {
        properties_length += 1 + 2 + properties[i].length; // ID + Length + Value
    }
    remaining_length += 2 + properties_length; // Properties length field

    if (remaining_length > MQTT_BUFFER_SIZE) {
        fprintf(stderr, "Error: Buffer size exceeded in mqtt_v5_publish_message.\n");
        return -2;
    }

    mqtt_message[0] = MQTT_PUBLISH;
    mqtt_message[1] = remaining_length;

    mqtt_message[2] = 0;
    mqtt_message[3] = topic_length;

    memcpy(&mqtt_message[4], topic, topic_length);
    memcpy(&mqtt_message[4 + topic_length], message, message_length);

    return 0;
}

// MQTT DISCONNECT message construction
int mqtt_v5_disconnect_message(uint8_t *mqtt_message) {
    if (!mqtt_message) {
        fprintf(stderr, "Error: NULL parameter provided.\n");
        return -1;
    }

    mqtt_message[0] = MQTT_DISCONNECT;
    mqtt_message[1] = 0;

    return 0;
}
