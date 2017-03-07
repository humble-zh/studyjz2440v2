/******************************************************************************
	使用buttons触发中断控制led灯，熟悉字符设备驱动中断控制的开发流程
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
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>

#define LED_MAJOR       0					/* major写0，自动分配设备号 */
#define DEVICE_NAME     "btn_int_leds"		/* 加载模式后，执行”cat /proc/devices”命令看到的设备名称 */
#define CLASS_NAME		"btn_int_leds_c"	/* 在/sys/ 下创建的类名 */
#define DEVICE_NODE		"buttons"			/* insmod后，在/dev/下生成的节点名称 */

int major = LED_MAJOR;
static struct class *btns_int_leds_class;
static struct class_device *btns_int_leds_class_dev;

static unsigned long gpio_va;
#define GPIO_OFT(x) ((x) - 0x56000000)
#define GPFCON  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000050)))
#define GPFDAT  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000054)))

#define GPGCON  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000060)))
#define GPGDAT  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000064)))

static volatile int ev_press = 0;/* 中断事件标志, 中断服务程序将它置1，buttons_int_leds_read将它清0 */
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

struct pin_desc{
	unsigned int pin;
	unsigned int key_val;
};
/* 键值: 按下时, 0x01, 0x02, 0x03, 0x04 */
/* 键值: 松开时, 0x81, 0x82, 0x83, 0x84 */
static unsigned char key_val;

struct pin_desc pins_desc[4] = {
	{S3C2410_GPF0, 0x01},
	{S3C2410_GPF2, 0x02},
	{S3C2410_GPG3, 0x03},
	{S3C2410_GPG11, 0x04},
};
//确定按键值
static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	struct pin_desc * pindesc = (struct pin_desc *)dev_id;
	unsigned int pinval;
	
	pinval = s3c2410_gpio_getpin(pindesc->pin);

	if (pinval){/* 松开 */
		key_val = 0x80 | pindesc->key_val;
	}
	else{/* 按下 */
		key_val = pindesc->key_val;
	}

    ev_press = 1;                  /* 表示中断发生了 */
    wake_up_interruptible(&button_waitq);   /* 唤醒休眠的进程 */

	return IRQ_RETVAL(IRQ_HANDLED);
}

static int buttons_int_leds_open(struct inode *inode,struct file *file)
{
	/* cat /proc/interrupts 可以查看请求得到的S2,S3,S4,S5 */
	request_irq(IRQ_EINT0,  buttons_irq, IRQT_BOTHEDGE, "S2", &pins_desc[0]);
	request_irq(IRQ_EINT2,  buttons_irq, IRQT_BOTHEDGE, "S3", &pins_desc[1]);
	request_irq(IRQ_EINT11, buttons_irq, IRQT_BOTHEDGE, "S4", &pins_desc[2]);
	request_irq(IRQ_EINT19, buttons_irq, IRQT_BOTHEDGE, "S5", &pins_desc[3]);
	return 0;
}

ssize_t buttons_int_leds_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	if (size != 1)
		return -EINVAL;

	/* 如果没有按键动作, 休眠 *//* 程序停在这里，一直不返回，但是CPU跑去执行其他的任务了 */
	wait_event_interruptible(button_waitq, ev_press);

	/* 如果有按键动作, 返回键值 */
	copy_to_user(buf, &key_val, 1);
	ev_press = 0;

	return 0;
}

int buttons_int_leds_close(struct inode *inode, struct file *file)
{
	free_irq(IRQ_EINT0, &pins_desc[0]);
	free_irq(IRQ_EINT2, &pins_desc[1]);
	free_irq(IRQ_EINT11, &pins_desc[2]);
	free_irq(IRQ_EINT19, &pins_desc[3]);
	return 0;
}

//数组chardev[]的第major项存放的结构体
static struct file_operations buttons_int_leds_fops = {
	.owner   = THIS_MODULE,
	.open    = buttons_int_leds_open,
	.read	 = buttons_int_leds_read,
	.release = buttons_int_leds_close,
};

int buttons_int_leds_init(void)
{
	//a.IO地址重映射
	//ioremap(起始物理地址，映射长度是多少字节)
	gpio_va = ioremap(0x56000000,0x100000);
	if (!gpio_va) {
		return -EIO;
	}
	
	//1.注册结构体(函数接口)
	//把结构体(fops)注册到数组chardev[]里面的第major项里面。如果register_chrdev第一个参数非0，创建成功就返回0，此时不应该把返回值赋给major
	major = register_chrdev(major,DEVICE_NAME,&buttons_int_leds_fops);
	printk("major:%u\n", major);
	if (major < 0) {
		printk(DEVICE_NAME " can't register major number\n");
		return major;
    }
	
	//2.在/sys目录下创建类
	btns_int_leds_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(btns_int_leds_class)){
		return PTR_ERR(btns_int_leds_class);
	}
	printk("Creat class '%s' succeed!\n", CLASS_NAME);
	
	//3.在类下创建设备文件(设备节点)
	//在CLASS_NAME这个类下面创建DEVICE_NODE设备，然后mdev就会创建/dev/DEVICE_NODE设备结点
	btns_int_leds_class_dev = class_device_create(btns_int_leds_class,NULL,MKDEV(major,0),NULL,DEVICE_NODE);

	printk("mknod \"/dev/%s \" succeed!\n", DEVICE_NODE);
	return 0;
}

void buttons_int_leds_exit(void)
{
	//1.注销结构体
	//出口函数，把字符驱动数组chardev[]里面的第major项卸载掉。
	unregister_chrdev(major,DEVICE_NAME);

	//2.注销类下的设备文件(节点)
	class_device_unregister(btns_int_leds_class_dev);
	
	//3.销毁类
	class_destroy(btns_int_leds_class);
	
	//a.IO地址取消重映射
	iounmap(gpio_va);
}


//module_init(x):定义一个结构体，结构体内部有一个函数指针指向函数x。当我们加载或安装一个驱动的时候，内核会找到结构体，用函数指针调用x函数，这个函数就把定义好的一个结构体(fops)告诉内核
module_init(buttons_int_leds_init);//指定执行insmod命令时调用的函数
module_exit(buttons_int_leds_exit);//指定执行rmmod命令时调用的函数

MODULE_AUTHOR("humble_zh");
MODULE_VERSION("0.0.1");
MODULE_DESCRIPTION("JZ2440v2 Buttons Interupt Leds Driver");
MODULE_LICENSE("GPL");



