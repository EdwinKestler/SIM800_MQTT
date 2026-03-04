/*
 * test_mqtt_v5.c
 * Desktop unit tests for YuaMQTT core library.
 * Compile: gcc -o test_mqtt_v5 test_mqtt_v5.c ../src/YuaMQTT.cpp -I../src -lstdc++
 *    or:   make
 *
 * Tests verify byte-level correctness of MQTT V5 packet construction
 * and parsing against the OASIS MQTT V5.0 specification.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "YuaMQTT.h"

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ(expected, actual, msg) do { \
    tests_run++; \
    if ((expected) == (actual)) { \
        tests_passed++; \
    } else { \
        tests_failed++; \
        printf("  FAIL: %s (expected %d, got %d)\n", msg, (int)(expected), (int)(actual)); \
    } \
} while(0)

#define ASSERT_MEM(expected, actual, len, msg) do { \
    tests_run++; \
    if (memcmp((expected), (actual), (len)) == 0) { \
        tests_passed++; \
    } else { \
        tests_failed++; \
        printf("  FAIL: %s (memory mismatch at byte ", msg); \
        for (int _i = 0; _i < (int)(len); _i++) { \
            if (((const uint8_t*)(expected))[_i] != ((const uint8_t*)(actual))[_i]) { \
                printf("%d: expected 0x%02X got 0x%02X", _i, \
                    ((const uint8_t*)(expected))[_i], ((const uint8_t*)(actual))[_i]); \
                break; \
            } \
        } \
        printf(")\n"); \
    } \
} while(0)

/* ========================================
 * VBI encoding/decoding tests
 * ======================================== */
void test_vbi_encode_decode(void) {
    printf("VBI encode/decode...\n");

    uint8_t buf[4];
    uint32_t decoded;
    int len;

    /* Single byte: 0 */
    len = mqtt_v5_encode_vbi(buf, 0);
    ASSERT_EQ(1, len, "VBI encode 0 length");
    ASSERT_EQ(0x00, buf[0], "VBI encode 0 value");
    len = mqtt_v5_decode_vbi(buf, &decoded);
    ASSERT_EQ(1, len, "VBI decode 0 length");
    ASSERT_EQ(0, (int)decoded, "VBI decode 0 value");

    /* Single byte: 127 */
    len = mqtt_v5_encode_vbi(buf, 127);
    ASSERT_EQ(1, len, "VBI encode 127 length");
    ASSERT_EQ(0x7F, buf[0], "VBI encode 127 value");
    len = mqtt_v5_decode_vbi(buf, &decoded);
    ASSERT_EQ(127, (int)decoded, "VBI decode 127 value");

    /* Two bytes: 128 */
    len = mqtt_v5_encode_vbi(buf, 128);
    ASSERT_EQ(2, len, "VBI encode 128 length");
    ASSERT_EQ(0x80, buf[0], "VBI encode 128 byte0");
    ASSERT_EQ(0x01, buf[1], "VBI encode 128 byte1");
    len = mqtt_v5_decode_vbi(buf, &decoded);
    ASSERT_EQ(128, (int)decoded, "VBI decode 128 value");

    /* Two bytes: 16383 */
    len = mqtt_v5_encode_vbi(buf, 16383);
    ASSERT_EQ(2, len, "VBI encode 16383 length");
    len = mqtt_v5_decode_vbi(buf, &decoded);
    ASSERT_EQ(16383, (int)decoded, "VBI decode 16383 value");

    /* Three bytes: 16384 */
    len = mqtt_v5_encode_vbi(buf, 16384);
    ASSERT_EQ(3, len, "VBI encode 16384 length");
    len = mqtt_v5_decode_vbi(buf, &decoded);
    ASSERT_EQ(16384, (int)decoded, "VBI decode 16384 value");

    /* Four bytes: max value 268435455 */
    len = mqtt_v5_encode_vbi(buf, 268435455UL);
    ASSERT_EQ(4, len, "VBI encode max length");
    len = mqtt_v5_decode_vbi(buf, &decoded);
    ASSERT_EQ(268435455, (int)decoded, "VBI decode max value");

    /* Error: over max */
    len = mqtt_v5_encode_vbi(buf, 268435456UL);
    ASSERT_EQ(-1, len, "VBI encode overflow");

    /* Error: NULL */
    len = mqtt_v5_encode_vbi(NULL, 0);
    ASSERT_EQ(-1, len, "VBI encode NULL buf");
    len = mqtt_v5_decode_vbi(NULL, &decoded);
    ASSERT_EQ(-1, len, "VBI decode NULL buf");
}

/* ========================================
 * CONNECT packet tests
 * ======================================== */
void test_connect_basic(void) {
    printf("CONNECT basic...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_connect_message(buf, "test", 15, NULL, 0);

    /* Expected packet:
     * 0x10         - CONNECT type
     * 0x11         - Remaining length = 17 (10 + 1(props-VBI=0) + 0 + 2 + 4)
     * 0x00 0x04    - Protocol name length
     * M Q T T      - Protocol name
     * 0x05         - Protocol version 5
     * 0x02         - Connect flags (Clean Start)
     * 0x00 0x0F    - Keep alive = 15
     * 0x00         - Properties length = 0
     * 0x00 0x04    - Client ID length = 4
     * t e s t      - Client ID
     */
    ASSERT_EQ(19, len, "CONNECT basic total length");
    ASSERT_EQ(0x10, buf[0], "CONNECT packet type");
    ASSERT_EQ(17, buf[1], "CONNECT remaining length");
    /* Protocol name */
    ASSERT_EQ(0x00, buf[2], "Protocol name len MSB");
    ASSERT_EQ(0x04, buf[3], "Protocol name len LSB");
    ASSERT_MEM("MQTT", &buf[4], 4, "Protocol name");
    /* Version, flags, keepalive */
    ASSERT_EQ(0x05, buf[8], "Protocol version");
    ASSERT_EQ(0x02, buf[9], "Connect flags");
    ASSERT_EQ(0x00, buf[10], "Keep alive MSB");
    ASSERT_EQ(0x0F, buf[11], "Keep alive LSB");
    /* Properties length = 0 */
    ASSERT_EQ(0x00, buf[12], "Properties length");
    /* Client ID */
    ASSERT_EQ(0x00, buf[13], "Client ID len MSB");
    ASSERT_EQ(0x04, buf[14], "Client ID len LSB");
    ASSERT_MEM("test", &buf[15], 4, "Client ID");
}

void test_connect_with_property(void) {
    printf("CONNECT with Session Expiry property...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    mqtt_property props[1];
    uint8_t session_expiry[4] = {0x00, 0x00, 0x00, 0x3C}; /* 60 seconds */
    props[0].property_id = 0x11;
    props[0].length = 4;
    props[0].value = session_expiry;

    int len = mqtt_v5_connect_message(buf, "test", 60, props, 1);

    /* Properties: 1 byte id + 4 bytes value = 5 bytes data
     * Props length VBI = 1 byte (value 5)
     * Total remaining = 10 + 1 + 5 + 2 + 4 = 22
     */
    ASSERT_EQ(24, len, "CONNECT with prop total length");
    ASSERT_EQ(0x10, buf[0], "CONNECT packet type");
    ASSERT_EQ(22, buf[1], "CONNECT remaining length");
    /* Keep alive = 60 */
    ASSERT_EQ(0x00, buf[10], "Keep alive MSB");
    ASSERT_EQ(0x3C, buf[11], "Keep alive LSB");
    /* Properties length = 5 */
    ASSERT_EQ(0x05, buf[12], "Properties length");
    /* Session Expiry property: id 0x11, 4-byte int */
    ASSERT_EQ(0x11, buf[13], "Session Expiry prop ID");
    ASSERT_EQ(0x00, buf[14], "Session Expiry byte 0");
    ASSERT_EQ(0x00, buf[15], "Session Expiry byte 1");
    ASSERT_EQ(0x00, buf[16], "Session Expiry byte 2");
    ASSERT_EQ(0x3C, buf[17], "Session Expiry byte 3");
}

void test_connect_custom_keepalive(void) {
    printf("CONNECT custom keep-alive...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_connect_message(buf, "c", 300, NULL, 0);

    /* remaining = 10 + 1(props VBI=0) + 0 + 2 + 1 = 14, total = 1 + 1 + 14 = 16 */
    ASSERT_EQ(16, len, "CONNECT short ID length");
    /* Keep alive 300 = 0x012C */
    ASSERT_EQ(0x01, buf[10], "Keep alive 300 MSB");
    ASSERT_EQ(0x2C, buf[11], "Keep alive 300 LSB");
}

void test_connect_null_params(void) {
    printf("CONNECT NULL params...\n");

    ASSERT_EQ(-1, mqtt_v5_connect_message(NULL, "test", 15, NULL, 0), "CONNECT NULL buf");
    uint8_t buf[MQTT_BUFFER_SIZE];
    ASSERT_EQ(-1, mqtt_v5_connect_message(buf, NULL, 15, NULL, 0), "CONNECT NULL client_id");
}

/* ========================================
 * PUBLISH packet tests
 * ======================================== */
void test_publish_basic(void) {
    printf("PUBLISH basic...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_publish_message(buf, "a/b", "hello", NULL, 0);

    /* Remaining: 2 + 3(topic) + 1(props len=0) + 5(payload) = 11 */
    ASSERT_EQ(13, len, "PUBLISH basic total length");
    ASSERT_EQ(0x30, buf[0], "PUBLISH packet type");
    ASSERT_EQ(11, buf[1], "PUBLISH remaining length");
    /* Topic */
    ASSERT_EQ(0x00, buf[2], "Topic len MSB");
    ASSERT_EQ(0x03, buf[3], "Topic len LSB");
    ASSERT_MEM("a/b", &buf[4], 3, "Topic");
    /* Props length = 0 */
    ASSERT_EQ(0x00, buf[7], "Properties length");
    /* Payload */
    ASSERT_MEM("hello", &buf[8], 5, "Payload");
}

void test_publish_with_content_type(void) {
    printf("PUBLISH with Content Type property...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    mqtt_property props[1];
    uint8_t content_type[] = "application/json";
    props[0].property_id = 0x03;
    props[0].length = 16;
    props[0].value = content_type;

    int len = mqtt_v5_publish_message(buf, "t", "x", props, 1);

    /* Props data: 1(id) + 2(string len) + 16(string) = 19
     * Props len VBI = 1 byte (value 19)
     * Remaining = 2 + 1(topic) + 1 + 19 + 1(payload) = 24
     */
    ASSERT_EQ(26, len, "PUBLISH with Content Type total");
    ASSERT_EQ(0x30, buf[0], "PUBLISH packet type");
}

/* ========================================
 * SUBSCRIBE packet tests
 * ======================================== */
void test_subscribe_basic(void) {
    printf("SUBSCRIBE basic...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_subscribe_message(buf, 1, "a/b", 0, NULL, 0);

    /* Remaining: 2(pkt id) + 1(props len=0) + 2(topic len) + 3(topic) + 1(QoS) = 9 */
    ASSERT_EQ(11, len, "SUBSCRIBE basic total length");
    ASSERT_EQ(0x82, buf[0], "SUBSCRIBE packet type");
    ASSERT_EQ(9, buf[1], "SUBSCRIBE remaining length");
    /* Packet ID = 1 */
    ASSERT_EQ(0x00, buf[2], "Packet ID MSB");
    ASSERT_EQ(0x01, buf[3], "Packet ID LSB");
    /* Props length = 0 */
    ASSERT_EQ(0x00, buf[4], "Properties length");
    /* Topic */
    ASSERT_EQ(0x00, buf[5], "Topic len MSB");
    ASSERT_EQ(0x03, buf[6], "Topic len LSB");
    ASSERT_MEM("a/b", &buf[7], 3, "Topic");
    /* QoS */
    ASSERT_EQ(0x00, buf[10], "QoS byte");
}

void test_subscribe_custom_packet_id(void) {
    printf("SUBSCRIBE custom packet ID...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_subscribe_message(buf, 0x1234, "x", 1, NULL, 0);

    ASSERT_EQ(0x12, buf[2], "Packet ID 0x1234 MSB");
    ASSERT_EQ(0x34, buf[3], "Packet ID 0x1234 LSB");
    /* QoS = 1 */
    int topic_end = 4 + 1 + 2 + 1; /* after props len + topic len + topic */
    ASSERT_EQ(0x01, buf[topic_end], "QoS 1");
    (void)len;
}

/* ========================================
 * DISCONNECT / PINGREQ tests
 * ======================================== */
void test_disconnect(void) {
    printf("DISCONNECT...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_disconnect_message(buf);

    ASSERT_EQ(2, len, "DISCONNECT length");
    ASSERT_EQ(0xE0, buf[0], "DISCONNECT type");
    ASSERT_EQ(0x00, buf[1], "DISCONNECT remaining");
}

void test_pingreq(void) {
    printf("PINGREQ...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_pingreq_message(buf);

    ASSERT_EQ(2, len, "PINGREQ length");
    ASSERT_EQ(0xC0, buf[0], "PINGREQ type");
    ASSERT_EQ(0x00, buf[1], "PINGREQ remaining");
}

/* ========================================
 * CONNACK parsing tests
 * ======================================== */
void test_parse_connack_success(void) {
    printf("Parse CONNACK success...\n");

    /* Minimal CONNACK: type, rem_len=3, ack_flags=0, reason=0x00, props_len=0 */
    uint8_t connack[] = {0x20, 0x03, 0x00, 0x00, 0x00};
    uint8_t session_present, reason_code;

    int result = mqtt_v5_parse_connack(connack, &session_present, &reason_code);
    ASSERT_EQ(0, result, "CONNACK parse success return");
    ASSERT_EQ(0, session_present, "Session not present");
    ASSERT_EQ(0x00, reason_code, "Reason code success");
}

void test_parse_connack_session_present(void) {
    printf("Parse CONNACK session present...\n");

    uint8_t connack[] = {0x20, 0x03, 0x01, 0x00, 0x00};
    uint8_t session_present, reason_code;

    int result = mqtt_v5_parse_connack(connack, &session_present, &reason_code);
    ASSERT_EQ(0, result, "CONNACK with session present return");
    ASSERT_EQ(1, session_present, "Session present flag");
}

void test_parse_connack_refused(void) {
    printf("Parse CONNACK refused...\n");

    /* Reason code 0x87 = Not Authorized */
    uint8_t connack[] = {0x20, 0x03, 0x00, 0x87, 0x00};
    uint8_t session_present, reason_code;

    int result = mqtt_v5_parse_connack(connack, &session_present, &reason_code);
    ASSERT_EQ(-3, result, "CONNACK refused return");
    ASSERT_EQ(0x87, reason_code, "Refused reason code");
}

void test_parse_connack_wrong_type(void) {
    printf("Parse CONNACK wrong packet type...\n");

    uint8_t not_connack[] = {0x30, 0x03, 0x00, 0x00, 0x00};
    uint8_t sp, rc;

    ASSERT_EQ(-2, mqtt_v5_parse_connack(not_connack, &sp, &rc), "Not CONNACK returns -2");
}

/* ========================================
 * SUBACK parsing tests
 * ======================================== */
void test_parse_suback(void) {
    printf("Parse SUBACK...\n");

    /* SUBACK: type=0x90, rem_len=4, pkt_id=0x0001, props_len=0, reason=0x00 */
    uint8_t suback[] = {0x90, 0x04, 0x00, 0x01, 0x00, 0x00};
    uint16_t packet_id;
    uint8_t reason_code;

    int result = mqtt_v5_parse_suback_message(suback, &packet_id, &reason_code);
    ASSERT_EQ(0, result, "SUBACK parse return");
    ASSERT_EQ(1, packet_id, "SUBACK packet ID");
    ASSERT_EQ(0x00, reason_code, "SUBACK reason code");
}

void test_parse_suback_high_packet_id(void) {
    printf("Parse SUBACK high packet ID...\n");

    uint8_t suback[] = {0x90, 0x04, 0x12, 0x34, 0x00, 0x02};
    uint16_t packet_id;
    uint8_t reason_code;

    int result = mqtt_v5_parse_suback_message(suback, &packet_id, &reason_code);
    ASSERT_EQ(0, result, "SUBACK high ID parse");
    ASSERT_EQ(0x1234, packet_id, "SUBACK packet ID 0x1234");
    ASSERT_EQ(0x02, reason_code, "SUBACK reason code 0x02");
}

/* ========================================
 * PUBLISH parsing tests
 * ======================================== */
void test_parse_publish(void) {
    printf("Parse PUBLISH...\n");

    /* Build a PUBLISH packet, then parse it back */
    uint8_t buf[MQTT_BUFFER_SIZE];
    int len = mqtt_v5_publish_message(buf, "test/topic", "hello world", NULL, 0);

    char topic[64];
    uint8_t payload[128];
    uint16_t topic_len, payload_len;

    int result = mqtt_v5_parse_publish(buf, len, topic, sizeof(topic),
                                        payload, sizeof(payload),
                                        &topic_len, &payload_len);

    ASSERT_EQ(0, result, "Parse PUBLISH return");
    ASSERT_EQ(10, topic_len, "Parse PUBLISH topic length");
    ASSERT_MEM("test/topic", topic, 10, "Parse PUBLISH topic");
    ASSERT_EQ(11, payload_len, "Parse PUBLISH payload length");
    ASSERT_MEM("hello world", payload, 11, "Parse PUBLISH payload");
}

void test_parse_publish_with_props(void) {
    printf("Parse PUBLISH with properties...\n");

    uint8_t buf[MQTT_BUFFER_SIZE];
    mqtt_property props[1];
    uint8_t content_type[] = "text/plain";
    props[0].property_id = 0x03;
    props[0].length = 10;
    props[0].value = content_type;

    int len = mqtt_v5_publish_message(buf, "t", "data", props, 1);

    char topic[64];
    uint8_t payload[128];
    uint16_t topic_len, payload_len;

    int result = mqtt_v5_parse_publish(buf, len, topic, sizeof(topic),
                                        payload, sizeof(payload),
                                        &topic_len, &payload_len);

    ASSERT_EQ(0, result, "Parse PUBLISH with props return");
    ASSERT_EQ(1, topic_len, "Topic length");
    ASSERT_MEM("t", topic, 1, "Topic");
    ASSERT_EQ(4, payload_len, "Payload length");
    ASSERT_MEM("data", payload, 4, "Payload");
}

/* ========================================
 * PINGRESP detection test
 * ======================================== */
void test_is_pingresp(void) {
    printf("PINGRESP detection...\n");

    uint8_t pingresp[] = {0xD0, 0x00};
    ASSERT_EQ(1, mqtt_v5_is_pingresp(pingresp), "Is PINGRESP");

    uint8_t not_pingresp[] = {0xC0, 0x00};
    ASSERT_EQ(0, mqtt_v5_is_pingresp(not_pingresp), "Not PINGRESP");

    ASSERT_EQ(0, mqtt_v5_is_pingresp(NULL), "NULL is not PINGRESP");
}

/* ========================================
 * Round-trip tests
 * ======================================== */
void test_roundtrip_publish(void) {
    printf("Round-trip PUBLISH...\n");

    const char *topics[] = {"a", "sensor/temperature", "a/very/long/topic/path/here"};
    const char *payloads[] = {"", "42", "{\"key\":\"value\",\"num\":123}"};

    for (int i = 0; i < 3; i++) {
        uint8_t buf[MQTT_BUFFER_SIZE];
        int len = mqtt_v5_publish_message(buf, topics[i], payloads[i], NULL, 0);

        char topic[64];
        uint8_t payload[128];
        uint16_t topic_len, payload_len;

        int result = mqtt_v5_parse_publish(buf, len, topic, sizeof(topic),
                                            payload, sizeof(payload),
                                            &topic_len, &payload_len);

        ASSERT_EQ(0, result, "Round-trip PUBLISH parse");
        ASSERT_EQ((int)strlen(topics[i]), (int)topic_len, "Round-trip topic length");
        ASSERT_EQ((int)strlen(payloads[i]), (int)payload_len, "Round-trip payload length");
    }
}

/* ========================================
 * Main
 * ======================================== */
int main(void) {
    printf("=== YuaMQTT Unit Tests ===\n\n");

    test_vbi_encode_decode();
    test_connect_basic();
    test_connect_with_property();
    test_connect_custom_keepalive();
    test_connect_null_params();
    test_publish_basic();
    test_publish_with_content_type();
    test_subscribe_basic();
    test_subscribe_custom_packet_id();
    test_disconnect();
    test_pingreq();
    test_parse_connack_success();
    test_parse_connack_session_present();
    test_parse_connack_refused();
    test_parse_connack_wrong_type();
    test_parse_suback();
    test_parse_suback_high_packet_id();
    test_parse_publish();
    test_parse_publish_with_props();
    test_is_pingresp();
    test_roundtrip_publish();

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(", %d FAILED", tests_failed);
    }
    printf(" ===\n");

    return tests_failed > 0 ? 1 : 0;
}
