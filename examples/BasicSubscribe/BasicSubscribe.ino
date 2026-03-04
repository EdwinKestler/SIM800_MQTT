// BasicSubscribe example
// Demonstrates subscribing to an MQTT topic and parsing SUBACK via MQTT V5.

#include <YuaMQTT.h>
#include <SoftwareSerial.h>

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM900(SIM_TX, SIM_RX);

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

    int len = mqtt_v5_connect_message(preallocated_mqtt_buffer, "arduino_client", MQTT_DEFAULT_KEEP_ALIVE, connect_properties, 1);
    if (len > 0) {
        send_message(preallocated_mqtt_buffer, len);
    }

    // Subscribe to a topic
    mqtt_property subscribe_properties[1];
    uint8_t subscription_identifier[2] = {0x00, 0x01};
    subscribe_properties[0].property_id = 0x0B; // Subscription Identifier
    subscribe_properties[0].length = 2;
    subscribe_properties[0].value = subscription_identifier;

    len = mqtt_v5_subscribe_message(preallocated_mqtt_buffer, 1, topic_to_subscribe, 1, subscribe_properties, 1);
    if (len > 0) {
        send_message(preallocated_mqtt_buffer, len);
    }
}

void loop() {
    // Handle incoming SUBACK response
    if (SIM900.available()) {
        uint8_t incoming_message[MQTT_BUFFER_SIZE];
        int read_len = SIM900.readBytes(incoming_message, MQTT_BUFFER_SIZE);

        uint16_t packet_id;
        uint8_t reason_code;
        if (mqtt_v5_parse_suback_message(incoming_message, &packet_id, &reason_code) == 0) {
            Serial.print("SUBACK received. Packet ID: ");
            Serial.print(packet_id);
            Serial.print(", Reason Code: ");
            Serial.println(reason_code);
        }
    }
}

void send_message(const uint8_t *message, uint16_t length) {
    SIM900.write(message, length);
}
