//#include "usart.h"
#include<stdio.h>      /*标准输入输出定义*/    
#include<stdlib.h>     /*标准函数库定义*/    
#include<unistd.h>     /*Unix 标准函数定义*/    
#include<sys/types.h>     
#include<sys/stat.h>       
#include<fcntl.h>      /*文件控制定义*/    
#include<termios.h>    /*PPSIX 终端控制定义*/    
#include<errno.h>      /*错误号定义*/    
#include<string.h>

#include <pthread.h>
#include "camera.h"
#include "common.h"

#include <signal.h>

#include <modbus.h>
#ifdef _WIN32
# include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/* For MinGW */
#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

#include "unit-test.h"
#include <time.h>

enum {
    TCP,
    TCP_PI,
    RTU
};

#define MODBUS_TEST     1
#define RUN_LIBMODBUS_BANDWITH_TEST  0
#define RUN_LIBMODBUS_UT_TEST  1
#define MODBUS_MODE_SELECT  TCP

#define SERVER_IP_DEFAULT "127.0.0.1"
#define SERVER_IP_AUTO "::0"
#define COM_PORT "/dev/ttyUSB0"

#define BUG_REPORT(_cond, _format, _args ...) \
    printf("\nLine %d: assertion error for '%s': " _format "\n", __LINE__, # _cond, ## _args)

#define ASSERT_TRUE(_cond, _format, __args...) {  \
    if (_cond) {                                  \
        printf("OK\n");                           \
    } else {                                      \
        BUG_REPORT(_cond, _format, ## __args);    \
        goto close;                               \
    }                                             \
};


#define NB_CONNECTION    5

static modbus_t *ctx = NULL;
static modbus_mapping_t *mb_mapping;

static int server_socket = -1;

static void close_sigint(int dummy)
{
    if (server_socket != -1) {
        close(server_socket);
    }
    modbus_free(ctx);
    modbus_mapping_free(mb_mapping);

    exit(dummy);
}


void *modbus_thread(void *argv)
{
   //int s = -1;
    //modbus_t *ctx;
    //modbus_mapping_t *mb_mapping;
    //int rc;
    int Test_count = 0;
    volatile int minute_last = 0;
    int i;
    int use_backend;
    uint8_t *query;
    int header_length=0;

    int master_socket;
    int rc;
    fd_set refset;
    fd_set rdset;
    /* Maximum file descriptor number */
    int fdmax;

    char *SERVER_IP = (char*)malloc(255);
    strcpy(SERVER_IP, SERVER_IP_DEFAULT);

    // print date and time
    time_t now = time(0);
    char *dt = ctime(&now);
    printf("\nModbus thread: local date and time: %s\n",dt);
    tm *ltm = localtime(&now);

    if (use_backend == TCP) 
        printf("Modbus RTU not support.\n");

    if(argv != NULL)
    {
        strcpy(SERVER_IP, (char*)argv);
    }
    printf("Modbus TCP SERVER_IP = %s\n",SERVER_IP);


    use_backend = MODBUS_MODE_SELECT; // default

    if (use_backend == TCP) {
        ctx = modbus_new_tcp(SERVER_IP, 1502);
        query = (uint8_t *)malloc(MODBUS_TCP_MAX_ADU_LENGTH);
    } else if (use_backend == TCP_PI) {
        ctx = modbus_new_tcp_pi(SERVER_IP_AUTO, "1502");
        query = (uint8_t *)malloc(MODBUS_TCP_MAX_ADU_LENGTH);
    } else {
        ctx = modbus_new_rtu(COM_PORT, 115200, 'N', 8, 1);
        modbus_set_slave(ctx, SERVER_ID);
        query = (uint8_t *)malloc(MODBUS_RTU_MAX_ADU_LENGTH);
    }
    header_length = modbus_get_header_length(ctx);


    modbus_set_debug(ctx, TRUE);

#if (RUN_LIBMODBUS_BANDWITH_TEST == 1)
    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0,
                                    MODBUS_MAX_READ_REGISTERS, 0);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        pthread_exit(NULL);
    }
#else
    mb_mapping = modbus_mapping_new_start_address(
        UT_BITS_ADDRESS, UT_BITS_NB,
        UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
        UT_REGISTERS_ADDRESS, UT_REGISTERS_NB_MAX,
        UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        pthread_exit(NULL);
    }

    /* Examples from PI_MODBUS_300.pdf.
       Only the read-only input values are assigned. */

    /* Initialize input values that's can be only done server side. */
    modbus_set_bits_from_bytes(mb_mapping->tab_input_bits, 0, UT_INPUT_BITS_NB,
                               UT_INPUT_BITS_TAB);

    /* Initialize values of INPUT REGISTERS */
    for (i=0; i < UT_INPUT_REGISTERS_NB; i++) {
        mb_mapping->tab_input_registers[i] = UT_INPUT_REGISTERS_TAB[i];;
    }

    /* Initialize values of INPUT REGISTERS */  // 2020-04-23
    for (i=0; i < UT_REGISTERS_NB_MAX; i++) {
        mb_mapping->tab_registers[i] = 0;
    }
#endif

    printf("** modbus listen... **\n");
    if (use_backend == TCP) {
        server_socket = modbus_tcp_listen(ctx, 1);
        if (server_socket == -1) {
            fprintf(stderr, "Unable to listen TCP connection\n");
            modbus_free(ctx);
            pthread_exit(NULL);
        }
        printf("server_socket %d\n", server_socket);
    } else if (use_backend == TCP_PI) {
        server_socket = modbus_tcp_pi_listen(ctx, 1);
    } else {
        rc = modbus_connect(ctx);
        if (rc == -1) {
            fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            pthread_exit(NULL);
        }
    }

    signal(SIGINT, close_sigint);

    /* Clear the reference set of socket */
    FD_ZERO(&refset);
    /* Add the server socket */
    FD_SET(server_socket, &refset);

    /* Keep track of the max file descriptor */
    fdmax = server_socket;

    printf("fdmax %d\n", fdmax);

    for (;;) {
        rdset = refset;
        if (select(fdmax+1, &rdset, NULL, NULL, NULL) == -1) {
            perror("Server select() failure.");
            close_sigint(1);
        }

        #if (MODBUS_TEST  == 1)
            if(0xFF == dataSendTotal)
            {
                // print date and time
                now = time(0);
                ltm = localtime(&now);
                dt = ctime(&now);
                printf("\nModbus thread ltm->tm_min = %d, local date and time: %s\n", ltm->tm_min, dt);
                if(ltm->tm_min != minute_last) 
                {
                    minute_last = ltm->tm_min;
                    printf("\nModbus thread local date and time: %s\n",dt);
                    printf("Modbus thread To update All registers with random data................................\n");
                    // generate random data for testing
                    for(int i = IP_VALID_START; i < IP_VALID_END+1; i++)
                    {
                        ipaddr_int[i] = i;
                        num_of_persons[i] = (uint16_t) (500.0 * rand() / (RAND_MAX + 1.0));
                    }
                    num_of_persons[IP_VALID_START] = 1111;
                    num_of_persons[IP_VALID_END] = 9999;
                }
            }
        #endif

        /* Run through the existing connections looking for data to be
         * read */
        for (master_socket = 0; master_socket <= fdmax; master_socket++) {

            if (!FD_ISSET(master_socket, &rdset)) {
                continue;
            }

            if (master_socket == server_socket) {
                /* A client is asking a new connection */
                socklen_t addrlen;
                struct sockaddr_in clientaddr;
                int newfd;

                /* Handle new connections */
                addrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, sizeof(clientaddr));
                newfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);
                if (newfd == -1) {
                    perror("Server accept() error");
                } else {
                    FD_SET(newfd, &refset);

                    if (newfd > fdmax) {
                        /* Keep track of the maximum */
                        fdmax = newfd;
                    }
                    printf("New connection from %s:%d on socket %d\n",
                           inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
                }
            } else {
                modbus_set_socket(ctx, master_socket);
                rc = modbus_receive(ctx, query);
                if (rc > 0) {
                    // print date and time
                    now = time(0);
                    dt = ctime(&now);
                    printf("Modbus local date and time: %s\n",dt);
                    printf("modbus_receive count: %d \n",Test_count++);
                    printf("Modbus server_socket(%d)fdmax(%d)  received data on socket %d \n",server_socket,fdmax, master_socket);

                    printf("reply:\n");

                    // get data from socket thread
                    if(dataSendTotal > 0)
                    {
                        uint16_t regAddressOffset = ipaddr_int[IP_VALID_START] & 0xFF;  // not larger than 255
                        if((regAddressOffset > 255)  || (regAddressOffset < 2))
                            printf("Error: regAddressOffset=%d , address should be 2 ~ 255\n",regAddressOffset);
                        else
                        {
                            printf("Modbus update registers [%d] -> [%d] values(hex):\n",IP_VALID_START,IP_VALID_END);
                        }
                        for(int i = IP_VALID_START; i < IP_VALID_END+1; i++)
                        {
                            mb_mapping->tab_registers[i] = (uint16_t)(num_of_persons[i] & 0xFFFF);
                            printf("%04x\t",mb_mapping->tab_registers[i]);
                        }
                        printf("\n");
                    }

                    rc = modbus_reply(ctx, query, rc, mb_mapping);
                    if (rc == -1) {
                        break;
                    }
                    printf("modbus_reply completed.\n\n");


                } else if (rc == -1) {
                    /* This example server in ended on connection closing or
                     * any errors. */
                    printf("Connection closed on socket %d\n", master_socket);
                    close(master_socket);

                    /* Remove from reference set */
                    FD_CLR(master_socket, &refset);

                    if (master_socket == fdmax) {
                        fdmax--;
                    }
                }
            }
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));


    /* Close the connection */
    if (use_backend == TCP) {
        if (server_socket != -1) {
            close(server_socket);
        }
    }
    modbus_mapping_free(mb_mapping);
    free(query);
    /* For RTU */
    modbus_close(ctx);
    modbus_free(ctx);

    pthread_exit(NULL);
}    
