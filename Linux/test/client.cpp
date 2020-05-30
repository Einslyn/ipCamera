#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//
//compile:  g++ client.cpp -o client
//

#define MAX_BYTES   2048

//#define IP_ADDR   ("192.168.0.4")
#define IP_ADDR   ("127.0.0.1")
#define SOCKET_PORT  8888 //9734 //8888
#define NUM_CAMARAS   5


unsigned int IPtoInt(char *str_ip) 
{ 
    in_addr addr; 
    unsigned int int_ip;
    if(inet_aton(str_ip,&addr))
    {
        int_ip = ntohl(addr.s_addr);
    }
    return int_ip;
}

int main()
{
    int running_count = 0;
    char *sendData = (char *)malloc(MAX_BYTES);
	int ret=0;
	int len=0;
	char cmd = 0;

    while(1)
    {
      //创建套接字
      int sock = socket(AF_INET, SOCK_STREAM, 0);
      if( sock == -1)
      {
            printf("Error: socket\n");
            sleep(1);
            continue;
      }

      //向服务器（特定的IP和端口）发起请求
      struct sockaddr_in serv_addr;
      memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
      serv_addr.sin_family = AF_INET;  //使用IPv4地址
      serv_addr.sin_addr.s_addr = inet_addr(IP_ADDR);  //具体的IP地址
      serv_addr.sin_port = htons(SOCKET_PORT);  //端口

      if ( connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
      {
            printf("Error: connect\n");
            shutdown(sock,2);
            close(sock);
            sleep(1);
            continue;
      }

      while(1)
      {
			printf("running count: %d\n",running_count++);

            memset(sendData, 0, MAX_BYTES);
			sendData[0] = '#';
			sendData[1] = 'P';
			len = 2;
			ret = send(sock,sendData, len, 0);
            if(ret <= 0)
            {
                  printf("Error: send communication\n");
                  //关闭套接字
                  shutdown(sock,2);
                  close(sock);
                  break;
            }else
            {
                  printf("Sent: %d bytes.\n",ret);
                  printf("%s\n",sendData);
            }


			if(len > 0)
			{
				printf("recv:");
		        char recData[255];
		        ret = recv(sock, recData, 255, 0);
		        if(ret > 0)
		        {
		              recData[ret&0xFF] = 0;
		              printf("%s\n",recData);
		        }
		        else
		        {
		              break;
		        }
			}

            //printf("Enter g to get picture or others:");
            //cmd = getchar();
            if((cmd != 0) && (cmd != '\n'))
            {

				if('x' == cmd)
					exit(0);
                  getchar();
                  memset(sendData, 0, MAX_BYTES);
                  *(sendData+0) = cmd;
                  ret = send(sock,sendData, strlen(sendData), 0);
                  if(ret <= 0)
                  {
                        printf("Error: send communication\n");
                        //关闭套接字
                        shutdown(sock,2);
                        close(sock);
                        continue;
                  }else
                  {
                        printf("Sent: %d bytes.\n",ret);
                        printf("%s\n",sendData);
                  }
				cmd = 0;
            }
			
            //usleep(251000);
            sleep(10);
            
      }
      close(sock);
    }
      return 0;
}


