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
int isServerUp = 0;

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
    // rc = UDP_Read(sd, &addr2, (char*)rx, sizeof(MFS_message_t));
    do {
        FD_ZERO(&rfds);
        FD_SET(sd,&rfds);
        UDP_Write(sd, &addr, (char*)tx, sizeof(MFS_message_t));
        // printf("Sent message to server via UDP\n");
        if(select(sd, &rfds, NULL, NULL, &tv)) {
            rc = UDP_Read(sd, &addr2, (char*)rx, sizeof(MFS_message_t));
            printf("UDP_Recv size = %d\n", rc);
            if(rc > 0)
            {
                UDP_Close(sd);
                return 0;
            } 
        } else {
            // printf("Helloxxx? %d\n", numTries);
            numTries--;
        }
    } while(numTries > 0);
    
    return 0;
}

int MFS_Init(char *hostname, int port) {
    //printf("Hostname = %s, port = %d\n", hostname, port);
    printf("REQ_INIT\n");
    printf("MFSInit::: isServerUp = %d\n", isServerUp);
    host = strdup(hostname);
    portNum = port; 
    isServerUp = 1;
    return 0;
}

int MFS_Lookup(int pinum, char *name) {
    printf("MFSLookup::: isServerUp = %d\n", isServerUp);
    if(!isServerUp) return -1;
    // check invalid pinum and name
    MFS_message_t send, rcv;
    
    send.pinum = pinum;
    send.requestType = REQ_LOOKUP;
    strcpy((char *)&send.name, name);
    printf("send . pinum = %d\n", send.pinum);
    int rc =  UDP_Send(&send, &rcv, host, portNum);
    return rc == 0 ? rcv.inum : -1;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
    printf("MFSStat::: isServerUp = %d\n", isServerUp);
    if(!isServerUp) return -1;
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
    printf("stat type = %d, size = %d\n", m->type, m->size);
    return 0;
}

int MFS_Write(int inum, char *buffer, int block) {
    printf("MFSWrite::: isServerUp = %d\n", isServerUp);
    if(!isServerUp) return -1;

    // check inum, block, and type
    MFS_Stat_t stat;
    int stat_rc = MFS_Stat(inum, &stat);
    printf("mfs_write: stat type = %d\n", stat.type);
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
	
	return 0;
}

//probably need to change rc in all fns below
int MFS_Read(int inum, char *buffer, int block){
    printf("MFSRead::: isServerUp = %d\n", isServerUp);
    if(!isServerUp) return -1;

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

    return 0;
}

int MFS_Creat(int pinum, int type, char *name){
    printf("MFSCreat::: isServerUp = %d\n", isServerUp);
    if(!isServerUp) return -1;

    if(strlen(name) > 60 || name == NULL)
		return -1;

	MFS_message_t tx;

	strcpy(tx.name, name);
	tx.pinum = pinum;
	tx.type = type;
	tx.requestType = REQ_CREAT;

	MFS_message_t rx;	
	if(UDP_Send( &tx, &rx, host, portNum) < 0)
		return -1;

	return 0;
}

int MFS_Unlink(int pinum, char *name) {
    printf("MFSUnlink::: isServerUp = %d\n", isServerUp);
    if(!isServerUp) return -1;

    if(strlen(name) > 60 || name == NULL)
		return -1;
	
	MFS_message_t tx;

	tx.pinum = pinum;
	tx.requestType = REQ_UNLINK;
	strcpy(tx.name, name);

	MFS_message_t rx;	
	if(UDP_Send( &tx, &rx, host, portNum ) < 0)
		return -1;

	return 0;
}

int MFS_Shutdown() {
    MFS_message_t tx;
	tx.requestType = REQ_SHUTDOWN;

    printf("port = %d\n", portNum);
	MFS_message_t rx;
	if(UDP_Send( &tx, &rx, host, portNum) < 0)
		return -1;
	
	return 0;
}
