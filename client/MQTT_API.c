#include "project.h"
#include "MQTT_API.h"
#include "hashtable.h"
#include "client.h"
/* MQTT configuration */
#define ADDRESS  "127.0.0.1:1881" //Numero de puerto
char CLIENTID[15];
// Quality of System (0 para no esperar confirmaciones)
int QOS;

int longPaq = 0;

#define TIMEOUT     1000L

void (* msg_arrived)(char * , char *);

void (* msg_delivered)(char *);

static MQTTClient client;
static MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
static MQTTClient_message pubmsg = MQTTClient_message_initializer;
static MQTTClient_deliveryToken token;

volatile MQTTClient_deliveryToken deliveredtoken;

/* ****************************************
 * Function name: delivered()
 *
 * Description:
 * 	Confirmation in case QOS>0
 *
 *************************************** */
void delivered(void *context, MQTTClient_deliveryToken dt) {
	
    //printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
	fflush(stdout);
    fflush(stdin);
}

/* ****************************************
 * Function name: msgarrvd()
 *
 * Description:
 * 		Receives the message and
 * 		show it
 *
 * 
 * Cargar el mensaje, ver el mensaje, y que segun lo que le llega mandarselo a quien sea 
 * Compruebo cual es el topic y se lo mando al cliente que este suscrito a ese tema 
 *************************************** */
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
	int i;
	char* payloadptr;
	double a;
	char buff[10096];
	memset(&buff, '0', sizeof(buff));

	printf("Message arrived\n");
	printf("     topic: %s\n", topicName);
	printf("   message: ");
	payloadptr = message->payload;

	for(i=0; i<message->payloadlen; i++)
	{
		a=*payloadptr++;
		putchar(a);
		buff[i]=a;
	}
	buff[message->payloadlen]='\0';
	putchar('\n');
	fflush(stdin);
	fflush(stdout);

	// Tengo alguien suscrito en ese tema? Hacer lista de suscriptores 
	msg_arrived(topicName, message->payload);
	memset(buff, '\0', sizeof(buff));
	memcpy(buff, message->payload, message->payloadlen);
	MQTTClient_freeMessage(&message);
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
    fflush(stdout);
    fflush(stdin);
}



/* ****************************************
 * Function name: MQTT_configuration()
 *
 * Description:
 * 		MQTT client creation and adjust
 * 		connection parameters
 *
 *************************************** */
void MQTT_configuration(int quality){
	
	while(quality>=3 || quality<0){
		printf("quality error..\n");
		printf(" Introduce a new quality identifier: ");
		scanf( "%d", &quality);
	}

	QOS=quality;

	/* Connection parameters */
	conn_opts.keepAliveInterval = 120;
	conn_opts.cleansession = 1;
	conn_opts.reliable = 0;
}

/* ****************************************
 * Function name: MQTT_connect()
 *
 * Description:
 * 		Function connects to MQTT Broker
 * 		with the client name and parameters
 * 		set before
 *
 *************************************** */
void MQTT_connect(char name[], void (* on_mess)(char * topic, char* payload)){
	int rc;
	strcpy(CLIENTID, name);

	/* MQTT initialization */
	if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
		MQTTCLIENT_PERSISTENCE_NONE, NULL))!= MQTTCLIENT_SUCCESS) {
		printf("Failed to create client, error %d \n",rc);
		exit(-1);
	}
    printf("Client created \n");
	
	msg_arrived = on_mess;

	/* MQTT callback for subscription */
	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
	
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
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
void MQTT_subs(char *TOPIC, char name[], ht_t *ht) {
	MQTTClient_subscribe(client, TOPIC, QOS);
	printf("Subscrito a %s\n", TOPIC);
	ht_set(ht, TOPIC, name); // Insert the pair Topic, client name 
	entry_t * entry = ht_get(ht, TOPIC);
	printf(" Client list suscribe to the topic: %s\n", TOPIC);
	ht_print_entry(entry);
}


/* ****************************************
 * Function name: MQTT_publish()
 *
 * Description:
 * 		Function receives MQTT topic and message
 * 		and send it to the Broker
 *
 *************************************** */
void MQTT_publish(char *PAYLOAD, char *TOPIC, const ht_t *ht, void (* on_deliv)(char * payloadlen)){
	
	/* Publish message */
	pubmsg.payload = PAYLOAD;
	pubmsg.payloadlen = strlen(PAYLOAD);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;

	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	MQTTClient_waitForCompletion(client, token, TIMEOUT);
	if(QOS > 0){
		printf("Message with token value %d delivery confirmed\n", token);
		msg_delivered = on_deliv;
		msg_delivered(pubmsg.payload);
	}
}


/* ****************************************
 * Function name: MQTT_end()
 *
 * Description:
 * 		MQTT client destroy
 *
 *************************************** */
void MQTT_end(char *TOPIC, char name[]){
	/* Disconnect */
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	//deleteClient(TOPIC, name);
}





