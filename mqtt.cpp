
/*
* this code was last updatesd by Edwin Kestler on 7/1/2024
* this code is mqtt.cpp
* 
* Key Updates:
	Buffer size validation:

	Checked if the constructed message exceeds MQTT_BUFFER_SIZE.
	Return error codes for buffer overflow or NULL parameters.
	Constants for magic numbers:

	Replaced magic numbers (e.g., 0x10, 0x30) with named constants like MQTT_CONNECT and MQTT_PUBLISH.
	Error logging and codes:

	Added error messages via fprintf(stderr, ...).
	Return distinct error codes for different failure cases (-1 for NULL parameters, -2 for buffer overflow).
* Optimized for performance: memory reuse, use of memcpy, reduced fragmentation
*/

#include "mqtt.h"

// Preallocated buffer to minimize temporary allocations
static uint8_t preallocated_mqtt_buffer[MQTT_BUFFER_SIZE];

int mqtt_connect_message(uint8_t *mqtt_message, const char *client_id) {
    if (!mqtt_message || !client_id) {
        fprintf(stderr, "Error: NULL parameter provided.\n");
        return -1; // Error code for NULL parameter
    }

    uint8_t client_id_length = strlen(client_id);
    if (14 + client_id_length > MQTT_BUFFER_SIZE) {
        fprintf(stderr, "Error: Buffer size exceeded in mqtt_connect_message.\n");
        return -2; // Error code for buffer overflow
    }

    mqtt_message[0] = MQTT_CONNECT;
    mqtt_message[1] = 14 + client_id_length;

    mqtt_message[2] = 0;
    mqtt_message[3] = 6;
    memcpy(&mqtt_message[4], "MQIsdp", 6);
    mqtt_message[10] = MQTT_PROTOCOL_VERSION;
    mqtt_message[11] = 2; // Connection flags
    mqtt_message[12] = 0;
    mqtt_message[13] = 15; // Keep-alive

    mqtt_message[14] = 0;
    mqtt_message[15] = client_id_length;

    memcpy(&mqtt_message[16], client_id, client_id_length);

    return 0; // Success
}

int mqtt_publish_message(uint8_t *mqtt_message, const char *topic, const char *message) {
    if (!mqtt_message || !topic || !message) {
        fprintf(stderr, "Error: NULL parameter provided.\n");
        return -1; // Error code for NULL parameter
    }

    uint8_t topic_length = strlen(topic);
    uint8_t message_length = strlen(message);
    if (2 + topic_length + message_length > MQTT_BUFFER_SIZE) {
        fprintf(stderr, "Error: Buffer size exceeded in mqtt_publish_message.\n");
        return -2; // Error code for buffer overflow
    }

    mqtt_message[0] = MQTT_PUBLISH;
    mqtt_message[1] = 2 + topic_length + message_length;

    mqtt_message[2] = 0;
    mqtt_message[3] = topic_length;

    memcpy(&mqtt_message[4], topic, topic_length);
    memcpy(&mqtt_message[4 + topic_length], message, message_length);

    return 0; // Success
}

int mqtt_disconnect_message(uint8_t *mqtt_message) {
    if (!mqtt_message) {
        fprintf(stderr, "Error: NULL parameter provided.\n");
        return -1; // Error code for NULL parameter
    }

    mqtt_message[0] = MQTT_DISCONNECT;
    mqtt_message[1] = 0;

    return 0; // Success
}