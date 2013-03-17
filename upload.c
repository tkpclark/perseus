#include "com.h"

/********************
1.11 resloved the problem of cannot set system clock,you have to sleep for a while after ntpd
1.12 write version number by -v arugments
1.20	pack old logs when turned on
1.21 print log to the same logfile (.log) with other models
	log don't encrypt
1.22 ignore signal SIGCHLD,avoid zombie
1.30 ftp -v ,jugde whether it's successful ,and not delete oold logs
1.30t move all logs from logbak to log,print verbose if ftp failed
1.31 drop function of "pack_old_logs" 		2013-03-04
1.32 "."--> "_" in need upload
1.33 do not upload day log any more
1.34 modify judgement of ftp upload
1.35 modify log format,splited with tab
1.36 mv copy_day_log_to_sdcard() from apk_install
1.37 I just guess,it's no diff with 1.36,but not sure
1.38 	check whether /dev/sdcard exists before copy day log 2013-03-16
	modify judement of whether sdcard exist(/sdcard-->/dev/sdcard),because 

*********************/

typedef struct
{
	char ip[32];
	char username[32];
	char password[32];

	
}SERVER_INFO;

typedef struct
{
	char log_dir[128];
	char app_config[128];
	char logbak_dir[128];
	SERVER_INFO server_info;
}PROG_ARGU;

PROG_ARGU prog_argu[2];
static const char *failed_upload_dir="/sdcard/log/";
static const char *send_pos_file="send_log.pos";


extern const int des_len;
extern const char *key;

int debug;
static const char *prog="upload";
static const char *version="1.38";

void proclog(const char *fmt,...)
{
	char ts[32];
	//char buf[des_len];
	char buf[1024];
	time_t tt;

	//char tmp[des_len];
	char tmp[1024];
	
	memset(tmp,0,sizeof(tmp));
	va_list vs;
	va_start(vs,fmt);
	vsprintf(tmp,fmt,vs);
	va_end(vs);
	
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%F %X",(const struct tm *)localtime(&tt));
	sprintf(buf,"%s\t%s\t%s\t%s",ts,prog,version,tmp);
	printf("%s",buf);
	
	//get log file name
	char filename[128];
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	get_box_id(box_id);
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.sys",prog_argu[debug].log_dir,ts,box_id);

	int fd;
	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600); 
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
	}
	//T_DES(1,key,des_len,buf,buf);
	flock(fd,LOCK_EX);
	//write(fd,buf,sizeof(buf));
	write(fd,buf,strlen(buf));
	flock(fd,LOCK_UN);
	close(fd);
}

static int ftp_upload(const char *filename)
{
	//download
//	printf("downloading %s from server\n",filename);
	if(!is_online(prog_argu[debug].server_info.ip,21))
	{
		prt_screen(3, 0, 1,"网络中断,上传失败!\n");
		return -1;
	}
	prt_screen(3, 0,1, "正在上传日志...\n");
	FILE *fp;
	char cmd[256];
	sprintf(cmd,"./ftp_upload %s %s %s %s %s 2>&1",prog_argu[debug].server_info.ip,prog_argu[debug].server_info.username,prog_argu[debug].server_info.password,filename,prog_argu[debug].log_dir);
	proclog("%s\n",cmd);
	if((fp = popen(cmd,"r")) == NULL)
	{
		proclog("ERR:Fail to execute:%s\n",cmd);
		pclose(fp);
		return -1;
	}
	char buffer[1024];
	fread(buffer , sizeof(buffer) , sizeof(char) , fp);



	//proclog("\n\n\n[%s]\n\n\n",buffer);

	if(strstr(buffer,"File receive OK"))
	{
	//	alarm(0);
		prt_screen(3, 0, 1,"上传日志成功!\n");
		proclog("%s upload successfully!\n",filename);
		pclose(fp);
		return 0;
	}
	else
	{
		prt_screen(3, 0, 1,"上传日志失败!\n");
		proclog("%s upload failed!\n",filename);
		proclog("\n\n\n[%s]\n\n\n",buffer);
		return -1;
	}

}
static int  need_upload(char *filename)
{
	char name[128];
	char *p;
	strcpy(name,filename);
	p=strrchr(name,'_');
	if(p==NULL)
		return 0;

	*p=0;

	char ts[32];
	time_t tt;
	tt=time(0);
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	

	if(strcmp(ts,name) >0)
		return 1;
	else
		return 0;
}
/*
static int  need_move(char *filename)
{
	char name[128];
	char *p;
	strcpy(name,filename);
	p=strrchr(name,'.');
	if(p==NULL)
		return 0;

	*p=0;

	char ts[32];
	time_t tt;
	tt=time(0)-3600*5;
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));


	if(strcmp(ts,name) >= 0)
		return 1;
	else
		return 0;
}
*/

static int tar_today()
{
	int fd;
	char day[16];
	char filename[128];
	sprintf(filename,"../logbak/%s.log.tar.gz",get_day(day));
	fd=open(filename,0);
	if(fd<0)
	{
		return 0;
	}
	else
	{
		close(fd);
		return 1;
	}
}

static short upload_log_ftp()
{

	char hour[8];
	char day[8];

	char cmd[256];
	DIR * dp;
	char upload_file[128];
	struct dirent *name;
	char path[256];
	char logfilename[128];
	
	//printf("debug:%d\n",debug);
	dp = opendir(prog_argu[debug].log_dir);

	if(!dp)
	{
		return -1;
	}
	char filename[128];
	int tarflag=0;//are there any files upload failed

	while(name = readdir(dp))
	{
		if(strlen(name->d_name)<3)
			continue;
		if(strstr(name->d_name,"day"))
			continue;
		//printf("%s\n",name->d_name);
		sprintf(upload_file,"%s/%s",prog_argu[debug].log_dir,name->d_name);
		
		if(need_upload(name->d_name))
		{
			
			if(!ftp_upload(name->d_name))
			{
				sprintf(cmd,"mv %s %s",upload_file,prog_argu[debug].logbak_dir);
				proclog("%s\n",cmd);
				system(cmd);
				
			}


		}

		
		
	}
	closedir(dp);

}

static read_server_config()
{
	read_app_config(prog_argu[debug].app_config,"ip",prog_argu[debug].server_info.ip);
	read_app_config(prog_argu[debug].app_config,"username",prog_argu[debug].server_info.username);
	read_app_config(prog_argu[debug].app_config,"password",prog_argu[debug].server_info.password);
	
	//proclog("downloading version.config from %s %s %s\n",prog_argu[debug].server_info.ip,prog_argu[debug].server_info.username,prog_argu[debug].server_info.password);
}
static void procquit(void)
{
	proclog("quiting...\n");
}

/*
static pack_old_logs()
{
	if(debug)//don't pack old logs when debug
		return;
	proclog("begin to check whether need to pack old logs...\n");
	if(is_online(prog_argu[debug].server_info.ip,21))
	{
		//netowrk is ok,don't need to pack
		proclog("network is ok ! don't need to pack.\n");
		return;
	}

	if(!sdcard_exists())
	{
		proclog("no sdcard ,don't pack.\n");
		return;
	}

	//if it's been packed already today,then return
	char gzfilepath[128];
	char day[32];
	get_day(day);
	printf("day:%s\n",day);
	sprintf(gzfilepath,"/sdcard/%s.log.tar.gz",day);
	
	if(is_file_exist(gzfilepath))
	{
		proclog("already packed,return\n");
		return;
	}

	
	char path[128];
	char cmd[128];
	
	
	getcwd(path,sizeof(path));
	chdir(prog_argu[debug].log_dir);
	sprintf(cmd,"tar zcvf %s.log.tar.gz *",day);
	proclog("%s\n",cmd);
	system(cmd);

	sprintf(cmd,"cp %s.log.tar.gz /sdcard/",day);
	proclog("%s\n",cmd);
	system(cmd);

	sprintf(cmd,"mv * %s",prog_argu[debug].logbak_dir);
	proclog("%s\n",cmd);
	system(cmd);

	chdir(path);
	
	
}
*/
static void acalarm(int signo)
{
	proclog("sigalarm,time out!\n");
}
static tmp_copy()
{
	char *tmp_copy="";
	if(is_file_exist("tmp.copy"))
		return;
	system("cp ../logbak/* ../log/ && touch tmp.copy");
}
static copy_day_log_to_sdcard()
{

	if(!sdcard_exists())
	{
		proclog("sdcard doesn't exist,do not copy,return\n");
		return;
	}
	char path[128];
	char cmd[512];
	char today[32];
	char yesterday[32];
	memset(today,0,sizeof(today));
	memset(yesterday,0,sizeof(yesterday));
	get_day(today);
	get_yesterday(yesterday);
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	memset(path,0,sizeof(path));

	getcwd(path,sizeof(path));
	chdir(prog_argu[debug].log_dir);
	
	sprintf(cmd,"tar zcvf %s_%s.day.tar.gz `ls *.day.sys *.day.record|grep -v %s`&& rm -f `ls *.day.sys *.day.record|grep -v %s` && mkdir -p /sdcard/daylog && mv *.day.tar.gz /sdcard/daylog/",yesterday,get_box_id(box_id),today,today);
	proclog("%s\n",cmd);
	system(cmd);


	chdir(path);
}

main(int argc,char **argv)
{

	write_version("../disp/vul",version,strlen(version));
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
	
	ch_root_dir();
	

	debug=1;
	strcpy(prog_argu[debug].app_config,"../.config/app.config");
	strcpy(prog_argu[debug].log_dir,"../.log");
	strcpy(prog_argu[debug].logbak_dir,"../.logbak");
	//pack_old_logs();
	read_server_config();
	
	debug=0;
	strcpy(prog_argu[debug].app_config,"../config/app.config");
	strcpy(prog_argu[debug].log_dir,"../log");
	strcpy(prog_argu[debug].logbak_dir,"../logbak");
	read_server_config();
	//pack_old_logs();

	//tmp_copy();
	copy_day_log_to_sdcard();
	
	debug=1;
	upload_log_ftp();
	debug=0;
	upload_log_ftp();
	
}

