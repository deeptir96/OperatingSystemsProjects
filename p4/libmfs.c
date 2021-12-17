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

    int sd = UDP_Open(0);
    if(sd < -1)
    {
        perror("udp_send: failed to open socket.");
        return -1;
    }

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

    int numTries = 5;
    do {
        FD_ZERO(&rfds);
        FD_SET(sd,&rfds);
        UDP_Write(sd, &addr, (char*)tx, sizeof(MFS_message_t));
        printf("Sent message to server via UDP\n");
        if(select(sd+1, &rfds, NULL, NULL, &tv)) {
            rc = UDP_Read(sd, &addr2, (char*)rx, sizeof(MFS_message_t));
            printf("UDP_Recv size = %d\n", rc);
            if(rc > 0)
            {
                UDP_Close(sd);
                return 0;
            }
        } else {
            printf("Hello? %d\n", numTries);
            numTries--;
        }
    } while(numTries > 0);
    
    return 0;
}

int MFS_Init(char *hostname, int port) {
    //printf("Hostname = %s, port = %d\n", hostname, port);
    printf("REQ_INIT\n");
    host = strdup(hostname);
    portNum = port;

    return 0;
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
    m->type = send.type;
    m->size = send.size;
    return 0;
}

int MFS_Write(int inum, char *buffer, int block) {
    // check inum, block, and type
    MFS_Stat_t stat;
    int stat_rc = MFS_Stat(inum, &stat);
    if(stat_rc != 0 || stat.type != 1) {
        return -1;
    }
    MFS_message_t send, rcv;
    send.inum = inum;
    send.block = block;
    send.type = stat.type;
    send.requestType = REQ_WRITE;
    strcpy(send.data, buffer);
    for(int i=0; i<MFS_BLOCK_SIZE; i++)
	  send.data[i]=buffer[i];
	
	if(UDP_Send( &send, &rcv, host, portNum) < 0)
		return -1;
	
	return rcv.inum;
}

//probably need to change rc in all fns below
int MFS_Read(int inum, char *buffer, int block){
    MFS_message_t tx;
    tx.inum = inum;
    tx.block = block;
    tx.requestType = REQ_READ;

    MFS_message_t rx;	
    if(UDP_Send( &tx, &rx, host, portNum) < 0)
        return -1;

    if(rx.inum > -1) {
        for(int i=0; i<MFS_BLOCK_SIZE; i++)
        buffer[i] = rx.data[i];
    }

    return rx.inum;
}

int MFS_Creat(int pinum, int type, char *name){
    if(strlen(name) > 60 || name == NULL)
		return -1;

	MFS_message_t tx;

	strcpy(tx.name, name);
	tx.inum = pinum;
	tx.type = type;
	tx.requestType = REQ_CREAT;

	MFS_message_t rx;	
	if(UDP_Send( &tx, &rx, host, portNum) < 0)
		return -1;

	return rx.inum;
}

int MFS_Unlink(int pinum, char *name) {
    if(strlen(name) > 60 || name == NULL)
		return -1;
	
	MFS_message_t tx;

	tx.inum = pinum;
	tx.requestType = REQ_UNLINK;
	strcpy(tx.name, name);

	MFS_message_t rx;	
	if(UDP_Send( &tx, &rx, host, portNum ) < 0)
		return -1;

	return rx.inum;
}

int MFS_Shutdown() {
    MFS_message_t tx;
	tx.requestType = REQ_SHUTDOWN;

	MFS_message_t rx;
	if(UDP_Send( &tx, &rx, host, portNum) < 0)
		return -1;
	
	return 0;
}
