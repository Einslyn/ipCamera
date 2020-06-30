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
#include "sockserver.h"

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

enum {
    TCP,
    TCP_PI,
    RTU
};

#define RUN_LIBMODBUS_BANDWITH_TEST  0
#define RUN_LIBMODBUS_UT_TEST  1
#define MODBUS_MODE_SELECT  TCP_PI

#define SERVER_IP "192.168.0.34"
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
    int i;
    int use_backend;
    uint8_t *query;
    int header_length;

    int master_socket;
    int rc;
    fd_set refset;
    fd_set rdset;
    /* Maximum file descriptor number */
    int fdmax;

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

                    #if 1
                       printf("reply:\n");
                       uint16_t *pbuf = mb_mapping->tab_registers;
                       uint16_t sData = 0x0002;
                       for(int i=0; i<UT_REGISTERS_NB_MAX; i++)
                       {
                           *pbuf++ = sData++;
                       }

                       dataSendTotal = 2;
                       ipaddr_int[0] = 2;
                       ipaddr_int[1] = 3;
                       num_of_persons[0] = 8;
                       num_of_persons[1] = 9;

                       // get data from socket thread
                       if(dataSendTotal > 0)
                       {
                           pbuf = mb_mapping->tab_registers;  //reset
                           uint16_t regAddressOffset = (ipaddr_int[0] & 0xFFFF - UT_REGISTERS_ADDRESS) % 65535;
                           for(int i = 0; i < dataSendTotal; i++)
                           {
                               *(pbuf + i + regAddressOffset) = num_of_persons[i] & 0xFFFF;
                               printf("registers[%d] : %d\n",i + regAddressOffset, pbuf[i + regAddressOffset]);
                           }

                           dataSendTotal = 0;
                       }
                    #endif

                    #if(RUN_LIBMODBUS_UT_TEST == 1)

                      /* Special server behavior to test client */
                      if (query[header_length] == 0x03) {
                          /* Read holding registers */

                          if (MODBUS_GET_INT16_FROM_INT8(query, header_length + 3)
                              == UT_REGISTERS_NB_SPECIAL) {
                              printf("Set an incorrect number of values\n");
                              MODBUS_SET_INT16_TO_INT8(query, header_length + 3,
                                                       UT_REGISTERS_NB_SPECIAL - 1);
                          } else if (MODBUS_GET_INT16_FROM_INT8(query, header_length + 1)
                                     == UT_REGISTERS_ADDRESS_SPECIAL) {
                              printf("Reply to this special register address by an exception\n");
                              modbus_reply_exception(ctx, query,
                                                     MODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY);
                              continue;
                          } else if (MODBUS_GET_INT16_FROM_INT8(query, header_length + 1)
                                     == UT_REGISTERS_ADDRESS_INVALID_TID_OR_SLAVE) {
                              const int RAW_REQ_LENGTH = 5;
                              uint8_t raw_req[] = {
                                  (use_backend == RTU) ? INVALID_SERVER_ID : 0xFF,
                                  0x03,
                                  0x02, 0x00, 0x00
                              };

                              printf("Reply with an invalid TID or slave\n");
                              modbus_send_raw_request(ctx, raw_req, RAW_REQ_LENGTH * sizeof(uint8_t));
                              continue;
                          } else if (MODBUS_GET_INT16_FROM_INT8(query, header_length + 1)
                                     == UT_REGISTERS_ADDRESS_SLEEP_500_MS) {
                              printf("Sleep 0.5 s before replying\n");
                              usleep(500000);
                          } else if (MODBUS_GET_INT16_FROM_INT8(query, header_length + 1)
                                     == UT_REGISTERS_ADDRESS_BYTE_SLEEP_5_MS) {
                              /* Test low level only available in TCP mode */
                              /* Catch the reply and send reply byte a byte */
                              uint8_t req[] = "\x00\x1C\x00\x00\x00\x05\xFF\x03\x02\x00\x00";
                              int req_length = 11;
                              int w_s = modbus_get_socket(ctx);
                              if (w_s == -1) {
                                  fprintf(stderr, "Unable to get a valid socket in special test\n");
                                  continue;
                              }

                              /* Copy TID */
                              req[1] = query[1];
                              for (i=0; i < req_length; i++) {
                                  printf("(%.2X)", req[i]);
                                  usleep(5000);
                                  rc = send(w_s, (const char*)(req + i), 1, MSG_NOSIGNAL);
                                  if (rc == -1) {
                                      break;
                                  }
                              }
                              continue;
                          }
                      }
                    #endif


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
