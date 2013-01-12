#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>


main(int argc,char **argv)
{
	if(argc!=2)
	{
		printf("please input filename!\n");
		exit(0);
	}
	
	printf("%X\n",get_file_crc(argv[1]));
}

