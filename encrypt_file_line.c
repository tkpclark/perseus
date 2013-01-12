#include "com.h"


const int des_len=256;
const char *key="medusa2012";

//static const char *app_config="../config/app.config";


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
int encrypt_file_line(char *filename)
{
	FILE *fp;//for read source file line by line
	int fd;//for write dest file by binary
	
	int zero_number;

	//open source file
	fp=fopen(filename,"r");
	if(fp==NULL)
	{
		printf("failed to open file %s,%s\n",filename,strerror(errno));
		return -1;
	}

	//open dest file

	char filenametmp[128];
	sprintf(filenametmp,"%s.encrypt",filename);
	fd=open(filenametmp, O_CREAT|O_WRONLY|O_APPEND|O_TRUNC,0600);
	if(fd<0)
	{
		printf("failed to open file %s,%s\n",filenametmp,strerror(errno));
		return -1;	
	}
	char buffer[des_len];
	while(1)
	{
		memset(buffer,0,sizeof(buffer));
		if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			break;
		}
		T_DES(1,key,des_len,buffer,buffer);
		write(fd,buffer,sizeof(buffer));
	}
	
	fclose(fp);
	close(fd);

	//unlink(filename);
	//rename(filenametmp,filename);
	/*
	print_HEX(data,128);
	T_DES(1,"123222222",128,data,data);
	print_HEX(data,128);
	T_DES(0,"123222222",128,data,data);
	print_HEX(data,128);
	*/

}

int decrypt_file_line(char *filename)
{
	int fd1,fd2;
	fd1=open(filename, 0);
	if(fd1<0)
	{
		printf("failed to open file %s,%s\n",filename,strerror(errno));
		return -1;	
	}

	char filenametmp[128];
	sprintf(filenametmp,"%s.decrypt",filename);
	fd2=open(filenametmp, O_CREAT|O_WRONLY|O_APPEND|O_TRUNC,0600);
	if(fd2<0)
	{
		printf("failed to open file %s,%s\n",filenametmp,strerror(errno));
		return -1;	
	}

	char buffer[des_len];
	int n;
	while(1)
	{
		memset(buffer,0,sizeof(buffer));
		n=read(fd1,buffer,sizeof(buffer));
		//printf("n:%d\n",n);
		if(!n)
			break;
		T_DES(0,key,des_len,buffer,buffer);
		write(fd2,buffer,strlen(buffer));
		//sleep(1);
	}




	close(fd1);
	close(fd2);
}

