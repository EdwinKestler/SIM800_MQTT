#ifndef MQTT_H_INCLUDED
#define MQTT_H_INCLUDED

#include <string.h>
#include <stdint.h>

void mqtt_connect_message(uint8_t *, char *);

void mqtt_publish_message(uint8_t *, char *, char *);

void mqtt_disconnect_message(uint8_t *);


#endif // MQTT_H_INCLUDED
