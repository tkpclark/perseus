
#include "com.h"
extern const int des_len;
extern const char *key;
extern int debug;
//extern const char *prog;
//extern const char *version;

char *trim(char *str)
{
	char *p=NULL;
	char _str[512];
	memset(_str,0,sizeof(_str));
	p=str;

	//trim header
	while(1)
	{
		if(*p==' '||*p=='\n'||*p=='\r')
		p++;
		else
		break;
	}

	strcpy(_str,p);
	//finish trim header

	//trim tail
	p=_str+strlen(_str)-1;
	while(1)
	{
		if(*p==' ' ||*p=='\n'||*p=='\r')
		p--;
		else
		break;
	}
	*(p+1)=0;

	strcpy(str,_str);
	return str;
}
char *get_box_id(char *box_id)
{
        int fd;
        char *filename="../disp/box_id";
        fd=open(filename,0);
        if(fd<=0)
        {
                fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600);
                struct timeval tv;
                gettimeofday(&tv,NULL);
                sprintf(box_id,"%u%06u",tv.tv_sec,tv.tv_usec);
                write(fd,box_id,strlen(box_id));
        }
        else
        {
                char tmp[256];
			memset(tmp,0,sizeof(tmp));
                read(fd,tmp,sizeof(tmp));
                strcpy(box_id,tmp);
		trim(box_id);
        }
        close(fd);
        return box_id;
}
/*
void proclog(const char *fmt,...)
{
	char ts[32];
	char buf[des_len];
	time_t tt;

	char tmp[des_len];
	memset(tmp,0,sizeof(tmp));
	va_list vs;
	va_start(vs,fmt);
	vsprintf(tmp,fmt,vs);
	va_end(vs);


	
	//get log format
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%F %X",(const struct tm *)localtime(&tt));
	
	sprintf(buf,"[%s][%s]:%s",ts,version,tmp);
	
	//get log file name
	int fd;
	char filename[128];
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.%s",prog_argu.log_dir,ts,get_box_id(box_id),prog);

	//open and write log
	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600); 
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	T_DES(1,key,des_len,buf,buf);
	flock(fd,LOCK_EX);	
	write(fd,buf,sizeof(buf));
	flock(fd,LOCK_UN);
	close(fd);

	
}
*/
off_t get_file_size(int fd)
{
	struct stat statbuf;
	if(!fstat(fd,&statbuf))
	{
		return statbuf.st_size;
	}
	else
	{
		return 0;
	}

}
char *get_day(char *day)
{
        time_t tt;

        tt=time(0);
        strftime(day,30,"%Y%m%d",(const struct tm *)localtime(&tt));

	return day;
}
char *get_yesterday(char *yesterday)
{
        time_t tt;

        tt=time(0)-3600*24;
        strftime(yesterday,30,"%Y%m%d",(const struct tm *)localtime(&tt));

	return yesterday;
}

char *get_hour(char *hour)
{
        time_t tt;

        tt=time(0);
        strftime(hour,30,"%H",(const struct tm *)localtime(&tt));

	return hour;
}

void ch_root_dir()
{
	int fd;
	fd=open("/etc/perseus.conf",0);
	if(fd<0)
		exit(0);
	char buffer[128];
	memset(buffer,0,sizeof(buffer));
	read(fd,buffer,sizeof(buffer));
	close(fd);

	chdir(trim(buffer));

}


int read_app_config(char *config_name,char *name,char *value)
{
	int fd1;
	fd1=open(config_name, 0);
	if(fd1<0)
	{
		printf("failed to open %s,%s\n",config_name,strerror(errno));
		return -1;	
	}

	char buffer[des_len];
	int n;
	char *p=NULL;

	char col_name[64];
	char col_value[64];
	while(1)
	{
		memset(buffer,0,sizeof(buffer));
		n=read(fd1,buffer,sizeof(buffer));
		//printf("n:%d\n",n);
		if(!n)
			break;
		T_DES(0,key,des_len,buffer,buffer);


		memset(col_name,0,sizeof(col_name));
		memset(col_value,0,sizeof(col_value));
		
		p=strtok(buffer,"=");
		if(p!=NULL)
			strcpy(col_name,trim(p));
		p=strtok(NULL,"=");
		if(p!=NULL)
			strcpy(col_value,trim(p));

		if(!strcmp(col_name,name))
		{
			strcpy(value,col_value);
			break;
		}
		
		//sleep(1);
	}




	close(fd1);

}



prt_screen(int file_flag,int trunc_flag, int debug_disp,const char *fmt,...)
{
	//debug_disp true:debug also display,false:debug don't display
	if(!debug_disp && debug)
		return;
	char ts[32];
	char buf[des_len];
	time_t tt;

	char tmp[des_len];
	memset(tmp,0,sizeof(tmp));
	va_list vs;
	va_start(vs,fmt);
	vsprintf(tmp,fmt,vs);
	va_end(vs);
	
	tt=time(0);
	memset(buf,0,sizeof(buf));
	
	

	int fd;

	//print screen
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(buf,"[%s]%s",ts,tmp);

	char filename[128];
	switch(file_flag)
	{
		case 1:
			strcpy(filename,"../disp/box_status");
			break;
		case 2:
			strcpy(filename,"../disp/error");
			break;
		case 3:
			strcpy(filename,"../disp/upload");
			break;
		default:
			return;
	}



	if(trunc_flag==0)
		fd=open(filename, O_CREAT|O_TRUNC|O_WRONLY,0600);
	else//if(flag==2)
		fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600); 
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
		return;
	}

	flock(fd,LOCK_EX);
	write(fd,tmp,strlen(tmp));
	flock(fd,LOCK_UN);
	close(fd);
}

int is_online(char *ip,int port)
{

	int sockfd;
	struct sockaddr_in servaddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr.sin_zero, 8);
	servaddr.sin_family = AF_INET;
	struct  hostent *he;
	he=gethostbyname(ip);
	servaddr.sin_addr.s_addr=*(unsigned long *)he->h_addr; 
	servaddr.sin_port = htons(port);

	int n=-1;
	int try_count=0;
conn:

	if(++try_count > 1)
		return 0;


	//print ip
	/*
	FILE *fp;
	
	fp = popen("ifconfig |grep Bcast ","r");
	char buffer[128];
	if(fgets(buffer, sizeof(buffer)-1, fp)==NULL)
		strcpy(buffer,"NULL\n");
	//proclog("ip address:%s",buffer);

	 */
	//////////
	alarm(10);
	n=connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	alarm(0);
	if(n<0)
	{
		//proclog("failed to connect to %s[%d]\n",ip,try_count);
		sleep(1);
		goto conn;
	}
	else
	{
		close(sockfd);
		return 1;
	}
}
int is_file_exist(char *filename)
{
	int fd;
	fd=open(filename,0);
	if(fd<=0)
	{
		return 0;
	}
	else
	{
		close(fd);
		return 1;
	}
}
int sdcard_exists()
{
	return is_file_exist("/dev/sdcard");
}
write_version(char *filename,char *version,int len)
{
	int fd;
	fd=open(filename, O_CREAT|O_TRUNC|O_WRONLY,0600);
	if(fd<0)
		return;
	write(fd,version,strlen(version));
	close(fd);
}
