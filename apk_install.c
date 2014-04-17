#include "com.h"
/*
1.31 resloved the problem of cannot set system clock,you have to sleep for a while after ntpd
1.32 write version number by -v arugments
1.33 printf when proclog 2012-11-29
1.34 log don't encrypt, add pull_imei function
1.35 modify pull_mei(./adb shell run-as com.aisidi.AddShortcutFormPKN cat imei.aaa)
1.60 modify log format
	install_id
	logtime to nana second
	start two server background(tmp solution)
	2013-03-27
1.61 install_seq (after read screen)
1.62 modify the sequence of log format
	 set install_id to fix length 
1.70 add a column(apk_name) in model.config,suppose both two column
1.71 value pkg_name when reading config instead of when installing
1.72 memset all variables for device info
	check_imei
1.80 phone_id
1.90 don't do anything until device is online
1.91 manufacturer<>samsung, pkg == com.taobao.taobao,then do nothing
1.92 fix debug of 1.91, misunderstanding..,do skip only when the pkg exists!
1.93 remove the space before pkgname when comparing
1.94 modify position of wait_device_online()
1.95 modify started service
1.96 widen imei check, pull_imei first, write phoneid to the second line of push config
1.97 start_all_activity
1.98 adb=>./adb
1.99 ./adb => ./adb -s
2.00 nano_sleep between starting activities
2.01 adjust the sequenece of start_monitor() and get_device_info()
2.02 modify the format of record(record_app and record_phone)
	add crc to record file(new crc algorithm)
	remove the first line of inception.config( old first line is shortcut number)
2.03 fix following bugs in version 2.02
2.04 @field in model.config !strcmp==>strstr
2.05 use a new method to install a sieral types of phones like lenovo and so on.
2.06 enlarge result. 32-->128, sometimes it's not enough
2.07 skip some function
2.08 fix "skip_come" bug of 2.07
2.10 modify log schedule
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
static char install_id[32];
static int install_seq=0;
static const char *install_seq_file="../disp/install_seq";
static const char *version="o2.12";
static time_t phone_install_start_time,phone_install_finish_time;


	
extern const int des_len;
extern const char *key;
int debug;
//char apks[100][128];
typedef struct
{
	char pkg_name[128];
	int shortcut;
	char apk_name[128];
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
	char phone_id[32];
	int install_method;
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

static char *string_to_lower(char *str, int len,char *lower_string)
{
	int i;
	for(i=0;i<len;i++)
	{
		lower_string[i]=tolower(str[i]);
	}
	lower_string[i+1]=0;

	//printf("lower_string:%s\n",lower_string);
	return lower_string;
}
static int select_install_method()
{
	char lower_string[32];
	if(strstr("lenovo", string_to_lower(device_info.manufacturer, strlen(device_info.manufacturer),lower_string)))
	{
		printf("install method :2\n");
		device_info.install_method = 2;
	}
	else
	{
		printf("install method :1\n");
		device_info.install_method = 1;
	}
}
static void proclog(const char *fmt,...)
{
	char ts[32];
	char buf[des_len];
	time_t tt;
	struct timeval tv;
        

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

	char ts_nano[64];
	gettimeofday (&tv, NULL);
	sprintf(ts_nano,"%s.%06d",ts, tv.tv_usec);
	

	int fd;

	
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
	//log content
	sprintf(buf,"%s\t%s\t%s\t%s\t%d\t%s",ts_nano,prog,version,install_id,getpid(),tmp);
	printf("%s",buf);



	//get box id
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	get_box_id(box_id);
		

	char filename[128];
	//hour log

	/*
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.sys",prog_argu[debug].log_dir,ts,box_id);

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

	// day log
	strftime(ts,30,"%Y%m%d",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.sys.orig",prog_argu[debug].log_dir,ts,box_id);

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

static void write_to_record_file(char *buf)
{
	//get box_id
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	get_box_id(box_id);


	int fd;
	time_t tt;
	char ts[32];
	char filename[128];

	tt=time(0);

	//hour log
	/*
	strftime(ts,30,"%Y%m%d%H",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.record",prog_argu[debug].log_dir,ts,box_id);
	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600);
	if(fd <0)
	{
		//printscreen("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	flock(fd,LOCK_EX);
	write(fd,buf,des_len);
	flock(fd,LOCK_UN);
	close(fd);
	*/
	//day log
	strftime(ts,30,"%Y%m%d",(const struct tm *)localtime(&tt));
	sprintf(filename,"%s/%s_%s.record.orig",prog_argu[debug].log_dir,ts,box_id);
	fd=open(filename, O_CREAT|O_WRONLY|O_APPEND,0600);
	if(fd <0)
	{
		printf("open %s failed!%s\n",filename,strerror(errno));
		return;
	}
	flock(fd,LOCK_EX);
	write(fd,buf,des_len);
	flock(fd,LOCK_UN);
	close(fd);
}
/*

标准格式：日志类型 \t 日志正文

其中“日志类型”目前包含两种：0 应用安装日志； 1 手机安装日志

如果是应用安装日志（0），则记录：

安装开始时间 \t 安装结束时间 \t 内部流水号 \t 应用文件名 \t CRC校验码 \t 安装结果及报告 （共7个字段）

如果是手机安装日志 （1），则记录：

安装开始时间 \t 安装结束时间 \t 内部流水号 \t IMEI \t 厂家 \t 型号 \t 操作系统版本号 \t 配置文件版本号 \t 安装序列号 （共9个字段）
*/

static void get_model_config_version(char *model_config_version)
{
	char *filename="../disp/model.config.version";
	int fd;
	fd=open(filename,0);
	if(fd<0)
	{
		strcpy(model_config_version,"NONE");
		return;
	}

	char buf[32];
	memset(buf,0,sizeof(buf));
	read(fd,buf,20);
	strcpy(model_config_version, trim(buf));
	close(fd);



}

static void record_app(time_t app_install_start_time, time_t app_install_finish_time,char *apkname,char *result)
{


	//
	char crc_value[32];
	sprintf(crc_value,"%X",get_file_crc_general(apkname));

	//
	char start_time[32];
	strftime(start_time,30,"%F %X",(const struct tm *)localtime(&app_install_start_time));


	char buf[des_len];
	sprintf(buf, "0\t%s\t%d\t%s\t%s\t%s\t%s\n",
			start_time,
			app_install_finish_time-app_install_start_time,
			install_id,
			apkname,
			crc_value,
			result
			);
	//proclog("record_app:%s\n", buf);
	T_DES(1,key,des_len,buf,buf);

	write_to_record_file(buf);


}
static void record_phone()
{
	/*
	 * 安装开始时间 \t 安装结束时间 \t 内部流水号 \t IMEI \t 厂家 \t 型号 \t 操作系统版本号 \t 配置文件版本号 \t 安装序列号
	 */
	//
	char start_time[32];
	strftime(start_time,30,"%F %X",(const struct tm *)localtime(&phone_install_start_time));

	//get model_config_version
	char model_config_version[32];
	get_model_config_version(model_config_version);


	//sprintf(buf, "%s\t%s\t%s\t%s\t %s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", 	install_start_time, install_finish_time，install_id,device_info.imei, 			device_info.manufacturer,device_info.model,device_info.os_version,			model_config_version, apkname, crc_value, install_seq,result);
	char buf[des_len];
	sprintf(buf, "1\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%d\n",
			start_time,
			phone_install_finish_time-phone_install_start_time,
			install_id,
			device_info.imei,
			device_info.manufacturer,
			device_info.model,
			device_info.os_version,
			model_config_version,
			install_seq
			);
	//proclog("record_phone:%s\n", buf);
	T_DES(1,key,des_len,buf,buf);


	write_to_record_file(buf);


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
	
	fgets(buffer,sizeof(buffer),fp);
	device_info.apk_num=atoi(trim(buffer));

	int i;
	for(i=0;i<device_info.apk_num;i++)
	{
		if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
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

static char *last_modify_day(int fd,char *day)
{
        struct stat statbuf;
        fstat(fd,&statbuf);
        strftime(day,30,"%Y%m%d",(const struct tm *)localtime(&statbuf.st_mtime));
        //printf("%s\n",ts);
	return day;

}

static count_seq()
{
        char buf[1024];
        int n=0;

        int ret=-1;
        int count=0;

   

        int fd;
        fd=open(install_seq_file, O_CREAT|O_RDWR|O_APPEND,0600);
        if(fd <0)
        {
         	proclog("ERR:open %s failed!%s\n",install_seq_file,strerror(errno));
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
                proclog("ERR:read %s failed!%s\n",install_seq_file,strerror(errno));
                ret=-1;
        }

	
        if(n==0)//file just be created
        {
                ftruncate(fd,0);
                write(fd,"1",1);
                install_seq=1;
        }
        else //n>0,most of the times
        {
                ftruncate(fd,0);
                count=atoi(buf)+1;
                sprintf(buf,"%d",count);
                write(fd,buf,strlen(buf));
                install_seq=count;
        }
        flock(fd,LOCK_UN);
        close(fd);

		proclog("SYS:install_seq:%d\n",install_seq);
}
static trim_comment(char *buffer)
{
	char *p=NULL;
	if((p=strstr(buffer,"#")))
	{
		*p=0;
	}
}
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
		trim_comment(buffer);
		if(buffer[0]=='@')
		{
			strcpy(model,trim(buffer+1));
			
			if(!strcmp(model,"default") || strstr(model,device_info.model))
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
		trim_comment(buffer);

		//meet the next model definition
		if(buffer[0]=='@' )//||buffer[0]==0x0d)
		{
			break;
		}
		//blank line or comment
		if(strlen(buffer)==0)
		{
			continue;
		}



		//pkg_name
		p=strtok(trim(buffer)," ");
		if(p!=NULL)
		{
			strcpy(prog_argu[debug].apks[prog_argu[debug].apk_num].pkg_name,p);
		}

		//shortcut
		p=strtok(NULL," ");
		if(p!=NULL)
		{
			prog_argu[debug].apks[prog_argu[debug].apk_num].shortcut=atoi(p);
		}


		//apk_name
		p=strtok(NULL," ");
		if(p!=NULL)
		{
			strcpy(prog_argu[debug].apks[prog_argu[debug].apk_num].apk_name,p);
		}
		else
		{
			sprintf(prog_argu[debug].apks[prog_argu[debug].apk_num].apk_name,"%s.apk",prog_argu[debug].apks[prog_argu[debug].apk_num].pkg_name);
		}

		prog_argu[debug].apk_num++;

	}


get_config_apks_encrypt_end:
	close(fd1);
}

static push_config()
{
	int i;
	//generate config
	char config_name[128];
	char tmp[128];
	char enter[]="\n";
	sprintf(config_name,"../tmp/%s.inception.config",device_info.id);
	int fd;
	fd=open(config_name, O_CREAT|O_WRONLY|O_APPEND|O_TRUNC,0600); 

	//write shortcut count to the first line

	/*2.02版本更改，取消写入shortcut_num到inception.config文件

	int shortcut_num=0;
	for(i=0;i<prog_argu[debug].apk_num;i++)
	{
		if(prog_argu[debug].apks[i].shortcut==1)
		{
			shortcut_num++;
		}
	}
	
	sprintf(tmp,"%d",shortcut_num);
	write(fd,tmp,strlen(tmp));
	write(fd,enter,strlen(enter));
	*/

	//write phone_id(20bytes) to the second line
	write(fd,install_id,strlen(install_id));
	write(fd,enter,strlen(enter));

	//write shortcut of packlist
	for(i=0;i<prog_argu[debug].apk_num;i++)
	{
		if(prog_argu[debug].apks[i].shortcut)
		{
			write(fd,prog_argu[debug].apks[i].pkg_name,strlen(prog_argu[debug].apks[i].pkg_name));
			write(fd,enter,strlen(enter));
		}
	}
	close(fd);


	
	FILE *fp;
	char buffer[200] = {0};

	
	char cmd[512];
	sprintf(cmd,"%s -s %s push %s %sinception.config 2>&1",adb,device_info.id,config_name,device_config_dir);
	proclog("%s\n",cmd);
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}
	if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
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
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}

	int i=0;
	for(i=0;i<1;i++)
	{
		if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:read failed in uninstall_monitor\n");
			proclog("ERR:read failed in uninstall:%s\n",cmd);
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
/*
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
*/
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
static void tab2space(char *str)
{
        char *p=str;
        while(*p)
        {
                if(*p=='\t')
                        *p=' ';
                p++;
        }
}

static  __install_one_apk_1(char *apk,char *pkg, char *result)
{
	FILE *fp;
	char cmd[512];
	char buffer[200] = {0};
	//install
	int try_count=0;
	install:
		if(++try_count >=3)
			return;

	sprintf(cmd,"%s -s %s install -r %s",adb,device_info.id,apk);
	proclog("%s\n",cmd);

	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		goto install;
	}

	int i=0;
	for(i=0;i<2;i++)
	{
		//
		if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:read failed in install_one_apk\n");
			pclose(fp);
			sleep(1);
			goto install;
		}
	}

	strcpy(result,buffer);
	tab2space(trim(result));
	proclog("%s %s",apk,buffer);


}
static  __install_one_apk_2(char *apk,char *pkg, char *result)
{
	FILE *fp;
	char cmd[512];
	char buffer[200] = {0};

	strcpy(result,"NULL");

	//get apk file name
	char apkfilename[32];
	sprintf(apkfilename,strrchr(apk,'/')+1);
	//proclog("#####installing %s#####\n",apkfilename);

	//push
	sprintf(cmd,"%s -s %s push %s /data/local/tmp",adb,device_info.id,apk);
	proclog("%s\n",cmd);
	system(cmd);

	//make ins.sh
	int fd;
	char insfilename[128];

	sprintf(insfilename,"../tmp/install.sh");
	fd=open(insfilename, O_CREAT|O_WRONLY|O_TRUNC,0600);
	sprintf(buffer,"pm install -r /data/local/tmp/%s",apkfilename);
	write(fd,buffer,strlen(buffer));
	close(fd);

	//push install.sh
	sprintf(cmd,"%s -s %s push %s /data/local/tmp",adb,device_info.id,insfilename);
	proclog("%s\n",cmd);
	system(cmd);

	//chmod 777
	sprintf(cmd,"%s -s %s shell chmod 777 /data/local/tmp/install.sh",adb,device_info.id);
	proclog("%s\n",cmd);
	system(cmd);


//install
	int try_count=0;
install:
	if(++try_count >1)
		goto install_end;

	sprintf(cmd,"%s -s %s shell /data/local/tmp/install.sh",adb,device_info.id);
	proclog("%s\n",cmd);
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		goto install;
	}

	int i=0;
	for(i=0;i<2;i++)
	{
		//
		if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
		{
			//printscreen("ERR:read failed in install_one_apk\n");
			pclose(fp);
			sleep(1);
			goto install;
		}
	}

	strcpy(result,buffer);
	tab2space(trim(result));
	proclog("%s %s",apk,buffer);


install_end:
	sprintf(cmd,"%s -s %s shell rm /data/local/tmp/%s",adb,device_info.id,apkfilename);
	proclog("%s\n",cmd);
	system(cmd);
}
static int is_installed(char *pkg)
{
	FILE *fp;
	char cmd[512];
	char buffer[200] = {0};
	sprintf(cmd,"%s -s %s shell pm path %s",adb,device_info.id,pkg);
	proclog("%s\n",cmd);
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		return 0;
	}

	if(fgets(buffer, sizeof(buffer)-1, fp) != NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
static __uninstall_one_apk(char *pkg)
{
	char cmd[512];
	sprintf(cmd,"%s -s %s uninstall %s",adb,device_info.id,pkg);
	proclog("%s\n",cmd);
	system(cmd);
	
}
static int is_skip(char *pkg)
{
	char skip_pkg[]="com.taobao.taobao";
	if(!strstr(skip_pkg,pkg))
		return 0;
	///	manufacturer<>samsung, pkg == com.taobao.taobao,then do nothing
	if( ( strcmp(device_info.manufacturer,"samsung" )) && ( !strcmp(pkg, "com.taobao.taobao")))
	{
		proclog("manufacturer %s, pkg:%s, skip!\n", device_info.manufacturer, pkg);
		return 1;
	}
	return 0;
}
static install_one_apk(char *apk,char *pkg)
{
	char result[128];


	time_t app_install_start_time,app_install_finish_time;


	//printscreen("%s start installing...\n",apk);
	alarm(60);

	if(!apk_exist(apk))
	{
		//printscreen("ERR:%s doesn't exists!\n",apk);
		proclog("ERR:%s doesn't exists!\n",apk);
		alarm(0);
		return;
	}

	app_install_start_time=time(0);

	if(is_installed(pkg))
	{
		if(is_skip(pkg))
		{
			return;
		}
		else
		{
			__uninstall_one_apk(pkg);
		}
	}


	memset(result,0,sizeof(result));
	//install
	if(device_info.install_method==2)
	//if(1)
		__install_one_apk_2(apk,pkg,result);
	else
		__install_one_apk_1(apk,pkg,result);

	app_install_finish_time = time(0);

	if(!strstr(apk,monitor_apk))
		record_app(app_install_start_time,app_install_finish_time,apk,result);
	
	alarm(0);

}

static install_monitor()
{
	install_one_apk((char*)monitor_apk,"add.pkg.name");
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
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}

	int i=0;
	for(i=0;i<1;i++)
	{
		if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
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


	phone_install_start_time = time(0);
	for(i=0;i<prog_argu[debug].apk_num;i++)
	{
		if(prog_argu[debug].apks[i].shortcut==2)//
			continue;
		//apk_name was value when read_config,if there's no the third column in config file,it'd valued with apk append pkgname
		sprintf(apk_name,"%s/%s",prog_argu[debug].apk_dir,prog_argu[debug].apks[i].apk_name);
		install_one_apk(apk_name,prog_argu[debug].apks[i].pkg_name);
	}
	phone_install_finish_time =  time(0);

	//printscreen("all done, bye!\n");
}
static start_all_activity()
{
	int i;
	char cmd[512];
	for(i=0;i<prog_argu[debug].apk_num;i++)
	{
		if(prog_argu[debug].apks[i].shortcut==2)//
		{
			sprintf(cmd,"%s -s %s shell am start %s",adb,device_info.id,prog_argu[debug].apks[i].apk_name);
			proclog("%s\n",cmd);
			system(cmd);
			my_nano_sleep(300000000);
		}

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
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}
	fgets(buffer,sizeof(buffer)-1,fp);
	strcpy(serialno,buffer);
	trim(serialno);
	strcpy(device_info.serialno,serialno);

    pclose(fp);	
}
static int check_imei(char *imei)
{
        char *p=NULL;
        p=imei;
        int len=0;
        while(*p)
        {
               if(!isdigit(*p) && !isalpha(*p))
                        return -1;
                len++;
                p++;

        }
        if(len<3)
        		return -1;
        /*
        if(len!=15)
                return -1;
	*/
	//proclog("SYS:imei:%s\n",imei);
        return 0;
}

static get_imei_by_dumpsys()
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
		return;
	proclog("%s[%d]\n",cmd,try_count);

	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		sleep(2);
		goto get_imei;
	}

	int i;

	for(i=0;i<3;i++)
	{
		memset(buffer,0,sizeof(buffer));
		if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
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
	memset(imei,0,sizeof(imei));
	strcpy(imei,p+2);
	if(!check_imei(trim(imei)))
	{
		strcpy(device_info.imei,imei);
		proclog("SYS:get imei by dumpsys:%s\n",device_info.imei);

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
		
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}


	fgets(buffer, sizeof(buffer)-1, fp) ;
		
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
static pull_imei()
{
	FILE *fp;
	char buffer[200] = {0};
	char cmd[512];

	//pull
	sprintf(cmd,"%s -s %s shell run-as com.aisidi.AddShortcutFormPKN cat imei.aaa 2>&1",adb,device_info.id);
	proclog("%s\n",cmd);
		
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}

	memset(buffer,0,sizeof(buffer));
	if(fgets(buffer, sizeof(buffer)-1, fp) ==NULL)
	{
		proclog("read imei failed!\n");
		uninstall_apk(monitor_apk_pkg);
		exit(0);
	}
	if(!check_imei(trim(buffer)))
	{
		strcpy(device_info.imei,buffer);
		proclog("SYS:get imei by imei.aaa:%s\n",device_info.imei);
	}

	//

	
}

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
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}

	memset(buffer,0,sizeof(buffer));
	while(fgets(buffer,sizeof(buffer)-1,fp))
	{
		memset(_name,0,sizeof(_name));
		memset(_value,0,sizeof(_value));
		get_name(buffer,_name);
		if(strcmp(_name,name))
			continue;
		get_value(buffer,_value);
		strcpy(value,_value);
			break;
		memset(buffer,0,sizeof(buffer));
	//printf("name:%s,value:%s\n",name,value);
	}
	pclose(fp);
	proclog("get %s:%s\n",name,value);
}
static get_imei()
{
	pull_imei();
	if(check_imei(device_info.imei))
		get_imei_by_dumpsys();
	if(check_imei(device_info.imei))
	{
		strcpy(device_info.imei,"NONE");
		proclog("get imei failed!");
	}
}
static get_manufacturer()
{
	char manufacturer[64];
	memset(manufacturer,0,sizeof(manufacturer));
	get_prop("ro.product.manufacturer",manufacturer);
	strcpy(device_info.manufacturer,manufacturer);
}
static get_os_version()
{
	char os_version[64];
	memset(os_version,0,sizeof(os_version));
	get_prop("ro.build.version.release",os_version);
	strcpy(device_info.os_version,os_version);
}
static get_model()
{
	char model[64];
	memset(model,0,sizeof(model));
	get_prop("ro.product.model",model);
	strcpy(device_info.model,model);
}

static void procquit(void)
{
	proclog("quiting...\n");
}
static start_service()
{
	char cmd[512];

	/*
	sprintf(cmd,"%s -s %s shell am startservice com.shellapp.ui/.ShellService",adb,device_info.id);
	proclog("%s\n",cmd);
	//adb shell am startservice com.shellapp.ui/.ShellService
	system(cmd);
	*/

	sprintf(cmd,"%s -s %s shell am startservice com.shellservice/.ShellService",adb,device_info.id);
	proclog("%s\n",cmd);
	//adb shell am startservice com.shellapp.ui/.ShellService
	system(cmd);

	sprintf(cmd,"%s -s %s shell am startservice --user 0  com.shellservice/.ShellService",adb,device_info.id);
	proclog("%s\n",cmd);
	system(cmd);






	
}
static int get_old_install_id()
{
	char buffer[512];
	memset(buffer,0,sizeof(buffer));
	FILE *fp;
	char cmd[512];

	sprintf(cmd,"%s -s %s shell cat %sinception.config 2>&1",adb,device_info.id,device_config_dir);
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		return 0;
	}
	if(fgets(buffer, sizeof(buffer)-1, fp) == NULL)
	{
		//printscreen("ERR:read failed in push_config\n");
		proclog("ERR:read failed in push_config\n");
		return 0;
	}
	trim(buffer);
	if((buffer[0]=='P') && isdigit(buffer[1]) && isdigit(buffer[2])&& isdigit(buffer[3])&& isdigit(buffer[4]) )
	{
		//printf("push config successfully!\n");
		pclose(fp);
		strcpy(install_id, buffer);
		return 1;
	}
	return 0;
}
static get_install_id()
{
	if(!get_old_install_id())
	{
		struct timeval tv;
		gettimeofday (&tv, NULL);
		sprintf(install_id,"P%d%06d",tv.tv_sec , tv.tv_usec );
	}

}
/*
static get_phone_id()
{
	//get box id
	char box_id[32];
	memset(box_id,0,sizeof(box_id));
	get_box_id(box_id);

	//copy the last 4 bytes to phone_id
	char *p=box_id+strlen(box_id)-4;
	strcpy(device_info.phone_id,p);

	if(check_imei(device_info.imei))
	{
		//failed to get imei
		strcat(device_info.phone_id,install_id);
	}
	else
	{
		//succeed to get imei
		strcat(device_info.phone_id,"m");
		strcat(device_info.phone_id,device_info.imei);
	}

}
*/
static is_device_online()
{
	FILE *fp;
	char buffer[512];
	char cmd[512];
	sprintf(cmd,"%s devices",adb);

	//proclog("%s\n",cmd);
	if((fp = popen(cmd,"r")) == NULL)
	{
		//printscreen("ERR:Fail to execute:%s\n",cmd);
		proclog("ERR:Fail to execute:%s\n",cmd);
		exit(1);
	}

	memset(buffer,0,sizeof(buffer));
	while(fgets(buffer,sizeof(buffer)-1,fp))
	{
		if( strstr(buffer,device_info.id)  &&  strstr(buffer,"device") )
		{
			return 1;
		}
		else
		{
			continue;
		}
		memset(buffer,0,sizeof(buffer));
	//printf("name:%s,value:%s\n",name,value);
	}
	pclose(fp);
	return 0;
}
static wait_device_online()
{
	int i;
	for(i=0;i<20;i++)
	{
		if(is_device_online())
			return ;
		else
			sleep(1);
	}
	proclog("device %s is not online ,quiting...\n",device_info.id);
	exit(0);
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
	strcpy(device_info.id,argv[1]);



	struct sigaction signew;

	debug=0;
	memset(&prog_argu[debug],0,sizeof(PROG_ARGU));
	strcpy(prog_argu[debug].apk_dir,"../apk");
	strcpy(prog_argu[debug].config_dir,"../config");
	strcpy(prog_argu[debug].log_dir,"../log");
	strcpy(prog_argu[debug].model_config,"../config/model.config");


	ch_root_dir();



	wait_device_online();
	get_install_id();
	proclog("SYS:device installation started!\n");


	get_manufacturer();
	select_install_method();
	install_monitor();
	start_monitor(monitor_apk_pkg_init);
	//get_device_info();

	sleep(1);
	get_model();
	get_os_version();
	get_imei();
	//get_phone_id();
	get_config_apks_encrypt();
	push_config();
	start_monitor(monitor_apk_pkg_setup);
	install_all();
	start_all_activity();
	start_monitor(monitor_apk_pkg_end);
	start_service();

	/*__install_one_apk_2 can't read the installing result
	 * maybe device is already not online,so check it first
	 */
	if(!is_device_online())
	{
		proclog("device not online!\n");
		exit(0);
	}

	count_seq();
	record_phone();

	sleep(4);
	uninstall_apk(monitor_apk_pkg);



	proclog("SYS:device installation finished!\n");
	
}
