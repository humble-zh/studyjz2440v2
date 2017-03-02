/******************************************************************************
	使用leds，熟悉应用程序开发流程
		1.打开设备文件
		2.读写设备文件
	驱动入口：
		1.注册结构体
		2.创建类
		3.在类下创建设备文件(包括主和次设备文件)
	驱动出口：
		1.注销结构体
		2.注销类下的设备文件
		3.销毁类
	(入口初始化了什么，出口就恢复什么，注意顺序)
******************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include<unistd.h>

void usage(void);

int main(int argc,char **argv)
{
	int fd;
	char* filename;
	char val;
	if(3!=argc){
		usage();
		return 0;
	}
	filename = argv[1];
	/* open(),wirte()函数是由C库实现的，实际上执行了一条swi val指令引发异常（中断），跳到内
	   核异常处理函数（系统调用接口）。然后系统调用接口根据发生中断原因(val)调用不同处理函数。
	*/
	fd = open(filename,O_RDWR);
	if (fd < 0){
		printf("ERROR: %s open failed.\n",filename);
		return 0;
	}
	if(!strcmp("on",argv[2])){//点灯
		val=0;
		write(fd, &val, 1);
	}
	else if(!strcmp("off",argv[2])){//灭灯
		val=1;
		write(fd, &val, 1);
	}
	else{
		usage();
		return 0;
	}
	return 0;
}

void usage(void)
{
#define USAGE "\
Usage:\n\
  Please use command like this : \n\
    ./app /dev/led<1-3|s> <on|off>\n"
	printf(USAGE);
}


/*
VFS（虚拟文件系统）：sys_open,sys_read,sys_write 根据程序打开的文件的类型或者属性进行调用相应的驱动程序，例：open("/dev/led",O_RDWR)————led_open()
*/

