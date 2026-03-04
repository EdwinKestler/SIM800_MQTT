// FullLoopSIM800 example
// Full loop with GPRS configuration, connect, subscribe, publish on SIM800.

#include <YuaMQTT.h>
#include <SoftwareSerial.h>

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM800(SIM_TX, SIM_RX);

#define MAX_MESSAGE_SIZE 128
char inputBuffer[MAX_MESSAGE_SIZE];
volatile uint16_t bufferIndex = 0;

#define CHUNK_SIZE 32

void readSerialChunks() {
  while (SIM800.available()) {
    char chunk[CHUNK_SIZE + 1] = {0};
    int len = SIM800.readBytes(chunk, CHUNK_SIZE);
    chunk[len] = '\0';

    if (bufferIndex + len < MAX_MESSAGE_SIZE) {
      memcpy(inputBuffer + bufferIndex, chunk, len);
      bufferIndex += len;
      inputBuffer[bufferIndex] = '\0';
    } else {
      Serial.println("Error: Buffer Overflow in readSerialChunks");
      memset(inputBuffer, 0, MAX_MESSAGE_SIZE);
      bufferIndex = 0;
      break;
    }
  }
}

// Forward declarations
void sendATCommand(const char *command, const char *expectedResponse = "OK", unsigned long timeout = 2000);

// Configuration for GPRS and MQTT
const char *apn = "your-apn";  // Replace with your APN
const char *mqtt_broker = "mqtt.example.com";  // Replace with your MQTT broker
const int mqtt_port = 1883;
const char *client_id = "arduino_client";
const char *topic_publish = "test/publish";
const char *topic_subscribe = "test/subscribe";

const char *payload = "{\"message\": \"Hello, MQTT V5!\"}";

bool gprsReady = false;

void setup() {
  Serial.begin(9600);
  SIM800.begin(9600);

  Serial.println("Initializing SIM800...");

  sendATCommand("AT");
  sendATCommand("AT+CSQ");

  gprsReady = configureGPRS();
  if (!gprsReady) {
    Serial.println("GPRS configuration failed. Restarting...");
    while (1);
  }

  connectToMQTTBroker();
  subscribeToTopic(topic_subscribe);
  publishMessage(topic_publish, payload);
}

void loop() {
  static unsigned long lastPublishTime = 0;
  if (millis() - lastPublishTime > 5000) {
    publishMessage(topic_publish, "{\"message\": \"Periodic Hello, MQTT V5!\"}");
    lastPublishTime = millis();
  }

  readSerialChunks();
  processMQTTMessages();
}

void sendATCommand(const char *command, const char *expectedResponse = "OK", unsigned long timeout = 2000) {
  Serial.print("Sending: ");
  Serial.println(command);

  SIM800.println(command);
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (SIM800.available()) {
      String response = SIM800.readString();
      Serial.println("Response: " + response);
      if (response.indexOf(expectedResponse) != -1) {
        return;
      }
    }
  }
  Serial.println("Command timeout or failed.");
}

bool configureGPRS() {
  Serial.println("Configuring GPRS...");

  String command = String("AT+CGDCONT=1,\"IP\",\"") + apn + "\"";
  sendATCommand(command.c_str());
  sendATCommand("AT+CGACT=1,1");
  sendATCommand("AT+CGREG?");
  return true;
}

void connectToMQTTBroker() {
  Serial.println("Connecting to MQTT broker...");

  mqtt_property connect_properties[1];
  uint8_t session_expiry[4] = {0, 0, 0, 60};
  connect_properties[0].property_id = 0x11;
  connect_properties[0].length = 4;
  connect_properties[0].value = session_expiry;

  int len = mqtt_v5_connect_message(preallocated_mqtt_buffer, client_id, MQTT_DEFAULT_KEEP_ALIVE, connect_properties, 1);
  if (len > 0) {
    sendMQTTMessage(preallocated_mqtt_buffer, len);
    Serial.println("CONNECT message sent.");
  } else {
    Serial.println("Error constructing CONNECT message.");
  }
}

void subscribeToTopic(const char *topic) {
  Serial.print("Subscribing to topic: ");
  Serial.println(topic);

  mqtt_property subscribe_properties[1];
  uint8_t subscription_identifier[2] = {0x00, 0x01};
  subscribe_properties[0].property_id = 0x0B;
  subscribe_properties[0].length = 2;
  subscribe_properties[0].value = subscription_identifier;

  int len = mqtt_v5_subscribe_message(preallocated_mqtt_buffer, 1, topic, 1, subscribe_properties, 1);
  if (len > 0) {
    sendMQTTMessage(preallocated_mqtt_buffer, len);
    Serial.println("SUBSCRIBE message sent.");
  } else {
    Serial.println("Error constructing SUBSCRIBE message.");
  }
}

void publishMessage(const char *topic, const char *message) {
  Serial.print("Publishing message to topic: ");
  Serial.println(topic);

  mqtt_property publish_properties[1];
  uint8_t content_type[] = "application/json";
  publish_properties[0].property_id = 0x03;
  publish_properties[0].length = strlen((char *)content_type);
  publish_properties[0].value = content_type;

  int len = mqtt_v5_publish_message(preallocated_mqtt_buffer, topic, message, publish_properties, 1);
  if (len > 0) {
    sendMQTTMessage(preallocated_mqtt_buffer, len);
    Serial.println("PUBLISH message sent.");
  } else {
    Serial.println("Error constructing PUBLISH message.");
  }
}

void sendMQTTMessage(const uint8_t *message, uint16_t length) {
  SIM800.println("AT+CIPSEND");
  delay(500);
  SIM800.write(message, length);
  SIM800.write((char)26);
}

void processMQTTMessages() {
  if (bufferIndex > 0) {
    Serial.print("Processing MQTT Message: ");
    Serial.println(inputBuffer);

    memset(inputBuffer, 0, MAX_MESSAGE_SIZE);
    bufferIndex = 0;
  }
}
