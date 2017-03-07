#!/bin/bash
#
# 执行该脚本，驱动程序以及应用程序生成的文件会被移动到NFS，然后在开发板挂载驱动程序，即可使用应用程序测试
#
PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/embedded/tools/gcc-3.4.5-glibc-2.3.6/bin"
export PATH

#Driver
make -C drv/
mv drv/*.ko /jz2440v2/fs_qtopia
make clean -C drv/

echo -e "\n\n\n"
#Application
make -C app/
mv app/app /jz2440v2/fs_qtopia
#make clean -C app/
