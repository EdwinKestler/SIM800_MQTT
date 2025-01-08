// ATSIM900MQTT_HTU.ino
// description: This example demonstrates how to use the MQTT library with the SIM900 module to subscribe to an MQTT topic using the MQTT 5.0 protocol.


#include <mqtt_v5.h>
#include <SoftwareSerial.h>

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM900(SIM_TX, SIM_RX);

extern uint8_t preallocated_mqtt_buffer[];

const char *topic_to_subscribe = "sensor/data";

void setup() {
    Serial.begin(9600);
    SIM900.begin(9600);

    // Connect to MQTT broker
    mqtt_property connect_properties[1];
    uint8_t session_expiry[4] = {0, 0, 0, 60};
    connect_properties[0].property_id = 0x11;
    connect_properties[0].length = 4;
    connect_properties[0].value = session_expiry;
    mqtt_v5_connect_message(preallocated_mqtt_buffer, "arduino_client", connect_properties, 1);
    send_message(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));

    // Subscribe to a topic
    mqtt_property subscribe_properties[1];
    uint8_t subscription_identifier[2] = {0x00, 0x01}; // Subscription Identifier
    subscribe_properties[0].property_id = 0x0B;
    subscribe_properties[0].length = 2;
    subscribe_properties[0].value = subscription_identifier;
    mqtt_v5_subscribe_message(preallocated_mqtt_buffer, topic_to_subscribe, 1, subscribe_properties, 1);
    send_message(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
}

void loop() {
    // Handle incoming SUBACK response
    if (SIM900.available()) {
        uint8_t incoming_message[MQTT_BUFFER_SIZE];
        int len = SIM900.readBytes(incoming_message, MQTT_BUFFER_SIZE);

        uint8_t packet_id;
        uint8_t return_code;
        if (mqtt_v5_parse_suback_message(incoming_message, &packet_id, &return_code) == 0) {
            Serial.print("SUBACK received. Packet ID: ");
            Serial.print(packet_id);
            Serial.print(", Return Code: ");
            Serial.println(return_code);
        }
    }
}

void send_message(const uint8_t *message, uint16_t length) {
    SIM900.write(message, length);
}
