#include <stdio.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

extern int sd;
extern int portNum;

// client code
int main(int argc, char *argv[]) {
    struct sockaddr_in addrSnd, addrRcv;
    
    int serverPort = atoi(argv[1]);
    sd = UDP_Open(20100);
    int mfs_rc = MFS_Init("localhost", serverPort);
    printf("sd =  %d, port = %d, serverPort = %d\n", sd, portNum, serverPort);
    printf("Random txt %d\n", mfs_rc);
    int rc = UDP_FillSockAddr(&addrSnd, "localhost", serverPort);
    
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

