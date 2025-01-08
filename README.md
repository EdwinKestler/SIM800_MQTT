# SIM800 MQTT Library

## Description
This Arduino library enables communication with MQTT brokers via a SIM800/SIM900 GPRS module. It provides functionality for sending MQTT CONNECT, PUBLISH, and DISCONNECT messages, tailored for lightweight embedded systems such as Arduino boards.

## Features
- **MQTT Protocol Support**: Send CONNECT, PUBLISH, and DISCONNECT messages.
- **Hardware Serial Support**: Uses hardware UART (`Serial1`) for efficient communication on boards with multiple serial ports.
- **Buffer Optimizations**: Preallocated MQTT message buffer for efficient memory management.
- **Error Handling**: Returns error codes for invalid parameters or buffer overflows, and logs messages for debugging.

## Recent Updates
1. **Performance Optimizations**:
   - Replaced manual loops with `memcpy` for copying data into buffers.
   - Introduced a preallocated static buffer to minimize memory fragmentation and improve performance.
   - Reduced temporary allocations by reusing a single buffer across all MQTT operations.
2. **Support for Hardware Serial**:
   - Dedicated `Serial1` for SIM900 communication on boards with multiple UARTs.
   - Compatibility maintained with single UART boards by allowing configurable serial handling.
3. **Enhanced Documentation**:
   - Added detailed usage instructions and examples.
   - Expanded troubleshooting and best practices sections.

## Installation
1. Download the library and add it to your Arduino IDE:
   - Navigate to `Sketch` -> `Include Library` -> `Add .ZIP Library...`
2. Include the library in your sketch:
   ```cpp
   #include <mqtt.h>
   ```

## Usage

### Basic Setup
```cpp
#include <mqtt.h>

#define SIM900 Serial1 // Use Serial1 for SIM900 communication

extern uint8_t preallocated_mqtt_buffer[];

void setup() {
    Serial.begin(9600);  // Debugging
    SIM900.begin(9600);  // Initialize SIM900 communication

    // Connect to MQTT broker
    mqtt_connect_message(preallocated_mqtt_buffer, "arduino_client");
    send_message_to_sim(preallocated_mqtt_buffer, 16 + strlen("arduino_client"));
}

void loop() {
    // Publish messages periodically
    mqtt_publish_message(preallocated_mqtt_buffer, "sensor/data", "{\"temperature\":25,\"humidity\":60}");
    send_message_to_sim(preallocated_mqtt_buffer, 2 + strlen("sensor/data") + strlen("{\"temperature\":25,\"humidity\":60}"));
    delay(5000);
}

void send_message_to_sim(const uint8_t *message, uint16_t length) {
    SIM900.write(message, length);
}
```

### Supported Boards
- **Single UART Boards**:
   - Arduino Uno, Nano (requires `SoftwareSerial` for SIM900 communication).
- **Multi-UART Boards**:
   - Arduino Mega, Leonardo, Due (recommended for better performance with `Serial1`).

## Best Practices
1. **Use Hardware UART**:
   - For boards with multiple UARTs, dedicate a hardware serial port to the SIM900 module for optimal performance.
2. **Manage Shared Serial Ports**:
   - For single UART boards, disconnect the SIM900 module while uploading code or debugging via USB.
3. **Test with Public MQTT Brokers**:
   - Use brokers like `test.mosquitto.org` for initial testing.

## Troubleshooting
- **SIM900 Not Responding**:
   - Ensure proper wiring and power supply to the module.
   - Use `AT` commands to test module responsiveness before running the sketch.
- **MQTT Connection Issues**:
   - Verify the broker IP, port, and network connectivity.
   - Ensure the SIM card has an active data plan.

## Future Enhancements
1. **QoS Support**: Add quality of service levels to MQTT PUBLISH messages.
2. **TLS Security**: Integrate SSL/TLS for secure MQTT communication.
3. **SUBSCRIBE Support**: Enable parsing and handling of incoming MQTT messages.

## License
This library is open-source and licensed under the MIT License. Feel free to modify and use it in your projects.

## Acknowledgments
Special thanks to Edwin Kestler for recent contributions and optimizations to improve performance and usability.
