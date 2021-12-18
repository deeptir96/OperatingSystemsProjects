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
   
   int inum = MFS_Lookup(0, "test1");
   printf("old inum = %d\n", inum);
    MFS_Creat(0, MFS_REGULAR_FILE, "test1");
      inum = MFS_Lookup(0, "test1");
      printf("new inum  =%d\n", inum);
      char* buf1 = "asdfghjkl";
      MFS_Write(inum, buf1, 0);

      char* buf2 = (char *)malloc(4096);
      MFS_Read(inum, buf2, 0);

      printf("%s\n", buf2);
     
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
    MFS_Shutdown();
    return 0;
}

