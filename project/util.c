/****************
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
**************/
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


int get_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    int n = read(fd, buf, BLKSIZE);
    if (n < 0) printf("get_block: [%d %d] error\n", dev, blk);
}

int put_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    int n = write(fd, buf, BLKSIZE);
    if (n != BLKSIZE) printf("put_block: [%d %d] error\n", dev, blk);
}

MINODE* mialloc()   // allocate a free inode
{
    int i;
    for (i = 0; i<NMINODE; i++){
        MINODE *mp = &minode[i];
        if (mp->refCount == 0){
            mp->refCount = 1;
            return mp;
        }
    }
    printf("FS panic: out of minodes\n");
    return 0;
}

int midalloc(MINODE *mip)   // release a used minode
{
    mip->refCount = 0;  // just resetting the refCount is enough
                        // to reset as that memory location is not
                        // locked anymore
}

// this function returns a pointer to the in-memory minode containing
// the INODE of (dev, ino). If not found then allocate a new minode
MINODE* iget(int dev, int ino)
{
    MINODE *mip;
    MTABLE *mp;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];

    // search in-memory minodes first
    for (i = 0; i<NMINODE; i++){
        MINODE *mip = &minode[i];
        if (mip->refCount  && 
           (mip->dev == dev)  &&
           (mip->ino == ino)){
            mip->refCount++;
            return mip;
           }
    }
    // needed INODE=(dev, ino) not in memory
    mip = mialloc();          // allocate a free minode
    mip->dev = dev; mip->ino = ino;  // assign to (dev, ino)

    block = (ino - 1) / 8 + iblk;
    offset = (ino - 1) % 8;
    get_block(dev, block, buf);
    
    ip = (INODE*) buf + offset;
    mip->INODE = *ip;    // this is where inode on disk is placed in memory
    
    // initialize minode
    mip->refCount = 1;
    mip->mounted = 0;
    mip->dirty = 0;
    mip->mptr = 0;
    return mip;
}

// This function releases a used minode pointed by mip. Each minode
// has a refCount, which represents the number of users that are
// using the minode, iput() decrements refCount by 1. If caller is
// last user (i.e. refCount 0 after decrement) AND modified (dirty)
// the INODE is written back to disk
int iput(MINODE *mip)
{
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];

    if (mip == 0) return 0;
    mip->refCount--;   // dec refCount by 1
    if (mip->refCount > 0) return 0;  // still has a user
    if (mip->dirty == 0) return 0;    // no need to write back
                                    // as no changes have been made
    //write INODE back to disk
    block = (mip->ino - 1) / 8 + iblk;
    offset = (mip->ino - 1) % 8;

    // get block containing this inode
    get_block(mip->dev, block, buf);
    ip = (INODE  *)buf + offset;      // ip now points to INODE of mip
    *ip = mip->INODE;           // copy INODE to inode in block
    put_block(mip->dev, block, buf);  // write back to disk
    midalloc(mip);                  // mip->refCount = 0;
}

int tokenize(char *path)
{
      int i = 0;
      char *s;

      strcpy(gpath, path);
      s = strtok(gpath, "/");
      while (s != NULL)
	{
	  name[i] = s;    // store pointer to token in *name[] array  
	  s = strtok(0, "/");
	  i++;
	}
      return i;
}

int search(MINODE *mip, char *name)
// which searches the DIRectory's data blocks for a name string; 
// return its inode number if found; 0 if not.
{
    char *cp, dbuf[BLKSIZE], temp[256];
    int i;     
    for (i=0; i < 12; i++){  // assuming only 12 entries
        if (mip->INODE.i_block[i] == 0)
            return 0;

        get_block(fd, mip->INODE.i_block[i], dbuf);  // get disk block
        dp = (DIR *)dbuf;
        cp = dbuf;
        
        printf("inode       rec len       name len       name\n");
        while (cp < dbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%10d %10d %10d %5s\n", 
                dp->inode, dp->rec_len, dp->name_len, temp);
            if (strcmp(name, temp) == 0){
                printf("found %s : inumber = %d\n", name, dp->inode);
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    printf("\n");
    return 0;
}

int getino(char *pathname)
{
    MINODE *mip;
    int i, n, ino;
    char ibuf[BLKSIZE];
    
    if (strcmp(pathname, "/") == 0)
        return 2;    // return ino of root, i.e. 2
    if (pathname[0] == '/'){      
        mip = root;
        printf("mip=root\n");
    }
    else{
        mip = running->cwd;
        printf("mip=running->cwd dev:%d ino:%d", mip->dev, mip->ino);
    }

    n = tokenize(pathname);  // tokenize pathname and store in *tkn[]

    for (i = 0; i < n; i++){  // iterate for all the names in the path, i.e. /a/b/c -> n = 3
        if (!S_ISDIR(mip->INODE.i_mode)){
            printf("%s is not a directory\n", name[i]);
            iput(mip);
            return 0;
        }
        ino = search(mip, name[i]);
        if (!ino){
            printf("can't find %s\n", name[i]); 
            return 0;
        }
        iput(mip);          // release current inode
        mip = iget(dev, ino);  // switch to new minode
    }
    iput(mip);
    return ino;
}
