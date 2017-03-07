/******************************************************************************
	使用leds，熟悉字符设备驱动开发流程
		定义读写结构体(函数接口)
	驱动入口：
		a.io映射
		1.注册结构体
		2.创建类
		3.在类下创建设备文件(包括主和次设备文件)
	驱动出口：
		1.注销结构体
		2.注销类下的设备文件
		3.销毁类
		a.取消io映射
	(入口初始化了什么，出口就恢复什么，注意顺序)
******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>

//#define LED_MAJOR		231			/* 手动分配主设备号，注意register_chrdev的调用 */
#define LED_MAJOR		0			/* major写0，自动分配设备号 */
#define DEVICE_NAME		"leds"		/* 加载模式后，执行”cat /proc/devices”命令看到的设备名称 */
#define CLASS_NAME		"leds_c"	/* 在/sys/ 下创建的类名 */

int major = LED_MAJOR;
static struct class *leds_class;
static struct class_device *leds_class_devs[4];

static unsigned long gpio_va;
#define GPIO_OFT(x) ((x) - 0x56000000)
#define GPFCON  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000050)))
#define GPFDAT  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000054)))

static int leds_open(struct inode *inode,struct file *file)
{
	//获取次设备号
	int minor = MINOR(inode->i_rdev);
	printk("In \'leds_open()\' func: \n\tmajor:%d\tminior:%d\n",major,minor);
	switch(minor){
	case 0: /* /dev/leds 配置GPF 4 5 6 引脚为输出引脚,并输出1 灭灯 */
		GPFCON &= ~((3<<(4*2)) | (3<<(5*2)) | (3<<(6*2)));
		GPFCON |=  ((1<<(4*2)) | (1<<(5*2)) | (1<<(6*2)));
		break;
	case 1: /* /dev/led1 配置GPF 4 引脚为输出引脚,并输出1 灭灯 */
		GPFCON &= ~(3<<(4*2));
		GPFCON |=  (1<<(4*2));
		break;
	case 2:/*  /dev/led2 配置GPF 5 引脚为输出引脚,并输出1 灭灯 */
		GPFCON &= ~(3<<(5*2));
		GPFCON |=  (1<<(5*2));
		break;
	case 3:/*  /dev/led3 配置GPF 6 引脚为输出引脚,并输出1 灭灯 */
		GPFCON &= ~(3<<(6*2));
		GPFCON |=  (1<<(6*2));
		break;
	}
	printk("Register GPFCON: 0x%x\n", (int)&GPFCON);
	return 0;
}

static ssize_t leds_write(struct file *file,const char __user *buf,size_t count,loff_t *ppos)
{
	//获取次设备号
	int minor = MINOR(file->f_dentry->d_inode->i_rdev);
	char val;
	//内核要用这个函数来获取用户传进来的buf数组，放到val里面，一共count个字节。相反的，copy_to_user();
	copy_from_user(&val, buf, 1);
	printk("In \'leds_write()\' func : \n    major:%d    minor:%d    val:%d\n",major,minor,val);
	switch(minor){
	case 0: // /dev/leds
		if(0==val) //点灯
			GPFDAT &= ~((1<<4) | (1<<5) | (1<<6));
		else		   //灭灯
			GPFDAT |= (1<<4) | (1<<5) | (1<<6); 
		break;
	case 1: // /dev/led1
		if(0==val) //点灯
			GPFDAT &= ~(1<<4);
		else		   //灭灯
			GPFDAT |= (1<<4); 
		break;
	case 2: // /dev/led1 
		if(0==val) //点灯
			GPFDAT &= ~(1<<5);
		else		   //灭灯
			GPFDAT |= (1<<5); 
		break;
	case 3: // /dev/led1
		if(0==val) //点灯
			GPFDAT &= ~(1<<6);
		else		   //灭灯
			GPFDAT |= (1<<6); 
		break;
	}
	printk("Register GPFDAT : 0x%x\n",(int)&GPFDAT);
	return 0;
}

//数组chardev[]的第major项存放的结构体
static struct file_operations leds_fops = {
	.owner = THIS_MODULE,
	.open  = leds_open,
	.write = leds_write,
};

int leds_init(void)
{
	int minor = 0;
	//a.IO地址重映射
	//ioremap(起始物理地址，映射长度是多少字节)
	gpio_va = ioremap(0x56000000,16);
	if (!gpio_va) {
		return -EIO;
	}
	
	//1.注册结构体(函数接口)
	//把结构体(fops)注册到数组chardev[]里面的第major项里面。如果register_chrdev第一个参数非0，创建成功就返回0，此时不应该把返回值赋给major
	major = register_chrdev(major,DEVICE_NAME,&leds_fops);
	printk("major:%u\n", major);
	if (major < 0) {
		printk(DEVICE_NAME " can't register major number\n");
		return major;
    }
	
	//2.在/sys目录下创建类：
	leds_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(leds_class)){
		return PTR_ERR(leds_class);
	}
	printk("Creat class '%s' succeed!\n", CLASS_NAME);
	
	//3.在类下创建设备文件(设备节点)
	//在这个类下面创建leds设备，然后mdev就会创建/dev/leds设备结点
	leds_class_devs[0] = class_device_create(leds_class,NULL,MKDEV(major,0),NULL,"leds");
	for(minor=1;minor<4;minor++)
	{
		leds_class_devs[minor] = class_device_create(leds_class,NULL,MKDEV(major,minor),NULL,"led%d",minor);
		if (unlikely(IS_ERR(leds_class_devs[minor]))){
			return PTR_ERR(leds_class_devs[minor]);
		}
	}
	printk("mknod \"/dev/leds and led1-3\" succeed!\n");
	return 0;
}

void leds_exit(void)
{
	int minor;
	//1.注销结构体
	//出口函数，把字符驱动数组chardev[]里面的第major项卸载掉。
	unregister_chrdev(major,DEVICE_NAME);

	//2.注销类下的设备文件(节点)
	for(minor=0;minor<4;minor++)
	{
		class_device_unregister(leds_class_devs[minor]);
	}
	
	//3.销毁类
	class_destroy(leds_class);
	
	printk("All leds exited!\n");
	
	//a.IO地址取消重映射
	iounmap(gpio_va);
}


//module_init(x):定义一个结构体，结构体内部有一个函数指针指向函数x。当我们加载或安装一个驱动的时候，内核会找到结构体，用函数指针调用x函数，这个函数就把定义好的一个结构体(fops)告诉内核
module_init(leds_init);//指定执行insmod命令时调用的函数
module_exit(leds_exit);//指定执行rmmod命令时调用的函数

MODULE_AUTHOR("humble_zh");
MODULE_VERSION("0.0.2");
MODULE_DESCRIPTION("JZ2440v2 LEDs Driver");
MODULE_LICENSE("GPL");



