// HardwareSerialPublish example
// Demonstrates YuaMQTT with hardware UART (Serial1) for boards like
// Arduino Mega, Leonardo, Due, ESP32, STM32, RP2040.
//
// Wiring: SIM800/SIM900 TX → board RX1, SIM800/SIM900 RX → board TX1

#include <YuaMQTT.h>
#include <YuaMQTT_SIM800.h>

// Use hardware serial — no SoftwareSerial needed
YuaSIM800 sim(Serial1, &Serial);  // SIM800 on Serial1, debug on Serial

const char *apn = "your-apn";          // Replace with your carrier APN
const char *broker = "broker.example.com";
const uint16_t broker_port = 1883;

const char *client_id = "arduino_hw";
const char *topic = "sensor/data";

bool connected = false;

void setup() {
  Serial.begin(115200);   // Debug serial
  Serial1.begin(9600);    // SIM800 module

  Serial.println("YuaMQTT HardwareSerial example");
  delay(3000);

  // Initialize module
  if (!sim.begin()) {
    Serial.println("SIM800 not responding.");
    while (1);
  }

  int rssi = sim.signalQuality();
  Serial.print("Signal RSSI: ");
  Serial.println(rssi);

  // Establish GPRS and TCP
  if (!sim.gprsConnect(apn)) {
    Serial.println("GPRS failed.");
    while (1);
  }

  if (!sim.tcpConnect(broker, broker_port)) {
    Serial.println("TCP connect failed.");
    while (1);
  }

  // Send MQTT CONNECT
  int len = mqtt_v5_connect_message(preallocated_mqtt_buffer,
              client_id, MQTT_DEFAULT_KEEP_ALIVE, NULL, 0);
  if (len > 0 && sim.tcpSend(preallocated_mqtt_buffer, len)) {
    Serial.println("CONNECT sent.");
    connected = true;
  }
}

void loop() {
  if (!connected) return;

  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 5000) {
    const char *payload = "{\"temp\":25,\"humidity\":60}";

    int len = mqtt_v5_publish_message(preallocated_mqtt_buffer,
                topic, payload, NULL, 0);
    if (len > 0) {
      sim.tcpSend(preallocated_mqtt_buffer, len);
      Serial.println("PUBLISH sent.");
    }
    lastPublish = millis();
  }

  // Keep-alive
  static unsigned long lastPing = 0;
  if (millis() - lastPing > 10000) {
    int len = mqtt_v5_pingreq_message(preallocated_mqtt_buffer);
    if (len > 0) sim.tcpSend(preallocated_mqtt_buffer, len);
    lastPing = millis();
  }
}
