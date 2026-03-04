# Contributing to YuaMQTT

Thanks for your interest in contributing! This library is designed to be small and focused — it targets Arduino Uno's 2KB RAM / 32KB flash constraints.

## Design Principles

1. **No dynamic allocation** — all buffers are preallocated or caller-provided
2. **Transport-agnostic core** — `YuaMQTT.h/cpp` must not depend on Arduino headers
3. **Minimal footprint** — every byte of flash and RAM counts on an Uno
4. **Module helpers are optional** — SIM800/SIM900 helpers are separate includes

## How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make your changes
4. Run the unit tests: `cd test && make && ./test_mqtt_v5`
5. Ensure examples compile: `arduino-cli compile --fqbn arduino:avr:uno examples/BasicPublish`
6. Commit with a clear message
7. Open a Pull Request

## What We Need Help With

Check the **Future Work** table in the README for planned features:
- QoS 1/2 packet construction and parsing
- UNSUBSCRIBE / UNSUBACK
- Username/password authentication in CONNECT
- Will messages

## Code Style

- C99 for the core library (`YuaMQTT.h/cpp`)
- Keep functions short and well-commented
- Use `uint8_t`, `uint16_t`, `uint32_t` from `<stdint.h>`
- Error codes: return negative int on failure, positive packet length on success
- No `fprintf`, `printf`, or other stdio — this must compile on bare-metal

## Reporting Issues

Please include:
- Board type (Uno, Mega, ESP32, etc.)
- GSM module (SIM800, SIM900)
- Broker software and version
- Minimal sketch that reproduces the issue
