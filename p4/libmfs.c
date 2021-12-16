#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "mfs.h"
#include "udp.h"

int sd, portNum;
char *host = NULL;
struct sockaddr_in addrSnd;

int UDP_Send( MFS_message_t *tx, MFS_message_t *rx, char *hostname, int port)
{

    // int sd = UDP_Open(0);
    // if(sd < -1)
    // {
    //     perror("udp_send: failed to open socket.");
    //     return -1;
    // }

    struct sockaddr_in addr, addr2;
    int rc = UDP_FillSockAddr(&addr, hostname, port);
    if(rc < 0)
    {
        perror("upd_send: failed to find host");
        return -1;
    }

    fd_set rfds;
    struct timeval tv;
    tv.tv_sec=3;
    tv.tv_usec=0;

    int trial_limit = 5;	/* trial = 5 */
    do {
        FD_ZERO(&rfds);
        FD_SET(sd,&rfds);
        UDP_Write(sd, &addr, (char*)tx, sizeof(MFS_message_t));
        if(select(sd+1, &rfds, NULL, NULL, &tv))
        {
            rc = UDP_Read(sd, &addr2, (char*)rx, sizeof(MFS_message_t));
            if(rc > 0)
            {
                UDP_Close(sd);
                return 0;
            }
        }else {
            trial_limit--;
        }
    }while(1);
}

int MFS_Init(char *hostname, int port) {
    //printf("Hostname = %s, port = %d\n", hostname, port);
    host = strdup(hostname);
    portNum = port;
    sd = UDP_Open(20100);
    if(sd < -1)
    {
        perror("mfs_init: failed to open socket.");
        return -1;
    }
    
    // int rc = UDP_FillSockAddr(&addrSnd, "localhost", port);
    //portNum = port;
    
    return 0;//rc;
}

int MFS_Lookup(int pinum, char *name) {
// check invalid pinum and name
    MFS_message_t send, rcv;
    
    send.pinum = pinum;
    send.requestType = REQ_LOOKUP;
    strcpy((char *)&send.name, name);

    int rc =  UDP_Send(&send, &rcv, host, portNum);
    return rc == 0 ? rcv.inum : -1;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
    // check inum does not exist
    MFS_message_t send, rcv;

    send.inum = inum;
    send.requestType = REQ_STAT;
    int rc = UDP_Send(&send, &rcv, host, portNum);
    if(rc != 0) {
        return -1;
    }
    m->type = rcv.type;
    m->size = rcv.size;
    return 0;
}

int MFS_Write(int inum, char *buffer, int block) {
    return 0;
}
int MFS_Read(int inum, char *buffer, int block){
    return 0;
}
int MFS_Creat(int pinum, int type, char *name){
    return 0;
}
int MFS_Unlink(int pinum, char *name) {
    return 0;
}
int MFS_Shutdown() {
    //persist inodeMap to disk
    exit(0);
    return 0;
}
