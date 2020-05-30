
## memory

char *pJPEGBuf = new char[JPEG_DATA_LEN];
delete[] pJPEGBuf;

- if you didn't delete after use it, the memory goes higher and higher

### file: open and close

opendir() function!!!!

    if(NULL==opendir("data"))   //// bug bug
      mkdir("data",0775);

be sure to close it after all.


### malloc

char *recvData = (char*)malloc(MAX_BYTES);
/*  ..... */
memset(recvData, 0, sizeof(recvData));

警告:argument to 'sizeof' in 'void* memset(void*, int, size_t)' call is the same expression as the destination; did you mean to provide an explicit length? [-Wsizeof-pointer-memaccess]

### char : -128 -- 127

- when define IP address, be careful!



## Modbus
- iptable must enable to open PORT 1502 !
- setting must be loaded when startup






