#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <signal.h>
#include <stdbool.h>

enum State {CONNECTED, DISCONNECTED, RECONNECTING} state;

struct sockaddr_in serv_addr, client;
const char serial_port[] = "/dev/serial0";
int fd, baudrate=B115200, server_socket, client_socket;

bool accepted = false, desconectado = false, reconnecting = false;

pthread_mutex_t mutex;

void accept_client();

/* ****************************************
 * Function name: setup_uart()
 *
 * Description:
 * 		Open UART communication
 *
 *************************************** */
int setup_uart() {
	/* Start UART communication */
	if((fd = open(serial_port, O_RDWR)) < 0) {
		return 1;
	}

	/* Configure Port */
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	/* Error Handling */
	if (tcgetattr(fd, &tty) != 0) {
		printf("[ERROR] result %i from tcgetattr: %s\n", errno, strerror(errno));
	}
	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	tty.c_cc[VTIME] = 0; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;
	// Set in/out baud rate to be 115200
	cfsetispeed(&tty, baudrate);
	cfsetospeed(&tty, baudrate);
	// Save tty settings, also checking for error
	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    		printf("[ERROR] result %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	// Enlace simbolico de puerto AMA

	char target_path[256];
	const char* link_path = serial_port;

	/* Attempt to read the target of the symbolic link. */
	int len = readlink (link_path, target_path, sizeof (target_path));
	
	if (len == -1) {
		/* The call failed. */
		if (errno == EINVAL)
		/* It's not a symbolic link; report that. */
		fprintf (stderr, "%s is not a symbolic link\n", link_path);
		else
		/* Some other problem occurred; print the generic message. */
		perror ("readlink");
		return 1;
	}

	printf("[INFO] Serial communication opened, using port %s.\n", target_path);
	fflush(stdout);

	return 0;
}

/* ****************************************
 * Function name: setup_socket()
 * 
 * Description:
 * 		Create and configure client socket
 * 
 *************************************** */
int setup_socket() {
	int opt = 1;
	server_socket = 0;
	/* Listening socket */
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		printf("[ERROR] can't create socket.\n");
		return -1;
	}

	// /* Allow address reuse on socket */
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("[ERROR] setsockopt");
		exit(EXIT_FAILURE);
	}
	/* Address, port and socket protocol */
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(1881);
	int addrlen = sizeof(struct sockaddr_in);
	/* Binds server socket to localhost */
	if (bind(server_socket, (struct sockaddr *)&serv_addr, addrlen) < 0) {
		perror("[ERROR] can't bind socket.");
		exit(EXIT_FAILURE);
	}
	/* Allows 1 client */
	if (listen(server_socket, 3) < 0) {
		perror("[ERROR] can't listen to socket.");
		exit(EXIT_FAILURE);
	}

	/* Accept a connection */
	accept_client();

	return 0;
}

/* ****************************************
 * Function name: accept_client()
 * 
 * Description:
 * 		Reconnect socket
 * 
 *************************************** */
void accept_client() {
	socklen_t sin_size = sizeof(struct sockaddr_in);
	//close(client_socket);
	client_socket = -1;
	if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &sin_size)) < 0) {
		perror("[WARNING] can't connect to remote server, will try again in 5 seconds...\n");
		sleep(5);
	}
	char *client_ip = inet_ntoa(client.sin_addr);
	printf("[INFO] accepted new connection from client %s:%d\n", client_ip, ntohs(client.sin_port));

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

/* ****************************************
 * Function name: reconnect_uart()
 * 
 * Description:
 * 		Reconnect UART
 * 
 *************************************** */
void reconnect_uart() {
	close(fd);
	fd = -1;

	setup_uart();
	
}

/* ****************************************
 * Function name: thread_socketsss
 *
 * Description:
 * 		Forward socket data to UART
 *
 *************************************** */
 void *thread_socket(void * arg) {
	int valread;
	char buffer[4096] = {0};
	/* Reading all the time */
	for(;;) {
		if (state == CONNECTED)
		{
		memset(buffer, 0, sizeof(buffer));
		valread = read(client_socket, buffer, sizeof(buffer));
		//valread = recv(client_socket, buffer, sizeof(buffer), MSG_WAITALL);
		if(valread > 0) {
			printf("[INFO] forwarding data from socket to UART...\n");
		
			int written = write(fd, buffer, valread);
			printf("Buffer : %s \n" ,buffer);
			//int written = send(fd, buffer, valread, 0);
			if(written < 0) {
				printf("[WARNING] can't write UART.\n");
				pthread_mutex_lock(&mutex);
				if (state == CONNECTED)
				{
					reconnecting = true; // Reconnect_uart
				}
				pthread_mutex_unlock(&mutex);
				
			}
		} else if(valread <= 0) {
			printf("[WARNING] can't read socket.\n");
			pthread_mutex_lock(&mutex);
			if (state == CONNECTED )
			{
				accepted = true;
			}
			pthread_mutex_unlock(&mutex);
			
		}
		// if(accepted){
		// 	pthread_mutex_lock(&mutex);
		// 	accept_client();
		// 	accepted = false;
		// 	pthread_mutex_unlock(&mutex);
		// }
	}
}
 }

/* ****************************************
 * Function name: thread_uart
 * 
 * Description:
 * 		Forward UART data to socket
 * 
 *************************************** */
 void * thread_uart(void * arg) {
	int valread;
	char buffer[4096] = {0};
	/* Reading all the time */
	for(;;) {
		if (state == CONNECTED){
		
		memset(buffer, 0, sizeof(buffer));
		valread = read(fd, buffer, sizeof(buffer));
		//valread = recv(fd, buffer, sizeof(buffer), MSG_WAITALL);
		if (valread < 0) {
			printf("[WARNING] can't read UART.\n");
			pthread_mutex_lock(&mutex);
			if (state == CONNECTED)
			{
				reconnecting = true;
			}
			pthread_mutex_unlock(&mutex);
		}

		else if(valread > 0) {
			printf("[INFO] forwarding data from UART to socket...\n");
			int written = write(client_socket, buffer, valread);
			//ssize_t written = send(client_socket, buffer, valread, 0);
			if (written < 0) {
				printf("[WARNING] can't write socket... \n");
				pthread_mutex_lock(&mutex);
				if (state == CONNECTED)
				{
					accepted = true;
				}
				pthread_mutex_unlock(&mutex);
			}
		}

		}
		
	}
}

/* ****************************************
 * Function name: main()
 *
 * Description:
 * 		Create both threads and call
 * 		setup function
 *
 *************************************** */
int main() {

	pthread_t h1, h2;
	state = DISCONNECTED;
	// setup_uart();
	// setup_socket();
    pthread_mutex_init(&mutex, NULL);
	pthread_create(&h1, NULL, thread_socket, NULL);
   	pthread_create(&h2, NULL, thread_uart, NULL);
	
	// 
	for(;;) {
		switch (state)
		{
			case DISCONNECTED:
				// Iniciar la comunicacion
				pthread_mutex_lock(&mutex);		
				int uart = setup_uart();
				int socket = setup_socket();

				if(uart == 0 && socket == 0){
					state = CONNECTED;
				}
				desconectado = false;
				accepted = false;
				pthread_mutex_unlock(&mutex);
			break;
			
			case CONNECTED:
				if(desconectado) {
					printf("Desconectado \n");
					pthread_mutex_lock(&mutex);
					state = DISCONNECTED;
					pthread_mutex_unlock(&mutex);
				
				}
				else if(accepted){
					printf(" Aceptamos nuevas conexiones \n");
					pthread_mutex_lock(&mutex);
					accept_client();
				
					accepted = false;
					pthread_mutex_unlock(&mutex);
				}

				else if(reconnecting){
					pthread_mutex_lock(&mutex);
					state = RECONNECTING;
					pthread_mutex_unlock(&mutex);
				}
			break;

			case RECONNECTING:

				pthread_mutex_lock(&mutex);
				reconnect_uart();
				reconnecting = false;
				state = CONNECTED;
				pthread_mutex_unlock(&mutex);
		
			break;


			default:
			break;
		}
	}

	return 0;
}