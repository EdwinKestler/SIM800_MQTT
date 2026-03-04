# Changelog

All notable changes to YuaMQTT will be documented in this file.

## [1.1.0] - 2026-03-04

### Added
- **CONNACK parsing** (`mqtt_v5_parse_connack`) — verify broker accepted connection
- **PINGREQ construction** (`mqtt_v5_pingreq_message`) — keep-alive heartbeat
- **PINGRESP detection** (`mqtt_v5_is_pingresp`) — confirm broker is alive
- **Incoming PUBLISH parsing** (`mqtt_v5_parse_publish`) — extract topic and payload from broker messages
- **SIM800 AT command helper** (`YuaMQTT_SIM800.h`) — `YuaSIM800` class for GPRS TCP via SIM800
- **SIM900 AT command helper** (`YuaMQTT_SIM900.h`) — `YuaSIM900` class for GPRS TCP via SIM900
- **HardwareSerialPublish example** — demonstrates Serial1 usage for Mega/ESP32/STM32/RP2040
- Configurable `keep_alive` parameter in `mqtt_v5_connect_message()`
- Configurable `packet_id` parameter in `mqtt_v5_subscribe_message()`
- `YUAMQTT_VERSION` compile-time version macros
- `extern "C"` guards for C compatibility
- `MQTT_DEFAULT_KEEP_ALIVE` configurable define
- Desktop unit test harness (gcc/g++)
- GitHub Actions CI for compile checks and unit tests
- AT command reference docs (`docs/SIM800_AT_Commands.md`, `docs/SIM900_AT_Commands.md`)
- `library.json` for PlatformIO
- `CONTRIBUTING.md`, `CHANGELOG.md`, `LICENSE`, `.gitignore`

### Fixed
- **Critical**: Remaining Length now uses MQTT Variable Byte Integer encoding (was raw byte)
- **Critical**: Properties Length now uses VBI encoding (was 2-byte big-endian)
- **Critical**: Property values now use type-aware encoding per MQTT V5 spec (was wrong TLV format)
- **Critical**: PUBLISH now actually writes properties between topic and payload
- SUBSCRIBE remaining length now correctly includes Packet Identifier
- SUBACK parser now reads 2-byte Packet Identifier and decodes VBI lengths
- Removed `fprintf(stderr, ...)` calls that prevented Arduino compilation
- Fixed undeclared `vbi_len` variable in VBI property encoding
- Examples now use returned packet length instead of `strlen()` on binary buffers
- Fixed `simSerial->readBytes()` undefined variable in JSON example
- Upgraded `uint8_t` to `uint16_t` for topic/message lengths (supports >255 bytes)

### Changed
- Library renamed from `UARTMqttV5` to `YuaMQTT`
- File names: `mqtt_v5.h` → `YuaMQTT.h`, module helpers use `YuaMQTT_SIM800.h` / `YuaMQTT_SIM900.h`
- `architectures` set to `*` (pure C core runs on any platform)
- `mqtt_v5_connect_message()` signature now requires `keep_alive` parameter
- `mqtt_v5_subscribe_message()` signature now requires `packet_id` parameter
- Repository restructured to Arduino Library Manager conventions (`src/`, `examples/<Name>/<Name>.ino`)
- Legacy MQTT 3.1 code moved to `legacy/` folder

## [1.0.0] - 2024-01-01

### Added
- Initial MQTT V5 implementation: CONNECT, PUBLISH, SUBSCRIBE, DISCONNECT
- SUBACK parsing
- Preallocated 256-byte buffer
- Basic property support
- Legacy MQTT 3.1 library (tested since 2012)
