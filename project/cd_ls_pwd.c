/****************************************************************************
*                   cd, ls, pwd Program                                      *
*****************************************************************************/
/*******************
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
*********************/

/**** globals defined in main.c file ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap;
extern char line[256], cmd[32], pathname[256];

int print(MINODE *mip)
{
	int blk;
	char buf[1024], temp[256];
	int i;
	DIR *dp;
	char *cp;

	INODE *ip = &mip->INODE;
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

int ls(char *pathname)
{
	int ino, n;
	MINODE *mip;

	if (strcmp(pathname, "") != 0)
	{
		ino = getino(pathname);
		mip = iget(dev, ino);
		print(mip);
		return 0;
	}
	print(running->cwd);
}

int chdir(char *pathname)
{
	int ino, n;
	MINODE *mip;

	if (strcmp(pathname, "") == 0)
		running->cwd = root;
	else
	{
		ino = getino(pathname);
		mip = iget(dev, ino);
		printf("path mode is %d: HEX %04X\n", mip->INODE.i_mode, mip->INODE.i_mode);
		if (mip->INODE.i_mode != DIR_MODE)
		{
			printf("not a DIR\n");
			return 0;
		}
		iput(running->cwd);
		running->cwd = mip;
	}
}

int quit()
{
	int i;
	MINODE *mip;
	for (i = 0; i < NMINODE; i++)
	{
		mip = &minode[i];
		if (mip->refCount > 0)
			iput(mip);
	}
	exit(0);
}

int pwd(MINODE *wd)
{
	if (wd == root)
		printf("/");
	else
		rpwd(wd);

	printf("\n");
}

int rpwd(MINODE *wd)
{
	/*
        if (wd==root) return;
	from i_block[0] of wd->INODE: get my_ino of . parent_ino of ..
	  pip = iget(dev, parent_ino);
	from pip->INODE.i_block[0]: get my_name string as LOCAL

	  rpwd(pip);  // recursive call rpwd() with pip

	print "/%s", my_name;
	*/
	if (wd == root)
		return 0;

	u32 my_ino, parent_ino;
	char *cp, my_name[EXT2_NAME_LEN], buf[BLKSIZE];
	DIR *dp;
	MINODE *pip;
	/*
	my_ino = search(&wd->INODE, ".");
	parent_ino = search(&wd->INODE, "..");
	*/
	my_ino = search(wd, ".");
	parent_ino = search(wd, "..");

	pip = iget(dev, parent_ino);
	get_block(dev, pip->INODE.i_block[0], buf);
	dp = (DIR *)buf;
	cp = buf; // let cp point to the start of the buffer
	while (cp < buf + BLKSIZE)
	{
		cp += dp->rec_len;
		dp = (DIR *)cp;

		if (dp->inode == my_ino)
			break;
	}
	strncpy(my_name, dp->name, dp->name_len);
	my_name[dp->name_len] = 0;
	rpwd(pip);

	printf("/%s", my_name);
}
