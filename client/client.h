#include "project.h"
#include "hashtable.h"
#include <sys/time.h>

struct timeval cur_time;

void on_message(char * TOPIC, char * payload);

void on_delivered(char * payload);