#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define IOC_MAGIC 'p'
#define MY_IOCTL_LEN _IO(IOC_MAGIC, 1)
#define MY_IOCTL_AVAIL _IO(IOC_MAGIC, 2)
#define MY_IOCTL_RESET _IO(IOC_MAGIC, 3)

int main()
{
	int ret,fd;
	fd = open("/dev/psample 0", O_RDWR);
 	if(fd<0) {
		perror("open");
		exit(1);
	}
	/*char str[]="Hello World, Hi";
	int len=strlen(str);
	nbytes=write(fd,str,len);
	if(nbytes<0) {
		 perror("write");
	}
	char buf[64];
	int maxlen=64;
	nbytes=read(fd,buf,maxlen);*/
	ret = ioctl(fd, MY_IOCTL_LEN);
	if(ret<0) {
		perror("ioctl");
		exit(3);
	}
	ret = ioctl(fd, MY_IOCTL_AVAIL);
	if(ret<0)
	{
		perror("ioctl");
		exit(3);
	}
	ret = ioctl(fd, MY_IOCTL_RESET);
	if(ret<0) {
		perror("ioctl");
		exit(3);
	}
	close(fd);
	return 0;
}
