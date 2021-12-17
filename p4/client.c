#include <stdio.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

extern int sd;
extern struct sockaddr_in addrSnd;


// client code
int main(int argc, char *argv[]) {
    //struct sockaddr_in addrRcv;//, addrSnd;
    
    int serverPort = atoi(argv[1]);
    int rc = MFS_Init("localhost", serverPort); //UDP_FillSockAddr(&addrSnd, "localhost", serverPort);
    printf("sd =  %d, serverPort = %d\n", sd, serverPort);
   
    int inum = MFS_Lookup(3, "lookup");
    // int inum = 28;
    MFS_Stat_t stat;
    rc = MFS_Stat(inum, &stat);
    
    if (rc < 0) {
	    printf("client:: MFS_Stat failed\n");
	    //exit(1);
    }
    printf("client:: MFS_Stat request Type = %d, size = %d\n", stat.type, stat.size);

    //printf("client:: wait for reply...\n");
    //rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    //printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    printf("client: got reply inum = %d\n", inum);
    return 0;
}

