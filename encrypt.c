#include "com.h"
int debug=0;
main(int argc,char **argv)
{
	
	if(argc!=3)
	{
		printf("arg1: encrypt:1 decrypt:0\n");
		printf("arg2:filename\n");
		exit(0);
	}

	
	if(!strcmp(argv[1],"1"))
		encrypt_file_line(argv[2]);
	else if(!strcmp(argv[1],"0"))
		decrypt_file_line(argv[2]);
	else
		printf("arg1: encrypt:1 decrypt:0\n");
}

