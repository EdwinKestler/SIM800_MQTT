#include <mqtt_v5.h> // Include the updated MQTT 5.0 library
#include <SoftwareSerial.h> // For communication with SIM900

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM900(SIM_TX, SIM_RX);

extern uint8_t preallocated_mqtt_buffer[];

const char *client_id = "arduino_client";
const char *topic = "sensor/data";

void setup() {
  Serial.begin(9600);
  SIM900.begin(9600);

  // Wait for initialization
  delay(3000);

  mqtt_property properties[1];
  uint8_t session_expiry_value[4] = {0, 0, 0, 60};
  properties[0].property_id = 0x11;
  properties[0].length = 4;
  properties[0].value = session_expiry_value;

  mqtt_v5_connect_message(preallocated_mqtt_buffer, client_id, properties, 1);
  send_message(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
}

void loop() {
  // Publish dummy sensor data periodically
  static unsigned long last_publish_time = 0; // Track last publish time
  if (millis() - last_publish_time > 5000) { // Publish every 5 seconds
    const char *sensor_data = "{\"temperature\":25,\"humidity\":60}";

    // Define optional properties for MQTT 5.0
    mqtt_property properties[1];
    uint8_t content_type_value[] = "application/json";
    properties[0].property_id = 0x03; // Content Type identifier
    properties[0].length = strlen((char *)content_type_value);
    properties[0].value = content_type_value;

    // Construct the MQTT PUBLISH message
    int result = mqtt_v5_publish_message(preallocated_mqtt_buffer, topic, sensor_data, properties, 1);
    if (result != 0) {
      Serial.println("Error constructing MQTT PUBLISH message.");
      return;
    }

    // Send the PUBLISH message to the SIM900 module
    send_message(preallocated_mqtt_buffer, 2 + strlen(topic) + strlen(sensor_data));
    Serial.println("MQTT PUBLISH message sent.");

    last_publish_time = millis(); // Update last publish time
  }
}


void send_message(const uint8_t *message, uint16_t length) {
  SIM900.write(message, length);
}
