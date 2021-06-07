#include "project.h"
#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "SERVIDOR"
#define TOPIC       "top"
#define NUMPACKAGES 499
#define DISTANCE    40
volatile MQTTClient_deliveryToken deliveredtoken;

struct timeval cur_time1, cur_time2, tdiff;
FILE * fp;
int QOS = 0;
bool shipment_finished = false;

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
    sscanf(message->payload, "%d,%ld", &id, &timestamp);
    gettimeofday(&cur_time2,NULL);
    fprintf(fp, "R, %d, %ld, %ld, %ld\n", id, cur_time2.tv_sec, cur_time2.tv_usec, strlen(message->payload));
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
    int rc;

    while(QOS < 3){
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
        char name_file[30];
        sprintf(name_file, "Tam_pack_%dm_QOS_%d_250.txt", DISTANCE, QOS);
        fp = fopen(name_file, "w+");
        fprintf(fp, "Type, Id paq, time stamp segundos, micro, long paq \n");

        // Subscribing
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n", TOPIC, CLIENTID, QOS);
        MQTTClient_subscribe(client, TOPIC, QOS);

        while(!shipment_finished);

        shipment_finished = false;
        QOS++;

        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        fclose(fp);
    }

    return rc;
}