#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mfs.h"
#include "udp.h"

int fd = -1;
MFS_CheckpointRegion_t* cr = NULL;
int imapIdx = -1, iNodeIdx = -1, inodeDiskAddress = -1;
MFS_Imap_t imap;

int sanityCheckINum(int inum){
  if (inum < 0 || inum >= TOTAL_NUM_INODES) {
    printf("sanityCheck inum failed %d\n", inum);
    perror("invalid inum_1");
    return -1;
  }
  return 0;
}

int sanityCheckBlock(int block){
  if( block < 0 || block > NUM_INODE_POINTERS - 1) {
    printf("sanityCheck block failed %d\n", block);
    perror("invalid block_5");
    return -1;
  }
  return 0;
}

int findPopulateInode(int inum, MFS_Inode_t *currInode) {
  imapIdx = inum / IMAP_PIECE_SIZE;

  if(cr->imap[imapIdx] == -1){
    printf("findPopulateInode imapIdx failed\n");
    perror("server_read: invalid inum_2");
    return -1;
  }

  int imapOffset =  cr->imap[imapIdx];
  iNodeIdx = inum % IMAP_PIECE_SIZE;

  lseek(fd, imapOffset, SEEK_SET);
  read(fd, &imap, sizeof(MFS_Imap_t));

  inodeDiskAddress = imap.inodes[iNodeIdx];
  if(inodeDiskAddress == -1) {
    printf("findPopulateInode disk add failed\n");
    perror("server_get_offset: invalid inum");
    return -1;
  }

  lseek(fd, inodeDiskAddress, SEEK_SET);
  read(fd, currInode, sizeof(MFS_Inode_t));

  return 0;
}

void writeDataBlock(int offset, char* wr_buffer){
    cr->endLog += MFS_BLOCK_SIZE;
    lseek(fd, offset, SEEK_SET);
    write(fd, wr_buffer, MFS_BLOCK_SIZE);
}

void writeDirDataBlock(int offset, MFS_DirDataBlock_t db) {
  cr->endLog += sizeof(MFS_DirDataBlock_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &db, sizeof(MFS_DirDataBlock_t));
}

void writeINode(int offset, MFS_Inode_t inode_new){
    int step = sizeof(MFS_Inode_t);
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
    fsync(fd);
}

int server_lookup(int pinum, char* name){
  if(sanityCheckINum(pinum) != 0) return -1;

  MFS_Inode_t inode;
  int helper_rc = findPopulateInode(pinum, &inode);
  if(helper_rc != 0) return -1;

  if(inode.type != MFS_DIRECTORY) {
    perror("invalid pinum");
    return -1;
  }

  char data_buf[MFS_BLOCK_SIZE];
  for(int i=0; i< NUM_INODE_POINTERS; i++) {

    int currBlock = inode.data[i];
    if(currBlock == -1) continue;

    lseek(fd, currBlock, SEEK_SET);
    read(fd, data_buf, MFS_BLOCK_SIZE);

    MFS_DirDataBlock_t* dir_buf = (MFS_DirDataBlock_t*)data_buf;
    for(int j=0; j<NUM_DIR_ENTRIES; j++) {
      MFS_DirEnt_t *dirEntry = &dir_buf->entries[j];
      if(strncmp(dirEntry->name,name,LEN_NAME) == 0)
	       return dirEntry->inum;
    }
  }
  perror("lookup not found");
  return -1;
}

int server_stat(int inum, MFS_Stat_t* m){
  if(sanityCheckINum(inum) != 0) return -1;

  MFS_Inode_t inode;
  int helper_rc = findPopulateInode(inum, &inode);
  if(helper_rc != 0) return -1;

  m->size = inode.size;
  m->type = inode.type;

  return 0;
}

int server_write(int inum, char* buffer, int block){
  int isOldMap = 0, isOldNode = 0;
  int currInodeIdx = -1, currBlock = -1;
  int imapEntryIdx = -1;

  MFS_Inode_t currInode;
  MFS_Imap_t currImap;

  if(sanityCheckINum(inum) != 0 || sanityCheckBlock(block) != 0) return -1;

  char* ip = buffer;
  char wr_buffer[ MFS_BLOCK_SIZE ];
  for(int i=0; i<MFS_BLOCK_SIZE; i++) {
    if( ip != NULL ) {
      wr_buffer[i] = *ip;
      ip++;
    }
    else {
      wr_buffer[i] = '\0';
    }
  }

  int offset = cr->endLog;
  int imapPieceIdx = inum / IMAP_PIECE_SIZE;
  int imapPiece =  cr->imap[imapPieceIdx];

  if(imapPiece != -1){
    isOldMap = 1;
    imapEntryIdx = inum % IMAP_PIECE_SIZE;
    lseek(fd, imapPiece, SEEK_SET);
    read(fd, &currImap, sizeof(MFS_Imap_t));
    currInodeIdx = currImap.inodes[imapEntryIdx];
  }

  if(currInodeIdx != -1 && isOldMap) {
    isOldNode = 1;
    lseek(fd, currInodeIdx, SEEK_SET);
    read(fd, &currInode, sizeof(MFS_Inode_t));
    if(currInode.type != MFS_REGULAR_FILE) {
      perror("server_write: not a regular file_4");
      return -1;
    }
    currBlock = currInode.data[block];
  }

  if(currBlock != -1 && isOldNode && isOldMap) {
    offset = currBlock;
  }
  writeDataBlock(offset, wr_buffer);

  MFS_Inode_t newInode;
  if(isOldNode) {
    newInode.size = (block +1) * MFS_BLOCK_SIZE;

    newInode.type = currInode.type;
    for (int i = 0; i < 14; i++) newInode.data[i] = currInode.data[i];
    newInode.data[block] = offset;
  }
  else {
    newInode.size = 0;
    newInode.type = MFS_REGULAR_FILE;
    for (int i = 0; i < 14; i++) newInode.data[i] = -1;
    newInode.data[block] = offset;
  }

  offset = cr->endLog;
  writeINode(offset, newInode);

  MFS_Imap_t newImap;
  if(isOldMap) {
    for(int i = 0; i< IMAP_PIECE_SIZE; i++) newImap.inodes[i] = currImap.inodes[i] ;
    newImap.inodes[imapEntryIdx] = offset;
  }
  else {
    for(int i = 0; i< IMAP_PIECE_SIZE; i++) newImap.inodes[i] = -1 ;
    newImap.inodes[imapEntryIdx] = offset;
  }

  offset = cr->endLog;
  writeImap(offset, newImap);

  cr->imap[imapPieceIdx] = offset;
  writeCheckpointRegion();
  return 0;
}


int server_read(int inum, char* buffer, int block) {
  if (sanityCheckINum(inum) == -1 || sanityCheckBlock(block) == -1)
    return -1;

  MFS_Inode_t nd;
  int helper_rc = findPopulateInode(inum, &nd);
  if(helper_rc != 0) return -1;

  if(!(nd.type == MFS_REGULAR_FILE || nd.type == MFS_DIRECTORY)) {
    perror("server_read: not a valid file_4");
    return -1;
  }

  int dataDiskAddress = nd.data[block];
  lseek(fd, dataDiskAddress, SEEK_SET);
  read(fd, buffer, MFS_BLOCK_SIZE);
  return 0;
}

int server_creat(int pinum, int type, char* name){

  int i=0, j=0, k = 0, l=0;
  int offset = 0, step =0;
  int is_old_mp = 0;

  int mapDiskAdd = -1, nodeDiskAdd = -1, blockDiskAdd = -1;

  MFS_Imap_t mp;

  if (sanityCheckINum(pinum) == -1)return -1;

  int len_name = 0;
  for(i=0; name[i] != '\0'; i++, len_name ++)
    ;
  if(len_name > LEN_NAME) {
    perror("server_creat: name too long_1");
    return -1;
  }

  if(server_lookup(pinum, name) != -1) {
    return 0;
  }

  k = pinum / IMAP_PIECE_SIZE;
  mapDiskAdd =  cr->imap[k];
  if(mapDiskAdd == -1){
    perror("server_creat: invalid pinum_2");
    return -1;
  }

  l = pinum % IMAP_PIECE_SIZE;
  MFS_Imap_t mp_par;
  lseek(fd, mapDiskAdd, SEEK_SET);
  read(fd, &mp_par, sizeof(MFS_Imap_t));
  nodeDiskAdd = mp_par.inodes[l];
  if(nodeDiskAdd == -1) {
    perror("server_creat: invalid pinum_3");
    return -1;
  }

  MFS_Inode_t nd_par;
  lseek(fd, nodeDiskAdd, SEEK_SET);
  read(fd, &nd_par, sizeof(MFS_Inode_t));

  if(nd_par.type != MFS_DIRECTORY) {
    perror("server_creat: invalid pinum_4");
    return -1;
  }


  int free_inum = -1;
  int is_free_inum_found = 0;
  for(i=0; i<NUM_IMAP; i++) {
    mapDiskAdd =  cr->imap[i];
    if(mapDiskAdd != -1) {
      MFS_Imap_t mp_par;
      lseek(fd, mapDiskAdd, SEEK_SET);
      read(fd, &mp_par, sizeof(MFS_Imap_t));
      for(j=0; j<IMAP_PIECE_SIZE; j++) {
        nodeDiskAdd = mp_par.inodes[j];
        if(nodeDiskAdd == -1) {
            free_inum = i*IMAP_PIECE_SIZE + j;
            is_free_inum_found = 1;
            break;
        }
      }
    }
    else {
      MFS_Imap_t mp_new;
      for(j = 0; j< IMAP_PIECE_SIZE; j++)
        mp_new.inodes[j] = -1 ;
      offset = cr->endLog;
      writeImap(offset, mp_new);

      cr->imap[i] = offset;
      writeCheckpointRegion();

      for(j=0; j<IMAP_PIECE_SIZE; j++) {
        nodeDiskAdd = mp_new.inodes[j];
        if(nodeDiskAdd == -1) {
            free_inum = i*IMAP_PIECE_SIZE + j;
            is_free_inum_found = 1;
            break;
        }
      }
    }
    if (is_free_inum_found) break;
  }


  if (sanityCheckINum(free_inum) == -1)return -1;

  char data_buf[MFS_BLOCK_SIZE];
  MFS_DirDataBlock_t* dir_buf = NULL;
  int flag_found_entry = 0;
  int block_par = 0;
  MFS_Inode_t* p_nd = &nd_par;


  for(i=0; i< NUM_INODE_POINTERS; i++) {
    blockDiskAdd = p_nd->data[i];
    block_par = i;
    if(blockDiskAdd == -1) {
      MFS_DirDataBlock_t* p_dir = (MFS_DirDataBlock_t*) data_buf;
      for(i=0; i< NUM_DIR_ENTRIES; i++){
        strcpy(p_dir->entries[i].name, "\0");
        p_dir->entries[i].inum = -1;
      }
      offset = cr->endLog;
      writeDirDataBlock(offset, p_dir);
      blockDiskAdd = offset;

      MFS_Inode_t nd_dir_new;
      nd_dir_new.size = nd_par.size;
      nd_dir_new.type = MFS_DIRECTORY;
      for (i = 0; i < 14; i++) nd_dir_new.data[i] = nd_par.data[i];
      nd_dir_new.data[block_par] = offset;
      p_nd = &nd_dir_new;


      offset = cr->endLog;
      writeINode(offset, nd_dir_new);


      MFS_Imap_t mp_dir_new;
      for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_dir_new.inodes[i] = mp_par.inodes[i];
      mp_dir_new.inodes[l] = offset;

      offset = cr->endLog;
      writeImap(offset, mp_dir_new);

      cr->imap[k] = offset;
      writeCheckpointRegion();

    }
    lseek(fd, blockDiskAdd, SEEK_SET);
    read(fd, data_buf, MFS_BLOCK_SIZE);

    dir_buf = (MFS_DirDataBlock_t*)data_buf;
    for(j=0; j<NUM_DIR_ENTRIES; j++) {
      MFS_DirEnt_t* p_de = &dir_buf->entries[j];
      if(p_de->inum == -1) {
        p_de->inum = free_inum;
        strcpy(p_de->name, name);
        flag_found_entry = 1;
        break;
      }
    }

    if(flag_found_entry)
      break;
  }

  offset = cr->endLog;
  writeDirDataBlock(offset, *dir_buf);

  MFS_Inode_t nd_par_new;
  nd_par_new.size = p_nd->size;
  nd_par_new.type = MFS_DIRECTORY;
  for (i = 0; i < 14; i++) nd_par_new.data[i] = p_nd->data[i];
  nd_par_new.data[block_par] = offset;


  offset = cr->endLog;
  writeINode(offset, nd_par_new);


  MFS_Imap_t mp_par_new;
  for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_par_new.inodes[i] = mp_par.inodes[i];
  mp_par_new.inodes[l] = offset;

  offset = cr->endLog;
  writeImap(offset, mp_par_new);

  cr->imap[k] = offset;
  writeCheckpointRegion();

  char* ip = NULL;
  char wr_buffer[ MFS_BLOCK_SIZE ];
  for(i=0, ip=wr_buffer; i<MFS_BLOCK_SIZE; i++) {
    wr_buffer[i] = '\0';
  }

  int inum = free_inum;
  is_old_mp = 0, is_old_nd = 0, is_old_block = 0;
  mapDiskAdd = -1, nodeDiskAdd = -1, blockDiskAdd = -1;

  if(type == MFS_DIRECTORY) {
    MFS_DirDataBlock_t* p_dir = (MFS_DirDataBlock_t*) wr_buffer;
    for(i=0; i< NUM_DIR_ENTRIES; i++){
      strcpy(p_dir->entries[i].name, "\0");
      p_dir->entries[i].inum = -1;
    }
    strcpy(p_dir->entries[0].name, ".\0");
    p_dir->entries[0].inum = inum;
    strcpy(p_dir->entries[1].name, "..\0");
    p_dir->entries[1].inum = pinum;

    offset = cr->endLog;
    writeDataBlock(offset, wr_buffer);
  }

  k = inum / IMAP_PIECE_SIZE;
  mapDiskAdd =  cr->imap[k];
  if(mapDiskAdd != -1){
    is_old_mp = 1;
    l = inum % IMAP_PIECE_SIZE;
    lseek(fd, mapDiskAdd, SEEK_SET);
    read(fd, &mp, sizeof(MFS_Imap_t));
    nodeDiskAdd = mp.inodes[l];
  }

    MFS_Inode_t nd_new;
    nd_new.size = 0;
    nd_new.type = type;
    for (i = 0; i < 14; i++) nd_new.data[i] = -1;
    if(type == MFS_DIRECTORY)
      nd_new.data[0] = offset;


    offset = cr->endLog;
    writeINode(offset, nd_new);

    MFS_Imap_t mp_new;
    if(is_old_mp) {
      for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_new.inodes[i] = mp.inodes[i] ;
      mp_new.inodes[l] = offset;
    }
    else {
      for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_new.inodes[i] = -1 ;
      mp_new.inodes[l] = offset;
    }

    offset = cr->endLog;
    writeImap(offset, mp_new);

    cr->imap[k] = offset;
    writeCheckpointRegion();

    printf("\n server_creat: FIle created , name : %s, inum : %d", name,inum);
  return 0;
}

int server_unlink(int pinum, char* name){
  if(sanityCheckINum(pinum)) return -1;

  int offset = 0;
  int parentDirDataBlock = -1, currDirDataBlock  = -1;

  // check if parent is directory
  MFS_Stat_t par_stat;
  int stat = server_stat(pinum, &par_stat);
  if(stat != 0) {
    printf("server_unlink: stat returned -1");
    return -1;
  }
  if(par_stat.type != MFS_DIRECTORY) return -1;

  int inum = server_lookup(pinum, name);
  if(inum == -1) {
    return 0;
  }

  MFS_Inode_t currInode;
  int helper_rc = findPopulateInode(inum, &currInode);
  if(helper_rc != 0) return -1;

  // check if curr dir is empty
  MFS_DirDataBlock_t* dirBlock;
  if(currInode.type == MFS_DIRECTORY) {
    char currDirBuf[MFS_BLOCK_SIZE];

    for(int i=0; i< NUM_INODE_POINTERS; i++) {
      currDirDataBlock = currInode.data[i];
      if(currDirDataBlock == -1) continue;

      lseek(fd, currDirDataBlock, SEEK_SET);
      read(fd, currDirBuf, MFS_BLOCK_SIZE);

      dirBlock = (MFS_DirDataBlock_t*)currDirBuf;
      for(int j=0; j<NUM_DIR_ENTRIES; j++) {
        MFS_DirEnt_t* currDirEntry = &dirBlock->entries[j];
        int currDirInum = currDirEntry->inum;
        if(currDirInum != -1 && currDirInum != pinum && currDirInum != inum)
          return -1;
      }
    }
  }

  MFS_Imap_t mp_new;
  for(int i = 0; i< IMAP_PIECE_SIZE; i++)
    mp_new.inodes[i] = imap.inodes[i];
  mp_new.inodes[iNodeIdx] = -1;

  offset = cr->endLog;
  writeImap(offset, mp_new);

  cr->imap[imapIdx] = offset;
  writeCheckpointRegion();

  MFS_Inode_t parentInode;
  helper_rc = findPopulateInode(pinum, &parentInode);

  if(parentInode.type != MFS_DIRECTORY) {
    perror("server_unlink: invalid pinum_7");
    return -1;
  }

  char parentDirBuf[MFS_BLOCK_SIZE];
  MFS_DirDataBlock_t* dir_buf = NULL;
  int found = 0;
  int parentDirDataBlockIdx = 0;

  for(int i=0; i< NUM_INODE_POINTERS && found != 1; i++) {
    parentDirDataBlock = parentInode.data[i];
    if(parentDirDataBlock == -1) continue;
    parentDirDataBlockIdx = i;
    lseek(fd, parentDirDataBlock, SEEK_SET);
    read(fd, parentDirBuf, MFS_BLOCK_SIZE);

    dir_buf = (MFS_DirDataBlock_t*)parentDirBuf;
    for(int j=0; j<NUM_DIR_ENTRIES && found != 1; j++) {
      MFS_DirEnt_t* currDirEntry = &dir_buf->entries[j];
      if(currDirEntry->inum == inum) {
	      currDirEntry->inum = -1;
      	strcpy(currDirEntry->name, "\0");
      	found = 1;
      	break;
      }
    }
  }

  if(!found) return 0;

  offset = cr->endLog;
  writeDirDataBlock(offset, *dir_buf);

  MFS_Inode_t parentInode_new;
  parentInode_new.type = MFS_DIRECTORY;

  for (int i = 0; i < 14; i++)
    parentInode_new.data[i] = parentInode.data[i];
  parentInode_new.data[parentDirDataBlockIdx] = offset;

  offset = cr->endLog;
  writeINode(offset, parentInode_new);

  MFS_Imap_t mp_par_new;
  for(int i = 0; i< IMAP_PIECE_SIZE; i++)
    mp_par_new.inodes[i] = imap.inodes[i];
  mp_par_new.inodes[iNodeIdx] = offset;

  offset = cr->endLog;
  writeImap(offset, mp_par_new);

  cr->imap[imapIdx] = offset;
  writeCheckpointRegion();
  return 0;
}

int server_shutdown() {
  fsync(fd);
  exit(0);
}

int server_init(int port, char* image_path) {

  fd = open(image_path, O_RDWR|O_CREAT, S_IRWXU);
  if (fd < 0) {
    perror("server_init: Cannot open file");
  }

  struct stat fs;
  if(fstat(fd,&fs) < 0) {
    perror("server_init: Cannot open file");
  }

  int i;

  cr = (MFS_CheckpointRegion_t *)malloc(sizeof(MFS_CheckpointRegion_t));
  int offset = 0;

  if(fs.st_size < sizeof(MFS_CheckpointRegion_t)){
    fd = open(image_path, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
    if(fd <0)
      return -1;

    if(fstat(fd,&fs) < 0) {
      perror("server_init: fstat error");
      return -1;
    }

    cr->inode_count = 0;
    cr->endLog = sizeof(MFS_CheckpointRegion_t);
    for(i=0; i<NUM_IMAP; i++)
      cr->imap[i] = -1;

    writeCheckpointRegion();

    MFS_DirDataBlock_t db;
    for(i=0; i< NUM_DIR_ENTRIES; i++){
      strcpy(db.entries[i].name, "\0");
      db.entries[i].inum = -1;
    }
    strcpy(db.entries[0].name, ".\0");
    db.entries[0].inum = 0;
    strcpy(db.entries[1].name, "..\0");
    db.entries[1].inum = 0;

    offset = cr->endLog;
    writeDirDataBlock(offset, db);

    MFS_Inode_t inode;
    inode.size = 0;
    inode.type = MFS_DIRECTORY;
    for (i = 0; i < 14; i++)
      inode.data[i] = -1;
    inode.data[0] = offset;

    offset = cr->endLog;
    writeINode(offset, inode);

    MFS_Imap_t mp;
    for(i = 0; i< IMAP_PIECE_SIZE; i++)
      mp.inodes[i] = -1;
    mp.inodes[0] = offset; /* gw: right, imap translate the inode number "0" to inode's address */

    offset = cr->endLog;
    writeImap(offset, mp);

    cr->imap[0] = offset; /* gw: right, contains the address of imap piece "0" */
    writeCheckpointRegion();
  }
  else {
    lseek(fd,0, SEEK_SET);
    read(fd, cr, sizeof(MFS_CheckpointRegion_t));
  }

  int sd = -1;
  if((sd =   UDP_Open(port))< 0){
    perror("server_init: port open fail");
    return -1;
  }

  struct sockaddr_in s;
  MFS_Message_t message,  reply;

  while (1) {
    if( UDP_Read(sd, &s, (char *)&message, sizeof(MFS_Message_t)) < 1)
      continue;

    if(message.requestType == REQ_LOOKUP){
      reply.inum = server_lookup(message.inum, message.name);
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
    } else if(message.requestType == REQ_STAT){
      reply.inum = server_stat(message.inum, &(reply.stat));
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
    } else if(message.requestType == REQ_WRITE){
      reply.inum = server_write(message.inum, message.data, message.block);
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
    } else if(message.requestType == REQ_READ){
      reply.inum = server_read(message.inum, reply.data, message.block);
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
    } else if(message.requestType == REQ_CREAT){
      reply.inum = server_creat(message.inum, message.type, message.name);
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
    } else if(message.requestType == REQ_UNLINK){
      reply.inum = server_unlink(message.inum, message.name);
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
    } else if(message.requestType == REQ_SHUTDOWN) {
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
      server_shutdown();
    } else if(message.requestType == REQ_RESPONSE) {
      reply.requestType = REQ_RESPONSE;
      UDP_Write(sd, &s, (char*)&reply, sizeof(MFS_Message_t));
    } else {
      perror("server_init: unknown request");
      return -1;
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		perror("Invalid num of args\n");
		exit(1);
	}
	server_init(atoi(argv[1]), argv[2] );
	return 0;
}
