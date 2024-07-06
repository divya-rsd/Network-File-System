#ifndef naming_H
#define naming_H
#define NAMING_SERVER_PORT 8080
#define MAX_STORAGE_SERVERS 1000
#define LOG_LEVEL 2 
#define PATH_MAX 1024
#define LOG_ERROR   LOG_ERR
#define LOG_INFO    LOG_INFO
#define LOG_DEBUG   LOG_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>

#include <stdarg.h>
#include <time.h>
struct StorageServerInfo
{
    char ip_address[16];
    int nm_port;
    int client_port;
    char accessible_paths[1024];
    char absolute_address[1024];
};
extern struct StorageServerInfo storageServerInfo;

struct ThreadArgs
{
    int socket;
    char request_type;
};
struct StorageServer
{
    struct StorageServerInfo info;
};
struct comclient
{
    int storageport;
    char command[10000];
};


struct Node {
    struct StorageServerInfo data;
    struct Node *next;
    struct Node *prev;
};
struct Node *head = NULL;
struct Node *tail = NULL;

struct comclient client_com[10];
int num_clients;
struct StorageServer storage_servers[MAX_STORAGE_SERVERS];
int num_storage_servers = 0;
void processStorageServerInfo(const struct StorageServerInfo *ss_info);
void *handleStorageServer(void *arg);
int findStorageServerPort(char *path, int *port);
int cacheSize = 5; // Set the desired cache size
void updateLRUCache(const struct StorageServerInfo *ss_info);
void displayLRUCache();

void *handleClient(void *arg);
int sendCommandToStorageServer(int storage_server_index, char command[]);
#define NFS_FILE_NOT_FOUND 1

#define COPY_FORMAT_INVALID 2
#endif