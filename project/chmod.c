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

int my_chmod(char mode[], char *pathname)
{
	int ino;
	MINODE *mip; //mip will point to the child Inode
	int newmode=0, i=0;
    ino = getino (pathname);

    mip = iget(dev, ino);
 //This converts the string from third to an octal int for the permissions
    newmode = (int) strtol(mode, (char **)NULL, 8);
    
    // Change the file's mode to new_mode 
    // The leading 4 bits of type indicate type
    // First clear its current mode by ANDing with 0xF000
// Then set its mode by ORing with new_mode 
    printf("%x", (mip->INODE.i_mode & 0xF000));
    printf("%d", (mip->INODE.i_mode & 0xF000));
    printf("%d", (mip->INODE.i_mode & 0xF000));

    mip->INODE.i_mode = (mip->INODE.i_mode & 0xF000) | newmode;


    mip->dirty = 1;
    iput(mip);


    return 0;

}