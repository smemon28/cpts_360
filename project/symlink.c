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

/*symlink oldNAME  newNAME    e.g. symlink /a/b/c /x/y/z

   ASSUME: oldNAME has <= 60 chars, inlcuding the ending NULL byte.

(1). verify oldNAME exists (either a DIR or a REG file)
(2). creat a FILE /x/y/z
(3). change /x/y/z's type to LNK (0120000)=(1010.....)=0xA...
(4). write the string oldNAME into the i_block[ ], which has room for 60 chars.
    (INODE has 24 unused bytes after i_block[]. So, up to 84 bytes for oldNAME) 

     set /x/y/z file size = number of chars in oldName

(5). write the INODE of /x/y/z back to disk.
*/
void my_symlink (char *old_file, char *new_file)
{
    int i;
	int oino, parent_ino;
	MINODE *omip; //mip will point to the child Inode
	MINODE *p_mip; //p_mip will point to the parent minode
	INODE *ip; //ip will point to the child Inode
	INODE *p_ip; //p_ip will point to the parent inode
	char temp[64];
	char child_name[64];

	char link_temp[64];
	char link_parent[64];
	char link_child[64];
	int link_pino, link_ino;
	MINODE *link_pmip, *link_mip;
	
	strcpy (temp, old_file);
	strcpy(child_name, basename(temp));

	oino =  getino(old_file);
	omip = iget(dev, oino);

	if (strlen(old_file) >= 60)
	{
		printf("name too long");
		return;
	}

	if (!omip)
	{
		printf("File doesnt exist");
	}
	
	strcpy(link_temp, new_file);
	dbname(link_temp, link_parent, link_child);



	link_pino = getino (link_parent);
	link_pmip = iget(dev, link_pino);

	if(!link_pmip)
	{
		printf("Error! Parent doesnt exist");
		return;
	}
	if(!(S_ISDIR(link_pmip->INODE.i_mode)))
	{
		printf("Error! parent is not a directory!");
		return;
	}
	if(getino(link_child)> 0)
	{
		printf("child already exists");
		return;
	}
	creat_file(new_file);
	link_ino = getino(new_file);
	link_mip = iget(dev, link_ino);
	link_mip -> INODE.i_mode = 0120777;
	link_mip -> INODE.i_size =2;
	link_mip -> dirty = 1;
	iput(link_mip);

	link_pmip -> INODE.i_atime = time(0L);
	link_pmip -> dirty =1;

	iput(link_pmip);
	

	
    
}