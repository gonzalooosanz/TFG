
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DISTANCE 1
#define NUMPACKAGES 499
#define DELAY 1250

FILE * fp;


struct timeval cur_time_S, cur_time_D,cur_time_R;
time_t  tiempo;
struct  tm  *tm;
char    out_time[128],  d[25];

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
  
    // Storing start time
    clock_t start_time = clock();
  
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds);
}
 

// Cuando de que ha llegado el mensaje
void on_message(char * TOPIC, char * payload) {
	int id;
	time_t timestamp;
	sscanf(payload, "%d,%ld", &id, &timestamp);
	gettimeofday(&cur_time_R,NULL);
	fprintf(fp, "R, %d, %ld, %ld, %ld\n", id, cur_time_R.tv_sec, cur_time_R.tv_usec, strlen(payload));
	fflush(fp);
}

// On delivered quien recibe la notificacion de que se ha enviado el mensaje
void on_delivered(char * payload){
	int id;
	time_t timestamp;
	sscanf(payload, "%d,%ld", &id, &timestamp);
	gettimeofday(&cur_time_D,NULL);
	fprintf(fp, "D, %d, %ld, %ld, %ld \n", id, cur_time_D.tv_sec, cur_time_D.tv_usec, strlen(payload));
	fflush(fp);
}

int main() {
	ht_t *ht;
	char mess[100000]; 
	char topic[] = "top1", client[] = "XBEE";

	for (int j = 0; j < 3; j++)
	{

		ht = ht_create();
		printf("Empieza la nueva calidad \n");
		memset(mess, 0, sizeof(mess));
		printf(" Connect\n");
		MQTT_configuration(j);
		MQTT_connect(client, &on_message); // ondelivered 
		printf(" Creando el fichero \n");
		char name_file[40];
		sprintf(name_file, "DISTANCIA%d_M_QOS_%d_DELAY_%d.txt",DISTANCE, j, DELAY);
		fp = fopen(name_file, "w+");

		MQTT_subs(topic, client, ht);
		fprintf(fp, "Type, Id paq, time stamp segundos, micro, long paq \n");

		for(int i=0; i<=(int)NUMPACKAGES; i++) {
			delay((int)DELAY);
			gettimeofday(&cur_time_S,NULL);
			//time_micro_sec = cur_time_S.tv_sec*1000000 + cur_time_S.tv_usec;
			sprintf(mess, "%d,%ld", i, cur_time_S.tv_sec*1000000 + cur_time_S.tv_usec);
			fprintf(fp, "S, %d, %ld, %ld, %ld\n", i, cur_time_S.tv_sec, cur_time_S.tv_usec, strlen(mess));
			MQTT_publish(mess, topic, ht, &on_delivered);  
		}
		delay(2000);
		MQTT_end(topic, client);
		fclose(fp);
	}

	return 0;
}
