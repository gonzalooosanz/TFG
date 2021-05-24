#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct timeval cur_time;

FILE * fp;

struct timeval cur_time1,cur_time2, tdiff;
time_t  tiempo;
struct  tm  *tm;
char    out_time[128],  d[25];
int i;
 
// Cuando de que ha llegado el mensaje
void on_message(char * TOPIC, char * payload) {
	int id;
	time_t timestamp;
	sscanf(payload, "%d,%ld", &id, &timestamp);
	gettimeofday(&cur_time2,NULL);
	fprintf(fp, "R, %d, %ld , %ld, %d \n", id, cur_time2.tv_sec, cur_time2.tv_usec, strlen(payload));
	fflush(fp);
}

// On delivered quien recibe la notificacion de que se ha enviado el mensaje
void on_delivered(char * payload){
	int id;
	time_t timestamp;
	sscanf(payload, "%d,%ld", &id, &timestamp);
	gettimeofday(&cur_time1,NULL);
	tdiff.tv_sec = cur_time1.tv_sec - cur_time2.tv_sec;
	tdiff.tv_usec = cur_time1.tv_usec - cur_time2.tv_usec;
	fprintf(fp, "D, %d, %ld, %ld, %d \n", id, cur_time1.tv_sec, cur_time1.tv_usec, strlen(payload));
	fflush(fp);
}

int main() {
	ht_t *ht;
	char mess[100000]; 
	// printf("Enter the topic name: ");
	// scanf("%s", topic);
	// printf("Enter the client name: ");
	// scanf("%s", client
	char topic[] = "top", client[] = "XBEE1";
	for (int j = 0; j < 3; j++)
	{
		ht = ht_create();
		printf("Empieza la nueva calidad \n");
		memset(mess, 0, sizeof(mess));
		printf(" Connect\n");
		MQTT_configuration(j);
		MQTT_connect(client, &on_message); // ondelivered 
		printf(" Creando el fichero \n");
		char name_file[20];
		sprintf(name_file, "DISTANCIA_25m%d.txt", j);
		fp = fopen(name_file, "w+");

		MQTT_subs(topic, client, ht);
		fprintf(fp, "Type, Id paq, time stamp segundos, micro, long paq \n");

		for(i=0; i<500; i++) {
			delay(1000);
			gettimeofday(&cur_time,NULL);
			sprintf(mess, "%d,%ld", i, cur_time.tv_sec*1000000 + cur_time.tv_usec);
			fprintf(fp, "S, %d, %ld, %ld, %d\n", i, cur_time.tv_sec, cur_time.tv_usec, strlen(mess));
			MQTT_publish(mess, topic, ht, &on_delivered); /* Publish message */
		}
		delay(2000);
		MQTT_end(topic, client);
		fclose(fp);	
		
	}

	return 0;
}
