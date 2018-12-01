/**** globals defined in main.c file ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include <time.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap;
extern char pathname[256];

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";