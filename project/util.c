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
// the INODE of (dev, ino). If NOT found then allocate a new minode
MINODE* iget(int dev, int ino)
{
    //MTABLE *mp;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];

    // search in-memory minodes first
    for (i = 0; i<NMINODE; i++){
        MINODE *mip = &minode[i];
        if (mip->refCount  && (mip->dev == dev)  && (mip->ino == ino)){
            mip->refCount++;
            return mip;
           }
    }

    MINODE *mip;
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

        get_block(dev, mip->INODE.i_block[i], dbuf);  // get disk block
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
        printf("root dev:%d ino:%d\n", mip->dev, mip->ino);
    }
    else{
        mip = running->cwd;
        printf("cwd dev:%d ino:%d\n", mip->dev, mip->ino);
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
        //iput(mip);          // release current inode ->LINE COMMENTED but was present in book
        mip = iget(dev, ino);  // switch to new minode
    }
    //iput(mip);   ->LINE COMMENTED but was present in book
    return ino;
}
/*
char* find_name(MINODE *mip)
{
    int my_ino = 0;
    int parent_ino = 0;
    findino(mip, &my_ino, &parent_ino);

    MINODE* parent_mip = iget(running->cwd->dev, parent_ino);

    char* my_name = NULL;
    findmyname(parent_mip, my_ino, &my_name);

    iput(parent_mip);
    return my_name;
}

int findmyname(MINODE *parent, int myino, char *myname)
{
	//Given the parent DIR (MINODE pointer) and myino, this function finds
	//the name string of myino in the parent's data block. This is the SAME
	//as SEARCH() by myino, then copy its name string into myname[ ].
	//It does this using the dir pointer to get the name
	
	int i;
	INODE *ip;
	char buf[BLKSIZE];
	char *cp;
	DIR *dp;

	//easy root case
	if(myino == root->ino)
	{
		strcpy(myname, "/");
		return 0;
	}

	//error
	if(!parent)
	{
		printf("ERROR: no parent\n");
		return 1;
	}

	//set ip so we can access the parent inodes data fields
	ip = &parent->INODE;

	//files don't have children
	if(!S_ISDIR(ip->i_mode))
	{
		printf("ERROR: not directory\n");
		return 1;
	}

	//search through each of the direct blocks of the inode
	for(i = 0; i < 12; i++)
	{
		//if a valid value is in the block
		if(ip->i_block[i])
		{
			//get and cast the block as a dir
			get_block(dev, ip->i_block[i], buf);
			dp = (DIR*)buf;
			cp = buf;

			//use cp to grab the block data
			while(cp < buf + BLKSIZE)
			{
				//if the current ino = the ino we're looking for, boom target found
				if(dp->inode == myino)
				{
					//copy the name into myname.
					strncpy(myname, dp->name, dp->name_len);
					//add terminal characteddr to string
					myname[dp->name_len] = 0;
					return 0;
				}
				else
				{
					//increment to the next section, keep looking for matching ino
					cp += dp->rec_len;
					dp = (DIR*)cp;
				}
			}
		}
}
	return 1;
}


//findino(mip, &ino, &parent_ino);  mip is the pointer to the node to removes parent, ino should be the child ino, parent ino should be the ino of dir that child is in
//find ino looks at the inode of the dir to remove, and gets the ino's of the dir and parent dir from the dirs ip->block[0] and then again to get parent
int findino(MINODE *mip, int *myino, int *parentino)
{
	//For a DIR Minode, extract the inumbers of . and ..
	//Read in 0th data block. The inumbers are in the first two dir entries.
	
	INODE *ip;
	char buf[1024];
	char *cp;
	DIR *dp;

	//check if exists
if(!mip)
	{
		printf("ERROR: ino does not exist!\n");
		return 1;
	}

	//point ip to minode's inode
	//ip is now pointing at the inode struct of the dir to remove
	ip = &mip->INODE;

	//check if directory
	if(!S_ISDIR(ip->i_mode))
	{
		printf("ERROR: ino not a directory\n");
		return 1;
	}

	//get the first block from the dir to find out its ino
	get_block(dev, ip->i_block[0], buf);
	dp = (DIR*)buf;
	cp = buf;

	//.
	*myino = dp->inode;

	//increment the pointer by rec len then cast again to get the parent ino 
	cp += dp->rec_len;
	dp = (DIR*)cp;

	//..
	*parentino = dp->inode;

//done!
	return 0;
}
*/
/*void ifree(int dev, int inode)
{
    if(inode <= 0)
        return;

    u8* imap = get_imap(dev);

    clear_bit(&imap, inode - 1);
    set_free_inodes(dev, +1);

    put_imap(dev, imap);
}

void bfree(int dev, int block)
{
    if(block <= 0)
        return;

    u8* bmap = get_bmap(dev);

    clear_bit(&bmap, block);
    set_free_blocks(dev, +1);

    put_bmap(dev, bmap);
}*/
int clearbit(char *buf, int BIT) { /*{{{*/
	int i, j;
	i = BIT / 8;
	j = BIT % 8;
	buf[i] &= ~(1 << j);
	return 1;
} /*}}}*/
/*
//deallocates an inode for a given ino on the dev
//This is used when we remove things
//Once dealocated, we increment the free inodes in the SUPER and in the group descriptor
int idealloc(int dev, int ino)
{
	char buf[1024];
	int byte;
	int bit;

	//clear bit(bmap, bno)
	get_block(dev, imap, buf);

	//Mailmans to where it is
	byte = ino / 8;
	bit = ino % 8;

	//Negate it
	buf[byte] &= ~(1 << bit);

	put_block(dev, imap, buf);

	//set free blocks
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);
}

//deallocate a block
//once deallocated we also increment the number of free blocks
int bdealloc(int dev, int bno)
{
	char buf[1024];
	int byte;
	int bit;

	//clear bit(bmap, bno)
	get_block(dev, bmap, buf);

	byte = bno / 8;
	bit = bno % 8;

	buf[byte] &= ~(1 << bit);

	put_block(dev, bmap, buf);

	//set free blocks
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);

	return 0;
}
*/