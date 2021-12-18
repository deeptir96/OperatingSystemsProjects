int server_creat(int pinum, int type, char* name){

  int i=0, j=0, k=0, l=0;
  int offset = 0, step =0;
  int is_old_mp = 0;
  int fp_mp = -1, fp_nd = -1, fp_block = -1;

  MFS_Imap_t mp;

  if (sanityCheckINum(pinum) == -1)return -1;
  int len_name = 0;
  for(i=0; name[i] != '\0'; i++, len_name ++)
    ;
  if(len_name > LEN_NAME) {
    perror("server_creat: name too long_1");
    return -1;
  }

  /* if exists, creat is success */
  if(server_lookup(pinum, name) != -1) {
    return 0;
  }

  MFS_Inode_t nd_par;
  /* here we know it does not exist */
  /********** pinum **********/

  int helper_rc = findPopulateInode(pinum, &nd_par);
  if(helper_rc != 0) return -1;
  /* assert dir */
  if(nd_par.type != MFS_DIRECTORY) {
    perror("server_creat: invalid pinum_4");
    return -1;
  }

  /* get the next free inode */
  int free_inum = -1;
  int is_free_inum_found = 0;
  MFS_Imap_t mp_par;
  int emptyInodeIdx;
  for(i=0; i<NUM_IMAP; i++) {
    fp_mp =  cr->imap[i];
    if(fp_mp != -1) {

      lseek(fd, fp_mp, SEEK_SET);
      read(fd, &mp_par, sizeof(MFS_Imap_t));
      for(j=0; j<IMAP_PIECE_SIZE; j++) {
      	fp_nd = mp_par.inodes[j]; /* gw: fp denotes file pointer (location within a file) */
      	if(fp_nd == -1) {
          emptyInodeIdx = j;
      	  free_inum = i*IMAP_PIECE_SIZE + j;
      	  is_free_inum_found = 1;
      	  break;
      	}
      }
    }
    else {

      MFS_Imap_t mp_new;
      for(j = 0; j< IMAP_PIECE_SIZE; j++) mp_new.inodes[j] = -1 ; /* copy old mp's data, mp is still in memory */

      offset = cr->endLog;
      step = sizeof(MFS_Imap_t); /* inode size */
      cr->endLog += step;
      lseek(fd, offset, SEEK_SET);
      write(fd, &mp_new, step);

      /* update cr */
      /* now the new imap and inode is written, we update imap table */
      cr->imap[i] = offset;	/* gw: changed for free_inum */
      lseek(fd, 0, SEEK_SET);
      write(fd, cr, sizeof(MFS_CheckpointRegion_t));

      fsync(fd);

      for(j=0; j<IMAP_PIECE_SIZE; j++) {
	fp_nd = mp_new.inodes[j]; /* gw: fp denotes file pointer (location within a file) */
	if(fp_nd == -1) {
	  free_inum = i*IMAP_PIECE_SIZE + j;
	  is_free_inum_found = 1;
	  break;
	}
      }
    }


    if (is_free_inum_found) break;
  }


  if(free_inum == -1 || free_inum > TOTAL_NUM_INODES - 1) { /* gw: added free_inum upper limit check, 05/12 */
    perror("server_creat: cannot find free inode_5 ");
    return -1;
  }


  /* read the datablock pointed by this nd */
  //  void* data_buf = malloc(MFS_BLOCK_SIZE);
  char data_buf[MFS_BLOCK_SIZE]; /* gw: use char buffer, (no need free) */
  MFS_DirDataBlock_t* dir_buf = NULL;
  int flag_found_entry = 0;
  int block_par = 0;
  //  int existing_block_cnt = 0;
  MFS_Inode_t* p_nd = &nd_par;


  for(i=0; i< NUM_INODE_POINTERS; i++) {

    //      fp_block = nd_par.data[i];	/* gw: tbc */
    fp_block = p_nd->data[i];	//p_nd may point to a new nd in next outer loop
    block_par = i;
    //      if(fp_block == -1) continue; /* gw: tbc, lazy scheme now assume no need to make new block for server_creat */
    if(fp_block == -1) {

      /* no empty entry found */

      //	if(!flag_found_entry && existing_block_cnt <= 14) {
      /* prepair new block */
      MFS_DirDataBlock_t* p_dir = (MFS_DirDataBlock_t*) data_buf;
      for(i=0; i< NUM_DIR_ENTRIES; i++){
	strcpy(p_dir->entries[i].name, "\0");
	p_dir->entries[i].inum = -1;
      }
      //      no need additional . .. in this new block
      /* gw: if it is the first block to be created */
      /* if(i==0) { */
      /* 	strcpy(p_dir->entries[0].name, ".\0"); */
      /* 	p_dir->entries[0].inum = free_inum; /\* gw: confirm this later *\/ */
      /* 	strcpy(p_dir->entries[1].name, "..\0"); */
      /* 	p_dir->entries[1].inum = pinum; /\* gw: confirm this later *\/ */
      /* } */

      /* gw: new p_dir the new block is ready for writting in */
      offset = cr->endLog;
      step = MFS_BLOCK_SIZE; /* DirDataBlock should be rounded to 4096 */
      cr->endLog += step;
      lseek(fd, offset, SEEK_SET);
      write(fd, p_dir, sizeof(MFS_DirDataBlock_t)); /* TODO verify this */

      /* gw: update fp_block */
      fp_block = offset;

      MFS_Inode_t nd_dir_new;
      nd_dir_new.size = nd_par.size; /* gw: tbc, assume no new block add to par dir */
      nd_dir_new.type = MFS_DIRECTORY;
      for (i = 0; i < 14; i++) nd_dir_new.data[i] = nd_par.data[i]; /* 14 pointers per inode */
      nd_dir_new.data[block_par] = offset; 	/* absolute offset */
      p_nd = &nd_dir_new;			/* gw: point to new nd  */


      offset = cr->endLog;
      step = sizeof(MFS_Inode_t); /* inode size */
      cr->endLog += step;
      lseek(fd, offset, SEEK_SET);
      write(fd, &nd_dir_new, step);


      MFS_Imap_t mp_dir_new;
      for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_dir_new.inodes[i] = mp_par.inodes[i]; /* gw: dubious about num inodes per imap */
      mp_dir_new.inodes[l] = offset;

      offset = cr->endLog;
      step = sizeof(MFS_Imap_t); /* inode size */
      cr->endLog += step;
      lseek(fd, offset, SEEK_SET);
      write(fd, &mp_dir_new, step);



      /* now the first imap and inode is written, we update imap table */
      /* this update is done after the (imap, inode, datablock creation is donea altogether ) -> atomic */
      cr->imap[k] = offset;
      lseek(fd, 0, SEEK_SET);
      write(fd, cr, sizeof(MFS_CheckpointRegion_t));


      fsync(fd);

      //	}
      /* else if(!flag_found_entry && existing_block_cnt > 14) { */
      /*   perror("server_creat: failed to creat new block for dir_6"); */
      /*   return -1; */
      /* } */
      /* else{ */
      /*   /\* perror("server_creat: should not come here"); *\/ */
      /*   /\* return -1; *\/ */
      /*   ;				/\* gw: do nothing *\/ */
      /* } */


    }
    //      existing_block_cnt ++;	   /* gw: tbc, should be ok */

    lseek(fd, fp_block, SEEK_SET);
    read(fd, data_buf, MFS_BLOCK_SIZE);

    dir_buf = (MFS_DirDataBlock_t*)data_buf;
    for(j=0; j<NUM_DIR_ENTRIES; j++) {
      MFS_DirEnt_t* p_de = &dir_buf->entries[j];
      if(p_de->inum == -1) {
	/* found an empty entry slot */
	p_de->inum = free_inum;
	strcpy(p_de->name, name);
	flag_found_entry = 1;
	break;
      }
    }

    if(flag_found_entry)
      break;
  }

  /* gw: => */
  //  if(!flag_found_entry)
  //    return -1;		/* gw: could not find an entry means big dir fail */



  /* datablock dir_buf is ready now*/

  /* GW: how??? */
  offset = cr->endLog;
  step = MFS_BLOCK_SIZE; /* DirDataBlock should be rounded to 4096 */
  cr->endLog += step;
  lseek(fd, offset, SEEK_SET);
  //	  write(fd, &db, step);	/* write data block sized 4096 */
  //	  write(fd, &dir_buf, sizeof(MFS_DirDataBlock_t)); /* TODO verify this */
  write(fd, dir_buf, sizeof(MFS_DirDataBlock_t)); /* TODO verify this */



  MFS_Inode_t nd_par_new;
  //  nd_par_new.size = nd_par.size; /* gw: tbc, assume no new block add to par dir */
  /* gw: updated for big dir*/
  nd_par_new.size = p_nd->size; /* gw: tbc, assume no new block add to par dir */
  nd_par_new.type = MFS_DIRECTORY;
  //  for (i = 0; i < 14; i++) nd_par_new.data[i] = nd_par.data[i]; /* 14 pointers per inode */
  for (i = 0; i < 14; i++) nd_par_new.data[i] = p_nd->data[i]; /* gw: updated for big dir */
  nd_par_new.data[block_par] = offset; 	/* absolute offset */


  offset = cr->endLog;
  step = sizeof(MFS_Inode_t); /* inode size */
  cr->endLog += step;
  lseek(fd, offset, SEEK_SET);
  write(fd, &nd_par_new, step);


  MFS_Imap_t mp_par_new;
  for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_par_new.inodes[i] = mp_par.inodes[i]; /* gw: dubious about num inodes per imap */
  mp_par_new.inodes[l] = offset;

  offset = cr->endLog;
  step = sizeof(MFS_Imap_t); /* inode size */
  cr->endLog += step;
  lseek(fd, offset, SEEK_SET);
  write(fd, &mp_par_new, step);

  /* now the first imap and inode is written, we update imap table */
  /* this update is done after the (imap, inode, datablock creation is donea altogether ) -> atomic */
  cr->imap[k] = offset;
  lseek(fd, 0, SEEK_SET);
  write(fd, cr, sizeof(MFS_CheckpointRegion_t));


  fsync(fd);

  /********** now the parent file operation completes **********/


  /* using child inum = free_inum */

  /********** file **********/
  /* make an empty block */
  /* generate clean buffer */
  char* ip = NULL;
  char wr_buffer[ MFS_BLOCK_SIZE ];
  /* init */
  for(i=0, ip = wr_buffer; i<MFS_BLOCK_SIZE; i++) {
    wr_buffer[i] = '\0';
    ip++;
  }

  /* pick an inode. use the first -1 nd */
  /* use free_inum found earlier */
  int inum = free_inum;
  is_old_mp = 0;//, is_old_nd = 0, is_old_block = 0;
  fp_mp = -1, fp_nd = -1, fp_block = -1;
  /* re-init if dir  */
  if(type == MFS_DIRECTORY) {
    MFS_DirDataBlock_t* p_dir = (MFS_DirDataBlock_t*) wr_buffer;
    for(i=0; i< NUM_DIR_ENTRIES; i++){
      strcpy(p_dir->entries[i].name, "\0");
      p_dir->entries[i].inum = -1;
    }
    strcpy(p_dir->entries[0].name, ".\0");
    p_dir->entries[0].inum = inum; /* gw: confirm this later */
    strcpy(p_dir->entries[1].name, "..\0");
    p_dir->entries[1].inum = pinum; /* gw: confirm this later */


    /* gw: write dir block */
    offset = cr->endLog;	/* after the latestly created block */
    step = MFS_BLOCK_SIZE; /* inode size */
    cr->endLog += step;
    lseek(fd, offset, SEEK_SET);
    write(fd, wr_buffer, step);


  }



  /* set default offset if not old mp */
  //offset = cr->endLog;




  /* fs operations starts */
  k = inum / IMAP_PIECE_SIZE; /* imap piece num */
  fp_mp =  cr->imap[k];
  if(fp_mp != -1){
    /* old mp exist */
    is_old_mp = 1;

    /* nav to that mp */
    l = inum % IMAP_PIECE_SIZE; /* inode index within a imap piece */
    /* move def out for later mp_new creation */
    //    MFS_Imap_t mp;
    lseek(fd, fp_mp, SEEK_SET);
    read(fd, &mp, sizeof(MFS_Imap_t));
    fp_nd = mp.inodes[l]; /* gw: fp denotes file pointer (location within a file) */

    /* offset will remain default until confirm old block */
  }

  /* gw: 0512: no init block0 if REG_FILE (note that directory block is handaled previously) */
  /* step = MFS_BLOCK_SIZE; /\* DirDataBlock should be rounded to 4096 *\/ */
  /* cr->endLog += step; */
  /* lseek(fd, offset, SEEK_SET); */
  /* write(fd, wr_buffer, MFS_BLOCK_SIZE); /\* write whole block *\/ */


    /* new nd */
    MFS_Inode_t nd_new;
    //  nd_new.size = MFS_BLOCK_SIZE;			/* gw: changed to 4096 */
    nd_new.size = 0;			/* gw: changed to 4096 */
    nd_new.type = type;			  /* gw: tbc */
    for (i = 0; i < 14; i++) nd_new.data[i] = -1; /* copy data from old nd */
    /* gw: 0512: no init block0 */
    if(type == MFS_DIRECTORY)
      nd_new.data[0] = offset;			/* assign to block 0 */


    offset = cr->endLog;	/* after the latestly created block */
    step = sizeof(MFS_Inode_t); /* inode size */
    cr->endLog += step;
    lseek(fd, offset, SEEK_SET);
    write(fd, &nd_new, step);


    /* make mp_new */
    /* update imap */
    MFS_Imap_t mp_new;
    if(is_old_mp) {
      for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_new.inodes[i] = mp.inodes[i] ; /* copy old mp's data, mp is still in memory */
      mp_new.inodes[l] = offset; 	/* fp_nd_new */
    }
    else {
      for(i = 0; i< IMAP_PIECE_SIZE; i++) mp_new.inodes[i] = -1 ; /* copy old mp's data, mp is still in memory */
      mp_new.inodes[l] = offset; 	/* fp_nd_new */
    }

    offset = cr->endLog;
    step = sizeof(MFS_Imap_t); /* inode size */
    cr->endLog += step;
    lseek(fd, offset, SEEK_SET);
    write(fd, &mp_new, step);

    /* update cr */
    /* now the new imap and inode is written, we update imap table */
    cr->imap[k] = offset; 	/* gw: fp_mp_new */
    lseek(fd, 0, SEEK_SET);
    write(fd, cr, sizeof(MFS_CheckpointRegion_t));

    fsync(fd);
    /* gw: debug */
    printf("\n server_creat: FIle created , name : %s, inum : %d", name,inum);

  return 0;


}
