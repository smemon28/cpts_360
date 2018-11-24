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
int creat_file(char *pathname)
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
    //   parent = dirname(pathname);   parent= "/a/b" OR "a/b"
    // child  = basename(pathname);  child = "c"

    dbname(pathname, parent, child);

    //3. Get the In_MEMORY minode of parent:
   //Verify : (1). parent INODE is a DIR (HOW?)   AND
     //       (2). child does NOT exists in the parent directory (HOW?);
    pino = getino(parent);
    pip = iget(dev, pino);
    printf("parent path mode is %d: HEX %04X\n", pip->INODE.i_mode, pip->INODE.i_mode);
    if (pip->INODE.i_mode != DIR_MODE)    {
        printf("not a DIR\n");
        return 0;
    }

    if (search(pip, child) != 0){
        printf("%s already exists in cwd:%s\n", child, parent);
        return 0;
    }
    //4. call my_creat(pip, child);
    my_creat(pip, child);
    //5. inc parent inodes's link count by 1; 
    // touch its atime and mark it DIRTY
    pip->INODE.i_links_count = 1;
    pip->INODE.i_atime = time(0L);
    //6. iput(pip);
    iput(pip);
}

int my_creat(MINODE *pip, char *name)
{
    MINODE *mip;
    int ino, bno, i;
    char *cp, buf[BLKSIZE];
    DIR *dp;

    ino = ialloc(dev);  // get new inode num
    bno = balloc(dev);  // get new block

    // to load the inode into a minode[] (in order to write contents to the INODE in memory)
    mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    //Use ip-> to acess the INODE fields:

    ip->i_mode = FILE_MODE;		// FILE type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = 0;		// Size in bytes - empty file
    ip->i_links_count = 1;	        // Links count=1 because it's a file
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new FILE has one data block   
    for (i=1; i<15; i++)
        ip->i_block[i] = 0;

    mip->dirty = 1;               // mark minode dirty
    iput(mip);                    // write INODE to disk

    enter_name_creat(pip, ino, name);
}

int enter_name_creat(MINODE *pip, int myino, char *myname)
{
    char *cp, buf[BLKSIZE];
    int i, need_length, remaining;
    INODE *pinode;
    DIR *dp;

    pinode = &pip->INODE;
    for (i=0; i<12; i++){
        if (pinode->i_block[i] == 0) break;

        need_length = 4*( (8 + strlen(myname) + 3)/4 );  // a multiple of 4

        // step to LAST entry in block: int blk = parent->INODE.i_block[i];
        // which is equivalent to pinode->i_block[i];
        get_block(dev, pinode->i_block[i], buf);  // get disk block
        dp = (DIR *)buf;
        cp = buf;

        printf("step to LAST entry in data block %d\n", pinode->i_block[i]);
        while (cp + dp->rec_len < buf + BLKSIZE){
            /****** Technique for printing, compare, etc.******/
            char c;
            c = dp->name[dp->name_len];
            dp->name[dp->name_len] = 0;
            printf("%s ", dp->name);
            dp->name[dp->name_len] = c;
            /***********************************/
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("\n");
        // dp NOW points at last entry in block
        // remain = LAST entry's rec_len - last entry IDEAL_LENGTH;
        int ideal_len = 4*( (8 + dp->name_len + 3)/4 );
        remaining = dp->rec_len - ideal_len;

        if (remaining > need_length) {
            dp->rec_len = ideal_len;    // update rec_len of last entry - will become 2nd to last now
            cp += dp->rec_len;
            dp = (DIR *)cp;

            dp->inode = myino;
            dp->rec_len = remaining;    // remining bytes are assigned to added entry (now last)
            dp->name_len = need_length;
            strncpy(dp->name, myname, dp->name_len);
        }
        else {
            //#5
            int nbno;
            dp->rec_len = ideal_len;    // update rec_len of last entry - will become 2nd to last now
            pinode->i_size += BLKSIZE;  // because allocating new data block to contain new dir

            nbno = balloc(dev);
            pinode->i_block[i+1] = nbno;
            get_block(dev, pinode->i_block[i+1], buf);  // get next disk block
            dp = (DIR *)buf;
            dp->inode = myino;
            dp->rec_len = BLKSIZE;
            dp->name_len = need_length;
            strncpy(dp->name, myname, dp->name_len);
        }
        put_block(dev, pinode->i_block[i], buf);
    }
}