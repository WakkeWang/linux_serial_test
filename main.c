#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <syslog.h>

struct BaudRate{
	int speed;
	int bitmap;
};
struct BaudRate baudlist[] =
{
{ 1200, B1200 },
{ 1800, B1800 },
{ 2400, B2400 },
{ 4800, B4800 },
{ 9600, B9600 },
{ 19200, B19200 },
{ 38400, B38400 },
{ 57600, B57600 },
{ 115200, B115200 },
{ 230400, B230400 },
{ 460800, B460800 },
{ 500000, B500000 },
{ 576000, B576000 },
{ 921600, B921600 },
};
int comDatabits[] =
{ 5, 6, 7, 8 };
int comStopbits[] =
{ 1, 2 };
int comParity[] =
{ 'n', 'o', 'e' };
char g_comDevicesName[256][256];
int g_comNums;

void LOG(const char* ms, ... )
{
	char wzLog[1024] = {0};
	char buffer[1024] = {0};
	va_list args;
	va_start(args, ms);
	vsprintf( wzLog ,ms,args);
	va_end(args);

	time_t now;
	time(&now);
	struct tm *local;
	local = localtime(&now);
	sprintf(buffer,"%04d-%02d-%02d %02d:%02d:%02d %s\n", local->tm_year+1900, local->tm_mon,
				local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,
				wzLog);
	printf("%s",buffer);
	FILE* file = fopen("testResut.log","a+");
	fwrite(buffer,1,strlen(buffer),file);
	fclose(file);
	return ;
}


int set_com(int fd,int speed,int databits,int stopbits,int parity)
{
	int i;
	struct termios opt;

	if( tcgetattr(fd ,&opt) != 0)
	{
		perror("get attr failed!\n");
		return -1;
	}

	for (i = 0; i < sizeof(baudlist) / sizeof(baudlist[0]); i++)
	{
		struct BaudRate *rate = &baudlist[i];
		if (speed == rate->speed)
		{
			cfsetispeed(&opt, rate->bitmap);
			cfsetospeed(&opt, rate->bitmap);
			break;
		}
	}

	opt.c_cflag &= ~CSIZE;
	switch (databits)
	{
	case 5:
		opt.c_cflag |= CS5;
		break;
	case 6:
		opt.c_cflag |= CS7;
		break;
	case 7:
		opt.c_cflag |= CS7;
		break;
	case 8:
		opt.c_cflag |= CS8;
		break;
	default:
		printf("Unsupported data size\n");
		return -1;
	}

	switch(parity)
	{
		case 'n':
		case 'N':
			opt.c_cflag &= ~PARENB;
			opt.c_iflag &= ~INPCK;
			break;
		case 'o':
		case 'O':
			opt.c_cflag |= (PARODD|PARENB);
			opt.c_iflag |= INPCK;
			break;
		case 'e':
		case 'E':
			opt.c_cflag |= PARENB;
			opt.c_cflag &= ~PARODD;
			opt.c_iflag |= INPCK;
			break;
		default:
			printf("Unsupported parity\n");
			return -1;
	}

	switch(stopbits)
	{
		case 1:
			opt.c_cflag &= ~CSTOPB;
			break;
		case 2:
			opt.c_cflag |=  CSTOPB;
			break;
		default:
			printf("Unsupported stop bits\n");
			return -1;
	}

	opt.c_iflag &= ~(IXON | IXOFF | IXANY | BRKINT | ICRNL | INPCK | ISTRIP);
	opt.c_lflag &=  ~(ICANON | ECHO | ECHOE | IEXTEN | ISIG);
	opt.c_oflag &= ~OPOST;
	opt.c_cc[VTIME] = 100;
	opt.c_cc[VMIN] = 0;

	tcflush(fd, TCIOFLUSH);
	if (tcsetattr(fd, TCSANOW, &opt) != 0)
	{
		perror("set attr failed!\n");
		return -1;
	}
	return 0;
}

int OpenDev(char* Dev)
{
	int fd = open(Dev,O_RDWR | O_NOCTTY );
	if( -1 == fd)
	{
		perror("open failed!\n");
		return -1;
	}
	else
		return fd;
}

int openDevice(const char* Dev,int speed,int databits,int stopbits,int parity)
{
	int fd;
	fd = open(Dev, O_RDWR | O_NOCTTY);
	if (-1 == fd)
	{
		perror("open failed!\n");
		return -1;
	}

	if (set_com(fd, speed, databits, stopbits, parity) != 0)
	{
		printf("Set Com Error\n");
		return -1;
	}

	return fd;
}

int pairTest(char * coma,char * comb,int speed,int databits,int stopbits,int parity)
{
	char MSG[] = {1,2,3,4,5,6,7,8,9,0};
	int ret;
	char buf[256];
	char result[256];
	int fd1,fd2;
	int idx;
	ssize_t wsize;


	fd1 = openDevice(coma, speed, databits, stopbits, parity);
	if (fd1 <= 0)
	{
		printf("open device %s failed!\n", coma);
		exit(1);
	}

	fd2 = openDevice(comb, speed, databits, stopbits, parity);
	if (fd2 <= 0)
	{
		printf("open device %s failed!\n", comb);
		exit(1);
	}

	wsize = write(fd1,MSG,sizeof(MSG));
//	usleep(1000000/speed*(4+databits)*sizeof(MSG)*2);

	memset(buf, 0, sizeof(buf));
	idx = 0;
	while ( idx < wsize )
	{
		ret = read(fd2, buf + idx, wsize - idx );
		if ( ret <= 0 )
		{
			break;
		}
		idx += ret;
	}
	if(memcmp(MSG,buf,sizeof(MSG)) != 0){
		sprintf(result," fail");
		LOG("test %s to %s %d %d %d %c %s",coma,comb, speed, databits, stopbits, parity,result);
	}else{
		sprintf(result," OK");
		printf("test %s to %s %d %d %d %c %s\n",coma,comb, speed, databits, stopbits, parity,result);
	}

	close(fd1);
	close(fd2);



	return 0;
}

int speedTest(char * coma,char * comb)
{
	int speedIdx, wordIdx, stopIdx, parityIdx;
	time_t old, new;

	old = time(NULL);
	for (speedIdx = 0; speedIdx < sizeof(baudlist) / sizeof(baudlist[0]);
			speedIdx++)
	{
		for (wordIdx = 0;
				wordIdx < sizeof(comDatabits) / sizeof(comDatabits[0]);
				wordIdx++)
		{
			for (stopIdx = 0;
					stopIdx < sizeof(comStopbits) / sizeof(comStopbits[0]);
					stopIdx++)
			{
				for (parityIdx = 0;
						parityIdx < sizeof(comParity) / sizeof(comParity[0]);
						parityIdx++)
				{
					struct BaudRate *rate = &baudlist[speedIdx];
					pairTest(coma, comb, rate->speed, comDatabits[wordIdx],
							comStopbits[stopIdx], comParity[parityIdx]);
					pairTest(comb, coma, rate->speed, comDatabits[wordIdx],
							comStopbits[stopIdx], comParity[parityIdx]);
				}
			}

		}

	}
	new = time(NULL);
	LOG("speedtest %s and %s cost %5ld seconds",coma,comb, new - old);

	return 0;
}

static void print_app_usage ( void )
{
	printf( "usage: [OPTIONS]\n");
	printf( "\n" );
	printf( "Valid options:\n" );
	printf( "  -speedtest coma comb     : traversal test speed ,databits,stopbits,parity\n" );
	printf( "  -looptest coma comb     : traversal test speed ,databits,stopbits,parity\n" );
	printf( "  example1: ./ComTest -speedtest /dev/ttyS0 /dev/ttyS1 \n" );
	printf( "  example2: ./ComTest -looptest /dev/ttyS0 /dev/ttyS1 \n" );

}

int main(int argc, char *argv[] )
{
	if( argc < 4)
	{
		print_app_usage();
		return 0;
	}
	if(argc > 1)
	{
		if(strcmp(argv[1], "-speedtest") == 0)
		{
			if( argc < 4)
			{
				print_app_usage();
				return 0;
			}
			speedTest(argv[2],argv[3]);
			return 0;
		}else if(strcmp(argv[1], "-looptest") == 0)
		{
			while(1){
				speedTest(argv[2],argv[3]);
			}
			return 0;
		}

	}
	else{
		print_app_usage();
		return 0;
	}
	return 0;
}





