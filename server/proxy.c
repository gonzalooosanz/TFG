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

struct sockaddr_in serv_addr, client;
const char serial_port[] = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0"; 
int fd, baudrate=B115200, client_socket;

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
	/* Configure Port */	///dev/ttyAMA0
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
	 
	// Enlace simbolico de puerto USB

	char target_path[256];
	char* link_path = serial_port;

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


// void reconnect_to_port(){

// 	printf("Reconnecting to the new port \n");
// 	if((fd = open(serial_port, O_RDWR)) < 0) {
// 		return 1;
// 	}
// }

/* ****************************************
 * Function name: setup_socket()
 * 
 * Description:
 * 		Create and configure client socket
 * 
 *************************************** */
int setup_socket() {
	int opt = 1;
	client_socket = 0;
	/* Listening socket */
	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
		printf("\n[ERROR] can't create socket \n"); 
		return -1; 
	} 
	/* Allow address reuse on socket */
	if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
	 	perror("[ERROR] setsockopt");
		exit(EXIT_FAILURE);
	}
	/* Address, port and socket protocol */
	memset(&client, '0', sizeof(client));
	client.sin_addr.s_addr = inet_addr("127.0.0.1");
	client.sin_family = AF_INET;
	client.sin_port = htons(1883);
	//Connect to remote server
	if (connect(client_socket, (struct sockaddr *)&client , sizeof(client)) < 0) {
		perror("[ERROR] can't connect to remote server.");
		return 1;
	}
	printf("[INFO] socket created, listening to new connections. \n");
	// Ignore broken pipe to avoid program finalization.
	signal(SIGPIPE, SIG_IGN);
	return 0;
}

/* ****************************************
 * Function name: reconnect_socket()
 * 
 * Description:
 * 		Reconnect socket
 * 
 *************************************** */

void reconnect_socket() {
	
	close(client_socket);

	printf("[INFO] Reconnecting to SOCKET ...\n");

	setup_socket();
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
	//fd = -1;

	printf("[INFO] Reconnecting to UART ...\n");

	setup_uart();
}


/* ****************************************
 * Function name: thread_socket
 * 
 * Description:
 * 		Forward socket data to UART
 * 
 *************************************** */
void *thread_socket() {
	int valread;
	char buffer[4096] = {0};
	/* Reading all the time */
	for(;;) {
		memset(buffer, 0, sizeof(buffer));
		valread = read(client_socket, buffer, sizeof(buffer));
		if(valread > 0) {
			printf("[INFO] forwarding data from socket to UART...\n");
			int written = write(fd, buffer, valread);
			if(written < 0) {
				printf("[WARNING] can't write UART.\n");
				reconnect_uart();
			}
		} else if(valread <= 0) {
			printf("[WARNING] can't read socket.\n");
			if((fd = open(serial_port, O_RDWR)) < 0) {
				perror("[ERROR] can't connect to UART, will try again in 5 seconds...");
			}
			reconnect_socket();
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
void *thread_uart() {
	int valread;
	char buffer[4096] = {0}; 
	/* Reading all the time */
	for(;;) {
		memset(buffer, 0, sizeof(buffer));
		valread = read(fd, buffer, sizeof(buffer));
		if (valread < 0) {
			printf("[WARNING] can't read UART.\n");
			reconnect_uart();
		} 
		else if(valread > 0) {
			printf("[INFO] forwarding data from UART to socket...\n");
			int written = write(client_socket, buffer, valread);
			printf("Buffer : %s \n" ,buffer);
			
			if (written < 0) {
				printf("[WARNING] can't write socket... \n");
				reconnect_socket();
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
	setup_uart();
	setup_socket();
	pthread_create(&h1, NULL, thread_socket, NULL);
   	pthread_create(&h2, NULL, thread_uart, NULL);
	
	for(;;) {}

	return 0;
}
