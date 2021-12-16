#include <stdio.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

extern int sd;
extern struct sockaddr_in addrSnd;


// client code
int main(int argc, char *argv[]) {
    struct sockaddr_in addrRcv;//, addrSnd;
    
    int serverPort = atoi(argv[1]);
    int rc = MFS_Init("localhost", serverPort); //UDP_FillSockAddr(&addrSnd, "localhost", serverPort);
    printf("sd =  %d, serverPort = %d\n", sd, serverPort);
    
    char message[BUFFER_SIZE];
    sprintf(message, "hello world");

    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
	printf("client:: failed to send\n");
	exit(1);
    }

    printf("client:: wait for reply...\n");
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    return 0;
}

