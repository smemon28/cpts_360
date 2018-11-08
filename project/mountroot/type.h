#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define BLKSIZE  1024

#define NMINODE    64
#define NOFT       64
#define NFD        10
#define NMOUNT      4
#define NPROC       2

// Block number of EXT2 FS on FD
#define SUPERBLOCK   1
#define GDBLOCK      2
#define ROOT_INODE   2

// Default dir and regular file modes
#define DIR_MODE     0x41ED
#define FILE_MODE    0x81AE
#define SUPER_MAGIC  0xEF53
#define SUPER_USER   0

// Proc status
#define FREE         0
#define BUSY         1

typedef struct minode{
  INODE      INODE;
  int        refCount;
  int        dirty;
  int        mounted;
  struct mntable *mptr;
}MINODE;

typedef struct oft{
  int      mode;
  int      refCount;
  MINODE   *mptr;
  int      offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          status;
  int          uid, gid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;
