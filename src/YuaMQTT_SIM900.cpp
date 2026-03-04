/******************************************
* YuaMQTT_SIM900.cpp
* SIM900 AT command helper implementation.
* Kept minimal for Arduino Uno flash/RAM budget.
******************************************/

#include "YuaMQTT_SIM900.h"

YuaSIM900::YuaSIM900(Stream &simSerial, Stream *debugSerial)
    : _sim(simSerial), _dbg(debugSerial) {}

bool YuaSIM900::waitResponse(const char *expected, unsigned long timeout) {
    unsigned long start = millis();
    String response = "";
    while (millis() - start < timeout) {
        while (_sim.available()) {
            char c = _sim.read();
            response += c;
        }
        if (response.indexOf(expected) != -1) {
            if (_dbg) { _dbg->print("< "); _dbg->println(response); }
            return true;
        }
    }
    if (_dbg) { _dbg->print("< TIMEOUT: "); _dbg->println(response); }
    return false;
}

bool YuaSIM900::sendATCommand(const char *cmd, const char *expected, unsigned long timeout) {
    if (_dbg) { _dbg->print("> "); _dbg->println(cmd); }
    _sim.println(cmd);
    return waitResponse(expected, timeout);
}

bool YuaSIM900::begin() {
    // Wake up module
    sendATCommand("AT");
    delay(500);
    return sendATCommand("AT");
}

int YuaSIM900::signalQuality() {
    if (_dbg) { _dbg->println("> AT+CSQ"); }
    _sim.println("AT+CSQ");

    unsigned long start = millis();
    String response = "";
    while (millis() - start < SIM900_AT_TIMEOUT) {
        while (_sim.available()) {
            char c = _sim.read();
            response += c;
        }
        if (response.indexOf("OK") != -1) break;
    }

    // Parse "+CSQ: XX,YY"
    int idx = response.indexOf("+CSQ:");
    if (idx == -1) return -1;
    int comma = response.indexOf(',', idx);
    if (comma == -1) return -1;
    String rssi_str = response.substring(idx + 5, comma);
    rssi_str.trim();
    return rssi_str.toInt();
}

bool YuaSIM900::gprsConnect(const char *apn, const char *user, const char *pass) {
    // Check GPRS attachment
    if (!sendATCommand("AT+CGATT=1")) return false;

    // Set APN - SIM900 uses AT+CSTT (same as SIM800)
    char cmd[80];
    snprintf(cmd, sizeof(cmd), "AT+CSTT=\"%s\",\"%s\",\"%s\"", apn, user, pass);
    if (!sendATCommand(cmd)) return false;

    // Bring up wireless connection
    if (!sendATCommand("AT+CIICR", "OK", 10000)) return false;

    // Get local IP (response is the IP address, not "OK")
    if (_dbg) { _dbg->println("> AT+CIFSR"); }
    _sim.println("AT+CIFSR");
    delay(2000);
    // Read and discard IP response
    while (_sim.available()) {
        char c = _sim.read();
        if (_dbg) _dbg->write(c);
    }
    if (_dbg) _dbg->println();

    return true;
}

bool YuaSIM900::gprsDisconnect() {
    return sendATCommand("AT+CIPSHUT", "SHUT OK", 5000);
}

bool YuaSIM900::tcpConnect(const char *host, uint16_t port) {
    char cmd[80];
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",\"%u\"", host, port);
    return sendATCommand(cmd, "CONNECT OK", SIM900_CIPSTART_TIMEOUT);
}

bool YuaSIM900::tcpClose() {
    return sendATCommand("AT+CIPCLOSE", "CLOSE OK", 5000);
}

bool YuaSIM900::tcpSend(const uint8_t *data, uint16_t len) {
    if (!data || len == 0) return false;

    if (_dbg) { _dbg->println("> AT+CIPSEND"); }
    _sim.println("AT+CIPSEND");
    delay(SIM900_CIPSEND_DELAY);

    // Wait for '>' prompt
    unsigned long start = millis();
    bool prompt = false;
    while (millis() - start < SIM900_AT_TIMEOUT) {
        if (_sim.available()) {
            char c = _sim.read();
            if (c == '>') { prompt = true; break; }
        }
    }
    if (!prompt) return false;

    // Send data
    _sim.write(data, len);
    _sim.write((uint8_t)0x1A); // Ctrl+Z to end

    // Wait for SEND OK
    return waitResponse("SEND OK", 5000);
}

uint16_t YuaSIM900::tcpRead(uint8_t *buf, uint16_t buf_size, unsigned long timeout_ms) {
    if (!buf || buf_size == 0) return 0;

    uint16_t count = 0;
    unsigned long start = millis();
    while (millis() - start < timeout_ms && count < buf_size) {
        if (_sim.available()) {
            buf[count++] = _sim.read();
            start = millis(); // Reset timeout on data received
        }
    }
    return count;
}
