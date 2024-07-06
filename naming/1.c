
#include "1.h"

pthread_mutex_t storage_servers_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to display the contents of the LRU cache
void displayLRUCache()
{
    struct Node *current = head;
    while (current != NULL)
    {
        printf("IP Address: %s, NM Port: %d, Client Port: %d\n", current->data.ip_address, current->data.nm_port,current->data.client_port);
        current = current->next;
    }
}


void updateLRUCache(const struct StorageServerInfo *ss_info)
{
    // Check if the storage server info is already in the cache
    struct Node *current = head;
    while (current != NULL)
    {
        if (strcmp(current->data.absolute_address, ss_info->absolute_address) == 0)
        {
            // Move the accessed node to the front of the list (MRU position)
            if (current != head)
            {
                if (current == tail)
                {
                    tail = current->prev;
                    tail->next = NULL;
                }
                else
                {
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                }

                current->next = head;
                current->prev = NULL;
                head->prev = current;
                head = current;
            }
            return; // Node found and updated
        }
        current = current->next;
    }

    // If the cache is full, remove the least recently used node (tail)
    if (head != NULL && cacheSize > 0 && cacheSize == num_storage_servers)
    {
        struct Node *temp = tail;
        tail = tail->prev;
        if (tail != NULL)
        {
            tail->next = NULL;
        }
        free(temp);
    }

    // Add the new storage server info to the front of the list
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->data = *ss_info;
    newNode->next = head;
    newNode->prev = NULL;

    if (head != NULL)
    {
        head->prev = newNode;
    }
    head = newNode;

    // If it's the first node, update the tail
    if (tail == NULL)
    {
        tail = head;
    }
}


void logMessage(int priority, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsyslog(priority, format, args);
    va_end(args);
}


void copyFile(const char *source, const char *destination)
{
    FILE *source_file = fopen(source, "rb");
    if (!source_file)
    {
        perror("Error opening source file");
        return;
    }
    FILE *dest_file = fopen(destination, "wb");
    if (!dest_file)
    {
        perror("Error opening destination file");
        fclose(source_file);
        return;
    }

    char buffer[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
    {
        if (fwrite(buffer, 1, bytesRead, dest_file) != bytesRead)
        {
            perror("Error writing to destination file");
            break;
        }
    }
    fclose(source_file);
    fclose(dest_file);
}



void copyDirectory(const char* source, const char* destination) {
    DIR* dir;
    struct dirent* entry;
    struct stat statbuf;
  char sourcePath[512];
char destPath[512];

    dir = opendir(source);
    if (dir == NULL) {
        printf("Failed to open source directory.\n");
        return;
    }
    if (mkdir(destination, 0777) != 0 && errno != EEXIST) {
        printf("Failed to create destination directory.\n");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(sourcePath, sizeof(sourcePath), "%s/%s", source, entry->d_name);
        snprintf(destPath, sizeof(destPath), "%s/%s", destination, entry->d_name);
        if (stat(sourcePath, &statbuf) != 0) {
            printf("Failed to get file/directory information.\n");
            closedir(dir);
            return;
        }
        if (S_ISREG(statbuf.st_mode)) {
            FILE* sourceFile = fopen(sourcePath, "rb");
            FILE* destFile = fopen(destPath, "wb");
            if (sourceFile == NULL || destFile == NULL) {
                printf("Failed to open file for copying.\n");
                closedir(dir);
                return;
            }

            char buffer[1024];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
                fwrite(buffer, 1, bytesRead, destFile);
            }

            fclose(sourceFile);
            fclose(destFile);
        }
        else if (S_ISDIR(statbuf.st_mode)) {
            copyDirectory(sourcePath, destPath);
        }
    }

    closedir(dir);
   
}

void processStorageServerInfo(const struct StorageServerInfo *ss_info)
{
    pthread_mutex_lock(&storage_servers_mutex);

    if (num_storage_servers < MAX_STORAGE_SERVERS)
    {
        memcpy(&storage_servers[num_storage_servers].info, ss_info, sizeof(struct StorageServerInfo));
        num_storage_servers++;
        /*printf("Received information for SS:\n");
        printf("IP Address: %s\n", ss_info->ip_address);
        printf("NM Port: %d\n", ss_info->nm_port);
        printf("Client Port: %d\n", ss_info->client_port);
        printf("Address of ss: %s\n", ss_info->absolute_address);
        printf("Accessible Paths:\n%s\n", ss_info->accessible_paths);*/
    }
    else
    {
        printf("Storage server array is full. Cannot store information for SS: %s:%d\n", ss_info->ip_address, ss_info->nm_port);
    }
 //displayLRUCache();
    pthread_mutex_unlock(&storage_servers_mutex);
}

void *handleStorageServer(void *arg)
{
    struct ThreadArgs *thread_args = (struct ThreadArgs *)arg;
    char request_type = thread_args->request_type;
    int ss_socket = thread_args->socket;

    // Receive storage server information
    struct StorageServerInfo ss_info;
    if (recv(ss_socket, &ss_info, sizeof(ss_info), 0) <= 0)
    {
        perror("Receiving storage server info failed");
        close(ss_socket);
        free(thread_args);
        pthread_exit(NULL);
    }
    logMessage(LOG_INFO, "Received storage server information from IP: %s, Port: %d",
               ss_info.ip_address, ss_info.nm_port);
    processStorageServerInfo(&ss_info);
    int count = 0;
    while (count != -1)
    {
        for (int i = 0; i < num_clients; i++)
        {
            if (ss_info.client_port == client_com[i].storageport)
            {
                count = 1;
                client_com[i].storageport = -1;
                logMessage(LOG_INFO, "Received information for SS: IP Address: %s, NM Port: %d, Client Port: %d, Absolute Address: %s",
                           ss_info.ip_address, ss_info.nm_port, ss_info.client_port, ss_info.absolute_address);
                if (send(ss_socket, &i, sizeof(i), 0) <= 0)
                {
                    perror("Receiving storage server info failed");
                    close(ss_socket);
                    free(thread_args);
                    pthread_exit(NULL);
                }
                if (send(ss_socket, client_com[i].command, sizeof(client_com[i].command), 0) <= 0)
                {
                    perror("Receiving storage server info failed");
                    close(ss_socket);
                    free(thread_args);
                    pthread_exit(NULL);
                }

            }
        }
       
        sleep(1);
    }
    free(thread_args);
    pthread_exit(NULL);
}

void *handleClient(void *arg)
{
    struct ThreadArgs *thread_args = (struct ThreadArgs *)arg;
    int client_socket = thread_args->socket;
    char request_type = thread_args->request_type;

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    char a[1024];
    strcpy(a, buffer);
    char command[50];
    char path[974];
    char *token = strtok(a, " ");
    int storage_server_port = -1;
    strcpy(command, token);
    if (strcmp(command, "COPY") == 0)
    {
        char source[10000];
        char destination[100000];
        if (sscanf(buffer, "%s %s %s", command, source, destination) != 3)
        {
            perror("Invalid COPY command format");
            close(client_socket);
            free(thread_args);
            pthread_exit(NULL);
        }
        int source_storage_port = -1;
        if (findStorageServerPort(source, &source_storage_port))
        {
            int destination_storage_port = -1;
            if (findStorageServerPort(destination, &destination_storage_port))
            {
                copyFile(source, destination);
                printf("File copied from %s to %s\n", source, destination);
                storage_server_port = 0;
                 logMessage(LOG_INFO, "File copied from %s to %s", source, destination);
            }
        }
    }
    else if (strcmp(command,"COPYDIR") == 0){
        char source[10000];
        char destination[100000];
        if (sscanf(buffer, "%s %s %s", command, source, destination) != 3)
        {
            perror("Invalid COPY command format");
            close(client_socket);
            free(thread_args);
            pthread_exit(NULL);
        }
        int source_storage_port = -1;
        if (findStorageServerPort(source, &source_storage_port))
        {
            int destination_storage_port = -1;
            if (findStorageServerPort(destination, &destination_storage_port))
            {
                copyDirectory(source, destination);
                 printf("Directory copied from %s to %s\n", source, destination);
                storage_server_port = 0;
                 logMessage(LOG_INFO, "Directory copied from %s to %s", source, destination);
            }
        }
    }
    else
    {
        while (token != NULL)
        {
            strcpy(path, token);
            printf("%s\n", path);
            token = strtok(NULL, " ");
        }

        if (bytes_received <= 0)
        {
            perror("Receiving data from client failed");
            close(client_socket);
            free(thread_args);
            pthread_exit(NULL);
        }

        printf("Received data from client: %s\n", buffer);

        strcpy(client_com[num_clients].command, buffer);
        printf("Command from client: %s\n", command);
        printf("Path from client: %s\n", path);

        if (findStorageServerPort(path, &storage_server_port))
        {
            printf("Client requested path: %s\n", path);
            printf("Found storage server port: %d\n", storage_server_port);

              // Log client request information
            logMessage(LOG_INFO, "Client requested path: %s", path);
            logMessage(LOG_INFO, "Found storage server port: %d", storage_server_port);
            client_com[num_clients].storageport = storage_server_port;
            // Send the storage server port back to the client
            if (send(client_socket, &storage_server_port, sizeof(storage_server_port), 0) == -1)
            {
                perror("Sending storage server port to client failed");
            }
            num_clients++;
        }
        else
        {
            printf("Client requested path: %s\n", path);
            printf("Path not found in accessible paths\n");

              logMessage(LOG_INFO, "Path not found in accessible paths for client request: %s", path);
            storage_server_port = -1;
            if (send(client_socket, &storage_server_port, sizeof(storage_server_port), 0) == -1)
            {
                perror("Sending error message to client failed");
            }
            num_clients++;
        }
    }

    close(client_socket);
    free(thread_args);

    pthread_exit(NULL);
}

int findStorageServerPort(char *path, int *port)
{
    pthread_mutex_lock(&storage_servers_mutex);

    // Check if the storage server info is already in the cache
    struct Node *current = head;
    while (current != NULL)
    {
        int len=strlen(current->data.absolute_address);
        if (strncmp(current->data.absolute_address, path,len) == 0)
        {
            *port = current->data.client_port;
            // Move the accessed node to the front of the list (MRU position)
            if (current != head)
            {
                if (current == tail)
                {
                    tail = current->prev;
                    tail->next = NULL;
                }
                else
                {
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                }

                current->next = head;
                current->prev = NULL;
                head->prev = current;
                head = current;
            }

            pthread_mutex_unlock(&storage_servers_mutex);
            return 1; 
        }
        current = current->next;
    }

    for (int i = 0; i < num_storage_servers; i++)
    {
        int l = strlen(storage_servers[i].info.absolute_address);
        if (strncmp(storage_servers[i].info.absolute_address, path, l) == 0)
        {
            strcpy(path, path + l);
            char newPath[strlen(path) + 2]; // +2 for the dot and null terminator
            strcpy(newPath, ".");
            strcat(newPath, path);
            strcpy(path, newPath);
            if (strstr(storage_servers[i].info.accessible_paths, path) != NULL)
            {
                *port = storage_servers[i].info.client_port;
                pthread_mutex_unlock(&storage_servers_mutex);
                updateLRUCache(&storage_servers[i].info);
                return 1; // Path found, cache updated
            }
        }
    }

    pthread_mutex_unlock(&storage_servers_mutex);
    return 0; // Path not found
}

int sendCommandToStorageServer(int storage_server_port, char command[])
{

    int ss_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_socket == -1)
    {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in ss_address;
    ss_address.sin_family = AF_INET;
    ss_address.sin_port = htons(storage_servers[0].info.nm_port);
    ss_address.sin_addr.s_addr = inet_addr(storage_servers[0].info.ip_address);

    if (connect(ss_socket, (struct sockaddr *)&ss_address, sizeof(ss_address)) == -1)
    {
        perror("Connection to the storage server failed");
        close(ss_socket);
        return -1;
    }

    // Send the length of the command first
    size_t command_length = strlen(command);
    if (send(ss_socket, &command_length, sizeof(command_length), 0) == -1)
    {
        perror("Sending command length to storage server failed");
        close(ss_socket);
        return -1;
    }

    // Send the command string
    if (send(ss_socket, command, command_length, 0) == -1)
    {
        perror("Sending command to storage server failed");
        close(ss_socket);
        return -1;
    }

    close(ss_socket);
    return 0;
}

int main()
{

    openlog("NamingServer", LOG_PID, LOG_USER);
    int ns_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ns_socket == -1)
    {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in ns_address;
    ns_address.sin_family = AF_INET;
    ns_address.sin_port = htons(NAMING_SERVER_PORT);
    ns_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(ns_socket, (struct sockaddr *)&ns_address, sizeof(ns_address)) == -1)
    {
        perror("Bind failed");
        close(ns_socket);
        exit(1);
    }

    if (listen(ns_socket, 5) == -1)
    {
        perror("Listen failed");
        close(ns_socket);
        exit(1);
    }

    while (1)
    {
        int client_socket = accept(ns_socket, NULL, NULL);
        if (client_socket == -1)
        {
            perror("Accepting client connection failed");
            continue; // Continue to the next iteration to keep listening
        }

        char request_type;
        if (recv(client_socket, &request_type, sizeof(request_type), 0) <= 0)
        {
            perror("Receiving request type from client failed");
            close(client_socket);
            continue;
        }

        struct ThreadArgs *thread_args = malloc(sizeof(struct ThreadArgs));
        thread_args->socket = client_socket;
        thread_args->request_type = request_type;

        // Create a thread based on the request type
        pthread_t thread;
        if (request_type == 'I')
        {
            if (pthread_create(&thread, NULL, handleStorageServer, thread_args) != 0)
            {
                perror("Failed to create storage server thread");
                free(thread_args);
                close(client_socket);
            }
        }
        else if (request_type == 'P')
        {
            if (pthread_create(&thread, NULL, handleClient, thread_args) != 0)
            {
                perror("Failed to create client thread");
                free(thread_args);
                close(client_socket);
            }
        }
        else
        {
            free(thread_args);
            close(client_socket);
        }
    }
      closelog();
    close(ns_socket);
    return 0;
}
