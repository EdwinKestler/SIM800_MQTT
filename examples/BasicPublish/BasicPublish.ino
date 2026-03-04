// BasicPublish example
// Demonstrates publishing sensor data via MQTT V5 using SIM900 module.

#include <YuaMQTT.h>
#include <SoftwareSerial.h>

#define SIM_TX 10
#define SIM_RX 11

SoftwareSerial SIM900(SIM_TX, SIM_RX);

const char *client_id = "arduino_client";
const char *topic = "sensor/data";

void setup() {
  Serial.begin(9600);
  SIM900.begin(9600);

  // Wait for initialization
  delay(3000);

  mqtt_property properties[1];
  uint8_t session_expiry_value[4] = {0, 0, 0, 60};
  properties[0].property_id = 0x11; // Session Expiry Interval
  properties[0].length = 4;
  properties[0].value = session_expiry_value;

  int len = mqtt_v5_connect_message(preallocated_mqtt_buffer, client_id, MQTT_DEFAULT_KEEP_ALIVE, properties, 1);
  if (len > 0) {
    send_message(preallocated_mqtt_buffer, len);
  }
}

void loop() {
  static unsigned long last_publish_time = 0;
  if (millis() - last_publish_time > 5000) {
    const char *sensor_data = "{\"temperature\":25,\"humidity\":60}";

    mqtt_property properties[1];
    uint8_t content_type_value[] = "application/json";
    properties[0].property_id = 0x03; // Content Type
    properties[0].length = strlen((char *)content_type_value);
    properties[0].value = content_type_value;

    int len = mqtt_v5_publish_message(preallocated_mqtt_buffer, topic, sensor_data, properties, 1);
    if (len > 0) {
      send_message(preallocated_mqtt_buffer, len);
      Serial.println("MQTT PUBLISH message sent.");
    } else {
      Serial.println("Error constructing MQTT PUBLISH message.");
    }

    last_publish_time = millis();
  }
}

void send_message(const uint8_t *message, uint16_t length) {
  SIM900.write(message, length);
}
