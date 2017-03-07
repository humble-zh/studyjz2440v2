使用leds，熟悉应用程序字符设备驱动程序开发流程
直接在当前目录执行脚本 ./make.sh，即可同时编译应用程序(app)和驱动程序(drv)，并且应用程序(app/app)及驱动程序(drv/frame0_drv.ko)会被移动到网络文件系统的目录(/jz2440v2/fs_qtopia/)下面

随后在开发板终端使用命令测试：

lsmod

cat /proc/devices

ls /dev/led*

ls /sys/class/led*

insmod frame0_drv.ko

lsmod

cat /proc/devices

ls /dev/led*

ls /sys/class/led*

cat /sys/class/leds_c/leds/dev

./app

./app /dev/leds on

./app /dev/led1 off
