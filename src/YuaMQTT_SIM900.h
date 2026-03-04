// YuaMQTT_SIM900.h
/******************************************
* YuaMQTT - SIM900 Module Helper
* AT command wrappers for establishing GPRS TCP
* connections and sending/receiving MQTT packets
* via a SIM900 GSM/GPRS module.
*
* The SIM900 shares the SIMCom AT command set with
* SIM800 but targets boards with hardware UART
* (Mega, Leonardo, Due using Serial1).
*
* Usage:
*   #include <YuaMQTT.h>
*   #include <YuaMQTT_SIM900.h>
*
* See docs/SIM900_AT_Commands.md for full AT reference.
******************************************/

#ifndef YUAMQTT_SIM900_H
#define YUAMQTT_SIM900_H

#include <Arduino.h>
#include <Stream.h>

#ifdef __cplusplus

// Default timeouts (milliseconds)
#define SIM900_AT_TIMEOUT     2000
#define SIM900_CIPSTART_TIMEOUT 10000
#define SIM900_CIPSEND_DELAY  500

class YuaSIM900 {
public:
    // Initialize with the serial stream connected to SIM900
    // and optional debug stream (pass NULL to disable debug)
    YuaSIM900(Stream &simSerial, Stream *debugSerial = NULL);

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
                       unsigned long timeout = SIM900_AT_TIMEOUT);

private:
    Stream &_sim;
    Stream *_dbg;

    // Read response into internal buffer, return true if expected string found
    bool waitResponse(const char *expected, unsigned long timeout);
};

#endif // __cplusplus
#endif // YUAMQTT_SIM900_H
