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


int main(int argc, char *argv[])
{
         if (argc < 3)
	   {
	       printf("enter filenames\n");
	       return -1;
	   }
	 return 0; // myrcp(argv[1], argv[2]);
}

int myrcp(char *f1, char *f2)
{
  
}
