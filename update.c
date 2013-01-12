/****
version
1.12 resloved the problem of cannot set system clock,you have to sleep for a while after ntpd
1.13 write version number by -v arugments
1.14 updated function ntpupdate(); add "hwclock -w" to procquit 	2012-11-27
1.15 modified prompt words
1.16 print log to the same logfile (.log) with other models
	log don't encrypt
1.17 ignore signal SIGCHLD,avoid zombie
	
*****/


#include "com.h"

#define UPD_LST_MAX_NUM 300

typedef struct
{
	char ip[32];
	char username[32];
	char password[32];
	
}SERVER_INFO;

typedef struct
{
	char filename[128];
	int version;
	char local_dir[128];
	char server_dir[128];
	char crc[32];
	
}FILE_INFO;

typedef struct
{
	char apk_dir[128];
	char config_dir[128];
	char log_dir[128];
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
static const char *version="1.17";

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
	sprintf(buf,"[%s][%s][%s]:%s",ts,prog,version,tmp);
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
			proclog("Fail,FTPSERVER return:%s",buffer);
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
		return -1;
	}

	//proclog("%s download successfully!\n",filename);

	

	char tmp[128];
	sprintf(tmp,"../tmp/%s",gzfile);
	if(!is_file_exist(tmp))
		return -1;
	sprintf(cmd,"tar zxvf ../tmp/%s -C ../tmp",gzfile);
	proclog("%s\n",cmd);
	system(cmd);


	return 0;



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
		proclog("%s crc error[%s][%s]!\n",filename,crc_value,official_crc);
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
static int get_local_version(char *name)
{
	int i=0;
	while(1)
	{
		if(!prog_argu[debug].file_info_local[i].filename[0])
		{
			return 0;
		}
		if(!strcmp(prog_argu[debug].file_info_local[i].filename,name))
		{
			return prog_argu[debug].file_info_local[i].version;
		}
		i++;
	}
}
static is_new(int i)
{
	if(prog_argu[debug].file_info_server[i].version > get_local_version(prog_argu[debug].file_info_server[i].filename))
		return 1;
	else
		return 0;
}
static fetch_all_files()
{
	int i=0;
	char cmd[512];
	char destfile[128];
	char buffer[des_len];
	time_t tt;

	int fd;
	prt_screen(1, 1, 0,"正在更新安装程序和apk文件......\n");
	fd=open(prog_argu[debug].version_config_local, O_CREAT|O_WRONLY|O_APPEND|O_TRUNC,0600);

	
	while(1)
	{
		if(!prog_argu[debug].file_info_server[i].filename[0])
			break;

		sprintf(destfile,"../%s/%s",prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_server[i].filename);

		memset(buffer,0,sizeof(buffer));
		//delte file
		if(prog_argu[debug].file_info_server[i].version==0)
		{
			unlink(destfile);
			i++;
			continue;
			
		}
		else if(is_new(i))
		{
			if(fetch(prog_argu[debug].file_info_server[i].filename, prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].crc)==0)
			{
				unlink(destfile);
				sprintf(cmd,"chmod +x ../tmp/%s",prog_argu[debug].file_info_server[i].filename);
				proclog("%s\n",cmd);
				system(cmd);
				
				sprintf(cmd,"cp ../tmp/%s ../%s",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].local_dir);
				proclog("%s\n",cmd);
				system(cmd);
				sprintf(buffer,"%s %s %s %d %s\n",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_server[i].version,prog_argu[debug].file_info_server[i].crc);
			}
			else
			{
				//proclog("download %s failed!,quiting\n",prog_argu[debug].file_info_server[i].filename);
				sprintf(buffer,"%s %s %s %d %s\n",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_local[i].version,prog_argu[debug].file_info_server[i].crc);
			}
			
			
		}
		else//dont need update
		{
			sprintf(buffer,"%s %s %s %d %s\n",prog_argu[debug].file_info_server[i].filename,prog_argu[debug].file_info_server[i].server_dir,prog_argu[debug].file_info_server[i].local_dir,prog_argu[debug].file_info_local[i].version,prog_argu[debug].file_info_server[i].crc);
		}

		T_DES(1,key,des_len,buffer,buffer);
		write(fd,buffer,sizeof(buffer));
		i++;
		
	}
	close(fd);
	


	

}
static read_server_config(char *config_name)
{
	read_app_config(config_name,"ip",prog_argu[debug].server_info.ip);
	read_app_config(config_name,"username",prog_argu[debug].server_info.username);
	read_app_config(config_name,"password",prog_argu[debug].server_info.password);
	
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
	proclog("%s:\n",config_file);
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
			file_info[i].version=atoi(p);

		p=strtok(NULL," ");
		if(p!=NULL)
			strcpy(file_info[i].crc,trim(p));

		proclog("%d:%s %s %s %d %s\n",i,file_info[i].filename,file_info[i].server_dir,file_info[i].local_dir,file_info[i].version,file_info[i].crc);

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
				proclog("time updated successfully! %u-->%u\n",old_sys_clock,new_sys_clock);
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
	strcpy(prog_argu[debug].version_config_local,"../.config/version.config");

	debug=0;
	memset(&prog_argu[debug],0,sizeof(PROG_ARGU));
	strcpy(prog_argu[debug].apk_dir,"../apk");
	strcpy(prog_argu[debug].config_dir,"../config");
	strcpy(prog_argu[debug].app_config,"../config/app.config");
	strcpy(prog_argu[debug].log_dir,"../log");
	strcpy(prog_argu[debug].version_config_local,"../config/version.config");
	
	ch_root_dir();




	//when box_id doesn't exist, time update is necessary,other wise don't do anything
	if(is_file_exist("../disp/box_id"))
	{
		ntpupdate(2);
	}
	else
	{
		ntpupdate(1);
		char box_id[32];
		get_box_id(box_id);
	}
	
	prt_screen(1, 0,0,"正在启动更新程序...\n");
	
	//debug
	debug=1;
	update();
	proclog("update finished!\n");

	//normal
	debug=0;
	update();
	proclog("update finished!\n");
	prt_screen(1, 1,0,"更新结束!\n");

	
	
	
}	

