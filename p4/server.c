#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

extern int portNum;
int fd;
MFS_CheckpointRegion_t *cr;
int TOTAL_NUM_INODES = 4096;
int DATA_BLOCK_SIZE = 4096;
int NUM_INODE_POINTERS = 14;
int IMAP_PIECE_SIZE = 16;
int LEN_NAME = 28;

int sanityCheckINum(int inum){
  if (inum < 0 || inum >= TOTAL_NUM_INODES) {
    perror("server_write: invalid inum_1");
    return -1;
  }
  return 1;
}

int sanityCheckBlock(int block){
  if( block < 0 || block > NUM_INODE_POINTERS - 1) {
    perror("server_write: invalid block_5");
    return -1;
  }
  return 1;
}

int findPopulateInode(int inum, MFS_Inode_t *currInode) {
  int imapIdx = inum / 16; /* imap piece num */

  if (cr->imap[imapIdx] == -1) {
    perror("server_read: invalid inum_2");
    return -1;
  }

  int imapDiskAdd =  cr->imap[imapIdx];
  int iNodeIdx = inum % IMAP_PIECE_SIZE; /* inode index within a imap piece */

  MFS_Imap_t imap;
  lseek(fd, imapDiskAdd, SEEK_SET);
  read(fd, &imap, sizeof(MFS_Imap_t));
  
  int inodeDiskAddress = imap.inodes[iNodeIdx]; /* gw: fp denotes file pointer (location within a file) */
  if(inodeDiskAddress == -1) {
    perror("server_read: invalid inum_3");
    return -1;
  }

  lseek(fd, inodeDiskAddress, SEEK_SET);
  read(fd, &currInode, sizeof(MFS_Inode_t));

  return 0;
}

int server_lookup(int pinum, char *name) {
    if (sanityCheckINum(pinum) == -1) return -1;

    MFS_Inode_t currInode;
    if (findPopulateInode(pinum, &currInode) == -1) return -1;
  
    char data_buf[MFS_BLOCK_SIZE]; /* gw: use char buffer, (no need free) */

    for(int i = 0; i < 14; i++) {

        int dataDiskAddress = currInode.data[i];	/* gw: tbc */
        if (dataDiskAddress == -1) continue;

        lseek(fd, dataDiskAddress, SEEK_SET);
        read(fd, data_buf, MFS_BLOCK_SIZE);
	  
        MFS_DirDataBlock_t *dir_buf = (MFS_DirDataBlock_t *)data_buf;
        for(int j = 0; j < 64; j++) {
            MFS_DirEnt_t *dirEntry = &dir_buf->entries[j];
            //      if(strcmp(p_de->name,name) == 0)
            if(dirEntry->inum == -1)
                continue;
            if(strncmp(dirEntry->name, name, LEN_NAME) == 0)
	            return dirEntry->inum;
        }
    }
    return -1;
}

int server_read(int inum, char* buffer, int block){
  if (sanityCheckINum(inum) == -1)return -1;
  if (sanityCheckBlock(block) == -1)return -1;

  MFS_Inode_t currInode;
  if (findPopulateInode(inum, &currInode) == -1) return -1;
  int dataDiskAddress = currInode.data[block]; 
  lseek(fd, dataDiskAddress, SEEK_SET);
  read(fd, buffer, MFS_BLOCK_SIZE);
  return 0;
}

void writeDataBlock(int offset, char* wr_buffer){
    cr->endLog += DATA_BLOCK_SIZE;
    lseek(fd, offset, SEEK_SET);
    write(fd, wr_buffer, MFS_BLOCK_SIZE);
}

void writeINode(int offset, MFS_Inode_t inode_new){
    int step = sizeof(MFS_Inode_t); /* inode size */
    cr->endLog += step;
    lseek(fd, offset, SEEK_SET);
    write(fd, &inode_new, step);
}

void writeImap(int offset, MFS_Imap_t imap_new){
    int step = sizeof(MFS_Imap_t);
    cr->endLog += step;
    lseek(fd, offset, SEEK_SET);
    write(fd, &imap_new, step);
}

void writeCheckpointRegion(){
    lseek(fd, 0, SEEK_SET);
    write(fd, cr, sizeof(MFS_CheckpointRegion_t));
}

int server_write(int inum, char* buffer, int block){
  MFS_Inode_t inode;
  MFS_Imap_t imap;
  MFS_Inode_t inode_new;

  if (sanityCheckINum(inum) == -1) return -1;
  if (sanityCheckBlock(block) == -1) return -1;

  char *ip = buffer;
  char wr_buffer[MFS_BLOCK_SIZE];
  for (int i = 0; i < MFS_BLOCK_SIZE; i++) {
    if( ip != NULL ) {
      wr_buffer[i] = *ip;
      ip++;
    } 
    else {
      wr_buffer[i] = '\0';
    }
  }

  int offset = cr->endLog;
  int writeToLog = 0;
  int imapIdx = inum / 16;
  int imapDiskAddress =  cr->imap[imapIdx];
  int inodeIdx = 0;
  if (imapDiskAddress != -1) {
    lseek(fd, imapDiskAddress, SEEK_SET);
    read(fd, &imap, sizeof(MFS_Imap_t));
    inodeIdx = inum % IMAP_PIECE_SIZE; 
    int inodeDiskAddress = imap.inodes[inodeIdx]; 

    if (inodeDiskAddress != -1){
      lseek(fd, inodeDiskAddress, SEEK_SET);
      read(fd, &inode, sizeof(MFS_Inode_t));
      if (inode.type != MFS_REGULAR_FILE) {
        perror("server_write: not a regular file_4");
        return -1;
      }
      // offset = inode.data[block];
      writeDataBlock(offset, wr_buffer);
      inode_new.size = (block +1) * MFS_BLOCK_SIZE;
      inode_new.type = inode.type;
      for (int i = 0; i < 14; i++) 
        inode_new.data[i] = inode.data[i]; /* copy data from old nd */
      inode_new.data[block] = offset;
    } else {
        writeToLog = 1;
    }
  } else {
    writeToLog = 1;
  }

  if (writeToLog) {
    writeDataBlock(offset, wr_buffer);
    inode_new.size = 0;
    //    nd_new.type = MFS_DIRECTORY;			  /* gw: tbc */
    inode_new.type = MFS_REGULAR_FILE;		  /* gw: tbc, likely this because write dont apply to dir */
    for (int i = 0; i < 14; i++) inode_new.data[i] = -1; /* copy data from old nd */
    inode_new.data[block] = offset;			  /* fp_block_new */
  }

  offset = cr->endLog;	/* after the latestly created block */
  writeINode(offset, inode_new);

  MFS_Imap_t imap_new;
  if (imapDiskAddress != -1) {
    for(int i = 0; i< IMAP_PIECE_SIZE; i++) 
      imap_new.inodes[i] = imap.inodes[i] ; /* copy old mp's data, mp is still in memory */
  } else {
    for(int i = 0; i< IMAP_PIECE_SIZE; i++) 
      imap_new.inodes[i] = -1 ; /* copy old mp's data, mp is still in memory */
  }
  imap_new.inodes[inodeIdx] = offset; 

  offset = cr->endLog;
  writeImap(offset, imap_new);
  cr->imap[imapIdx] = offset; 	/* gw: fp_mp_new */
  writeCheckpointRegion();
  fsync(fd);
  return 0;
}

int server_stat(int inum, MFS_Stat_t* m){
  if (sanityCheckINum(inum) == -1)return -1;
  MFS_Inode_t currInode;
  if (findPopulateInode(inum, &currInode) == -1)
    return -1;

  m->size = currInode.size;
  m->type = currInode.type;
  return 0;
}

int server_creat(int pinum, int type, char* name){
  if (sanityCheckINum(pinum) == -1)return -1;
  int len_name = 0;
  for (int i=0; name[i] != '\0'; i++, len_name++);
  if (len_name > LEN_NAME) {
    perror("server_creat: name too long_1");
    return -1;
  }

  /* if exists, creat is success */
  if(server_lookup(pinum, name) != -1) {
    return 0;
  }

  MFS_Inode_t currInode;
  if (findPopulateInode(pinum, &currInode) == -1)return -1;
  if (currInode.type != MFS_DIRECTORY)return -1;


  int free_inum = -1;
  int is_free_inum_found = 0;
  MFS_Imap_t imap;
  MFS_Imap_t mp_new;
  int imapIdx;
  int inodeIdx;
  for (int i=0; i < 256; i++) {
    imapIdx =  cr->imap[i];
    if (imapIdx != -1) {
      lseek(fd, imapIdx, SEEK_SET);
      read(fd, &imap, sizeof(MFS_Imap_t));
      for (int j=0; j<16; j++) {
        inodeIdx = imap.inodes[j]; /* gw: fp denotes file pointer (location within a file) */
        if (inodeIdx == -1) {
          free_inum = i*IMAP_PIECE_SIZE + j;
          is_free_inum_found = 1;
          break;
        }
      }
    } else {
      inodeIdx = 0;
      for(int j = 0; j< IMAP_PIECE_SIZE; j++) 
        mp_new.inodes[j] = -1 ; /* copy old mp's data, mp is still in memory */
      free_inum = i*IMAP_PIECE_SIZE;
      is_free_inum_found = 1;
    }
    if (is_free_inum_found) break;
  }
    
  char data_buf[4096];
  // char newbuf[4096];
  MFS_DirDataBlock_t* dir;
  MFS_Inode_t* newParInode = (MFS_Inode_t*)malloc(sizeof(MFS_Inode_t));
  newParInode->size = currInode.size;
  newParInode->type = currInode.type;
  for (int i = 0; i < 14; i++){
    newParInode->data[i] = currInode.data[i];
  }

  int offset = cr->endLog;
  MFS_Inode_t newInode;
  newInode.type = type;
  newInode.size = 0; 
  for (int i = 0; i < 14; i++){
    newInode.data[i] = -1;
  }
  if (type == MFS_DIRECTORY){
    char data_buf[MFS_BLOCK_SIZE]; 

    MFS_DirDataBlock_t* dir_buf = (MFS_DirDataBlock_t*)data_buf;
    strcpy(dir_buf->entries[0].name, ".");
    dir_buf->entries[0].inum = free_inum;//populate
    strcpy(dir_buf->entries[1].name, "..");
    dir_buf->entries[1].inum = pinum;//populate
    for(int i=2; i< 128; i++){
      strcpy(dir_buf->entries[i].name, "\0");
      dir_buf->entries[i].inum = -1;
    }
    writeDataBlock(offset, data_buf);
    newInode.data[0] = offset;    
  }

  int done = 0;
  int emptyIdx;
  for (int i = 0; i < 14 && done != 1; i++) {
    emptyIdx = i;
    if (newParInode->data[i] != -1){
      lseek(fd, newParInode->data[i], SEEK_SET);
      read(fd, data_buf, MFS_BLOCK_SIZE);
      dir = (MFS_DirDataBlock_t*)data_buf;
      for (int i=0; i< 128 && done != -1; i++){
        if (dir->entries[i].inum == -1){
          strcpy(dir->entries[i].name, name);
          dir->entries[i].inum = free_inum;
          done = 1;
        }
      }
    } else {
      dir = (MFS_DirDataBlock_t*)data_buf;
      strcpy(dir->entries[0].name, name);
      dir->entries[0].inum = free_inum;
      for (int i = 1; i< 128; i++){
        strcpy(dir->entries[i].name, "\0");
        dir->entries[i].inum = -1;
      }
    }
  }
  
  offset = cr->endLog;
  writeINode(offset, newInode);
  mp_new.inodes[inodeIdx] = offset;
  writeImap(offset, mp_new);
  cr->imap[imapIdx] = offset;
  offset = cr->endLog;
  writeDataBlock(offset, data_buf);
  newParInode->data[emptyIdx] = offset;
  offset = cr->endLog;
  writeINode(offset, *newParInode);
  imap.inodes[inodeIdx] = offset;
  offset = cr->endLog;
  writeImap(offset, imap);
  cr->imap[imapIdx] = offset;// check
  writeCheckpointRegion();
  fsync(fd);
  return 0;
}

int server_shutdown() {
  fsync(fd);
  exit(0);
}

// server code
int main(int argc, char *argv[]) {
    printf("hiii\n");
    int port = atoi(argv[1]);
    // char *fileimage = (char *)malloc(strlen(argv[2]) + 1);
    // strcpy(fileimage, argv[2]);
    //File exists
    if(access(argv[2], F_OK) == 0) {
        lseek(fd,0, SEEK_SET);
        read(fd, cr, sizeof(MFS_CheckpointRegion_t));
    } else {
        fd = open(argv[2], O_RDWR|O_CREAT, S_IRWXU);
    
        if(fd < 0) {
            printf("Cannot create file from image");
            return -1;
        }

        cr = (MFS_CheckpointRegion_t *) malloc(sizeof(MFS_CheckpointRegion_t));
        cr->inode_count = 0;
        cr->endLog = sizeof(MFS_CheckpointRegion_t);
        for(int i = 0; i < 256; i++) {
            cr->imap[i] = -1;
        }
    }
  
    int sd = UDP_Open(port);
    printf("port = %d\n", port);
    assert(sd > -1);
    while (1) {
	    struct sockaddr_in addr;
	    // char message[BUFFER_SIZE];
        MFS_message_t message, reply;
	    printf("server:: waiting...\n");
	    int rc = UDP_Read(sd, &addr, (char *) &message, sizeof(MFS_message_t));
	    printf("server:: read message [size:%d contents:(%d)]\n", rc, message.pinum);
	    if (rc > 0) {
            if(message.requestType == REQ_LOOKUP) {
                printf("REQ_LOOKUP\n");
                reply.pinum = message.pinum;
                //TODO: Add logic to fetch inum from the imap
                // reply.inum = 48;
                int rc = server_lookup(message.pinum, message.name);
                if(rc != 0) {
                  perror("server_lookup: error");
                  return -1;
                }
                //sprintf(reply, "goodbye world");
                rc = UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));//BUFFER_SIZE);            
            } else if(message.requestType == REQ_STAT) {
                printf("REQ_STAT\n");
                // reply.size = 28;
                // reply.type = 1;
                MFS_Stat_t m;

                int rc = server_stat(message.inum, &m);
                message.size = m.size;
                message.type = m.type;
                if(rc != 0) {
                  perror("server_stat: error");
                  return -1;
                }
                rc = UDP_Write(sd, &addr, (char *) &message, sizeof(MFS_message_t));
            } else if(message.requestType == REQ_WRITE) {
                // reply.size = 28;
                // reply.type = 1;
                printf("REQ_WRITE\n");
                int rc = server_write(message.inum, message.data, message.block);
                if(rc != 0) {
                  perror("server_write: error");
                  return -1;
                }
                rc = UDP_Write(sd, &addr, (char *) &message, sizeof(MFS_message_t));
            } else if(message.requestType == REQ_READ) {
                printf("REQ_READ\n");
                int rc = server_read(message.inum, message.data, message.block);
                if(rc != 0) {
                  perror("server_read: error");
                  return -1;
                }
                // reply.size = 28;
                // reply.type = 1;
                rc = UDP_Write(sd, &addr, (char *) &message, sizeof(MFS_message_t));
            } else if(message.requestType == REQ_UNLINK) {
                printf("UNLINK");
                // int rc = server_unlink(message.pinum, &message.name[0]);
            } else if(message.requestType == REQ_SHUTDOWN) { 
                printf("REQ_SHUTDOWN\n");

                message.requestType = REQ_RESPONSE;
                UDP_Write(sd, &addr, (char *) &message, sizeof(MFS_message_t));
                int rc = server_shutdown();
                if(rc != 0) {
                  perror("server_shutdown: error");
                  return -1;
                }

            } else {
                printf("ERROR\n");
            }   
	    } 
    }
    return 0; 
}