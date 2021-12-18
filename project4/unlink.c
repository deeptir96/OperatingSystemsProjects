#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "udp.h"
#include "mfs.h"

int server_unlink(int pinum, char* name){
  if(sanityCheckINum(pinum) ) return -1;

  int offset = 0;
  int fp_block_par = -1;
  int fp_block = -1;

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

  MFS_Inode_t nd;
  int helper_rc = findPopulateInode(inum, &nd);
  if(helper_rc != 0) return -1;

  // check if dir is empty
  MFS_DirDataBlock_t* dirBlock;
  if(nd.type == MFS_DIRECTORY) {

    char data_buf0[MFS_BLOCK_SIZE];

    for(int i=0; i< NUM_INODE_POINTERS; i++) {
      fp_block = nd.data[i];
      if(fp_block == -1) continue;

      lseek(fd, fp_block, SEEK_SET);
      read(fd, data_buf0, MFS_BLOCK_SIZE);

      dirBlock = (MFS_DirDataBlock_t*)data_buf0;
      for(int j=0; j<NUM_DIR_ENTRIES; j++) {
        MFS_DirEnt_t* currDirEntry = &dirBlock->entries[j];
        int currDirInum = currDirEntry->inum;
        if(currDirInum != -1 && currDirInum != pinum && currDirInum != inum)
          return -1;
      }
    } /* end if dir */
  }

  MFS_Imap_t mp_new;
  for(int i = 0; i< IMAP_PIECE_SIZE; i++)
    mp_new.inodes[i] = imap.inodes[i]; /* gw: dubious about num inodes per imap */
  mp_new.inodes[iNodeIdx] = -1;	/* actual unlink, clear the fp_nd to -1 */

  offset = cr->endLog;
  writeImap(offset, mp_new);

  cr->imap[imapIdx] = offset;
  writeCheckpointRegion();

  MFS_Inode_t nd_par;
  helper_rc = findPopulateInode(pinum, &nd_par);

  if(nd_par.type != MFS_DIRECTORY) {
    perror("server_unlink: invalid pinum_7");
    return -1;
  }

  /* read the datablock pointed by this nd */
  //  void* data_buf = malloc(MFS_BLOCK_SIZE);
  char data_buf[MFS_BLOCK_SIZE];
  MFS_DirDataBlock_t* dir_buf = NULL;
  int flag_found_entry = 0;
  int block_par = 0;
  for(int i=0; i< NUM_INODE_POINTERS; i++) {

    fp_block_par = nd_par.data[i];
    if(fp_block_par == -1) continue;
    block_par = i;
    lseek(fd, fp_block_par, SEEK_SET);
    read(fd, data_buf, MFS_BLOCK_SIZE);

    dir_buf = (MFS_DirDataBlock_t*)data_buf;
    for(int j=0; j<NUM_DIR_ENTRIES; j++) {
      MFS_DirEnt_t* currDirEntry = &dir_buf->entries[j];
      if(currDirEntry->inum == inum) {
	      currDirEntry->inum = -1;
      	strcpy(currDirEntry->name, "\0");
      	flag_found_entry = 1;
      	break;
      }
    }
    if(flag_found_entry)
      break;
  }

  if(!flag_found_entry) {
    return 0;
  }

  offset = cr->endLog;
  writeDirDataBlock(offset, *dir_buf);

  MFS_Inode_t nd_par_new;
  nd_par_new.type = MFS_DIRECTORY;

  for (int i = 0; i < 14; i++)
    nd_par_new.data[i] = nd_par.data[i]; /* 14 pointers per inode */
  nd_par_new.data[block_par] = offset; 	/* absolute offset */

  offset = cr->endLog;
  writeINode(offset, nd_par_new);

  MFS_Imap_t mp_par_new;
  for(int i = 0; i< IMAP_PIECE_SIZE; i++)
    mp_par_new.inodes[i] = imap.inodes[i]; /* gw: dubious about num inodes per imap */
  mp_par_new.inodes[iNodeIdx] = offset;

  offset = cr->endLog;
  writeImap(offset, mp_par_new);

  cr->imap[imapIdx] = offset;
  writeCheckpointRegion();
  return 0;
}
