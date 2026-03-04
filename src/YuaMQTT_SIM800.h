// YuaMQTT_SIM800.h
/******************************************
* YuaMQTT - SIM800 Module Helper
* AT command wrappers for establishing GPRS TCP
* connections and sending/receiving MQTT packets
* via a SIM800 GSM/GPRS module.
*
* Designed for Arduino Uno with SoftwareSerial.
*
* Usage:
*   #include <YuaMQTT.h>
*   #include <YuaMQTT_SIM800.h>
*
* See docs/SIM800_AT_Commands.md for full AT reference.
******************************************/

#ifndef YUAMQTT_SIM800_H
#define YUAMQTT_SIM800_H

#include <Arduino.h>
#include <Stream.h>

#ifdef __cplusplus

// Default timeouts (milliseconds)
#define SIM800_AT_TIMEOUT     2000
#define SIM800_CIPSTART_TIMEOUT 10000
#define SIM800_CIPSEND_DELAY  500

class YuaSIM800 {
public:
    // Initialize with the serial stream connected to SIM800
    // and optional debug stream (pass NULL to disable debug)
    YuaSIM800(Stream &simSerial, Stream *debugSerial = NULL);

    // Basic module commands
    bool begin();                                   // Send AT, verify module responds
    int  signalQuality();                           // AT+CSQ, returns RSSI (0-31, 99=unknown)

    // GPRS connection
    bool gprsConnect(const char *apn,               // AT+CSTT, AT+CIICR, AT+CIFSR
                     const char *user = "",
                     const char *pass = "");
    bool gprsDisconnect();                          // AT+CIPSHUT

    // TCP connection to MQTT broker
    bool tcpConnect(const char *host, uint16_t port); // AT+CIPSTART
    bool tcpClose();                                  // AT+CIPCLOSE

    // Send raw bytes over TCP (wraps AT+CIPSEND)
    bool tcpSend(const uint8_t *data, uint16_t len);

    // Read available data from module into buffer
    // Returns number of bytes read
    uint16_t tcpRead(uint8_t *buf, uint16_t buf_size, unsigned long timeout_ms = 2000);

    // Send AT command and wait for expected response
    bool sendATCommand(const char *cmd,
                       const char *expected = "OK",
                       unsigned long timeout = SIM800_AT_TIMEOUT);

private:
    Stream &_sim;
    Stream *_dbg;

    // Read response into internal buffer, return true if expected string found
    bool waitResponse(const char *expected, unsigned long timeout);
};

#endif // __cplusplus
#endif // YUAMQTT_SIM800_H
