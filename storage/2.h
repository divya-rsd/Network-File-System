#ifndef storage_H
#define storage_H 
#define STORAGE_SERVER_PORT 8888
#define NAMING_SERVER_IP "127.0.0.1"
#define NAMING_SERVER_PORT 8080
#define MAX_COMMAND_SIZE 1024
#define ACCESSIBLE_PATHS_INTERVAL 60
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
#include<signal.h>
struct StorageServerInfo
{
    char ip_address[16];
    int nm_port;
    int client_port;
     char accessible_paths[1024];
       char absolute_address[1024];
 };
int num_clients;
int clientarr[100];
void collectAccessiblePaths(const char *dir_path, char *accessible_paths, int *pos, int size);
void createFile();
void createDirectory();
void sendStorageServerInfoToNamingServer(int ns_socket, const struct StorageServerInfo *ss_info);
void *sendInfoToNamingServer(void *arg);
void *receiveCommandsFromNamingServer(void *arg);
void *handleClientRequest(void *arg);
pthread_t clientread[100];
struct readclient
{
  int clienti;
  char clientpath[100];
};



#endif