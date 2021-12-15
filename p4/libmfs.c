#include <stdio.h>
#include "mfs.h"
#include "udp.h"

int sd, portNum;

int MFS_Init(char *hostname, int port) {
    //printf("Hostname = %s, port = %d\n", hostname, port);
    sd = UDP_Open(20000);
    portNum = port;
    return 0;
}

int MFS_Lookup(int pinum, char *name) {
    return 0;
}
int MFS_Stat(int inum, MFS_Stat_t *m) {
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
    exit(0);
    return 0;
}
