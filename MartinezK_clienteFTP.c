#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>


int connectTCP(const char *host, const char *service );

void readFromShell( char *buf);

typedef struct {
    char ip[32];
    int port;
    bool ok;
} PassiveInfo;

PassiveInfo enterPassiveMode(int sd);

int main(int argc, char *argv[]){

	char *srv_add;

	switch (argc) {
		case 1: {
			srv_add = "localhost";
			break;
		}
		case 2: {
			srv_add = argv[1];
                        break;
		}
		default: {
			printf("Usage: ./MartinezK_clienteFTP <server>\nDefault host: localhost\n");
			return 1;
		}
	}

	// -*-*-*-*-*-*-* Master socket -*-*-*-*-*-*-*-*

	int sd = connectTCP(srv_add, "ftp");

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

	while(1) {
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



		// -------------- Validation ------------------
		if (buf[0] == '2') {
			break;
		}
	}


	// ---------------- Transfer mode ---------------

	printf("Passive mode: on\n");
	PassiveInfo info;
	info.ok = false;
	info.ip[0] = '\0';
	info.port = 0; 
	info = enterPassiveMode(sd);
	printf("\nIP:%s\nPuerto:%d\n", info.ip, info.port);
	// ------------- Prompt ----------------------------
	readFromShell(buf);
	printf("%s", buf);

	return 0;
}

void readFromShell( char *buf) {
	bool validInput = false;
	ssize_t bytes_recv;
	while(!validInput) {
		memset(buf, 0, sizeof(&buf));
		printf("ftp> ");
		fflush(stdout);
		bytes_recv = read(0, buf, sizeof(&buf) - 1);
		buf[bytes_recv] = '\0';
        	buf[strcspn(buf, "\r\n")] = '\0';

		if (!strcmp(buf, "list")) {
			validInput = true;
		} else {
			printf("Insert a valid input\n");
		}
	}
}

PassiveInfo enterPassiveMode(int sd) {
	PassiveInfo info;
	info.ok = false;
	memset(info.ip, 0, sizeof(info.ip));

	char msg[64];
	char buf[256];
	ssize_t bytes_sent, bytes_recv;

	snprintf(msg, sizeof(msg), "PASV\r\n");
	bytes_sent = send(sd, msg, strlen(msg), 0);
	if (bytes_sent <= 0) return info;
	memset(buf, 0, sizeof(buf));

	bytes_recv = recv(sd, buf, sizeof(buf) - 1, 0);
	if (bytes_recv <= 0) return info;

	buf[bytes_recv] = '\0';

	int h1, h2, h3, h4, p1, p2;
	int matched = sscanf(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
	if (matched != 6) return info;

	snprintf(info.ip, sizeof(info.ip), "%d.%d.%d.%d", h1, h2, h3, h4);
	info.port = p1 * 256 + p2;
	info.ok = true;
	return info;
}
