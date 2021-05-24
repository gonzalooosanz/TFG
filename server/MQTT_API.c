#include "MQTT_API.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#define Publisher

/* MQTT configuration */
#define ADDRESS  "127.0.0.1:1881" //Numero de puerto
#define CLIENTID "XBEE1"
// Quality of System (0 para no esperar confirmaciones)
#define QOS         0
#define TIMEOUT     10000L

static MQTTClient client;
static MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
static MQTTClient_message pubmsg = MQTTClient_message_initializer;
static MQTTClient_deliveryToken token;

volatile MQTTClient_deliveryToken deliveredtoken;

/* ****************************************
 * Function name: delivered()
 * 
 * Description:
 * 		Confirmation in case QOS>0 
 * 
 *************************************** */
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

/* ****************************************
 * Function name: msgarrvd()
 * 
 * Description:
 * 		Receives the message and 
 * 		show it
 * 
 *************************************** */
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *messageA)
{
	int i;
	char* payloadptr;
	double a;
	
	printf("Message arrived\n");
	printf("     topic: %s\n", topicName);
	printf("   message: ");

	payloadptr = messageA->payload;
	for(i=0; i<messageA->payloadlen; i++)
	{
		a=*payloadptr++;
		putchar(a);	
	}
	putchar('\n');
    
	MQTTClient_freeMessage(&messageA);
	MQTTClient_free(topicName);
	return 1;
}

/* ****************************************
 * Function name: connlost()
 * 
 * Description:
 * 		For subscriber, it sais if 
 * 		the connection with broker is lost
 * 
 *************************************** */
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}



/* ****************************************
 * Function name: MQTT_configuration()
 * 
 * Description:
 * 		MQTT client creation and adjust
 * 		connection parameters
 * 
 *************************************** */
void MQTT_configuration(){
	
	int rc;
	/* MQTT initialization */
	if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
		MQTTCLIENT_PERSISTENCE_NONE, NULL))!= MQTTCLIENT_SUCCESS){
	
		printf("Failed to create client, error %d \n",rc);
		exit(-1);
	}
   
        printf("Client created \n");
	/* Connection parameters */
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	/* MQTT callback for subscription */
	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    
   
	
}

void MQTT_connect(char *TOPIC){
	
	int rc;
	
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS){
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
	}
}


/* ****************************************
 * Function name: MQTT_subs()
 * 
 * Description:
 * 		Function subscribes to MQTT topic 
 * 		to receive the messages post on it
 * 
 *************************************** */
void MQTT_subs(char *TOPIC){
	
	MQTTClient_subscribe(client, TOPIC, QOS);
	printf("Subscrito a %s\n",TOPIC);
	
}


/* ****************************************
 * Function name: MQTT_publish()
 * 
 * Description:
 * 		Function receives MQTT topic and message
 * 		and send it to the Broker
 * 
 *************************************** */
void MQTT_publish(char *PAYLOAD, char *TOPIC){
	int rc;
	/* Publish message */
	pubmsg.payload = PAYLOAD;
	pubmsg.payloadlen = strlen(PAYLOAD);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
}


/* ****************************************
 * Function name: MQTT_end()
 * 
 * Description:
 * 		MQTT client destroy
 * 
 *************************************** */
void MQTT_end(){
	
	/* Disconnect */
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	
}