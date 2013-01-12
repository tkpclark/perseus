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

off_t get_file_size(int fd)
{
	struct stat statbuf;
	if(!fstat(fd,&statbuf))
	{
		return statbuf.st_size;
	}
	else
	{
		printf("ALERT:get file stat error! %s\n",strerror(errno));
		return 0;
	}

}
void print_HEX(char *p,int length)
{
	int i;
	for(i=0;i<length;i++)
	{
		printf("%02X ",*(unsigned char*)(p+i));
	}
	printf("\n");
}
int count_tail_zero(char *p,int len)
{
	void *q;
	int count=0;
	q=p+len-1;
	while(*(unsigned char*)q==0x0)
	{
		count++;
		q--;
	}
	return count;
}
encrypt_file(int action,char *filename,char *key)
{
	if(action!=1 && action!=0)
	{
		printf("1:encrypt 0:decrypt\n");
		return;
	}
	
	int fd;
	int des_len;
	int zero_number;
	fd=open(filename,2);
	int filesize=get_file_size(fd);
	des_len=count_des_len(filesize);
	char *p=malloc(des_len);
	memset(p,0,des_len);
	read(fd,p,des_len);
//	print_HEX(p,des_len);

	ftruncate(fd,0);
	lseek(fd,0,SEEK_SET);

	T_DES(action,key,des_len,p,p);

	if(action==1)//encrypt
		write(fd,p,des_len);	
	if(action==0)//decrypt
	{
		zero_number=count_tail_zero(p,filesize);
		write(fd,p,filesize-zero_number);
	}
//	print_HEX(p,des_len);
//	print_HEX(p,des_len);
	

	close(fd);
	/*
	print_HEX(data,128);
	T_DES(1,"123222222",128,data,data);
	print_HEX(data,128);
	T_DES(0,"123222222",128,data,data);
	print_HEX(data,128);
	*/

}
main(int argc,char **argv)
{
	
	if(argc!=4)
	{
		printf("arg1: encrypt:1 decrypt:0\n");
		printf("arg2:filename\n");
		printf("arg3:key\n");
	}
	encrypt_file(atoi(argv[1]),argv[2],argv[3]);
}
