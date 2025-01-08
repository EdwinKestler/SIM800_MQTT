// Full_loop_testing_mqtt_v5.ino
// Description: This example demonstrates a full loop testing scenario using the MQTT 5.0 protocol with the SIM900 module.
// 

#include <mqtt_v5.h>
#include <SoftwareSerial.h>

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM800(SIM_TX, SIM_RX);

#define MAX_MESSAGE_SIZE 128  // Maximum expected MQTT message size
char inputBuffer[MAX_MESSAGE_SIZE]; // Buffer to store incoming data
volatile uint16_t bufferIndex = 0; // Index to track position in the buffer

#define CHUNK_SIZE 32  // Read in chunks to avoid buffer overflow

void readSerialChunks() {
  while (SIM800.available()) {
    char chunk[CHUNK_SIZE + 1] = {0};  // Temporary buffer
    int len = SIM800.readBytes(chunk, CHUNK_SIZE);
    chunk[len] = '\0'; // Null-terminate

    // Append to the main buffer
    if (bufferIndex + len < MAX_MESSAGE_SIZE) {
      strcat(inputBuffer, chunk);
      bufferIndex += len;
    } else {
      Serial.println("Error: Buffer Overflow in readSerialChunks");
    }
  }
}

extern uint8_t preallocated_mqtt_buffer[];

// MQTT broker details
const char *topic = "test/topic";
const char *payload = "{\"message\": \"Hello, MQTT V5!\"}";

void setup() {
  Serial.begin(9600);
  SIM800.begin(9600);

  Serial.println("Initializing SIM800...");
  delay(3000);

  connectToMQTTBroker();
  subscribeToTopic();
}

void loop() {
  publishMessage();
  readSerialChunks(); // Check and read serial chunks
  processMQTTMessages(); // Process buffered messages
  delay(5000);
}

void connectToMQTTBroker() {
  Serial.println("Connecting to MQTT broker...");
  mqtt_property connect_properties[1];
  uint8_t session_expiry[4] = {0, 0, 0, 60};  // Session Expiry Interval: 60 seconds
  connect_properties[0].property_id = 0x11;
  connect_properties[0].length = 4;
  connect_properties[0].value = session_expiry;

  int result = mqtt_v5_connect_message(preallocated_mqtt_buffer, "arduino_client", connect_properties, 1);
  if (result != 0) {
    Serial.println("Error constructing CONNECT message.");
    return;
  }

  sendMQTTMessage(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
  Serial.println("CONNECT message sent.");
}

void subscribeToTopic() {
  Serial.print("Subscribing to topic: ");
  Serial.println(topic);

  mqtt_property subscribe_properties[1];
  uint8_t subscription_identifier[2] = {0x00, 0x01};  // Subscription Identifier
  subscribe_properties[0].property_id = 0x0B;
  subscribe_properties[0].length = 2;
  subscribe_properties[0].value = subscription_identifier;

  int result = mqtt_v5_subscribe_message(preallocated_mqtt_buffer, topic, 1, subscribe_properties, 1);
  if (result != 0) {
    Serial.println("Error constructing SUBSCRIBE message.");
    return;
  }

  sendMQTTMessage(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
  Serial.println("SUBSCRIBE message sent.");
}

void publishMessage() {
  Serial.print("Publishing message to topic: ");
  Serial.println(topic);

  mqtt_property publish_properties[1];
  uint8_t content_type[] = "application/json";  // Content type property
  publish_properties[0].property_id = 0x03;
  publish_properties[0].length = strlen((char *)content_type);
  publish_properties[0].value = content_type;

  int result = mqtt_v5_publish_message(preallocated_mqtt_buffer, topic, payload, publish_properties, 1);
  if (result != 0) {
    Serial.println("Error constructing PUBLISH message.");
    return;
  }

  sendMQTTMessage(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
  Serial.println("PUBLISH message sent.");
}

void sendMQTTMessage(const uint8_t *message, uint16_t length) {
  SIM800.println("AT+CIPSEND");
  delay(500);
  SIM800.write(message, length);
  SIM800.write((char)26);  // End of message (Ctrl+Z)
}

void processMQTTMessages() {
  if (bufferIndex > 0) {
    Serial.print("Processing MQTT Message: ");
    Serial.println(inputBuffer);

    // Reset the buffer after processing
    memset(inputBuffer, 0, MAX_MESSAGE_SIZE);
    bufferIndex = 0;
  }
}
