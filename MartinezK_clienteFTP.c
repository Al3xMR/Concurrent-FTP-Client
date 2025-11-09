#include <stdio.h>

int connectTCP(const char *host, const char *service );

int main(){
	
	int sd = connectTCP("localhost", "ftp");

	if (sd >= 0 ) {
		printf("Socket created with file descriptor: %d\n", sd);
	} else {
		printf("Error to create the socket\n");
	}

	return 0;
}
