#include <stdio.h>
#include <string.h>

#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

extern int portNum;

// server code
int main(int argc, char *argv[]) {
    printf("hiii\n");
    int port = atoi(argv[1]);
    printf("port = %d\n", port);
//    int mfs_rc = MFS_Init("localhost", 5008);
//    printf("portnum = %d, rc = %d\n", portNum, mfs_rc);
    int sd = UDP_Open(port);
//    int sd = UDP_Open(10100);
    printf("yellow server test\n");
    assert(sd > -1);
    while (1) {
	struct sockaddr_in addr;
	char message[BUFFER_SIZE];
	printf("server:: waiting...\n");
	int rc = UDP_Read(sd, &addr, message, BUFFER_SIZE);
	printf("server:: read message [size:%d contents:(%s)]\n", rc, message);
	if (rc > 0) {
            char reply[BUFFER_SIZE];
            sprintf(reply, "goodbye world");
            rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
	    printf("server:: reply\n");
	} 
    }
    return 0; 
}
    


 
