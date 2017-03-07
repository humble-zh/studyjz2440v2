#使用buttons触发中断控制led灯，熟悉字符设备驱动中断控制的开发流程(待续)
直接在当前目录执行脚本 ./make.sh，即可同时编译应用程序(app)和驱动程序(drv)，并且应用程序(app/app)及驱动程序(drv/buttons_int_leds.ko)会被移动到网络文件系统的目录(/jz2440v2/fs_qtopia/)下面

随后在开发板终端使用命令测试：

lsmod

cat /proc/devices

cat /proc/interrupts

insmod buttons_int_leds.ko

lsmod

cat /proc/devices

cat /proc/interrupts

./app &

ps

#(按下按键即可在终端查看打印)

top
