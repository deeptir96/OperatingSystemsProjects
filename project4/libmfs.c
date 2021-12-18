/* this is working fine I think it passed the result  */
/* packets are encapsulated correctly */
/* UD send and receive Ok */
/* gw: maybe need to load all imap piece into mem? */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "udp.h"

#include "mfs.h"

int UDP_Send( MFS_Message_t *msend, MFS_Message_t *mrecv, char *hostname, int port)
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

    fd_set xyz;
    struct timeval t;
    t.tv_sec=3;
    t.tv_usec=0;

    int trial_limit = 5;	/* trial = 5 */
    do {
        FD_ZERO(&xyz);
        FD_SET(sd,&xyz);
        UDP_Write(sd, &addr, (char*)msend, sizeof(MFS_Message_t));
        if(select(sd+1, &xyz, NULL, NULL, &t))
        {
            rc = UDP_Read(sd, &addr2, (char*)mrecv, sizeof(MFS_Message_t));
            if(rc > 0)
            {
                UDP_Close(sd);
                return 0;
            }
        }else {
            trial_limit --;
        }
    }while(1);
}

char* server_host = NULL;
int server_port = 3000;
int online = 0;

int MFS_Init(char *hostname, int port) {
	server_host = strdup(hostname);
	server_port = port;
	online = 1;
	return 0;
}

int MFS_Lookup(int pinum, char *name){
	if(!online)
		return -1;

	if(strlen(name) > 60 || name == NULL)
		return -1;

	MFS_Message_t msend;
	MFS_Message_t mrecv;

	msend.inum = pinum;
	msend.requestType = REQ_LOOKUP;

	strcpy((char*)&(msend.name), name);

	if(UDP_Send( &msend, &mrecv, server_host, server_port) < 0)
	  return -1;
	else
	  return mrecv.inum;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
	if(!online)
		return -1;

	MFS_Message_t msend;
	msend.inum = inum;
	msend.requestType = REQ_STAT;

	MFS_Message_t mrecv;
	if(UDP_Send( &msend, &mrecv, server_host, server_port) < 0)
		return -1;
	m->type = mrecv.stat.type;
	m->size = mrecv.stat.size;
	return 0;
}

int MFS_Write(int inum, char *buffer, int block){
	int i = 0;
	if(!online)
		return -1;

	MFS_Message_t msend;
	MFS_Message_t mrecv;

	msend.inum = inum;

	for(i=0; i<MFS_BLOCK_SIZE; i++)
	  msend.data[i]=buffer[i];

	msend.block = block;
	msend.requestType = REQ_WRITE;

	if(UDP_Send( &msend, &mrecv, server_host, server_port) < 0)
		return -1;

	return mrecv.inum;
}

int MFS_Read(int inum, char *buffer, int block){
  int i = 0;
  if (!online)
    return -1;

  MFS_Message_t msend;


  msend.inum = inum;
  msend.block = block;
  msend.requestType = REQ_READ;

  MFS_Message_t mrecv;
  if(UDP_Send( &msend, &mrecv, server_host, server_port) < 0)
    return -1;

  if(mrecv.inum > -1) {
    for(i=0; i<MFS_BLOCK_SIZE; i++)
      buffer[i]=mrecv.data[i];
  }
  return mrecv.inum;
}

int MFS_Creat(int pinum, int type, char *name){
	if(!online)
		return -1;

	if(strlen(name) > 60 || name == NULL)
		return -1;

	MFS_Message_t msend;

	strcpy(msend.name, name);
	msend.inum = pinum;
	msend.type = type;
	msend.requestType = REQ_CREAT;

	MFS_Message_t mrecv;
	if(UDP_Send( &msend, &mrecv, server_host, server_port) < 0)
		return -1;

	return mrecv.inum;
}

int MFS_Unlink(int pinum, char *name){
	if(!online)
		return -1;

	if(strlen(name) > 60 || name == NULL)
		return -1;

	MFS_Message_t msend;

	msend.inum = pinum;
	msend.requestType = REQ_UNLINK;
	strcpy(msend.name, name);

	MFS_Message_t mrecv;
	if(UDP_Send( &msend, &mrecv,server_host, server_port ) < 0)
		return -1;

	return mrecv.inum;
}

int MFS_Shutdown(){
  MFS_Message_t msend;
	msend.requestType = REQ_SHUTDOWN;
	MFS_Message_t mrecv;
	if(UDP_Send( &msend, &mrecv,server_host, server_port) < 0)
		return -1;

	return 0;
}