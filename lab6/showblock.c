/********* showblock.c code ***************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE 1024

char *disk, *pathname, *tkn[64];

char buf[BLKSIZE];
int fd;

/******************* in <ext2fs/ext2_fs.h>*******************************

**********************************************************************/

int get_block(int fd, int blk, char buf[ ])
{
      lseek(fd, (long)blk*BLKSIZE, 0);
      read(fd, buf, BLKSIZE);
}

int tokenize(char *path)
{
      int i = 0;
      char temp[64];
      char *s;

      strcpy(temp, path);
      s = strtok(temp, "/");
      while (s != NULL)
	{
	  tkn[i] = (char *)malloc(strlen(s)*sizeof(char));
	  memset(tkn[i], 0, strlen(s)*sizeof(char));
	  tkn[i] = s;    // store pointer to token in *tkn[] array  
	  s = strtok(0, "/");
	  i++;
	}
      return i;
}

int super()
{
      get_block(fd, 1, buf);  
      sp = (SUPER *)buf;

      // check for EXT2 magic number:

      printf("s_magic = %x\n", sp->s_magic);
      if (sp->s_magic != 0xEF53){
	printf("NOT an EXT2 FS\n");
	exit(1);
      }
}

int search(INODE *ip, char *name)
// which searches the DIRectory's data blocks for a name string; 
// return its inode number if found; 0 if not.
{
      char *cp, dbuf[BLKSIZE], temp[256];
      
      for (int i=0; i < 12; i++){  // assuming only 12 entries
	if (ip->i_block[i] == 0)
	  break;
	get_block(fd, ip->i_block[i], dbuf);  // get disk block
	
	dp = (DIR *)dbuf;
	cp = dbuf;
    
	while (cp < dbuf + BLKSIZE){
	  strncpy(temp, dp->name, dp->name_len);
	  temp[dp->name_len] = 0;
	  printf("%4d %4d %4d %s\n", 
		 dp->inode, dp->rec_len, dp->name_len, temp);
	  if (strcmp(name, temp) == 0)
	    return dp->inode;
	  cp += dp->rec_len;
	  dp = (DIR *)cp;
	}
      }
      printf("\n");
      return 0;
}

int dirp()
{
      int ino, iblock, blk, offset, n;
      char ibuf[BLKSIZE];
      
      n = tokenize(pathname);  // tokenize pathname and store in *tkn[]
      // read Group Descriptor 0
      get_block(fd, 2, buf);  // get B2 which has the group descriptor
      gp = (GD *)buf;
  
      iblock = gp->bg_inode_table;   // get inode start block num
      get_block(fd, iblock, buf);    // get inode start block

      ip = (INODE *)buf + 1; // get the 2nd INODE (or root INODE)
  
      for (int i=0; i < n; i++){  // iterate for all the names in the path, i.e. /a/b/c -> n = 3
	ino = search(ip, tkn[i]);
	if (ino==0){
	  printf("can't find %s\n", tkn[i]); exit(1);
	}
  
	// Mailman's algorithm: Convert (dev, ino) to inode pointer
	blk    = (ino - 1) / 8 + iblock;  // disk block contain this INODE 
	offset = (ino - 1) % 8;         // offset of INODE in this block
	get_block(fd, blk, ibuf);
	ip = (INODE *)ibuf + offset;    // ip -> new INODE
      }
}

void init()
{
      for (int i = 0; i < 64; i++)
	{
	  tkn[i] = (char *)malloc(20*sizeof(char));
	  memset(tkn[i], 0, 20*sizeof(char));
	}
}

void printbanner()
{
      printf("showblock   device   pathname\n");
      printf("---------   ------   --------\n");
}

int main(int argc, char *argv[ ])
{
      disk = argv[1];
      pathname = argv[2];

      printbanner();
      
      fd = open(disk, O_RDONLY);
      if (fd < 0){
	printf("open failed\n");
	exit(1);
      }

      //init();
      super();
      dirp();
}
