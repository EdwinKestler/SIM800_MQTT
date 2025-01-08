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
4. **MQTT V5 Protocol Support**:
   - Introduced separate files for MQTT V5-compatible operations.
   - Added support for properties in CONNECT and PUBLISH messages.
   - Improved compatibility with brokers supporting MQTT V5.

## Installation

1. Download the library and add it to your Arduino IDE:
   - Navigate to `Sketch` -> `Include Library` -> `Add .ZIP Library...`
2. Include the library in your sketch:

```cpp

   #include <mqtt.h>

```

### For MQTT V5 Support

- The files for MQTT V5-compatible operations are located in the folder "new MQTT protocol V5 compatible libraries".
Include the V5 header file in your sketch:

```cpp
#include <mqtt_v5.h>
```

## Usage

## Basic Setup (MQTT 3.1.1)

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

## MQTT V5 Example

```cpp
#include <mqtt_v5.h>

extern uint8_t preallocated_mqtt_buffer[];

void setup() {
    Serial.begin(9600);
    SIM900.begin(9600);

    mqtt_property properties[1];
    uint8_t session_expiry_value[4] = {0, 0, 0, 60};
    properties[0].property_id = 0x11; // Session Expiry Interval
    properties[0].length = 4;
    properties[0].value = session_expiry_value;

    mqtt_v5_connect_message(preallocated_mqtt_buffer, "arduino_client", properties, 1);
    send_message_to_sim(preallocated_mqtt_buffer, strlen((char *)preallocated_mqtt_buffer));
}

void loop() {
    // Example PUBLISH logic with MQTT V5
}

```

## Supported Boards

**Single UART Boards**:

- Arduino Uno, Nano (requires SoftwareSerial for SIM900 communication).

**Multi-UART Boards**:

- Arduino Mega, Leonardo, Due (recommended for better performance with Serial1).

## Best Practices

**Use Hardware UART**:
    - For boards with multiple UARTs, dedicate a hardware serial port to the SIM900 module for optimal performance.

**Manage Shared Serial Ports**:

- For single UART boards, disconnect the SIM900 module while uploading code or debugging via USB.

**Test with Public MQTT Brokers**:
    - Use brokers like test.mosquitto.org for initial testing.

## Troubleshooting

**SIM900 Not Responding**:
    - Ensure proper wiring and power supply to the module.
    - Use AT commands to test module responsiveness before running the sketch.

**MQTT Connection Issues**:
    - Verify the broker IP, port, and network connectivity.
    - Ensure the SIM card has an active data plan.

**Future Enhancements**:
    - QoS Support: Add quality of service levels to MQTT PUBLISH messages.
    - TLS Security: Integrate SSL/TLS for secure MQTT communication.
    - SUBSCRIBE Support: Enable parsing and handling of incoming MQTT messages.

## License

- This library is open-source and licensed under the MIT License. Feel free to modify and use it in your projects.

## Acknowledgments

- Special thanks to Edwin Kestler for recent contributions and optimizations to improve performance and usability.

---

### Key Updates in README

1. **Added Section for MQTT V5**:
   - Highlighted the new folder **"new MQTT protocol V5 compatible libraries"**.
   - Documented usage of the new files `mqtt_v5.h` and `mqtt_v5.cpp`.

2. **Detailed MQTT V5 Example**:
   - Included a practical example for setting up and publishing using MQTT V5 features.

3. **Best Practices and Future Enhancements**:
   - Retained existing recommendations while maintaining backward compatibility.
