
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DISTANCE 40
#define NUMPACKAGES 499
#define TAMPACKAGES 50

FILE * fp;

struct timeval cur_time1,cur_time2,cur_time;;
time_t  tiempo;
struct  tm  *tm;
char    out_time[128],  d[25], increase_load[TAMPACKAGES];

void increase_tam(){
	for (int i = 0; i < TAMPACKAGES - 2; i++)
	{
		increase_load[i] = 'x';
	}
	increase_load[TAMPACKAGES] = '\0';
}
 
// Cuando de que ha llegado el mensaje
void on_message(char * TOPIC, char * payload) {
	int id;
	time_t timestamp;
	sscanf(payload, "%d,%ld", &id, &timestamp);
	gettimeofday(&cur_time2,NULL);
	fprintf(fp, "R, %d, %ld, %ld, %d\n", id, cur_time2.tv_sec, cur_time2.tv_usec, strlen(payload));
	fflush(fp);
}

// On delivered quien recibe la notificacion de que se ha enviado el mensaje
void on_delivered(char * payload){
	int id;
	time_t timestamp;
	sscanf(payload, "%d,%ld",&id, &timestamp);
	gettimeofday(&cur_time1,NULL);
	fprintf(fp, "D, %d, %ld, %ld, %d\n", id, cur_time1.tv_sec, cur_time1.tv_usec, strlen(payload));
	fflush(fp);
}

int main() {
	ht_t *ht;
	char mess[100000]; 
	char topic[] = "top", client[] = "XBEE";

	increase_tam(); // incrementa el tamaño de paquetes
	
	for (int j = 1; j < 3; j++)
	{
		//int j = 0;
		ht = ht_create();
		printf("Empieza la nueva calidad \n");
		memset(mess, 0, sizeof(mess));
		printf(" Connect\n");
		MQTT_configuration(j);
		MQTT_connect(client, &on_message); // ondelivered 
		printf(" Creando el fichero \n");
		char name_file[40];
		sprintf(name_file, "Tam_pack_%dm_QOS_%d_NUMPACK_%d.txt",DISTANCE, j, (int)TAMPACKAGES);
		fp = fopen(name_file, "w+");
		MQTT_subs(topic, client, ht);
		fprintf(fp, "Type, Id paq, time stamp segundos, micro, long paq \n");

		for(int i=0; i<=(int)NUMPACKAGES; i++) {
			delay(600);
			gettimeofday(&cur_time,NULL);
			sprintf(mess, "%d,%ld, %s", i, cur_time.tv_sec*1000000 + cur_time.tv_usec, increase_load);
			fprintf(fp, "S, %d, %ld, %ld, %d\n", i, cur_time.tv_sec, cur_time.tv_usec, strlen(mess));
			MQTT_publish(mess, topic, ht, &on_delivered);  
		}
		delay(2000);
		MQTT_end(topic, client);
		fclose(fp);
	}

	return 0;
}
