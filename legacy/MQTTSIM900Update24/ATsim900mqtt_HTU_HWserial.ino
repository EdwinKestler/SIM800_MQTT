// ATSIM900MQTT_HTU.ino
// Updated to use hardware serial for communication with SIM900

#include <mqtt.h> // Include custom MQTT library

// Use the default hardware serial port for SIM900 communication
#define SIM900 Serial1

extern uint8_t preallocated_mqtt_buffer[]; // Use the preallocated MQTT buffer from mqtt.cpp

const char *client_id = "arduino_client"; // MQTT client identifier
const char *topic = "sensor/data";        // MQTT topic to publish
const char *broker_ip = "192.168.1.100";  // MQTT broker IP address
const int broker_port = 1883;             // MQTT broker port

void setup() {
  Serial.begin(9600);  // Initialize Serial for debugging
  SIM900.begin(9600);  // Initialize hardware Serial for SIM900 communication

  // Wait for the SIM900 to initialize
  Serial.println("Initializing SIM900...");
  unsigned long start_time = millis();
  while (millis() - start_time < 5000) { // Non-blocking delay for 5 seconds
    if (SIM900.available()) {
      // Check for any response from SIM900 during initialization
      Serial.write(SIM900.read());
    }
  }

  // Connect to the broker
  Serial.println("Connecting to MQTT broker...");
  mqtt_connect();
}

void loop() {
  // Publish dummy sensor data periodically
  static unsigned long last_publish_time = 0; // Track last publish time
  if (millis() - last_publish_time > 5000) { // Publish every 5 seconds
    publish_sensor_data();
    last_publish_time = millis(); // Update last publish time
  }
}

void mqtt_connect() {
  // Prepare and send MQTT CONNECT message
  int result = mqtt_connect_message(preallocated_mqtt_buffer, client_id);
  if (result != 0) {
    Serial.println("Error constructing MQTT CONNECT message.");
    return;
  }

  send_message_to_sim(preallocated_mqtt_buffer, 16 + strlen(client_id));
  Serial.println("MQTT CONNECT message sent.");
}

void publish_sensor_data() {
  // Prepare dummy sensor data
  const char *sensor_data = "{\"temperature\":25,\"humidity\":60}";

  // Prepare and send MQTT PUBLISH message
  int result = mqtt_publish_message(preallocated_mqtt_buffer, topic, sensor_data);
  if (result != 0) {
    Serial.println("Error constructing MQTT PUBLISH message.");
    return;
  }

  send_message_to_sim(preallocated_mqtt_buffer, 2 + strlen(topic) + strlen(sensor_data));
  Serial.println("MQTT PUBLISH message sent.");
}

void send_message_to_sim(const uint8_t *message, uint16_t length) {
  // Function to send MQTT message via SIM900
  if (message == nullptr || length == 0) {
    Serial.println("Error: Message is null or length is zero.");
    return;
  }

  int bytes_sent = SIM900.write(message, length); // Send message to SIM module
  if (bytes_sent != length) {
    Serial.println("Error: Message not fully sent to SIM900 module.");
    return;
  }

  Serial.print("Message sent to SIM900: ");
  for (uint16_t i = 0; i < length; i++) {
    Serial.print(message[i], HEX); // Print message in HEX format
    Serial.print(" ");
  }
  Serial.println();
}
