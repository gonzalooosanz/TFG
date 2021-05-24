#ifndef _MQTTcommunication_H
#define _MQTTcommunication_H

#include "MQTTClient.h"
#include "hashtable.h"
#include "string.h"
#include <stdlib.h>


void MQTT_configuration(int quality);
void MQTT_connect(char name[], void (* on_mess)(char * topic, char* payload));
void MQTT_subs(char *TOPIC, char name[], ht_t *ht);
void MQTT_publish(char *PAYLOAD, char *TOPIC, const ht_t *ht, void (* on_deliv)(char * payload));
void MQTT_end(char *TOPIC, char name[]);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
void delivered(void *context, MQTTClient_deliveryToken dt);
void connlost(void *context, char *cause);

#endif
