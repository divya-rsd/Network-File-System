#ifndef client_H
#define client_H
#define NAMING_SERVER_IP "127.0.0.1"
#define NAMING_SERVER_PORT 8080
#define STORAGE_SERVER_IP ""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include<ctype.h>
void communicateWithStorageServer(int storage_server_port);
#define NFS_FILE_NOT_FOUND 1

#define COPY_FORMAT_INVALID 2
#endif