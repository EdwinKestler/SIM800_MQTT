// Full_loop_testing_mqtt_v5.ino
// Description: This example demonstrates a full loop testing scenario using the MQTT 5.0 protocol with the SIM900 module.
// 

#include <mqtt_v5.h>
#include <SoftwareSerial.h>

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM900(SIM_TX, SIM_RX);

extern uint8_t preallocated_mqtt_buffer[];

// Define MQTT topic and payload
const char *topic = "test/topic";
const char *payload = "{\"message\": \"Hello, MQTT!\"}";

void setup() {
  Serial.begin(9600);
  SIM900.begin(9600);

  // Allow time for SIM900 initialization
  delay(3000);

  // Connect to the MQTT broker
  connect_to_broker();

  // Subscribe to the topic
  subscribe_to_topic();
}

void loop() {
  // Publish a message
  publish_message();

  // Check for incoming messages
  check_for_incoming_messages();

  delay(5000); // Wait 5 seconds before next loop iteration
}

void connect_to_broker() {
  Serial.println("Connecting to MQTT broker...");
  mqtt_property connect_properties[1];
  uint8_t session_expiry[4] = {0, 0, 0, 60}; // Session Expiry Interval: 60 seconds
  connect_properties[0].property_id = 0x11;
  connect_properties[0].length = 4;
  connect_properties[0].value = session_expiry;

  int result = mqtt_v5_connect_message(preallocated_mqtt_buffer, "arduino_client", connect_properties, 1);
  if (result != 0) {
    Serial.println("Error constructing CONNECT message.");
    return;
  }

  send_message(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
  Serial.println("CONNECT message sent.");
}

void subscribe_to_topic() {
  Serial.print("Subscribing to topic: ");
  Serial.println(topic);

  mqtt_property subscribe_properties[1];
  uint8_t subscription_identifier[2] = {0x00, 0x01}; // Subscription Identifier
  subscribe_properties[0].property_id = 0x0B;
  subscribe_properties[0].length = 2;
  subscribe_properties[0].value = subscription_identifier;

  int result = mqtt_v5_subscribe_message(preallocated_mqtt_buffer, topic, 1, subscribe_properties, 1);
  if (result != 0) {
    Serial.println("Error constructing SUBSCRIBE message.");
    return;
  }

  send_message(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
  Serial.println("SUBSCRIBE message sent.");
}

void publish_message() {
  Serial.print("Publishing message to topic: ");
  Serial.println(topic);

  mqtt_property publish_properties[1];
  uint8_t content_type[] = "application/json"; // Content type property
  publish_properties[0].property_id = 0x03;
  publish_properties[0].length = strlen((char *)content_type);
  publish_properties[0].value = content_type;

  int result = mqtt_v5_publish_message(preallocated_mqtt_buffer, topic, payload, publish_properties, 1);
  if (result != 0) {
    Serial.println("Error constructing PUBLISH message.");
    return;
  }

  send_message(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
  Serial.println("PUBLISH message sent.");
}

void check_for_incoming_messages() {
  if (SIM900.available()) {
    uint8_t incoming_message[MQTT_BUFFER_SIZE];
    int len = SIM900.readBytes(incoming_message, MQTT_BUFFER_SIZE);

    if (incoming_message[0] == MQTT_SUBACK) {
      uint8_t packet_id;
      uint8_t return_code;
      if (mqtt_v5_parse_suback_message(incoming_message, &packet_id, &return_code) == 0) {
        Serial.print("SUBACK received. Packet ID: ");
        Serial.print(packet_id);
        Serial.print(", Return Code: ");
        Serial.println(return_code);
      }
    } else if (incoming_message[0] == MQTT_PUBLISH) {
      // Handle incoming PUBLISH messages
      uint16_t topic_length = (incoming_message[2] << 8) | incoming_message[3];
      char received_topic[topic_length + 1];
      memcpy(received_topic, &incoming_message[4], topic_length);
      received_topic[topic_length] = '\0';

      uint16_t payload_start = 4 + topic_length;
      uint16_t payload_length = len - payload_start;
      char received_payload[payload_length + 1];
      memcpy(received_payload, &incoming_message[payload_start], payload_length);
      received_payload[payload_length] = '\0';

      Serial.print("Message received on topic ");
      Serial.print(received_topic);
      Serial.print(": ");
      Serial.println(received_payload);
    }
  }
}

void send_message(const uint8_t *message, uint16_t length) {
  SIM900.write(message, length);
}
