#include <stdio.h>       // for printf()
#include <stdlib.h>      // for exit()
#include <string.h>      // for strcpy(), strcmp(), etc.
#include <libgen.h>      // for basename(), dirname()
#include <fcntl.h>       // for open(), close(), read(), write()

// for stat syscalls
#include <sys/stat.h>
#include <unistd.h>
 
// for opendir, readdir syscalls
#include <sys/types.h>
#include <dirent.h>

#define BLKSIZE 4096

struct stat sb1, sb2;
char buf[BLKSIZE];

int fileexists(char *f1, struct stat *sb);
int myrcp(char *f1, char *f2);
int cpf2f(char *f1, char *f2);
int cpf2d(char *f1, char *f2);
int cpd2d(char *f1, char *f2);

int main(int argc, char *argv[])
{
         if (argc < 3)
	   {
	       printf("enter filenames\n");
	       return -1;
	   }
	 return myrcp(argv[1], argv[2]);
}

int fileexists(char *f, struct stat *sb)
{
         if (stat(f, sb) == -1)
	   {
	     perror("stat");
	     return -1;
	   }
	 printf("File name:                %s\n", f);
}

// f1=SOURCE f2=DEST
int myrcp(char *f1, char *f2)
{
    /*
   1. stat f1;   if f1 does not exist ==> exit.
                 f1 exists: reject if f1 is not REG or LNK file
   2. stat f2;   reject if f2 exists and is not REG or LNK file

   3. if (f1 is REG){
         if (f2 non-exist OR exists and is REG)
            return cpf2f(f1, f2);
	 else // f2 exist and is DIR
            return cpf2d(f1,f2);
      }
   4. if (f1 is DIR){
	if (f2 exists but not DIR) ==> reject;
        if (f2 does not exist)     ==> mkdir f2
	return cpd2d(f1, f2);
      }
  */
         if (fileexists(f1, &sb1) < 0)   exit(EXIT_FAILURE);  // must exit if f1 doesn't exist

	 printf("File type:                ");

	 switch (sb1.st_mode & S_IFMT)
	   {
	   /*********************************************/
	   case S_IFBLK:  printf("block device\n");
	     break;
	   /*********************************************/
	   case S_IFCHR:  printf("character device\n");
	     break;
	   /*********************************************/
	   case S_IFDIR:  printf("directory\n");
	     if (fileexists(f2, &sb2) < 0)
	       {
		 printf("file doesn't exist...making DIR\n");
		 // mkdir and continue
	       }
	     
	     switch (sb2.st_mode & S_IFMT)    
	       {
	       case S_IFDIR:           // if f2 is DIR
		 return cpd2d(f1, f1);
		 
	       case S_IFREG:           // if f2 is a REG File
		 printf("can't copy DIR to FILE\n");
		 break;
	       } 
	     break;
	   /*********************************************/
	   case S_IFLNK:  printf("symlink\n");
	     break;
	   /*********************************************/
	   case S_IFREG:  printf("regular file\n");
	     if (fileexists(f2, &sb2) < 0)
	       {
		 printf("file doesn't exist...making FILE\n");
		 // creat file and continue, cpf2f will run
	       }
	     
	     switch (sb2.st_mode & S_IFMT)    
	       {
	       case S_IFDIR:            // if f2 is DIR
		 return cpf2d(f1, f1);
	       
	       case S_IFREG:            // if f2 is REG file
		 return cpf2f(f1, f2);
	       }
	     break;
	   /*********************************************/
	   default:       printf("unknown?\n");
	     break;
	   }
}

// cp file to file
int cpf2f(char *f1, char *f2)
{
  /*
  1. reject if f1 and f2 are the SAME file
  2. if f1 is LNK and f2 exists: reject
  3. if f1 is LNK and f2 does not exist: create LNK file f2 SAME as f1
  4:
     open f1 for READ;
     open f2 for O_WRONLY|O_CREAT|O_TRUNC, mode=mode_of_f1;
     copy f1 to f2
  */
          printf("cpf2f running....\n");

	  int fd, gd;
	  int n, total=0;

	  fd = open(f1, O_RDONLY);
	  gd = open(f2, O_WRONLY|O_CREAT|O_TRUNC, sb1.st_mode);

	  while (n = read(fd, buf, BLKSIZE))
	    {
	      write(gd, buf, n);
	      total += n;
	    }
	  printf("total=%i\n", total);
	  close(fd); close(gd);
}

int cpf2d(char *f1, char *f2)
{
  /*
  1. search DIR f2 for basename(f1)
     (use opendir(), readdir())
     // x=basename(f1); 
     // if x not in f2/ ==>        cpf2f(f1, f2/x)
     // if x already in f2/: 
     //      if f2/x is a file ==> cpf2f(f1, f2/x)
     //      if f2/x is a DIR  ==> cpf2d(f1, f2/x)
     */
           printf("cpf2d running....\n");
	   
	   struct dirent *ep;       // use direct to search directories
	   DIR *dp = opendir(f2);   // open the DIR 'f2'
	   while (ep = readdir(dp))
	     {
	       if (strcmp(f1, ep->d_name) == 0)
		 {
		   printf("%s already exists in %s\n", f1, f2);
		   break;
		 }
	     }
}

int cpd2d(char *f1, char *f2)
{
    // recursively cp dir into dir
           printf("cpd2d running....\n");
}
