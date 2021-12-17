#ifndef __MFS_h__
#define __MFS_h__

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
    char name[28];  // up to 28 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

typedef struct __MFS_DirDataBlock_t {
  MFS_DirEnt_t entries[64];
} MFS_DirDataBlock_t;

typedef struct __MFS_Inode_t{
    int size;
    int type;
    int data[14]; 
} MFS_Inode_t;

typedef struct __MFS_InodeMap_t{
    int inodes[16]; 
} MFS_Imap_t;

enum MFS_REQUEST_TYPE {
  REQ_INIT,
  REQ_LOOKUP,
  REQ_STAT,
  REQ_WRITE,
  REQ_READ,
  REQ_CREAT,
  REQ_UNLINK,
  REQ_RESPONSE,
  REQ_SHUTDOWN
};

typedef struct message {
    enum MFS_REQUEST_TYPE requestType;

    int inum;  // file's inum
    int pinum; // parent/directory's inum
    int block; // file's block num
    int type;  // MFS_DIRECTORY or MFS_REGULAR
    int size;  // file's bytes
    
    char data[MFS_BLOCK_SIZE];
    char name[28];
} MFS_message_t;

typedef struct checkpointRegion {
    int inode_count;
    int endLog;
    int imap[256]; // 4096 / 16
} MFS_CheckpointRegion_t;


int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

#endif // __MFS_h__
