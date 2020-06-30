## 例子
1 cam249 192.168.0.249 9008 U249 123456 1
1 cam253 192.168.0.253 9008 U253 123456 1

1 cam249 192.168.0.249 9008 admin 123456 1
1 cam253 192.168.0.253 9008 admin 123456 1



## 文件编码转换

iconv -f GBK -t utf8 file1 -o file2


## 初始化时有这种输出

add a device 
The device (192.168.0.253:9008) has already existed.
../../DVR_NET_SDK/source/netservice/SWL_ListenProcEx.cpp, 225, pid = 6363
StartTryToConnect |getaddrinfo|ipv4=192.168.0.253,wDVRPort=9008,count=0
[../SDKDemo_linux/project/source/DeviceManager.cpp] line:1643  lLoginID:-1, deviceID=0,  deviceName=, localVideoInputNum=0, videoInputNum=0, videoOuputNum=0
[../SDKDemo_linux/project/source/DeviceManager.cpp] line:1666  NET_SDK_GetLastError:8,sDVRIP = 192.168.0.253
StartTryToConnect |getaddrinfo|ipv4=192.168.0.253,wDVRPort=9008,count=0
[../SDKDemo_linux/project/source/DeviceManager.cpp] line:1643  lLoginID:1, deviceID=0,  deviceName=IPC, localVideoInputNum=1, videoInputNum=1, videoOuputNum=1


## Linux例程有BUG
1. CaptureJPEGData拍照，经常出错，而且总是提示参数错误，重启了不一定好。。。
2. 调用一次GetNetTDvrDevice， 给个不在demo数据库中的ID做参数，程序会崩，FindNode: Assertion `FALSE' failed. Aborted (core dumped)
3. 在2之后，重启程序，就能拍照了。
4. NET_SDK_GetLastError给不出正确的指示，有时会返回错误码0（success）


## 写一个判断某个服务器关闭后，等待tcp端口释放完成，再启动的脚本

#!/bin/sh

while true

do

result=$(netstat -na | grep 5456 | wc -l)

if [ $result -gt 1 ]; then

echo "sorry,waiting for port release..."

sleep 2

else

./serverdemo &

   echo "start serverdemo!"

break;

   fi

done

exit 1

这个脚本通过循环检查端口，等待释放完了再启动服务器程序
