// FullLoopTest example
// Minimal functional MQTT V5 session: CONNECT, verify CONNACK, SUBSCRIBE,
// PUBLISH, parse incoming PUBLISH, and PINGREQ keep-alive.

#include <mqtt_v5.h>
#include <SoftwareSerial.h>

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM800(SIM_TX, SIM_RX);

#define MAX_MESSAGE_SIZE 128
uint8_t inputBuffer[MAX_MESSAGE_SIZE];
volatile uint16_t bufferIndex = 0;

#define CHUNK_SIZE 32

const char *topic = "test/topic";
const char *payload = "{\"message\": \"Hello, MQTT V5!\"}";

bool mqttConnected = false;
unsigned long lastPingTime = 0;

void setup() {
  Serial.begin(9600);
  SIM800.begin(9600);

  Serial.println("Initializing SIM800...");
  delay(3000);

  // Step 1: Send CONNECT
  connectToMQTTBroker();

  // Step 2: Wait for CONNACK
  delay(2000);
  readSerialData();
  if (bufferIndex > 0) {
    uint8_t session_present, reason_code;
    int result = mqtt_v5_parse_connack(inputBuffer, &session_present, &reason_code);
    if (result == 0) {
      Serial.println("CONNACK: Connected successfully!");
      Serial.print("  Session Present: ");
      Serial.println(session_present);
      mqttConnected = true;
    } else if (result == -3) {
      Serial.print("CONNACK: Connection refused, reason code: 0x");
      Serial.println(reason_code, HEX);
    } else {
      Serial.println("No valid CONNACK received.");
    }
    resetBuffer();
  }

  if (!mqttConnected) {
    Serial.println("Failed to connect. Halting.");
    while (1);
  }

  // Step 3: Subscribe
  subscribeToTopic();
  lastPingTime = millis();
}

void loop() {
  // Publish every 5 seconds
  static unsigned long lastPublishTime = 0;
  if (millis() - lastPublishTime > 5000) {
    publishMessage();
    lastPublishTime = millis();
  }

  // Send PINGREQ every 10 seconds (keep-alive is 15s)
  if (millis() - lastPingTime > 10000) {
    int len = mqtt_v5_pingreq_message(preallocated_mqtt_buffer);
    if (len > 0) {
      sendMQTTMessage(preallocated_mqtt_buffer, len);
      Serial.println("PINGREQ sent.");
    }
    lastPingTime = millis();
  }

  // Read and process incoming data
  readSerialData();
  processIncoming();
}

void readSerialData() {
  while (SIM800.available()) {
    uint8_t chunk[CHUNK_SIZE + 1];
    int len = SIM800.readBytes(chunk, CHUNK_SIZE);

    if (bufferIndex + len < MAX_MESSAGE_SIZE) {
      memcpy(inputBuffer + bufferIndex, chunk, len);
      bufferIndex += len;
    } else {
      Serial.println("Error: Buffer Overflow");
      resetBuffer();
      break;
    }
  }
}

void resetBuffer() {
  memset(inputBuffer, 0, MAX_MESSAGE_SIZE);
  bufferIndex = 0;
}

void processIncoming() {
  if (bufferIndex == 0) return;

  // Check what type of packet we received
  uint8_t packet_type = inputBuffer[0] & 0xF0;

  if (mqtt_v5_is_pingresp(inputBuffer)) {
    Serial.println("PINGRESP received - connection alive.");
  }
  else if (packet_type == MQTT_PUBLISH) {
    // Parse incoming PUBLISH
    char rx_topic[64];
    uint8_t rx_payload[128];
    uint16_t rx_topic_len, rx_payload_len;

    int result = mqtt_v5_parse_publish(inputBuffer, bufferIndex,
                   rx_topic, sizeof(rx_topic),
                   rx_payload, sizeof(rx_payload),
                   &rx_topic_len, &rx_payload_len);
    if (result == 0) {
      Serial.print("Received PUBLISH on topic: ");
      Serial.println(rx_topic);
      Serial.print("Payload: ");
      Serial.println((char *)rx_payload);
    } else {
      Serial.print("Failed to parse PUBLISH, error: ");
      Serial.println(result);
    }
  }
  else if (packet_type == MQTT_SUBACK) {
    uint16_t packet_id;
    uint8_t reason_code;
    if (mqtt_v5_parse_suback_message(inputBuffer, &packet_id, &reason_code) == 0) {
      Serial.print("SUBACK: Packet ID=");
      Serial.print(packet_id);
      Serial.print(", Reason=0x");
      Serial.println(reason_code, HEX);
    }
  }
  else {
    Serial.print("Unknown packet type: 0x");
    Serial.println(packet_type, HEX);
  }

  resetBuffer();
}

void connectToMQTTBroker() {
  Serial.println("Sending CONNECT...");
  mqtt_property connect_properties[1];
  uint8_t session_expiry[4] = {0, 0, 0, 60};
  connect_properties[0].property_id = 0x11;
  connect_properties[0].length = 4;
  connect_properties[0].value = session_expiry;

  int len = mqtt_v5_connect_message(preallocated_mqtt_buffer, "arduino_client", connect_properties, 1);
  if (len > 0) {
    sendMQTTMessage(preallocated_mqtt_buffer, len);
    Serial.println("CONNECT sent.");
  } else {
    Serial.println("Error constructing CONNECT.");
  }
}

void subscribeToTopic() {
  Serial.print("Subscribing to: ");
  Serial.println(topic);

  int len = mqtt_v5_subscribe_message(preallocated_mqtt_buffer, topic, 1, NULL, 0);
  if (len > 0) {
    sendMQTTMessage(preallocated_mqtt_buffer, len);
    Serial.println("SUBSCRIBE sent.");
  } else {
    Serial.println("Error constructing SUBSCRIBE.");
  }
}

void publishMessage() {
  int len = mqtt_v5_publish_message(preallocated_mqtt_buffer, topic, payload, NULL, 0);
  if (len > 0) {
    sendMQTTMessage(preallocated_mqtt_buffer, len);
    Serial.println("PUBLISH sent.");
  }
}

void sendMQTTMessage(const uint8_t *message, uint16_t length) {
  SIM800.println("AT+CIPSEND");
  delay(500);
  SIM800.write(message, length);
  SIM800.write((char)26);
}
