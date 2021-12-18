int server_write(int inum, char* buffer, int block){
  int isOldMap = 0, isOldNode = 0;
  int  currInodeIdx = -1, currBlock = -1;
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
