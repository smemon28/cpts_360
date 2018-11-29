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
    //printf("imode:%x\n", omip->INODE.i_mode);
    //getchar();
    if (omip->INODE.i_mode == FILE_MODE) {
        DIR *dp;    
        MINODE *nmip;
        dbname(newFileName, nparent, nchild);
        nino = getino(nparent);
        nmip = iget(dev, nino);
        if (nmip->dev !=  omip->dev) {
            printf("dev ids don't match - can't link files\n");
            return 0;
        }
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
    else if (omip->INODE.i_mode == DIR_MODE) {
        printf("%s is a DIR: can't link DIRs\n", oldFileName);
        iput(omip);
        return 0;
    }
    printf("not a FILE\n");
}

int truncate(INODE *ip)
 {
	char buf[1024], temp[256];
	int i;
	DIR *dp;
	char *cp;

	for (i = 0; i < 12; i++)
	{
		if (ip->i_block[i] == 0)
			return 0;
		get_block(dev, ip->i_block[i], buf);

		dp = (DIR *)buf;
		cp = buf;

		printf("inode    rec len    name len    name\n");
		while (cp < buf + 1024)
		{
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0; // removing end of str delimiter '/0', why tho?
			// make dp->name a string in temp[ ]
			printf("%2d %12d %12d %12s\n", dp->inode, dp->rec_len, dp->name_len, temp);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
 }
 
int unlink(char *pathname)
{
    int uino;
    MINODE *umip;
    INODE *ip;

    uino = getino(pathname);
    umip = iget(dev, uino);

    ip = &umip->INODE;
    if (ip->i_mode == FILE_MODE) {
        ip->i_links_count--;
        if (ip->i_links_count == 0)
            truncate(ip);
    }
    else if (ip->i_mode == DIR_MODE) {
        printf("can't be a DIR\n");
        return 0;
    }
}