## cannot find .so? 程序和so放在同一个目录，但是仍然提示找不到so，解决方法

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./

ldconfig


## About the bin directory

### zlog.conf

set the log file format.

### aaa.log
open source lib: zlog-master

the log output file

size limit: 100MB

### setting.ini
open source lib: iniparser-master


### runpy.sh

call python to run another program.


### result.csv

save the IP and the numbers of persons.

size limit: 100MB


### sdkDemo

the main program to run.



## About the Project

### function
- get the snap JPG from IP cameras
- save JPG in path "data" 
- run the script runpy.sh to call Python program
- Python output numbers in file result.txt 
- All data of one day save in result.csv
- Using Modbus tcp transfer IP and numbers to the 3rd  system.

### change log

#### 2020.04.24 更新
1. 加入了Modbus线程, 默认使用TCP不间断运行
2. 完善了地址和人数数据到Modbus的寄存器的映射。
* BUG: modbus通信似乎会干扰 相机的login

#### 2020.04.30 update to v0.4

ssh update the bin.

ehl@PCEHL:~/Work/tvtCamera_save/Linux/SDKDemo_linux$ scp ../bin/sdkDemo dage@192.168.0.180:~/wuyi/tvtCamera_save/Linux/bin

ssh update the source code
ehl@PCEHL:~/Work/tvtCamera_save/Linux/SDKDemo_linux$ scp ./project/source/* dage@192.168.0.180:~/wuyi/tvtCamera_save/Linux/SDKDemo_linux/project/source



































