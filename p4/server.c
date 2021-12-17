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
int NUM_INODE_POINTERS = 14;
int IMAP_PIECE_SIZE = 16;
int LEN_NAME = 28;

int sanityCheck(int inum, int block){
  if (inum < 0 || inum >= TOTAL_NUM_INODES) {
    perror("server_write: invalid inum_1");
    return -1;
  }


  if( block < 0 || block > NUM_INODE_POINTERS - 1) {
    perror("server_write: invalid block_5");
    return -1;
  }
  return 1;
}

int getOffsets(int inum, MFS_Inode_t *currInode) {
  int imapIdx = inum / 16; /* imap piece num */

  if(cr->imap[imapIdx] == -1){
    perror("server_read: invalid inum_2");
    return -1;
  }

  int imapOffset =  cr->imap[imapIdx];
  /* nav to mp */
  int iNodeIdx = inum % IMAP_PIECE_SIZE; /* inode index within a imap piece */

  MFS_Imap_t imap;
  lseek(fd, imapOffset, SEEK_SET);
  read(fd, &imap, sizeof(MFS_Imap_t));
  
  int inodeDiskAddress = imap.inodes[iNodeIdx]; /* gw: fp denotes file pointer (location within a file) */
  if(inodeDiskAddress == -1) {
    perror("server_read: invalid inum_3");
    return -1;
  }

  /* nav to inode */
//   MFS_Inode_t currInode;
  lseek(fd, inodeDiskAddress, SEEK_SET);
  read(fd, &currInode, sizeof(MFS_Inode_t));

  return 0;
}

int server_lookup(int pinum, char *name) {
    if(pinum < 0 || pinum >= 4096) {
      perror("server_lookup: invalid pinum_1");
      return -1;
    }

    MFS_Inode_t currInode;
    int helper_rc = getOffsets(pinum, &currInode);
  
    if(helper_rc == -1) {
        perror("server_read: helper returned invalid");
        return -1;
    }

     /* read the datablock pointed by this nd */

    char data_buf[MFS_BLOCK_SIZE]; /* gw: use char buffer, (no need free) */

    for(int i = 0; i < 14; i++) {

        int dataDiskAddress = currInode.data[i];	/* gw: tbc */
        if(dataDiskAddress == -1) continue;

        lseek(fd, dataDiskAddress, SEEK_SET);
        read(fd, data_buf, MFS_BLOCK_SIZE);
	  
        MFS_DirDataBlock_t *dir_buf = (MFS_DirDataBlock_t *)data_buf;
        for(int j = 0; j < 64; j++) {
            MFS_DirEnt_t *dirEntry = &dir_buf->entries[j];
            //      if(strcmp(p_de->name,name) == 0)
            if(dirEntry->inum == -1)
                continue;
            if(strncmp(dirEntry->name,name,LEN_NAME) == 0)
	            return dirEntry->inum;
        }
    }
    //  perror("server_lookup: invalid name_5");
    return -1;
}

int server_read(int inum, char* buffer, int block){
//   int i=0, j=0;
  // int imapIdx=0, iNodeIdx=0;

  if (sanityCheck(inum, block) == -1)
    return -1;

  MFS_Inode_t currInode;
  int helper_rc = getOffsets(inum, &currInode);
  
  if(helper_rc == -1) {
    perror("server_read: helper returned invalid");
    return -1;
  }
  /* assert dir */
  if(!(currInode.type == MFS_REGULAR_FILE || currInode.type == MFS_DIRECTORY)) {
    perror("server_read: not a valid file_4");
    return -1;
  }
//   int fp_data = currInode.data[0];  /* get fp_data */
//   int sz_data = currInode.size;
//   int num_blocks = sz_data / MFS_BLOCK_SIZE + 1;

  if( block < 0 || block > 14) {
    perror("server_read: invalid block_5");
    return -1;
  }

  // char* ip = NULL;

  int dataDiskAddress = currInode.data[block]; 
  lseek(fd, dataDiskAddress, SEEK_SET);
  read(fd, buffer, MFS_BLOCK_SIZE);

  return 0;
}

int server_write(int inum, char* buffer, int block){

  int l=0;
  // int offset = 0, step =0;
  // int is_old_mp = 0, is_old_nd = 0, is_old_block = 0;
  // int fp_mp = -1, fp_nd = -1, fp_block = -1;

  MFS_Inode_t inode;
  MFS_Imap_t imap;
  MFS_Inode_t inode_new;

  if (sanityCheck(inum, block) == -1) 
    return -1;

  /* generate clean buffer */
  char *ip = buffer;
  char wr_buffer[MFS_BLOCK_SIZE];
  /* gw: wipe out the remaining bytes, if any */
  for (int i = 0/*, ip = buffer*/; i < MFS_BLOCK_SIZE; i++) {
    if( ip != NULL ) {
      wr_buffer[i] = *ip;
      ip++;
    } 
    else {
      wr_buffer[i] = '\0';
    }
  }

  /* set default offset if not old mp */
  int offset = cr->endLog;
  int writeToLog = 0;
  int imapIdx = inum / 16;
  int imapDiskAddress =  cr->imap[imapIdx];
  if (imapDiskAddress != -1) {
    lseek(fd, imapDiskAddress, SEEK_SET);
    read(fd, &imap, sizeof(MFS_Imap_t));
    // is_old_mp = 1;
    int inodeInd = inum % IMAP_PIECE_SIZE; 
    int inodeDiskAddress = imap.inodes[inodeInd]; 
    offset = -1;
    if (inodeDiskAddress != -1){
      lseek(fd, inodeDiskAddress, SEEK_SET);
      read(fd, &inode, sizeof(MFS_Inode_t));
      if(inode.type != MFS_REGULAR_FILE) {
        perror("server_write: not a regular file_4");
        return -1;
      }
      offset = inode.data[block];
      cr->endLog += 4096;
      lseek(fd, offset, SEEK_SET);
      write(fd, wr_buffer, MFS_BLOCK_SIZE);
      inode_new.size = (block +1) * MFS_BLOCK_SIZE;
      inode_new.type = inode.type;
      for (int i = 0; i < 14; i++) 
        inode_new.data[i] = inode.data[i]; /* copy data from old nd */
      // inode_new.data[block] = offset;
    } else{
        writeToLog = 1;
    }
  } else{
    writeToLog = 1;
  }

  if (writeToLog) {
    cr->endLog += 4096;
    lseek(fd, offset, SEEK_SET);
    write(fd, wr_buffer, MFS_BLOCK_SIZE);
    inode_new.size = 0;
    //    nd_new.type = MFS_DIRECTORY;			  /* gw: tbc */
    inode_new.type = MFS_REGULAR_FILE;		  /* gw: tbc, likely this because write dont apply to dir */
    for (int i = 0; i < 14; i++) inode_new.data[i] = -1; /* copy data from old nd */
    inode_new.data[block] = offset;			  /* fp_block_new */
  }

  offset = cr->endLog;	/* after the latestly created block */
  int step = sizeof(MFS_Inode_t); /* inode size */
  cr->endLog += step;
  lseek(fd, offset, SEEK_SET);
  write(fd, &inode_new, step);

  MFS_Imap_t imap_new;
  if (imapDiskAddress != -1) {
    for(int i = 0; i< IMAP_PIECE_SIZE; i++) 
      imap_new.inodes[i] = imap.inodes[i] ; /* copy old mp's data, mp is still in memory */
  } else {
    for(int i = 0; i< IMAP_PIECE_SIZE; i++) 
      imap_new.inodes[i] = -1 ; /* copy old mp's data, mp is still in memory */
  }
  imap_new.inodes[l] = offset; 

  offset = cr->endLog;
  step = sizeof(MFS_Imap_t); /* inode size */
  cr->endLog += step;
  lseek(fd, offset, SEEK_SET);
  write(fd, &imap_new, step);

  cr->imap[imapIdx] = offset; 	/* gw: fp_mp_new */
  lseek(fd, 0, SEEK_SET);
  write(fd, cr, sizeof(MFS_CheckpointRegion_t));
  fsync(fd);
  return 0;
}

int server_stat(int inum, MFS_Stat_t* m){
  if(inum < 0 || inum >= TOTAL_NUM_INODES) {
    perror("server_stat: invalid inum_1");
    return -1;
  }
  MFS_Inode_t currInode;
  if (getOffsets(inum, &currInode) == -1)
    return -1;

  m->size = currInode.size;
  m->type = currInode.type;

  return 0;
}

int server_creat(int pinum, int type, char* name){
  if(pinum < 0 || pinum >= 4096) {
    perror("server_creat: invalid pinum_1");
    return -1;
  }
  int len_name = 0;
  for(int i=0; name[i] != '\0'; i++, len_name ++)
    ;
  if(len_name > LEN_NAME) {
    perror("server_creat: name too long_1");
    return -1;
  }

  /* if exists, creat is success */
  if(server_lookup(pinum, name) != -1) {
    return 0;
  }
  MFS_Inode_t currInode;
  int rc = getOffsets(pinum, &currInode);
  /* assert dir */
  if(rc != 0 || currInode.type != MFS_DIRECTORY) {
    perror("server_creat: invalid pinum_4");
    return -1;
  }
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
    

