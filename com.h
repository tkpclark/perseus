#define NONE "\033[m"

#define RED "\033[0;32;31m"

#define LIGHT_RED "\033[1;31m"

#define GREEN "\033[0;32;32m"

#define LIGHT_GREEN "\033[1;32m"

#define BLUE "\033[0;32;34m"

#define LIGHT_BLUE "\033[1;34m"

#define DARY_GRAY "\033[1;30m"

#define CYAN "\033[0;36m"

#define LIGHT_CYAN "\033[1;36m"

#define PURPLE "\033[0;35m"

#define LIGHT_PURPLE "\033[1;35m"

#define BROWN "\033[0;33m"

#define YELLOW "\033[1;33m"

#define LIGHT_GRAY "\033[0;37m"

#define WHITE "\033[1;37m"



#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <dirent.h>

char *trim(char *str);

off_t get_file_size(int fd);

void ch_root_dir();

