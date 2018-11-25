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
int mylink(char *oldFileName, char *newFileName)
{
    int oino, nino;
    MINODE *omip, *pip;
    char oparent[64], ochild[64];
    char nparent[64], nchild[64];

    printf("link oldfile:%s newfile:%s\n", oldFileName, newFileName);
    dbname(oldFileName, oparent, ochild);
    printf("oparent:%s ochild:%s\n", oparent, ochild);
    getchar();
    oino = getino(ochild);  // to chk if oldFileName is a DIR or not
    omip = iget(dev, oino);

    if (getino(newFileName) != 0) {
        printf("%s already exists\n", newFileName);
        return 0;
    }
    printf("imode:%x\n", omip->INODE.i_mode);
    getchar();
    if (omip->INODE.i_mode == DIR_MODE) {
        printf("%s is a DIR: can't link DIRs\n", oldFileName);
        iput(omip);
        return 0;
    }
    else if (omip->INODE.i_mode == FILE_MODE) {
        DIR *dp;    
        MINODE *nmip;
        dbname(newFileName, nparent, nchild);
        nino = getino(nparent);
        nmip = iget(dev, nino);
        if (nmip->INODE.i_mode != DIR_MODE) {
            printf("%s(newfile) destination not a dir", nparent);
            return 0;
        }

        omip->dirty = 1;
        // creat new file with same ino as old file
        enter_name_creat(nmip, omip->ino, nchild);
        // increment links count for INODE - same for both files
        omip->INODE.i_links_count++;
        iput(omip);
        iput(nmip);
        printf("link done\n");
        return 1;
    }
    printf("not a FILE\n");
}