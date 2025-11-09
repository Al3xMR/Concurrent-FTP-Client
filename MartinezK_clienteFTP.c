#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

int connectTCP(const char *host, const char *service );

int main(int argc, char *argv[]){

	char *srv_add;
	char *port;

	switch (argc) {
		case 1: {
			srv_add = "localhost";
			port = "ftp";
			break;
		}
		case 2: {
			srv_add = argv[1];
                        port = "ftp";
                        break;
		}
		case 3: {
			srv_add = argv[1];
                        port = argv[2];
                        break;
		}
		default: {
			printf("Usage: ./MartinezK_clienteFTP <server> <port>\nDefault host: localhost\nDefault port: 21\n");
			return 1;
		}
	}
	
	// -*-*-*-*-*-*-* Master socket -*-*-*-*-*-*-*-*
	
	int sd = connectTCP(srv_add, port);

	sd >= 0 ? printf("Socket created with file descriptor: %d\n", sd) : printf("Error to create the socket\n");

	char buf[500];
	char msg[600];

	ssize_t bytes_recv = recv(sd, buf, sizeof(buf) - 1, 0);
	ssize_t bytes_sent;

	if(bytes_recv > 0){ 
		buf[bytes_recv] = '\0';
		printf("Server response: %s\n", buf);
	}else{
		printf("Server did not respond\n");
	}

	// ------------- Username read -----------------	
	memset(buf, 0, sizeof(buf));
	printf("USER: ");
	fflush(stdout);
	bytes_recv = read(0, buf, sizeof(buf) - 1);
	buf[bytes_recv] = '\0';

	// ------------- Username formatting ----------
	buf[strcspn(buf, "\r\n")] = '\0';
	snprintf(msg, sizeof(msg), "USER %s\r\n", buf);

	// ------------- Username sent to server ------
	bytes_sent = send(sd, msg, strlen(msg), 0);
	memset(buf, 0, sizeof(buf));
	bytes_recv = recv(sd, buf, sizeof(buf) - 1, 0);
	buf[bytes_recv] = '\0';
	printf("SERVER ANSWER: %s\n", buf);

	// ------------- Password read -----------------
	memset(buf, 0, sizeof(buf));
	memset(msg, 0, sizeof(msg));
	printf("PASSWORD: ");
	fflush(stdout);
	bytes_recv = read(0, buf, sizeof(buf) - 1);
	buf[bytes_recv] = '\0';

	// ------------- Password formatting ----------
	buf[strcspn(buf, "\r\n")] = '\0';
	snprintf(msg, sizeof(msg), "PASS %s\r\n", buf);

	// ------------- Password sent to server ------
	bytes_sent = send(sd, msg, strlen(msg), 0);
	memset(buf, 0, sizeof(buf));
	bytes_recv = recv(sd, buf, sizeof(buf) - 1, 0);
	buf[bytes_recv] = '\0';
	printf("SERVER ANSWER: %s\n", buf);

	//while(1) {


	
	

	return 0;
}
