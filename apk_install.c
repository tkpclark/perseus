#include "com.h"
/*
1.31 resloved the problem of cannot set system clock,you have to sleep for a while after ntpd
1.32 write version number by -v arugments
1.33 printf when proclog 2012-11-29
1.34 log don't encrypt, add pull_imei function
1.35 modify pull_mei(./adb shell run-as com.aisidi.AddShortcutFormPKN cat imei.aaa)
1.36 ignore signal SIGCHLD,avoid zombie
1.37 add popen time out
1.38 write log to sdcard
1.39 install_seq for phone
1.40 stop writing log to sdcard (it's function of 1.38)
	write day log (for copying to sdcard everyday   2013-02-24
1.41 modify the format of log 
	write log one time for all at the end 2013-03-04
1.42 modify hour log name,drop "hour" at the beginning
	rename day_log  2013-03-05
1.43 check_imei, 
	start count_seq after getting imei successfully
	2013-03-05
1.44 move function of copy_day_log_to_sdcard() to upload module
	2013-03-07
1.45 remove the function of 1.36 ,dont' ignore SIGCHLD
*/

//static const char *app_config="../config/app.config";
//static const char *apk_dir="../apk/";
//static const char *model_config="../config/model.config";
static const char *device_config_dir="/data/local/tmp/";
static const char *monitor_apk="../apk/add.apk";
static const char *monitor_apk_pkg="com.aisidi.AddShortcutFormPKN";
static const char *monitor_apk_pkg_init="com.aisidi.AddShortcutFormPKN/.InitActivity";
static const char *monitor_apk_pkg_setup="com.aisidi.AddShortcutFormPKN/.AddShortcutMainActivity";
static const char *monitor_apk_pkg_end="com.aisidi.AddShortcutFormPKN/.EndActivity";
static const char *adb="./adb";
static const char *prog="apk_install";
static const char *install_seq_file="../disp/install_seq";
static int install_seq=-1;
static const char *version="1.45";



	
extern const int des_len;
extern const char *key;
int debug;
//char apks[100][128];
typedef struct
{
	char name[128];
	int shortcut;
}APK;

typedef struct
{
	char id[32];//address
	char serialno[64];
	char imei[64];
	char model[64];
	char	manufacturer[64];
	char os_version[64];
	char config_name[32];
//	int apk_num;
}DEVICE_INFO;
DEVICE_INFO device_info;


typedef struct
{
	char apk_dir[128];
	char config_dir[128];
	char log_dir[128];
	char app_config[128];
	char model_config[128];
	int apk_num;
	APK apks[100];
	char sys_log_buffer[10240];
	char record_log_buffer[10240];
	int record_log_buffer_offs;
	
}PROG_ARGU;

PROG_ARGU prog_argu[2];

//char device_info.model[128];
//char apk_names[50][64];


/*
static void printscreen(const char *fmt,...)
{
	char ts[32];
	char buf[2048];
	time_t tt;

	char tmp[2048];
	memset(tmp,0,sizeof(tmp));
	va_list vs;
	va_start(vs,fmt);
	vsprintf(tmp,fmt,vs);
	va_end(vs);
	
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%x %X",(const struct tm *)localtime(&tt));
	sprintf(buf,GREEN"[%s]"NONE RED"[%s]"NONE YELLOW"[%s]"NONE"%s",ts,device_info.id,version,tmp);

	int fd;
	fd=open("/dev/tty1", O_WRONLY|O_APPEND);
	if(fd <0)
	{
		printf("open /dev/tty1 failed!%s\n",strerror(errno));
		return;
	}
	flock(fd,LOCK_EX);
	write(fd,buf,strlen(buf));
	flock(fd,LOCK_UN);
	close(fd);
}
*/
static char *last_modify_day(int fd,char *day)
{
        struct stat statbuf;
        fstat(fd,&statbuf);
        strftime(day,30,"%Y%m%d",(const struct tm *)localtime(&statbuf.st_mtime));
        //printf("%s\n",ts);
	return day;

}


static int count_seq()
{
        char buf[1024];
        int n=0;

        int ret=-1;
        int count=0;

   

        int fd;
        fd=open(install_seq_file, O_CREAT|O_RDWR|O_APPEND,0600);
        if(fd <0)
        {
         	//proclog("open %s failed!%s\n",install_seq_file,strerror(errno));
		return -2;
        }
        flock(fd,LOCK_EX);

	//if it's the first time to write the file today,clear it first
	char last_mday[32];
	char dtoday[32];

	memset(last_mday,0,sizeof(last_mday));
	memset(dtoday,0,sizeof(dtoday));
	
	if(strcmp(last_modify_day(fd,last_mday),(char*)get_day(dtoday)))
	{
		 ftruncate(fd,0);
	}


	//begin to read
        n=read(fd,buf,4);
        if(n<0)//it's an error
        {
                //printf("n:%d\n",n);
                ret=-1;
        }

	
        if(n==0)//file just be created
        {
                ftruncate(fd,0);
                write(fd,"1",1);
                ret=1;
        }
        else //n>0,most of the times
        {
                ftruncate(fd,0);
                count=atoi(buf)+1;
                sprintf(buf,"%d",count);
                write(fd,buf,strlen(buf));
                ret=count;
        }
        flock(fd,LOCK_UN);
        close(fd);

        return ret;
}
static void write_sys_log()
{	
	char ts[32];
	int fd;
	time_t tt;
	tt=time(0);

	
	//get log file name
	char filename[128];
	char box_id[32];
	memset(box_id,0,sizeof(box_id));


	/////write hour log //////
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.sys",prog_argu[debug].log_dir,ts,get_box_id(box_id));

	
	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600); 
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	//T_DES(1,key,des_len,buf,buf);
	flock(fd,LOCK_EX);	
	//write(fd,buf,sizeof(buf));
	write(fd,prog_argu[debug].sys_log_buffer,strlen(prog_argu[debug].sys_log_buffer));
	flock(fd,LOCK_UN);
	close(fd);


	//////////write day log for copying to  sdcard//////
	strftime(ts,30,"%Y%m%d",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.day.sys",prog_argu[debug].log_dir,ts,get_box_id(box_id));

	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600); 
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	//T_DES(1,key,des_len,buf,buf);
	flock(fd,LOCK_EX);	
	//write(fd,buf,sizeof(buf));
	write(fd,prog_argu[debug].sys_log_buffer,strlen(prog_argu[debug].sys_log_buffer));
	flock(fd,LOCK_UN);
	close(fd);
	////////////////////////////
	

}
static void proclog(const char *fmt,...)
{

	char buf[des_len];
	time_t tt;
	char ts[32];

	char tmp[des_len];
	memset(tmp,0,sizeof(tmp));
	va_list vs;
	va_start(vs,fmt);
	vsprintf(tmp,fmt,vs);
	va_end(vs);

//get log time
	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%F %X",(const struct tm *)localtime(&tt));
	


	
//print screen
/*
	sprintf(buf,GREEN"[%s]"NONE RED"[%s]"NONE YELLOW"[%s]"NONE"%s",ts,device_info.id,version,tmp);
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


	//print log	

	sprintf(buf,"%s\t%s\t%s\t%d\t%s",ts,prog,version,install_seq,tmp);
	printf("%s",buf);

	strcat(prog_argu[debug].sys_log_buffer,buf);


	
}

static void acalarm(int signo)
{
	proclog("sigalarm,time out!\n");
	//system("nohup ./chk.sh &");
}
static int check_imei(char *imei)
{
        char *p=NULL;
        p=imei;
        int len=0;
        while(*p)
        {
                if(!isdigit(*p))
                        return -1;
                len++;
                p++;

        }
        if(len!=15)
                return -1;

	proclog("SYS:imei:%s\n",imei);
        return 0;
}
static FILE * popen_time(char *buf,char * mod)
{
	FILE *fp;
	alarm(30);
//	proclog("set alarming ....\n");
	fp=popen(buf,mod);
	alarm(0);
	return fp;
}
static char *fgets_time(char *s, int n, FILE *stream)
{
	char *p=NULL;
	alarm(30);
	p=fgets(s, n, stream);
	alarm(0);
	return p;
	
}
static write_record_log()
{
	char ts[32];
	int fd;
	time_t tt;
	tt=time(0);
	//get log file name
	char filename[128];
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.record",prog_argu[debug].log_dir,ts,get_box_id(box_id));

	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600);         
	if(fd <0)
	{
		//printscreen("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	
	flock(fd,LOCK_EX);	
	write(fd,prog_argu[debug].record_log_buffer,prog_argu[debug].record_log_buffer_offs);
	flock(fd,LOCK_UN);
	close(fd);



	//////////write temp log to sdcard//////
	/*
	sprintf(filename,"/sdcard/tmplog/%s.record",get_box_id(box_id));
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
	*/
	//////////write day log for copying to  sdcard//////
	strftime(ts,30,"%Y%m%d",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.day.record",prog_argu[debug].log_dir,ts,get_box_id(box_id));
	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600); 
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	flock(fd,LOCK_EX);	
	write(fd,prog_argu[debug].record_log_buffer,prog_argu[debug].record_log_buffer_offs);
	flock(fd,LOCK_UN);
	close(fd);

	
}
static void record(char *apkname,char *result)
{
	char ts[32];
	char buf[des_len];
	time_t tt;

	tt=time(0);
	memset(buf,0,sizeof(buf));
	strftime(ts,30,"%F %X",(const struct tm *)localtime(&tt));
	sprintf(buf,"%s\t%s\t%s\t%s\t%s\t%s\t%s\n",ts,device_info.imei,device_info.manufacturer,device_info.model,device_info.os_version,apkname,result);
	T_DES(1,key,des_len,buf,buf);
	memcpy(prog_argu[debug].record_log_buffer+prog_argu[debug].record_log_buffer_offs,buf,des_len);
	prog_argu[debug].record_log_buffer_offs+=des_len;


	
	////////////////////////////
}


/*
static get_config_apks()
{
	//chdir(this_config_dir);
	FILE *fp;
	char *default_config="default";
	char _config_name[128];

	// get fp of config file
	sprintf(_config_name,"%s%s",this_config_dir,device_info.model);
	fp=fopen(_config_name,"r");
	if(fp==NULL)
	{
		sprintf(_config_name,"%s%s",this_config_dir,default_config);
		fp=fopen(_config_name,"r");
		if(fp==NULL)
		{
			//printscreen("open default config failed!\n");
			exit(0);
		}
		
	}
	strcpy(device_info.config_name,_config_name);
	//printscreen("config:%s\n",device_info.config_name);
	proclog("config:%s\n",device_info.config_name);
	//read config
	char buffer[256];
	
	fgets_time(buffer,sizeof(buffer),fp);
	device_info.apk_num=atoi(trim(buffer));

	int i;
	for(i=0;i<device_info.apk_num;i++)
	{
		if(fgets_time(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:config:%s num error\n",device_info.config_name);
			proclog("ERR:config:%s num error\n",device_info.config_name);
			exit(0);
		}
		strcpy(apks[i],trim(buffer));
		//printf("%d:%s\n",i+1,apks[i]);
	}

	fclose(fp);
	proclog("get apk_num:%d\n",device_info.apk_num);
	
}
*/

static get_config_apks_encrypt()
{
	int fd1;
	fd1=open(prog_argu[debug].model_config, 0);
	if(fd1<0)
	{
		printf("failed to open %s,%s\n",prog_argu[debug].model_config,strerror(errno));
		return -1;	
	}

	char buffer[des_len];
	int n;
	char *p=NULL;
	char model[128];

	
	//find model
	while(1)
	{
		memset(buffer,0,sizeof(buffer));
		n=read(fd1,buffer,sizeof(buffer));
		//printf("n:%d\n",n);
		if(!n)
			goto get_config_apks_encrypt_end;
		T_DES(0,key,des_len,buffer,buffer);

		trim(buffer);
		if(buffer[0]=='@')
		{
			strcpy(model,trim(buffer+1));
			
			if(!strcmp(model,"default") || !strcmp(model,device_info.model))
			{
				//strcpy(device_info.model,model);
				break;
			}
		}	

	}

	//get apks of this model
	while(1)
	{
		memset(buffer,0,sizeof(buffer));
		n=read(fd1,buffer,sizeof(buffer));
		//printf("n:%d\n",n);
		if(!n)
			goto get_config_apks_encrypt_end;
		T_DES(0,key,des_len,buffer,buffer);

		trim(buffer);
		if(buffer[0]=='@' ||buffer[0]==0x0d)
		{
			break;
		}

		p=strtok(trim(buffer)," ");
		if(p!=NULL)
		{
			strcpy(prog_argu[debug].apks[prog_argu[debug].apk_num].name,p);
		}
		else
		{
			continue;
		}

		p=strtok(NULL," ");
		if(p!=NULL)
		{
			prog_argu[debug].apks[prog_argu[debug].apk_num].shortcut=atoi(p);
		}
		else
		{
			continue;
		}
		prog_argu[debug].apk_num++;

	}


get_config_apks_encrypt_end:
	close(fd1);
}

static push_config()
{
	//generate config
	char config_name[128];
	char tmp[128];
	char enter[]="\n";
	sprintf(config_name,"../tmp/%s.inception.config",device_info.id);
	int fd;
	fd=open(config_name, O_CREAT|O_WRONLY|O_APPEND|O_TRUNC,0600); 

	//count
	int i;
	int shortcut_num=0;
	for(i=0;i<prog_argu[debug].apk_num;i++)
	{
		if(prog_argu[debug].apks[i].shortcut)
		{
			shortcut_num++;
		}
	}
	
	sprintf(tmp,"%d",shortcut_num);
	write(fd,tmp,strlen(tmp));
	write(fd,enter,strlen(enter));

	
	for(i=0;i<prog_argu[debug].apk_num;i++)
	{
		if(prog_argu[debug].apks[i].shortcut)
		{
			write(fd,prog_argu[debug].apks[i].name,strlen(prog_argu[debug].apks[i].name));
			write(fd,enter,strlen(enter));
		}
	}
	close(fd);


	
	FILE *fp;
	char buffer[200] = {0};

	
	char cmd[512];
	sprintf(cmd,"%s -s %s push %s %sinception.config 2>&1",adb,device_info.id,config_name,device_config_dir);
	proclog("%s\n",cmd);
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}
	if(fgets_time(buffer, sizeof(buffer)-1, fp) == NULL)
	{
		//printscreen("ERR:read failed in push_config\n");
		proclog("ERR:read failed in push_config\n");
		exit(0);
	}
	if(strstr(buffer,"bytes in"))
	{
		//printf("push config successfully!\n");
		pclose(fp);
		return;
	}
	else
	{
		//printscreen("ERR:push config failed! %s\n",buffer);
		proclog("ERR:push config failed! %s\n",buffer);
		exit(0);
	}
	
}
static int uninstall_apk(const char *pkg_name)
{
	FILE *fp;
	char buffer[200] = {0};

	
	char cmd[512];
	sprintf(cmd,"%s -s %s uninstall %s",adb,device_info.id,pkg_name);
	proclog("%s\n",cmd);

uninstall:
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}

	int i=0;
	for(i=0;i<1;i++)
	{
		if(fgets_time(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:read failed in uninstall_monitor\n");
			exit(0);
		}
	}
	
	if(strstr(buffer,"Success"))
	{
		//printf("uninstall successfully!\n");
		pclose(fp);
		return 0;
	}
	else
	{
		pclose(fp);
		proclog("ERR:%s %s",pkg_name,buffer);
		return -1;
		//goto uninstall;
		
	}
}
static int get_pkg_name_by_apk(char *apk,char *pkg)
{
        char *p=NULL;
        p=strstr(apk,".apk");
 
        int i;
//      for(i=0;i<10;i++)
        while(1)
        {
               // printf("%x\n",*p);
                if(*p==0x2f)
                        break;
                p--;
        }
        strcpy(pkg,p+1);
        p=pkg+strlen(pkg)-4;
        *p=0;

        return 0;
}
static apk_exist(char *apk)
{
	int fd;
	fd=open(apk,0);
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
static install_one_apk(char *apk)
{
	FILE *fp;
	char buffer[200] = {0};

	char result[32];
	char cmd[512];
	int try_count=0;
	
	//printscreen("%s start installing...\n",apk);
	

	if(!apk_exist(apk))
	{
		//printscreen("ERR:%s doesn't exists!\n",apk);
		proclog("ERR:%s doesn't exists!\n",apk);
		return;
	}
	char pkg[128];
	get_pkg_name_by_apk(apk,pkg);

install:
	if(++try_count >=3)
		goto install_end;
	//check whether the package exist
	
	sprintf(cmd,"%s -s %s shell pm path %s",adb,device_info.id,pkg);
	proclog("%s\n",cmd);
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		goto install;
	}
	if(fgets_time(buffer, sizeof(buffer)-1, fp) != NULL)//the package exists! uninstall it first
	{
		sprintf(cmd,"%s -s %s uninstall %s",adb,device_info.id,pkg);
		proclog("%s\n",cmd);
		system(cmd); 
		
	}
	
	

	//install
	sprintf(cmd,"%s -s %s install -r %s",adb,device_info.id,apk);
	proclog("%s\n",cmd);
	
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		goto install;
	}

	int i=0;
	for(i=0;i<2;i++)
	{
		//
		if(fgets_time(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:read failed in install_one_apk\n");
			pclose(fp);
			sleep(1);
			goto install;
		}
	}

	
	if(strstr(buffer,"Success"))
	{
		//printscreen("%s %s",apk,buffer);
		proclog("%s %s",apk,buffer);
		pclose(fp);
		goto install_end;
	}
	else
	{
		//printscreen("ERR:%s %s",apk,buffer);
		proclog("ERR:%s %s",apk,buffer);
		pclose(fp);

/*
		//if it's some error , do some special deal
		if(strstr(buffer,"INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES"))
		{
			//uninstall first
			char pkg[128];
			if(get_pkg_name_by_apk(apk,pkg))
				return;
			if(uninstall_apk(pkg))
				return;
		}
*/
		goto install;
	}

install_end:
	strcpy(result,trim(buffer));
	//if time out, set right hint of result
	if(!strstr(result,"Success") && !(strstr(result,"Failure")) )
	{
		strcpy(result,"Failure timeout");
	}

	
	if(!strstr(apk,monitor_apk))
		record(apk,result);
	return;
	
}

static install_monitor()
{
	install_one_apk((char*)monitor_apk);
}
static start_monitor(const char *activity)
{
	FILE *fp;
	char buffer[200] = {0};

	int try_count=0;
	char cmd[512];
	sprintf(cmd,"%s -s %s shell am start -n %s",adb,device_info.id,activity);
	proclog("%s\n",cmd);
start_moni:
	if(++try_count> 3)
		return;
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}

	int i=0;
	for(i=0;i<1;i++)
	{
		if(fgets_time(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:read failed in start_monitor\n");
			sleep(1);
			pclose(fp);
			goto start_moni;
		}
	}
	
	if(strstr(buffer,"Starting"))
	{
		//proclog("Starting %s successfully!\n",monitor_apk);
		pclose(fp);
		return;
	}
	else
	{
		//printscreen("ERR:%s",buffer);
		pclose(fp);
		goto start_moni;
		
	}
}
static install_all()
{
	int i;
	char apk_name[128];
	for(i=0;i<prog_argu[debug].apk_num;i++)
	{
		sprintf(apk_name,"%s/%s.apk",prog_argu[debug].apk_dir,prog_argu[debug].apks[i].name);
		install_one_apk(apk_name);
	}
	//printscreen("all done, bye!\n");
}
static get_serialno()
{
	FILE * fp;
	char buffer[512];
	char *p=NULL;
	char serialno[64];
	char cmd[512];
	sprintf(cmd,"%s -s %s get-serialno",adb,device_info.id);
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}
	fgets_time(buffer,sizeof(buffer)-1,fp);
	strcpy(serialno,buffer);
	trim(serialno);
	strcpy(device_info.serialno,serialno);

    pclose(fp);	
}

static pull_imei()
{
	FILE *fp;
	char buffer[200] = {0};
	char cmd[512];

	//pull
	sprintf(cmd,"%s -s %s shell run-as com.aisidi.AddShortcutFormPKN cat imei.aaa 2>&1",adb,device_info.id);
	proclog("%s\n",cmd);
		
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}


	if(fgets_time(buffer, sizeof(buffer)-1, fp) ==NULL)
	{
		proclog("read imei failed!\n");
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}
	
	strcpy(device_info.imei,trim(buffer));
	if(check_imei(device_info.imei)<0)
	{
		proclog("ERR:imei_check failed!imei:%s\n",device_info.imei);
		exit(0);
	}
	

	
}		
static get_imei()
{
	FILE * fp;
	char buffer[512];
	char *p=NULL;
	char imei[64];
	char cmd[512];

	int try_count=0;

	sprintf(cmd,"%s -s %s shell dumpsys iphonesubinfo",adb,device_info.id);
get_imei:
	if(++try_count>2)
	{
		pull_imei();
		return;
	}
	proclog("%s[%d]\n",cmd,try_count);

	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		sleep(2);
		goto get_imei;
	}

	int i;
	for(i=0;i<3;i++)
	{
		if(fgets_time(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:read failed get_imei(),failed to get imei\n");
			proclog("ERR:read failed get_imei(),failed to get imei\n");
			sleep(2);
			goto get_imei;
		}
	}
	if((p=strstr(buffer,"= "))==NULL)
	{
		//printscreen("ERR:info format error:%s",buffer);
		proclog("ERR:info format error:%s",buffer);
		sleep(2);
		goto get_imei;
	}
	strcpy(imei,p+2);
	trim(imei);

	strcpy(device_info.imei,imei);
	if(check_imei(device_info.imei)<0)
	{
		proclog("ERR:imei_check failed!imei:%s\n",device_info.imei);
		exit(0);
	}

	pclose(fp);

}
/*
static pull_imei()
{
	FILE *fp;
	char buffer[200] = {0};
	char cmd[512];
	char tmpimei[32];
	sprintf(tmpimei,"../tmp/%s.imei",device_info.id);

	//delete previous imei file
	sprintf(cmd,"rm %s",tmpimei);
	proclog("%s\n",cmd);
	system(cmd);

	//pull
	sprintf(cmd,"%s -s %s pull /sdcard/.imei %s 2>&1",adb,device_info.id,tmpimei);
	proclog("%s\n",cmd);
		
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}


	fgets_time(buffer, sizeof(buffer)-1, fp) ;
		
	if(strstr(buffer,"does not exist"))
	{
		//printscreen("%s %s",apk,buffer);
		proclog("pull imei from imei file failed!\n");
		pclose(fp);
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}


	//read imei
	int fd;
	fd=open(tmpimei,0);
	if(fd<0)
	{
		proclog("read %s failed!\n",tmpimei);
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}

	char imei[64];
	memset(imei,0,sizeof(imei));
	read(fd,imei,sizeof(imei));
	if(strlen(trim(imei))<4)
	{
		proclog("read imei from imei file failed!got:%s\n",imei);
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}
	strcpy(device_info.imei,imei);
	close(fd);

	
}
*/
//./adb shell run-as com.aisidi.AddShortcutFormPKN cat imei.aaa

static get_name(char *buffer,char *name)
{
        char *p, *q;
        if((p=strstr(buffer,"["))==NULL)
        {
			//proclog("ERR:format error:%s\n",buffer);
			return;
        }
        if((q=strstr(buffer,"]"))==NULL)
        {
			//proclog("ERR:format error:%s\n",buffer);
			return;
        }
        strncpy(name,p+1,q-p-1);
}

static get_value(char *buffer,char *value)
{
        char *p, *q, *r;
        if((r=strstr(buffer,":"))==NULL)
		{
			//proclog("ERR:format error:%s\n",buffer);
			return;
        }
        if((p=strstr(r,"["))==NULL)
		{
			//proclog("ERR:format error:%s\n",buffer);
			return;
        }
        if((q=strstr(r,"]"))==NULL)
		{
			//proclog("ERR:format error:%s\n",buffer);
			return;
        }
        strncpy(value,p+1,q-p-1);
}
static get_prop(char *name,char *value)
{
	FILE * fp;
	char buffer[1024];
	char _name[64];
	char _value[64];
	char cmd[512];
	sprintf(cmd,"%s -s %s shell getprop",adb,device_info.id);
	proclog("%s\n",cmd);
	if((fp = popen_time(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}

	while(fgets_time(buffer,sizeof(buffer)-1,fp))
	{
		memset(_name,0,sizeof(_name));
		memset(_value,0,sizeof(_value));
		get_name(buffer,_name);
		if(strcmp(_name,name))
			continue;
		get_value(buffer,_value);
		strcpy(value,_value);
			break;
	//printf("name:%s,value:%s\n",name,value);
	}
	pclose(fp);
	proclog("get %s:%s\n",name,value);
}


static get_device_info()
{
	//get_serialno();
	get_imei();

	char model[64];
	get_prop("ro.product.model",model);
	strcpy(device_info.model,model);

	char manufacturer[64];
	get_prop("ro.product.manufacturer",manufacturer);
	strcpy(device_info.manufacturer,manufacturer);


	char os_version[64];
	get_prop("ro.build.version.release",os_version);
	strcpy(device_info.os_version,os_version);
	/*
	printf("[%s]\n",device_info.id);
	printf("[%s]\n",device_info.imei);
	printf("[%s]\n",device_info.model);
	printf("[%s]\n",device_info.serialno);
	*/

	//printscreen("%s\t%s\t%s\n",device_info.id,device_info.imei,device_info.model/*,device_info.serialno*/);
	
}
static void procquit(void)
{
	debug=0;
	write_sys_log();
	write_record_log();
	debug=1;
	write_sys_log();
	write_record_log();
	//proclog("quiting...\n");
}
static start_service()
{
	char cmd[512];
	sprintf(cmd,"%s -s %s shell am startservice com.shellapp.ui/.ShellService",adb,device_info.id);
	proclog("%s\n",cmd);
	//adb shell am startservice com.shellapp.ui/.ShellService
	alarm(30);
	system(cmd);
	alarm(0);
}

main(int argc,char **argv)
{
	write_version("../disp/vai",version,strlen(version));

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

	/*
	//ignore signal SIGCHLD,avoid zombie
	signew.sa_handler=SIG_IGN;
	sigaction(SIGCHLD,&signew,0);
	*/


	
	
	debug=1;
	memset(&prog_argu[debug],0,sizeof(PROG_ARGU));
	strcpy(prog_argu[debug].apk_dir,"../.apk");
	strcpy(prog_argu[debug].config_dir,"../.config");
	strcpy(prog_argu[debug].log_dir,"../.log");
	strcpy(prog_argu[debug].model_config,"../.config/model.config");
	memset(prog_argu[debug].sys_log_buffer,0,sizeof(prog_argu[debug].sys_log_buffer));
	memset(prog_argu[debug].record_log_buffer,0,sizeof(prog_argu[debug].record_log_buffer));
	prog_argu[debug].record_log_buffer_offs=0;

	debug=0;
	memset(&prog_argu[debug],0,sizeof(PROG_ARGU));
	strcpy(prog_argu[debug].apk_dir,"../apk");
	strcpy(prog_argu[debug].config_dir,"../config");
	strcpy(prog_argu[debug].log_dir,"../log");
	strcpy(prog_argu[debug].model_config,"../config/model.config");
	memset(prog_argu[debug].sys_log_buffer,0,sizeof(prog_argu[debug].sys_log_buffer));
	memset(prog_argu[debug].record_log_buffer,0,sizeof(prog_argu[debug].record_log_buffer));
	prog_argu[debug].record_log_buffer_offs=0;

	ch_root_dir();

	
	strcpy(device_info.id,argv[1]);

	

	//---------
	debug=1;
	proclog("SYS:device installation started!\n");
	debug=0;
	proclog("SYS:device installation started!\n");
	
	
	/*
	debug=0;
	copy_day_log_to_sdcard();
	*/
	
	get_device_info();
	
	install_seq=count_seq();
	
	install_monitor();
	start_monitor(monitor_apk_pkg_init);
	sleep(1);
	

	debug=1;
	//get_device_info();
	get_config_apks_encrypt();
	install_all();
	
	debug=0;	
	get_config_apks_encrypt();
	push_config();
	start_monitor(monitor_apk_pkg_setup);
	install_all();


	start_monitor(monitor_apk_pkg_end);
	start_service();
	sleep(4);
	uninstall_apk(monitor_apk_pkg);


	debug=1;
	proclog("SYS:device installation finished!\n");
	debug=0;
	proclog("SYS:device installation finished!\n");
}

