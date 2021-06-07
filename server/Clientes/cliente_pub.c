#include "project.h"
#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "SERVIDOR"
#define TOPIC       "top"
#define NUMPACKAGES 499
#define DISTANCE    1
#define DELAY       600
#define TAMPACKAGES 120
#define TIMEOUT     1000L

volatile MQTTClient_deliveryToken deliveredtoken;
static MQTTClient_deliveryToken token;
static MQTTClient_message pubmsg = MQTTClient_message_initializer;
//static MQTTClient client;

struct timeval cur_time_S, cur_time2, cur_time1;
FILE * fp;
int QOS = 2;
bool shipment_finished = false;
char src = 'P';

char increase_load[TAMPACKAGES];

void increase_tam(){
	for (int i = 0; i < TAMPACKAGES - 2; i++)
	{
		increase_load[i] = 'P';
	}
	increase_load[TAMPACKAGES] = '\0';
}

void publish(char *PAYLOAD, char *topic,  MQTTClient client){
	/* Publish message */
    int id;
    time_t timestamp;
    char src;
	pubmsg.payload = PAYLOAD;
	pubmsg.payloadlen = strlen(PAYLOAD);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;

	MQTTClient_publishMessage(client, topic, &pubmsg, &token);
	MQTTClient_waitForCompletion(client, token, TIMEOUT);
	if(QOS > 0){
        sscanf(pubmsg.payload, "%c, %d,%ld", &src, &id, &timestamp);
        gettimeofday(&cur_time1,NULL);
        fprintf(fp, "D, %d, %ld, %ld, %ld\n", id, cur_time1.tv_sec, cur_time1.tv_usec, strlen(pubmsg.payload));
        fflush(fp);
	}
}

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
  
    // Storing start time
    clock_t start_time = clock();
  
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds);
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
	char buff[10096];
    double a;
	memset(&buff, '0', sizeof(buff));

    printf("Message arrived QoS:%d\n", QOS);
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = message->payload;
    for(i=0; i<20; i++)
    {
		a = *payloadptr++;
        putchar(a);
		buff[i]=a;
    }
	buff[message->payloadlen] = '\0';
    putchar('\n');

	int id;
    time_t timestamp;
    char source;
    sscanf(message->payload, "%c,%d,%ld", &source, &id, &timestamp);
    gettimeofday(&cur_time2,NULL);
    if(source == 'B') fprintf(fp, "R, %d, %ld, %ld, %ld\n", id, cur_time2.tv_sec, cur_time2.tv_usec, strlen(message->payload));
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    
    if(id == (int)NUMPACKAGES){
        shipment_finished = true;
    }

    return 1;
}


void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}


int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    char mess[100000]; 
    char src = 'B';
    int rc;
    increase_tam(); // incrementa el tamaño de paquetes


    // while(QOS < 2){

	
        memset(mess, 0, sizeof(mess));
        MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
        conn_opts.keepAliveInterval = 120;
        conn_opts.cleansession = 1;
        conn_opts.reliable = 0;
        
        MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }

        // Creating file
        printf(" Creando el fichero \n");
        char name_file[40];
        sprintf(name_file, "Cong_%dm_QOS_%d_TAMPACK_%d.txt",DISTANCE, QOS, (int)TAMPACKAGES);
        fp = fopen(name_file, "w+");
        fprintf(fp, "Type, Id paq, time stamp segundos, micro, long paq \n");

        // Subscribing
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n", TOPIC, CLIENTID, QOS);
        MQTTClient_subscribe(client, TOPIC, QOS);

        for(int i=0; i<=(int)NUMPACKAGES; i++) {
			delay((int)DELAY);
			gettimeofday(&cur_time_S,NULL);
			sprintf(mess, "%c, %d,%ld, %s", src, i, cur_time_S.tv_sec*1000000 + cur_time_S.tv_usec, increase_load);
			fprintf(fp, "S, %d, %ld, %ld, %ld \n", i, cur_time_S.tv_sec, cur_time_S.tv_usec, strlen(mess));
            publish(mess, TOPIC, client);
		}
        shipment_finished = false;
        QOS++;
        delay(2000);
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        fclose(fp);
    // }

    return rc;
}
