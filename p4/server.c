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

int sanityCheckINum(int inum){
  if (inum < 0 || inum >= TOTAL_NUM_INODES) {
    printf("sanityCheck inum failed %d\n", inum);
    perror("server_write: invalid inum_1");
    return -1;
  }
  return 0;
}

int sanityCheckBlock(int block){
  if( block < 0 || block > NUM_INODE_POINTERS - 1) {
    printf("sanityCheck block failed %d\n", block);
    perror("server_write: invalid block_5");
    return -1;
  }
  return 0;
}

int findPopulateInode(int inum, MFS_Inode_t *currInode) {
  int imapIdx = inum / IMAP_PIECE_SIZE; /* imap piece num */

  if(cr->imap[imapIdx] == -1){
    printf("findPopulateInode imapIdx failed\n");
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
    printf("findPopulateInode disk add failed\n");
    perror("server_get_offset: invalid inum");
    return -1;
  }

  lseek(fd, inodeDiskAddress, SEEK_SET);
  read(fd, &currInode, sizeof(MFS_Inode_t));

  return 0;
}

void writeDataBlock(int offset, char* wr_buffer){
    cr->endLog += MFS_BLOCK_SIZE;
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

int server_lookup(int pinum, char *name) {
  if(sanityCheckINum(pinum) == -1) return -1;

  MFS_Inode_t currInode;
  int helper_rc = findPopulateInode(pinum, &currInode);

  if(helper_rc == -1) {
    printf("server_lookup: helper returned invalid inode\n");
    perror("server_lookup: helper returned invalid");
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
      for(int j = 0; j < 128; j++) {
          MFS_DirEnt_t *dirEntry = &dir_buf->entries[j];
          //      if(strcmp(p_de->name,name) == 0)
          if(dirEntry->inum == -1)
              continue;
          if(strncmp(dirEntry->name,name,LEN_NAME) == 0)
            return dirEntry->inum;
      }
  }
  //  perror("server_lookup: invalid name_5");
  printf("server_lookup: no such inum\n");

  return -1;
}

int server_read(int inum, char* buffer, int block){
  if (sanityCheckINum(inum) == -1 || sanityCheckBlock(block) == -1)
    return -1;

  MFS_Inode_t currInode;
  int helper_rc = findPopulateInode(inum, &currInode);
  
  if(helper_rc == -1) {
    printf("server_read helper failed\n");
    perror("server_read: helper returned invalid");
    return -1;
  }
  /* assert dir */
  if(!(currInode.type == MFS_REGULAR_FILE || currInode.type == MFS_DIRECTORY)) {
    printf("server_read not valid file\n");
    perror("server_read: not a valid file_4");
    return -1;
  }
  //   int fp_data = currInode.data[0];  /* get fp_data */
  //   int sz_data = currInode.size;
  //   int num_blocks = sz_data / MFS_BLOCK_SIZE + 1;

  int dataDiskAddress = currInode.data[block]; 
  lseek(fd, dataDiskAddress, SEEK_SET);
  read(fd, buffer, MFS_BLOCK_SIZE);

  return 0;
}

int server_write(int inum, char* buffer, int block){

  int l=0;

  MFS_Inode_t inode;
  MFS_Imap_t imap;
  MFS_Inode_t inode_new;

  if (sanityCheckINum(inum) || sanityCheckBlock(block) == -1) {
    printf("Check 1\n");
    return 0;
  }
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
        printf("server_write: not a regular file_4");
        // perror("server_write: not a regular file_4");
        // return -1;
        return 0;
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
  if(sanityCheckINum(inum))
    return -1;
  
  MFS_Inode_t currInode;
  if (findPopulateInode(inum, &currInode) == -1)
    return -1;

  m->size = currInode.size;
  m->type = currInode.type;

  return 0;
}

int unlink_helper(int pinum, MFS_DirDataBlock_t *dirBlock, MFS_DirEnt_t *dirEntry, 
                  MFS_Inode_t parentInode, int parIdx) {
  
  int imapIdx = pinum / IMAP_PIECE_SIZE;//cr.imap[crImapOffset];
  if(cr->imap[imapIdx] == -1){
    printf("unlink_helper: invalid inum_2");
    perror("unlink_helper: invalid inum_2");
    return -1;
  }

  int imapOffset =  cr->imap[imapIdx];
  /* nav to mp */
  int iNodeIdx = pinum % IMAP_PIECE_SIZE; /* inode index within a imap piece */

  MFS_Imap_t imap;
  lseek(fd, imapOffset, SEEK_SET);
  read(fd, &imap, sizeof(MFS_Imap_t));

  // update data blocks  => dir blocks
  dirEntry->inum = -1;
  int offset = cr->endLog;
  cr->endLog += sizeof(MFS_DirDataBlock_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, dirBlock, sizeof(MFS_DirDataBlock_t)); // TODO : check

  // update inode of parent
  MFS_Inode_t new_parentInode;
  // new_parentInode.size = parentInode.size - MFS_BLOCK_SIZE > 0? parentInode.size - MFS_BLOCK_SIZE : 0 ; /* reduce one block of size */

  new_parentInode.type = MFS_DIRECTORY;
  for (int i = 0; i < 14; i++) new_parentInode.data[i] = parentInode.data[i]; /* 14 pointers per inode */
  new_parentInode.data[parIdx] = offset; 	/* absolute offset */

  offset = cr->endLog;
  cr->endLog += sizeof(MFS_Inode_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &new_parentInode, sizeof(MFS_Inode_t));	

  // update map
  // imap.inodes[]
  
  //update imap
  // int inodeDiskAddress = imap.inodes[iNodeIdx]; /* gw: fp denotes file pointer (location within a file) */
  // imap.inodes[iNodeIdx] = -1;
  MFS_Imap_t newImap;
  for(int i = 0; i < IMAP_PIECE_SIZE; i++) {
    newImap.inodes[i] = imap.inodes[i];
  }
  newImap.inodes[iNodeIdx] = -1;

  offset = cr->endLog;
  cr->endLog += sizeof(MFS_Imap_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &newImap, sizeof(MFS_Imap_t));

  lseek(fd, 0, SEEK_SET);
  write(fd, cr, sizeof(MFS_CheckpointRegion_t));

  fsync(fd);

  return 0;
}

int server_unlink(int pinum, char *name) {
  if(sanityCheckINum(pinum) ) return -1;

  int len_name = 0;
  for(int i=0; name[i] != '\0'; i++, len_name ++)
    ;
  if(len_name > LEN_NAME) {
    printf("server_unlink: name too long_1");
    perror("server_unlink: name too long_1");
    return -1;
  }

  MFS_Inode_t parentInode;
  int helper_rc = findPopulateInode(pinum, &parentInode);

  if(parentInode.type != MFS_DIRECTORY) {
    printf("server_unlink: parent not a dir");
    perror("server_unlink: parent not a dir");
  }

  if(helper_rc == -1) {
      perror("server_lookup: helper returned invalid");
      return -1;
  }

  int found = 0;
  // Iterate over all data blocks of parent inode
  char buffer[MFS_BLOCK_SIZE];
  int dirEntryBlockOffset = -1;
  // int dirEntryOffset = -1;
  MFS_DirEnt_t *dirEntry;
  MFS_DirDataBlock_t *dirBlock = NULL;
  int j = 0, parIdx = -1;
  for(int i = 0; i < 14 && found != 1; i++) {
    // each of these 14 contain 128 dir entries
    // the 0th and 1st can be ignored as they are curr & parent directory entries
    dirEntryBlockOffset = parentInode.data[i];
    if(dirEntryBlockOffset == -1) {
      continue;
    }
    parIdx = i;
    lseek(fd, dirEntryBlockOffset, SEEK_SET);
    read(fd, buffer, MFS_BLOCK_SIZE);
    dirBlock = (MFS_DirDataBlock_t *) buffer;
    for(j = 0; j < 128 && found != 1; i++) {
      dirEntry = &dirBlock->entries[j];
      if(!(dirEntry->inum == pinum || dirEntry->inum == pinum || dirEntry->inum == -1)) {
        printf("server_unlink: dir not empty");
        perror("server_unlink: dir not empty");
        return -1;
      }

      if(strncmp(dirEntry->name, name, len_name) == 0) {
        found = 1;
        break;
      }
    }
  }

  // nothing to do as no such file exists to unlink/remove
  if(found == 0) {
    return 0;
  }

  //check if file or directory
  MFS_Stat_t m;
  int stat = server_stat(dirEntry->inum, &m);
  if(stat != 0) {
    printf("server_unlink: stat returned -1");
    return -1;
  }

  MFS_Inode_t currDirInode;
  int currDirOffsets = findPopulateInode(dirEntry->inum, &currDirInode);
  if(currDirOffsets != 0) {
    return -1;
  }
  if(m.type == MFS_DIRECTORY) {
    char currBuffer[MFS_BLOCK_SIZE];
    MFS_DirEnt_t *currDirEntry;
    int isEmpty = 1;
  
    for(int i = 0 ; i < 14; i++) {
      int currDirEntryBlockOffset = currDirInode.data[0];
      if(currDirEntryBlockOffset == -1) 
      continue;
      lseek(fd, currDirEntryBlockOffset, SEEK_SET);
      read(fd, currBuffer, MFS_BLOCK_SIZE);
      MFS_DirDataBlock_t *currDirBlock = (MFS_DirDataBlock_t *) currBuffer;
      for(int j = 0; j < 128; j++) {
        currDirEntry = &currDirBlock->entries[j];//(MFS_DirEnt_t) dirEntryBuffer;
        if(currDirEntry->inum == -1)
          return -1;
        if(strncmp(currDirEntry->name, ".", 1) != 0
          && strncmp(currDirEntry->name, "..", 2) != 0) {
            isEmpty = 0;
            return -1;
        }
      }
      
    }
    if(isEmpty == 1) {
      if(unlink_helper(pinum, dirBlock, dirEntry, parentInode, parIdx) != 0) {
        printf("server_unlink_helper error");
        perror("server_unlink_helper error");
        return -1;
      }
    }
  } else {
      if(unlink_helper(pinum, dirBlock, dirEntry, parentInode, parIdx) != 0) {
        printf("server_unlink_helper error");
        perror("server_unlink_helper error");
        return -1;
      }
  }

  return 0;
}

int server_creat(int pinum, int type, char* name){
  if (sanityCheckINum(pinum) == -1)return -1;
  int len_name = 0;
  for (int i=0; name[i] != '\0'; i++, len_name++);
  if (len_name > LEN_NAME) {
    printf("server_creat: name too long_1");
    perror("server_creat: name too long_1");
    return -1;
  }

  /* if exists, creat is success */
  if(server_lookup(pinum, name) != -1) {
    return 0;
  }

  MFS_Inode_t currInode;
  if (findPopulateInode(pinum, &currInode) == -1)return -1;
  if (currInode.type != MFS_DIRECTORY) {
    perror("server_creat: parent not dir");
    return -1;
  }

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
  //File exists
  cr = (MFS_CheckpointRegion_t *) malloc(sizeof(MFS_CheckpointRegion_t));
  if(access(argv[2], F_OK) == 0) {
      lseek(fd, 0, SEEK_SET);
      read(fd, cr, sizeof(MFS_CheckpointRegion_t));
  } else {
      fd = open(argv[2], O_RDWR|O_CREAT, S_IRWXU);
  
      if(fd < 0) {
          printf("Cannot create file from image");
          return -1;
      }

      cr->inode_count = 0;
      cr->endLog = sizeof(MFS_CheckpointRegion_t);
      for(int i = 0; i < 256; i++) 
          cr->imap[i] = -1;

      /* write content on the file using lseek and write */
    lseek(fd, 0, SEEK_SET);
    write(fd, cr, sizeof(MFS_CheckpointRegion_t));

    /* create and write to block 0, directory */
    MFS_DirDataBlock_t dataBlock;
    for(int i=0; i< 128; i++){
      strcpy(dataBlock.entries[i].name, "\0");
      dataBlock.entries[i].inum = -1;
    }
    strcpy(dataBlock.entries[0].name, ".\0");
    dataBlock.entries[0].inum = 0; /* gw: correct */
    strcpy(dataBlock.entries[1].name, "..\0");
    dataBlock.entries[1].inum = 0;

    /* GW: how??? */
    int offset = cr->endLog;
    // int step = MFS_BLOCK_SIZE; /* DirDataBlock should be rounded to 4096 */
    cr->endLog += MFS_BLOCK_SIZE;
    lseek(fd, offset, SEEK_SET);
    write(fd, &dataBlock, sizeof(MFS_DirDataBlock_t));

    MFS_Inode_t rootINode;
    rootINode.size = 0;
    rootINode.type = MFS_DIRECTORY;
    for(int i = 0; i < 14; i++) {
      rootINode.data[i] = -1;
    }
    rootINode.data[0] = offset;
    
    offset = cr->endLog;
    cr->endLog += sizeof(MFS_Inode_t);
    lseek(fd, offset, SEEK_SET);
    write(fd, &rootINode, sizeof(MFS_Inode_t));
    
    MFS_Imap_t imap;
    // imap.inodes[0] = offset;
    for(int i = 0; i < 16; i++) {
      imap.inodes[i] = -1;
    }
    imap.inodes[0] = offset;

    offset = cr->endLog;
    cr->endLog += sizeof(MFS_Imap_t);
    lseek(fd, offset, SEEK_SET);
    write(fd, &imap, sizeof(MFS_Imap_t));

    cr->imap[0] = offset;
    lseek(fd, 0, SEEK_SET);
    write(fd, &cr, sizeof(MFS_CheckpointRegion_t));
  }

  int sd = UDP_Open(port);
  printf("port = %d\n", port);
  assert(sd > -1);
  while (1) {
    struct sockaddr_in addr;
    MFS_message_t message, reply;
    printf("server:: waiting...\n");
    int rc = UDP_Read(sd, &addr, (char *) &message, sizeof(MFS_message_t));
    printf("server:: read message [size:%d contents:(%d), type: %d]\n", rc, message.pinum, message.requestType);
    
    if (rc > 0) {
      if(message.requestType == REQ_LOOKUP) {
          printf("REQ_LOOKUP\n");
          reply.pinum = message.pinum;
          reply.inum = server_lookup(message.pinum, message.name);
          reply.type = message.type;
          printf("LOOKUP inum = %d\n", reply.inum);
          // if(rc != 0) {
          //   perror("server_lookup: error");
          //   return -1;
          // }
          reply.requestType = REQ_RESPONSE;
          //sprintf(reply, "goodbye world");
          rc = UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));//BUFFER_SIZE);            
      } else if(message.requestType == REQ_STAT) {
          printf("REQ_STAT\n");
          MFS_Stat_t m;

          int rc = server_stat(message.inum, &m);
          reply.inum = message.inum;
          reply.size = m.size;
          reply.type = m.type;
          reply.requestType = REQ_RESPONSE;
          printf("server_requestType: %d\n", rc);
          // if(rc != 0) {
          //   printf("server_stat: error");
          //   perror("server_stat: error");
          //   return -1;
          // }
          rc = UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));
      } else if(message.requestType == REQ_WRITE) {
          printf("REQ_WRITE\n");
          int rc = server_write(message.inum, message.data, message.block);
          reply.inum = message.inum;
          reply.type = message.type;
          reply.requestType = REQ_RESPONSE;
          printf("REQ_WRITE rc = %d\n", rc);
          // if(rc != 0) {
          //   perror("server_write: error");
          //   return -1;
          // }
          rc = UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));
      } else if(message.requestType == REQ_READ) {
          printf("REQ_READ\n");
          int rc = server_read(message.inum, reply.data, message.block);
          // if(rc != 0) {
          //   printf("server_read: error");
          //   perror("server_read: error");
          //   return -1;
          // }
          printf("server_read: %d\n", rc);
          reply.inum = message.inum;
          reply.requestType = REQ_RESPONSE;
          rc = UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));
      } else if(message.requestType == REQ_UNLINK) {
          printf("REQ_UNLINK\n");
          int rc = server_unlink(message.pinum, message.name);
          // reply.inum = message.inum;
          reply.pinum = message.pinum;
          reply.requestType = REQ_RESPONSE;
          printf("server_unlink: %d\n", rc);
          // if(rc != 0) {
          //   printf("server_unlink: error");
          //   perror("server_unlink: error");
          //   return -1;
          // }
          rc = UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));
      } else if(message.requestType == REQ_SHUTDOWN) { 
          printf("REQ_SHUTDOWN\n");

          reply.requestType = REQ_RESPONSE;
          UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));
          int rc = server_shutdown();
          if(rc != 0) {
            printf("server_shutdown: error");
            perror("server_shutdown: error");
            return -1;
          }

      } else if(message.requestType == REQ_CREAT) {
          printf("REQ_CREAT\n");
          int rc = server_creat(message.pinum, message.type, message.name);
          reply.inum = message.inum;
          reply.pinum = message.pinum;
          reply.type = message.type;
          reply.requestType = REQ_RESPONSE;
          strcpy(reply.name, message.name);
          printf("server_creat: %d\n", rc);
          // if(rc != 0) {
          //   printf("server_creat: error");
          //   perror("server_creat: error");
          //   return -1;
          // }
          rc = UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));
      } else if(message.requestType == REQ_RESPONSE) {
          reply.requestType = REQ_RESPONSE;
          perror("REQ_RESPONSE");
          UDP_Write(sd, &addr, (char *) &reply, sizeof(MFS_message_t));
      } 
      else {
          printf("ERROR, %d\n", message.requestType);
      }   
    } 
  }
  return 0; 
}
    