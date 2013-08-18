/****
version
1.12 resloved the problem of cannot set system clock,you have to sleep for a while after ntpd
1.13 write version number by -v arugments
1.14 updated function ntpupdate(); add "hwclock -w" to procquit 	2012-11-27
1.15 modified prompt words
1.16 print log to the same logfile (.log) with other models
	log don't encrypt
1.17 ignore signal SIGCHLD,avoid zombie
1.18 add "ERR" before error logs
1.19 modify log format,splited with tab
	display number of updated symbols
	2013-03-06
1.20 modify update log,print "." every time one after updated
1.21 modify way of display when updating

2.00 tcp upload 2013-03-12
2.01 if box startup for 2 times a day,there would be two logs for this day, the second
	could cover the first,so modify"mv" to "cat >>"
2.02 remove comment of update procedure
2.03 reset new mac
2.04 add trunc before wirte ../disp/ip and ../disp/mac
2.05 delete files before N days
2.06 copy day logs under ../log and ../logbak when sdcard exists
2.07 delete files before 36days(old) ---> 21days(now)
2.10 abolish version ,use crc instead, from int to char*
2.11 modify get_local_ip
2.12 don't do anything before get the rightime when the first start
2.13 day.sys only be store for 3 days
2.14 try more times when downloading failed
	更改 "更新结束" 的显示位置
2.15 no more clear "error display area"
2.21 change_update_version
2.30	runing flag
2.32 fix the bug of 2.10
2.33 no more generate new mac every time ,generate once and use it forever
*****/


#include "com.h"
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#define UPD_LST_MAX_NUM 300

typedef struct
{
	char ip[32];
	char username[32];
	char password[32];

	char tcp_server_ip[32];
	char tcp_server_port[12];
	
}SERVER_INFO;

typedef struct
{
	char filename[128];
	char version[64];  //use crc instead
	char local_dir[128];
	char server_dir[128];
	char crc[64];
	
}FILE_INFO;

typedef struct
{
	char apk_dir[128];
	char config_dir[128];
	char log_dir[128];
	char logbak_dir[128];
	char app_config[128];
	char version_config_local[128];
	FILE_INFO file_info_local[UPD_LST_MAX_NUM];
	FILE_INFO file_info_server[UPD_LST_MAX_NUM];
	SERVER_INFO server_info;
}PROG_ARGU;

PROG_ARGU prog_argu[2];

//static const char *app_config="../config/app.config";
//static SERVER_INFO server_info;
//static FILE_INFO file_info_local[UPD_LST_MAX_NUM], file_info_server[UPD_LST_MAX_NUM];
//static const char *version_config_local="../config/version.config";

extern const int des_len;
extern const char *key;
int debug=0;
static int down_from=1;// 1:ftp_server 2:sdcard
static const char *prog="update";
static const char *version="2.33";
static const char *send_pos_file="send_log.pos";
static char bat_buffer[100*1024];
static int bat_offs=0;

static void proclog(const char *fmt,...)
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
	
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%F %X",(const struct tm *)localtime(&tt));
	

	int fd;

	//print screen
	
	/*
	sprintf(buf,GREEN"[%s]"NONE YELLOW"[%s]"NONE"%s",ts,version,tmp);
	fd=open("/dev/tty1", O_WRONLY|O_APPEND);
	if(fd <0)
	{
		printf("open /dev/tty1 failed!%s\n",strerror(errno));
	}
	flock(fd,LOCK_EX);

	write(fd,buf,strlen(buf));
	flock(fd,LOCK_UN);
	close(fd);
	*/

	//log content
	sprintf(buf,"%s\t%s\t%s\t%s",ts,prog,version,tmp);
	printf("%s",buf);
	
	//get log file name
	char filename[128];
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.sys",prog_argu[debug].log_dir,ts,get_box_id(box_id));

	//open file and write
	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600); 
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	//T_DES(1,key,des_len,buf,buf);
	flock(fd,LOCK_EX);
	//write(fd,buf,sizeof(buf));
	write(fd,buf,strlen(buf));
	flock(fd,LOCK_UN);
	close(fd);
}

static int ftp_download(const char *filename,const char *server_dir)
{
	//download
	
	char gzfile[128];
	sprintf(gzfile,"%s.tar.gz",filename);
	char cmd[256];
	int try_count=0;
	char tmp[128];

dl:
	if(++try_count > 3)
		return -1;
	if(down_from==1)
	{
		FILE *fp;
	//	printf("downloading %s from server\n",filename);
		
		sprintf(cmd,"./ftp_download %s %s %s %s %s 2>&1",prog_argu[debug].server_info.ip,prog_argu[debug].server_info.username,prog_argu[debug].server_info.password,server_dir,gzfile);
		proclog("%s\n",cmd);
		
		if((fp = popen(cmd,"r")) == NULL)
		{
			proclog("ERR:Fail to execute:%s\n",cmd);
			pclose(fp);
			return -1;
		}
		char buffer[128];
		if(fgets(buffer, sizeof(buffer)-1, fp)!=NULL)
		{
			alarm(0);
			proclog("ERR,FTPSERVER return:%s",buffer);
			pclose(fp);
			return -1;
		}
		pclose(fp);
	}
	else if(down_from==2)
	{
		char sdcard_file[128];
		sprintf(sdcard_file,"/sdcard/download/%s/%s",server_dir,gzfile);
		sprintf(cmd,"cp %s ../tmp/",sdcard_file);
		proclog("%s\n",cmd);
		system(cmd);
	}
	else
	{
		proclog("don't know where to download from,return\n");
		return -1;
	}

	//proclog("%s download successfully!\n",filename);

	


	sprintf(tmp,"../tmp/%s",gzfile);
	if( !is_file_exist(tmp) )
	{
		proclog("download %s failed ,try again...\n",gzfile);
		goto dl;
	}
	sprintf(cmd,"tar zxvf ../tmp/%s -C ../tmp",gzfile);
	proclog("%s\n",cmd);
	system(cmd);


	return 0;



}
static create_running_file()
{
	char filename[]="../disp/update.run";
	int fd = open(filename, O_CREAT|O_TRUNC|O_WRONLY,0600);
	close(fd);
}
static set_finished_flag()
{
	char filename[]="../disp/update.run";
	int fd=0;
	fd=open(filename,O_RDWR);
	write(fd,"1",1);
	close(fd);
}
static check_crc(char *filename,char *official_crc)
{
	//check whether download completely
	
	int filesize;
	char *buffer;
	char crc_value[32];
	int fd;

	char _filename[128];
	sprintf(_filename,"../tmp/%s",filename);
	sprintf(crc_value,"%X",get_file_crc(_filename));
	
	if(!strcmp(crc_value,official_crc))
	{
		proclog("%s download successfully!\n",filename);
		return 0;
	}
	else
	{
		proclog("ERR:%s crc error[%s][%s]!\n",filename,crc_value,official_crc);
		return -1;
	}
	
}

static fetch(char *filename,char *server_dir,char *official_crc)
{
	char cmd[128];
	int try_count=0;
download:
	if(try_count++>3)
		return -1;
	if(ftp_download(filename,server_dir)==-1)
	{
		goto download;
	}

	if(check_crc(filename,official_crc) )
	{
		goto download;
	}
	else
	{
		return 0;
	}
	
}
/*
static char* get_local_version(char *name,char *local_version)
{
	int i=0;
	while(1)
	{
		if(!prog_argu[debug].file_info_local[i].filename[0])
		{
			return NULL;
		}
		if(!strcmp(prog_argu[debug].file_info_local[i].filename,name))
		{
			strcpy(local_version,prog_argu[debug].file_info_local[i].version);
			//return prog_argu[debug].file_info_local[i].version;
			return local_version;
		}
		i++;
	}
}
*/
static char* get_local_crc(char *name,char *local_crc)
{
	int i=0;
	while(1)
	{
		if(!prog_argu[debug].file_info_local[i].filename[0])
		{
			return NULL;
		}
		if(!strcmp(prog_argu[debug].file_info_local[i].filename,name))
		{
			strcpy(local_crc,prog_argu[debug].file_info_local[i].crc);
			//return prog_argu[debug].file_info_local[i].version;
			return local_crc;
		}
		i++;
	}
}
static is_new(int i)
{
	char local_crc[64];
	memset(local_crc,0,sizeof(local_crc));
	get_local_crc(prog_argu[debug].file_info_server[i].filename,local_crc);
	//proclog("localcrc:[%s]-servercrc:[%s]\n",local_crc,prog_argu[debug].file_info_server[i].crc);
	if(strcmp(prog_argu[debug].file_info_server[i].crc ,local_crc))
	{
		return 1;
	}
	else
	{
		//proclog("local:%s-server:%s\n",local_version,prog_argu[debug].file_info_server[i].version);
		return 0;
	}

}
static change_update_version()//if all files updated successfully,then change this version number
{

	char model_config_version[32]={0};
	char model_config_version_file[]="../disp/model.config.version";
	int fd1;
	char model_config_file[]="../config/model.config";
	fd1=open(model_config_file, 0);
	if(fd1<0)
	{
		printf("failed to open %s,%s\n",model_config_file,strerror(errno));
		return -1;
	}
	//proclog("%s:\n",config_file);
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
		//proclog("buffer:%s\n",buffer);

		if(!strncmp(buffer,"@version",8))
		{
			read(fd1,buffer,sizeof(buffer));
			T_DES(0,key,des_len,buffer,buffer);
			strcpy(model_config_version,buffer);
			break;
		}



		//sleep(1);
	}

	close(fd1);


	//change model_config's version
	if(strlen(model_config_version)>2)
		write_version(model_config_version_file, model_config_version, strlen(model_config_version));



}
static fetch_all_files()
{
	int i=0;
	char cmd[512];
	char destfile[128];
	char buffer[des_len];
	time_t tt;

	int updated_num=0;
	int will_update_num=0;
	int fd;

	fd=open(prog_argu[debug].version_config_local, O_CREAT|O_WRONLY|O_APPEND|O_TRUNC,0600);



	//count how many files need to be updated

	i=0;
	while(1)
	{
		if(!prog_argu[debug].file_info_server[i].filename[0])
			break;
		if(!strcmp(prog_argu[debug].file_info_server[i].version,"0"))
		{
			;
		}
		else if(is_new(i))
		{
			will_update_num++;
		}
		i++;
	}
	
	prt_screen(1, 1, 0,"共有%d个文件需更新",will_update_num);
	proclog("excepting update num: %d\n", will_update_num);
	//begin to update
	prt_screen(0, 1, 0,"正在更新");
	i=0;
	while(1)
	{
		if(!prog_argu[debug].file_info_server[i].filename[0])
			break;

		sprintf(destfile,"../%s/%s",prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_server[i].filename);

		memset(buffer,0,sizeof(buffer));
		//delte file
		if(!strcmp(prog_argu[debug].file_info_server[i].version,"0"))
		{
			unlink(destfile);
			i++;
			continue;
			
		}
		else if(is_new(i))
		{
			if(fetch(prog_argu[debug].file_info_server[i].filename, prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].crc)==0)
			{
				prt_screen(1, 1, 0,".");
				unlink(destfile);
				sprintf(cmd,"chmod +x ../tmp/%s",prog_argu[debug].file_info_server[i].filename);
				//proclog("%s\n",cmd);
				system(cmd);
				
				sprintf(cmd,"cp ../tmp/%s ../%s",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].local_dir);
				//proclog("%s\n",cmd);
				system(cmd);
				sprintf(buffer,"%s %s %s %s %s\n",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_server[i].version,prog_argu[debug].file_info_server[i].crc);

				updated_num++;
				
				proclog("updated:%s",buffer);
			}
			else
			{
				proclog("download %s failed!,quiting\n",prog_argu[debug].file_info_server[i].filename);
				sprintf(buffer,"%s %s %s %s %s\n",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_local[i].version,prog_argu[debug].file_info_server[i].crc);
			}
			
			
		}
		else//dont need update
		{
			sprintf(buffer,"%s %s %s %s %s\n",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_local[i].version,prog_argu[debug].file_info_server[i].crc);
		}

		T_DES(1,key,des_len,buffer,buffer);
		write(fd,buffer,sizeof(buffer));
		i++;
		
	}
	close(fd);


	
	proclog("really updated num:%d\n",updated_num);

	if(updated_num ==  will_update_num)//all files updated successfully
	{
		if(updated_num)
			proclog("all %d files updated successfully!, update model.config...\n",updated_num);
		change_update_version();
	}
	sleep(2);
	

}
static read_server_config(char *config_name)
{
	read_app_config(config_name,"ip",prog_argu[debug].server_info.ip);
	read_app_config(config_name,"username",prog_argu[debug].server_info.username);
	read_app_config(config_name,"password",prog_argu[debug].server_info.password);

	read_app_config(prog_argu[debug].app_config,"tcp_server_ip",prog_argu[debug].server_info.tcp_server_ip);
	read_app_config(prog_argu[debug].app_config,"tcp_server_port",prog_argu[debug].server_info.tcp_server_port);

	
	proclog("config: ip:%s usr:%s pwd:%s\n",prog_argu[debug].server_info.ip,prog_argu[debug].server_info.username,prog_argu[debug].server_info.password);
}
static get_file_info(const char *config_file,FILE_INFO file_info[])// infomation of version.config files
{

	/*
	char buffer[1024];
	FILE *fp;
	char *p=NULL;

	decrypt_file_line(config_file);
	char config_file_decrypt[128];
	sprintf(config_file_decrypt,"%s.decrypt",config_file);
	
	fp=fopen(config_file_decrypt,"r");
	if(fp==NULL)
	{
		proclog("open %s failed!\n",config_file);
		exit(0);
	}
	int i=0;

	proclog("%s:\n",config_file);
	while(fgets(buffer, sizeof(buffer)-1, fp))
	{
		//printf("%s",buffer);
		p=strtok(trim(buffer)," ");
		if(p!=NULL)
			strcpy(prog_argu[debug].file_info_local[i].filename,trim(p));

		p=strtok(NULL," ");
		if(p!=NULL)
			strcpy(prog_argu[debug].file_info_local[i].local_dir,trim(p));
	
		p=strtok(NULL," ");
		if(p!=NULL)
			prog_argu[debug].file_info_local[i].version=atoi(p);

		p=strtok(NULL," ");
		if(p!=NULL)
			strcpy(prog_argu[debug].file_info_local[i].crc,trim(p));

		proclog("%d:%s %s %d %s\n",i,prog_argu[debug].file_info_local[i].filename,prog_argu[debug].file_info_local[i].local_dir,prog_argu[debug].file_info_local[i].version,prog_argu[debug].file_info_local[i].crc);

		i++;
	}


	fclose(fp);
	unlink(config_file_decrypt);
	*/
	
	
	int fd1;
	fd1=open(config_file, 0);
	if(fd1<0)
	{
		printf("failed to open %s,%s\n",config_file,strerror(errno));
		return -1;	
	}
	//proclog("%s:\n",config_file);
	char buffer[des_len];
	int n;
	char *p=NULL;

	int i=0;
	
	while(1)
	{
		memset(buffer,0,sizeof(buffer));
		n=read(fd1,buffer,sizeof(buffer));
		//printf("n:%d\n",n);
		if(!n)
			break;
		T_DES(0,key,des_len,buffer,buffer);

		p=strtok(trim(buffer)," ");
		if(p!=NULL)
			strcpy(file_info[i].filename,trim(p));

		p=strtok(NULL," ");
		if(p!=NULL)
			strcpy(file_info[i].server_dir,trim(p));

		p=strtok(NULL," ");
		if(p!=NULL)
			strcpy(file_info[i].local_dir,trim(p));
	
		p=strtok(NULL," ");
		if(p!=NULL)
			//file_info[i].version=atoi(p);
			strcpy(file_info[i].version,p);

		p=strtok(NULL," ");
		if(p!=NULL)
			strcpy(file_info[i].crc,trim(p));

		//proclog("%d:%s %s %s %s %s\n",i,file_info[i].filename,file_info[i].server_dir,file_info[i].local_dir,file_info[i].version,file_info[i].crc);

		i++;

		
		//sleep(1);
	}




	close(fd1);
	
}

static download_config()
{
	int fd;
	char box_id[32];
	get_box_id(box_id);
	char filename[128];
	char cmd[128];


	prt_screen(1, 1,0,"正在获取配置文件...\n");


	//download version.config.boxid
	sprintf(filename,"version.config.%s",box_id);
	ftp_download(filename,"config/");

	char _filename[128];
	sprintf(_filename,"../tmp/%s",filename);
	fd=open(_filename,0);
	if(fd >0)
	{
		close(fd);

		sprintf(cmd,"mv %s ../tmp/version.config",_filename);
		proclog("%s\n",cmd);
		system(cmd);
		return 0;
	}

	//download version.config(default)
	strcpy(filename,"version.config");
	ftp_download(filename,"config/");
	sprintf(_filename,"../tmp/%s",filename);
	fd=open(_filename,0);
	if(fd<0)
	{
		char str[128];
		proclog("failed to download %s!\n",filename);
		prt_screen(2, 0,0,"下载配置文件失败!\n");
		return -1;
		
	}
	return 0;

	
}
static void procquit(void)
{
	system("/sbin/hwclock -w");
	proclog("quiting...\n");
	set_finished_flag();
	sleep(2);
}

static ntpupdate(int flag)
{
	//flag 1:if right time is not update ,never return 2:doesn't matter
		
	prt_screen(1, 1,0, "正在更新系统时钟，请保持网络畅通......\n");
	char ntpserver[32];
	char cmd[32];
	read_app_config(prog_argu[debug].app_config,"ntpserver",ntpserver);
	sprintf(cmd,"ntpd -p %s",ntpserver);
	printf("%s\n",cmd);
	system(cmd);

	if(flag==1)
	{
		time_t new_sys_clock;
		time_t old_sys_clock;
		old_sys_clock=time(0);
		
		while(1)//loop until get the right time
		{
			printf("waiting for time update...\n");		
			sleep(3);
			new_sys_clock=time(0);
			if(new_sys_clock-old_sys_clock> 3600*24*3)//i think this means clock update successfully
			{
				printf("time updated successfully! %u-->%u\n",old_sys_clock,new_sys_clock);
				break;
			}
		}
	}
	else
	{
		sleep(15);
	}
	system("/sbin/hwclock -w");

}
static check_connection()
{
	prt_screen(1, 0,0, "正在检查网络状态...     ");
	proclog("checking network %s...\n",prog_argu[debug].server_info.ip);
	if(!is_online(prog_argu[debug].server_info.ip,21))
	{
		prt_screen(1, 1,0, "\n");
		proclog("network is not available!\n");
		//prt_screen(2, 1, 0,"网络不可用，更新失败!\n");
		return -1;
	}
	else
	{
		prt_screen(1, 1,0, "正常!\n");
		proclog("netowrk is available!\n");
		return 0;
	}
	
}
update()
{
	system("rm -rf ../tmp/*");
	proclog("update starting...\n");
	
	read_server_config(prog_argu[debug].app_config);
	if(check_connection()==0)
	{
		down_from=1;
		proclog("download from ftp...\n");
		prt_screen(1, 0,0, "正在从服务器下载...\n");
	}
	else if(sdcard_exists())
	{
		down_from=2;
		proclog("download from sdcard...\n");
		prt_screen(1, 0,0, "正在从SD卡下载...\n");
	}
	else
	{
		proclog("neither ftp nor sdcard is available\n");
		prt_screen(2, 1,0, "网络和SD卡都不可用，更新失败!\n");
		return;	
	}
	
	sleep(5);
	
	if(download_config()==-1)
		return ;
	sleep(5);
	//memset(prog_argu[debug].file_info_local,0,sizeof(FILE_INFO)*UPD_LST_MAX_NUM);
	//memset(prog_argu[debug].file_info_server,0,sizeof(FILE_INFO)*UPD_LST_MAX_NUM);
	get_file_info(prog_argu[debug].version_config_local,prog_argu[debug].file_info_local);
	get_file_info("../tmp/version.config",prog_argu[debug].file_info_server);

	fetch_all_files();


}
static void acalarm(int signo)
{
	proclog("sigalarm,time out!\n");
}




/*********for tcp upload************/
static short writeall(int sd,char *buf,int num)
{
int j;
lp:
if((j=write(sd,buf,num))!=num)
   if(j==-1)
     if(errno==EWOULDBLOCK || errno==EAGAIN)
      goto lp;
     else
      return(-1);
   else
     { 
       num-=j;
       buf+=j;       
       goto lp;
     }
return 0;
}

static int send_real(int sockfd,char *filename,int send_pos,int posfd)
{
	char buffer[256];
	*(int*)(bat_buffer)=htonl(bat_offs);
	alarm(20);
	if(writeall(sockfd,bat_buffer,bat_offs+sizeof(int))<0)
	{
		proclog("ERR:write all failed times, %s\n",strerror(errno));
		return -1;
	}
	//read response
	recv(sockfd,buffer,4,MSG_WAITALL);
	alarm(0);
	int resp=ntohl(*(int*)(buffer));
	printf("sending %d bytes,pos %d,resp:0x%x\n",bat_offs,send_pos,resp);

	if(resp!=0x4F500000)
	{
		proclog("server response failed:%x\n",resp);
		return -1;
	}

	

	//write pos file
	sprintf(buffer,"%s:%d",filename,send_pos);
	//printf("pos:%s\n",buffer);
	lseek(posfd,0,SEEK_SET);
	ftruncate(posfd,0);
	write(posfd,buffer,strlen(buffer));

	prt_screen(1, 1, 0, ".");

	bat_offs=0;
	return 0;
}
static int send_log_tcp(char *filename, int send_pos, int posfd,int sockfd)
{
	int fd=0;
	char buffer[1024];
	char cmd[128];
	int try_count=0;
	
	char tmp[256];

	printf("===file:%s,pos:%d===\n",filename,send_pos);

	//get extend name
	char *p=strrchr(filename,'.');
	char ext_name[12];
	strcpy(ext_name,p+1);
	strcat(ext_name,"\t");

	

	//open file
	if(( fd = open(filename,0)) < 0)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to open:%s,%s\n",filename,strerror(errno));
		return -1;
	}

	/*
	//get file size
	int filesize = lseek(fd,0,SEEK_CUR);
	if(filesize < 0)
	{
		proclog("get filesize failed,%s\n",strerror(errno));
		return -1;
	}
	*/
	//set file postion

	if(lseek(fd,send_pos,SEEK_SET)<0)
	{
		proclog("ERR:Fail to fseek of file %s,pos %d,%s\n",filename,send_pos,strerror(errno));
		return -1;
	}
	
	//send file line by line
	int n=0;
	int read_count=0;
	int send_count=0;
	int send_len=0;
	while(1)
	{
		read_count=0;
		send_count=0;
		
	read_data:
		if(++read_count >= 3)
				return -1;
		n=read(fd,buffer,des_len);
		if(n<0)//error
		{
			proclog("ERR:read failed %d times",read_count);
			goto read_data;
			
			
		}
		else if(n==0)//finished reading
		{
			if(bat_offs)
				if(send_real(sockfd, filename, send_pos,posfd)<0)
					return -1;

			
			//sprintf(cmd, "mv %s %s",filename,prog_argu[debug].logbak_dir);
			sprintf(cmd, "cat %s >> %s/%s  && rm %s",filename,prog_argu[debug].logbak_dir,filename,filename);
			printf("%s\n",cmd);
			system(cmd);
			ftruncate(posfd,0);
			printf("finished sending %s\n",filename);
			break;
	
		}
		else if(n==des_len)//get data
		{
			//get pos
			send_pos=lseek(fd,0,SEEK_CUR);
			
			//decrypt
			T_DES(0,key,des_len,buffer,tmp);

			strcpy(buffer,ext_name);
			strcat(buffer,tmp);

			
			memcpy(bat_buffer+sizeof(int)+bat_offs,buffer,strlen(buffer));
			bat_offs+=strlen(buffer);
			if(bat_offs > sizeof(bat_buffer)-1024)
				if(send_real(sockfd, filename, send_pos,posfd)<0)
					return -1;
		

			
		}
		else
		{
			proclog("read return [%d] bytes,%s",n,strerror(errno));
			return -1;
		}
		
	}
	close(fd);
	return 0;
}
static int connect_to_server()
{

	//connect to server
	int sockfd;
	struct sockaddr_in servaddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr.sin_zero, 8);
	servaddr.sin_family = AF_INET;
	struct  hostent *he;
	he=gethostbyname(prog_argu[debug].server_info.tcp_server_ip);
	servaddr.sin_addr.s_addr=*(unsigned long *)he->h_addr; 
	servaddr.sin_port = htons(atoi(prog_argu[debug].server_info.tcp_server_port));

	int n=-1;
	int ret=0;
	alarm(5);
	n=connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	alarm(0);
	if(n<0)
	{
		proclog("[%s|%s],connected return[%d]%s\n",prog_argu[debug].server_info.tcp_server_ip,prog_argu[debug].server_info.tcp_server_port,n,strerror(errno));
		close(sockfd);
		prt_screen(2, 0,0,"连接服务器失败!\n");
		return -1;
	}
	else
	{
		proclog("connected to [%s|%s] %s\n",prog_argu[debug].server_info.tcp_server_ip,prog_argu[debug].server_info.tcp_server_port,strerror(errno));
		return sockfd;
	}
}
static int server_login(int sockfd)
{
	char buffer[128];

	//
	*(int*)(buffer)=htonl(0x000D2F54);

	//box_id
	char box_id[32];
	get_box_id(box_id);
	proclog("boxid:%s\n",box_id);
	strcpy(buffer+4,box_id);

	//gzip or not
	*(int*)(buffer+20)=0;

	alarm(10);
	if(writeall(sockfd,buffer,24)<0)
	{
		proclog("login to [%s:%s] %s\n",prog_argu[debug].server_info.tcp_server_ip,prog_argu[debug].server_info.tcp_server_port,strerror(errno));
		return -1;
		
	}
	
	recv(sockfd,buffer,4,MSG_WAITALL);
	alarm(0);
	
	int resp=ntohl(*(int*)(buffer));
	if(resp==0x4C4E0000)
	{
		proclog("login to [%s:%s] %s\n",prog_argu[debug].server_info.tcp_server_ip,prog_argu[debug].server_info.tcp_server_port,strerror(errno));
		return 0;
	}
	else
	{
		proclog("login failed ,return %x\n", resp);
		prt_screen(2, 1,0,"服务器返回错误代码:%x\n",resp);
		return -1;
	}

}
static upload_log_tcp()
{

	prt_screen(1, 0, 0,"正在准备上传日志(tcp)...\n"); 
	//prt_screen(2,0,0,"");
	char cmd[256];
	DIR * dp;
	char upload_file[128];
	int upload_file_num=0;
	struct dirent *name;
	char path[256];
	char logfilename[128];
	char buf[256];
	int send_pos=0;
	int sockfd;
	
	

	//printf("debug:%d\n",debug);
	
	read_app_config(prog_argu[debug].app_config,"tcp_server_ip",prog_argu[debug].server_info.tcp_server_ip);
	if(strlen(prog_argu[debug].server_info.tcp_server_ip) <= 0 )
	{
		prt_screen(2,0,0,"读取配置文件错误！");
		return;
	}
	read_app_config(prog_argu[debug].app_config,"tcp_server_port",prog_argu[debug].server_info.tcp_server_port);
	if(strlen(prog_argu[debug].server_info.tcp_server_port) <= 0 )
	{
 		prt_screen(2,0,0,"读取配置文件错误！");
		return;
	}

		

	getcwd(path,sizeof(path));
	chdir(prog_argu[debug].log_dir);

	//connect to server
	prt_screen(1, 1,0,"正在连接服务器...");
	sockfd = connect_to_server();
	if(sockfd<0)
		return;

	//create session
	if(server_login(sockfd)<0)
		return;
	prt_screen(1, 1,0,"	成功!\n");
	//open pos file 
	
	prt_screen(1, 1,0,"开始上传日志 ");
	int posfd;
	int n;
	posfd=open(send_pos_file, O_CREAT|O_RDWR,0600); 
	if(posfd <0)
	{
		proclog("open %s failed!%s\n",send_pos_file,strerror(errno));
		return;
	}

	//get postion
	n=read(posfd,buf,sizeof(buf));
	if(n&&strstr(buf,":"))//file is just created
	{
		strcpy(logfilename,(char *)strtok(buf,":"));
		send_pos=atoi(strtok(NULL,":"));
		//printf("start from %s:%d\n",logfilename,send_pos);
		if(send_log_tcp(logfilename,send_pos,posfd,sockfd)<0)
			return -1;
		upload_file_num++;
	}
	//	
	dp = opendir(prog_argu[debug].log_dir);
	if(!dp)
	{
		return -1;
	}
	while(name = readdir(dp))
	{

		if(!strstr(name->d_name,"day.record"))
			continue;
		if(send_log_tcp(name->d_name,0,posfd,sockfd)<0)
			return -1;
		upload_file_num++;
		
	}
	
	closedir(dp);
	close(posfd);
	shutdown(sockfd,2);
	close(sockfd);
	chdir(path);
	if(upload_file_num)
		prt_screen(1, 1, 0, "\n上传日志成功!共%d个文件\n", upload_file_num);
	else
		prt_screen(1, 1, 0, "\n暂无日志文件上传!");

}

static get_local_ip()
{
	//ifconfig |sed -n '2p'|awk -F'addr:' '{print $2}'|awk '{print $1}'
	FILE *fp;
	char buffer[200] = {0};
	int try_count=0;
	char ip[32];

	char cmd[512];
	strcpy(cmd,"/sbin/ifconfig |/bin/sed -n '2p'|/usr/bin/awk -F'addr:' '{print $2}'|awk '{print $1}' ");
	printf("%s\n",cmd);
get_ip:
	if(try_count++>10)
	{
		printf("ERR:Fail to get ip1\n");
		strcpy(ip,"ip error");
		goto get_ip_end;
	}
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		printf("ERR:Fail to execute:%s\n",cmd);
		strcpy(ip,"ip error");
		goto get_ip_end;
	}


	if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
	{
		proclog("ERR:Fail to get ip:%s\n",cmd);
		strcpy(ip,"ip error");
		goto get_ip_end;
	}
	//proclog("buffer:%s\n",buffer);
	pclose(fp);
	strcpy(ip,trim(buffer));
	if(strlen(ip)<6)
	{
		sleep(1);
		goto get_ip;
	}
get_ip_end:
	printf("ipaddr:%s\n",ip);


	int fd;
	fd=open("../disp/ip", O_CREAT|O_TRUNC|O_WRONLY,0600);
	write(fd,ip,strlen(ip));
	close(fd);
}
/*
static get_local_ip(char *ip)
{
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;
	int try_count=0;
get_ip:
	if(try_count++>10)
	{
		proclog("ERR:Fail to get ip1\n");
		strcpy(ip,"ip error");
		return;
	}
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		proclog("ERR:Fail to get ip2\n");
		strcpy(ip,"ip error");
		return;
	}

	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;

	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		proclog("ERR:Fail to get ip3\n");
		strcpy(ip,"ip error");
		return;
	}

	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	//fprintf(stdout, "eth0: %s\n", inet_ntoa(sin.sin_addr));
	strcpy(ip,inet_ntoa(sin.sin_addr));
	if(strlen(ip)<6)
	{
		close(sock);
		sleep(1);
		goto get_ip;
	}
	proclog("ipaddr:%s\n",ip);
	//return 0;
}
*/

static reset_mac()
{
	
	//generate a new mac address
	char *mac_file="../disp/mac";
	char mac[128];
	if(!is_file_exist(mac_file))
	{

		memset(mac, 0, sizeof(mac));
	
		strcpy(mac, "08:90:00:");

		struct timeval tv;
		gettimeofday(&tv,NULL);
		char tmp[12];
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%06u",tv.tv_usec);
	
		char *p=tmp;

		strncat(mac,p,2);
		p+=2;
		strcat(mac,":");

		strncat(mac,p,2);
		p+=2;
		strcat(mac,":");

		strncat(mac,p,2);

		printf("new mac:%s\n", mac);



		//write mac to disp file
		int fd;
		fd=open(mac_file, O_CREAT|O_TRUNC|O_WRONLY,0600);
		write(fd,mac,strlen(mac));
		close(fd);
	}
	else
	{
		int fd;
		char buf[64];
		fd=open(mac_file, 0);
		read(fd,buf,sizeof(buf));
		strcpy(mac,trim(buf));
		close(fd);
	}
	//set mac
	
	char cmd[128];
	
	strcpy(cmd, "ifconfig eth0 down");
	printf("%s\n",cmd);
	system(cmd);
	
	sprintf(cmd, "ifconfig eth0 hw ether %s",mac);
	printf("%s\n",cmd);
	system(cmd);
	
	strcpy(cmd, "ifconfig eth0 up");
	printf("%s\n",cmd);
	system(cmd);
	

	//restart dhcp
	system("killall dhcpcd");
	sleep(1);
	system("dhcpcd -LK -d eth0");
	
	//sleep(15);

	//get_local_ip();


}
static delete_old_logs()
{
	
	char cmd[512];
	
	sprintf(cmd,"rm -rf ../logbak2");
	proclog("%s\n",cmd);
	system(cmd);
	
	debug=1;
	sprintf(cmd,"rm `find %s -mtime +21`",prog_argu[debug].logbak_dir);
	proclog("%s\n",cmd);
	system(cmd);
	
	sprintf(cmd,"rm `find %s -mtime +3|grep day.sys`",prog_argu[debug].log_dir);
	proclog("%s\n",cmd);
	system(cmd);



	debug=0;
	sprintf(cmd,"rm `find %s -mtime +21`",prog_argu[debug].logbak_dir);
	proclog("%s\n",cmd);
	system(cmd);
	
	sprintf(cmd,"rm `find %s -mtime +3|grep day.sys`",prog_argu[debug].log_dir);
	proclog("%s\n",cmd);
	system(cmd);

	
}
static copy_day_logs()
{
	char sdcard_day_dir[]="/sdcard/daylogs";
	char cmd[512];
	if(sdcard_exists())
	{
		sprintf(cmd,"mkdir -p /sdcard/daylogs");
		proclog("%s\n",cmd);
		system(cmd);

		sprintf(cmd,"cp %s/*day* %s",prog_argu[debug].log_dir,sdcard_day_dir);
		proclog("%s\n",cmd);
		system(cmd);

		sprintf(cmd,"cp %s/*day* %s",prog_argu[debug].logbak_dir,sdcard_day_dir);
		proclog("%s\n",cmd);
		system(cmd);

	}
	else
	{
		proclog("sdcard doesn't exist, skip copying day log!");
	}

}
//////////////////////////////////////////////
main(int argc,char **argv)
{
	write_version("../disp/vud",version,strlen(version));
	if(argc==2&&!strcmp(argv[1],"-v"))
	{
		printf("version:%s\n",version);
		exit(0);
	}
	

	//register quit function
	if(atexit(&procquit))
	{
	   printf("quit code can't install!");
	   exit(0);
	}



	create_running_file();
	

	struct sigaction signew;

	signew.sa_handler=acalarm;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags=0;
	sigaction(SIGALRM,&signew,0);

	//ignore signal SIGCHLD,avoid zombie
	signew.sa_handler=SIG_IGN;
	sigaction(SIGCHLD,&signew,0);

	debug=1;
	memset(&prog_argu[debug],0,sizeof(PROG_ARGU));
	strcpy(prog_argu[debug].apk_dir,"../.apk");
	strcpy(prog_argu[debug].config_dir,"../.config");
	strcpy(prog_argu[debug].app_config,"../.config/app.config");
	strcpy(prog_argu[debug].log_dir,"../.log");
	strcpy(prog_argu[debug].logbak_dir,"../.logbak");
	strcpy(prog_argu[debug].version_config_local,"../.config/version.config");

	debug=0;
	memset(&prog_argu[debug],0,sizeof(PROG_ARGU));
	strcpy(prog_argu[debug].apk_dir,"../apk");
	strcpy(prog_argu[debug].config_dir,"../config");
	strcpy(prog_argu[debug].app_config,"../config/app.config");
	strcpy(prog_argu[debug].log_dir,"../log");
	strcpy(prog_argu[debug].logbak_dir,"../logbak");
	strcpy(prog_argu[debug].version_config_local,"../config/version.config");
	
	ch_root_dir();

	prt_screen(1, 0,0,"正在启动更新程序...\n");
	

	//reset mac
	reset_mac();

	
	
	//when box_id doesn't exist, time update is necessary,other wise don't do anything
	if(is_file_exist("../disp/box_id"))
	{
		ntpupdate(2);
	}
	else
	{
		ntpupdate(1);
		set_box_id();
	}
	

	//delete old files
	delete_old_logs();
	
	//copy day logs when sdcard exists
	copy_day_logs();

	

	
	//debug
	debug=1;
	update();
	proclog("update finished!\n");

	//normal
	debug=0;
	update();
	proclog("update finished!\n");
	prt_screen(2, 0,0,"更新结束!\n");


	sleep(1);
	if(upload_log_tcp()<0)
		prt_screen(2, 0, 0, "日志上传失败!\n");

}	

