/**** globals defined in main.c file ****/
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[256], cmd[32], pathname[256];

/*************************FUNCTIONS*******************************/
int dbname(char *pathname, char *dname, char *bname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}

int make_dir(char *pathname)
{
    int pino;
    MINODE *mip, *pip;
    char parent[64], child[64];

//1. pahtname = "/a/b/c" start mip = root;         dev = root->dev;
  //          =  "a/b/c" start mip = running->cwd; dev = running->cwd->dev;
    if (pathname[0] == '/') {
        mip = root;
        dev = root->dev;
    }
    else {
        mip = running->cwd;
        dev = running->cwd->dev;
    }
//2. Let  
  //   parent = dirname(pathname);   parent= "/a/b" OR "a/b"
    // child  = basename(pathname);  child = "c"

   //WARNING: strtok(), dirname(), basename() destroy pathname
    dbname(pathname, parent, child);

//3. Get the In_MEMORY minode of parent:

  //       pino  = getino(parent);
    //     pip   = iget(dev, pino); 

   //Verify : (1). parent INODE is a DIR (HOW?)   AND
     //       (2). child does NOT exists in the parent directory (HOW?);
    pino = getino(parent);
    pip = iget(dev, pino);
    printf("parent path mode is %d: HEX %04X\n", pip->INODE.i_mode, pip->INODE.i_mode);
    if (mip->INODE.i_mode != DIR_MODE)
    {
        printf("not a DIR\n");
        return 0;
    }

    if (search(pip, child) != 0){
        printf("%s already exists in cwd:%s\n", child, parent);
        return 0;
    }
//4. call mymkdir(pip, child);
    mymkdir(pip, child);
//5. inc parent inodes's link count by 1; 
  // touch its atime and mark it DIRTY
    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
//6. iput(pip);
    iput(pip);
}

int mymkdir(MINODE *pip, char *name)
{
    MINODE *mip;
    int ino, bno, i;
    char buf[BLKSIZE];

    ino = ialloc(dev);  // get new inode num
    bno = balloc(dev);  // get new block

    // to load the inode into a minode[] (in order to write contents to the INODE in memory)
    mip = iget(dev, ino);
  INODE *ip = &mip->INODE;
  //Use ip-> to acess the INODE fields:

  ip->i_mode = 0x41ED;		// OR 040755: DIR type and permissions
  ip->i_uid  = running->uid;	// Owner uid 
  ip->i_gid  = running->gid;	// Group Id
  ip->i_size = BLKSIZE;		// Size in bytes 
  ip->i_links_count = 2;	        // Links count=2 because of . and ..
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
  ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
  ip->i_block[0] = bno;             // new DIR has one data block   
  for (i=1; i<15; i++)
      ip->i_block[i] = 0;

  mip->dirty = 1;               // mark minode dirty
  iput(mip);                    // write INODE to disk
}