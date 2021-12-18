#ifndef __MFS_h__
#define __MFS_h__

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)
#define MFS_BYTE_STEP_SIZE   (16384)
#define NUM_INODE_POINTERS   (14)
#define TOTAL_NUM_INODES (4096)
#define IMAP_PIECE_SIZE   (16)
#define NUM_IMAP TOTAL_NUM_INODES / IMAP_PIECE_SIZE
#define LEN_NAME (28)
#define NUM_DIR_ENTRIES 128


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

typedef struct __MFS_Stat_t {
    int type; 
    int size; 
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
  char name[28];
	int  inum; 
} MFS_DirEnt_t;

typedef struct message {
	enum MFS_REQUEST_TYPE requestType;

	int inum;
	int block;
	int type;

	char name[28];
	char data[MFS_BLOCK_SIZE];
	MFS_Stat_t stat;
} MFS_Message_t;


typedef struct __MFS_Inode_t{
	int size;
	int type;
	int data[NUM_INODE_POINTERS]; 
} MFS_Inode_t;

typedef struct __MFS_Imap_t{
	int inodes[IMAP_PIECE_SIZE];
} MFS_Imap_t;

typedef struct __MFS_CheckpointRegion_t{
	int inode_count;
	int endLog;
	int imap[NUM_IMAP];	
} MFS_CheckpointRegion_t;

typedef struct __MFS_DirDataBlock_t {
  MFS_DirEnt_t entries[NUM_DIR_ENTRIES]; 
} MFS_DirDataBlock_t;

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);

#endif