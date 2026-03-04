# SIM900 AT Commands Used by YuaMQTT

Quick reference for the AT commands used in `YuaMQTT_SIM900.h`.
For the full command manual see `SIM900_AT.pdf` in this folder.

## Module Initialisation

| Command | Response | Description |
|---------|----------|-------------|
| `AT` | `OK` | Test module is alive |
| `AT+CSQ` | `+CSQ: <rssi>,<ber>` | Signal quality. RSSI 0-31 (higher = better), 99 = unknown |

## GPRS Connection

| Command | Response | Description |
|---------|----------|-------------|
| `AT+CGATT=1` | `OK` | Attach to GPRS service |
| `AT+CSTT="<apn>","<user>","<pass>"` | `OK` | Set APN, username, password |
| `AT+CIICR` | `OK` | Bring up wireless connection (can take ~10 s) |
| `AT+CIFSR` | `<IP address>` | Get local IP. No `OK` — the response IS the IP |
| `AT+CIPSHUT` | `SHUT OK` | Deactivate GPRS PDP context |

## TCP Connection

| Command | Response | Description |
|---------|----------|-------------|
| `AT+CIPSTART="TCP","<host>","<port>"` | `CONNECT OK` | Open TCP socket (timeout ~10 s) |
| `AT+CIPCLOSE` | `CLOSE OK` | Close TCP socket |

## Sending Data

| Command | Response | Description |
|---------|----------|-------------|
| `AT+CIPSEND` | `>` (prompt) | Enter data-send mode |
| _write binary data_ | | Raw bytes sent after `>` prompt |
| `0x1A` (Ctrl+Z) | `SEND OK` | End data and transmit |

## Typical Session Flow

```
AT                          → OK          (wake up)
AT+CSQ                      → +CSQ: 15,0  (check signal)
AT+CGATT=1                  → OK          (attach GPRS)
AT+CSTT="apn","",""         → OK          (set APN)
AT+CIICR                    → OK          (bring up connection)
AT+CIFSR                    → 10.x.x.x   (get IP)
AT+CIPSTART="TCP","broker","1883" → CONNECT OK
AT+CIPSEND                  → >           (send MQTT CONNECT)
  <binary MQTT packet> 0x1A → SEND OK
AT+CIPSEND                  → >           (send MQTT PUBLISH)
  <binary MQTT packet> 0x1A → SEND OK
AT+CIPCLOSE                 → CLOSE OK
AT+CIPSHUT                  → SHUT OK
```

## SIM900 vs SIM800 Differences

The SIM900 and SIM800 share the SIMCom AT command set. For basic GPRS TCP
operations the commands are identical. Key hardware differences:

| Feature | SIM900 | SIM800 |
|---------|--------|--------|
| Bands | Quad-band 850/900/1800/1900 MHz | Quad-band 850/900/1800/1900 MHz |
| Form factor | Larger module | Smaller module |
| Typical board | Arduino GSM Shield (Serial1) | SIM800L/SIM800C breakout |
| Serial | Hardware UART recommended | SoftwareSerial OK at 9600 |

## Notes

- Default baud rate: 9600 (auto-baud supported).
- On Arduino Mega/Leonardo/Due use `Serial1` (hardware UART) for reliability.
- The SIM900 Arduino GSM Shield directly connects to pins 0/1 or 2/3 depending on jumper.
- `AT+CIICR` can take up to 10 seconds on first connection.
- `AT+CIFSR` does NOT return `OK` — the IP address string IS the response.
- After `AT+CIPSEND`, wait for the `>` prompt before writing data.
- Terminate data with `0x1A` (Ctrl+Z), NOT a newline.
