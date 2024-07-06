#include "2.h"

// Function to recursively collect accessible paths
void collectAccessiblePaths(const char *dir_path, char *accessible_paths, int *pos, int size)
{
    DIR *dir = opendir(dir_path);
    if (!dir)
    {
        perror("Opening directory failed");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG || entry->d_type == DT_DIR)
        {
            struct stat statbuf;
            char full_path[512]; // Adjust this size as needed
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

            if (stat(full_path, &statbuf) == 0)
            {
                if (S_ISDIR(statbuf.st_mode))
                {
                    // It's a directory
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                    {
                        // Skip current and parent directory entries
                        int len = snprintf(accessible_paths + (*pos), size - (*pos), "%s\n", full_path);
                        (*pos) += len;
                        collectAccessiblePaths(full_path, accessible_paths, pos, size);
                    }
                }
                else if (S_ISREG(statbuf.st_mode))
                {
                    // It's a regular file
                    int len = snprintf(accessible_paths + (*pos), size - (*pos), "%s\n", full_path);
                    (*pos) += len;
                }
            }
        }
    }

    closedir(dir);
}

void createFile(const char *file_path)
{
    FILE *file = fopen(file_path, "w");
    if (file == NULL)
    {
        perror("File creation failed");
        return;
    }
    fclose(file);
    printf("Empty file created: %s\n", file_path);
}


void createDirectory()
{
    if (mkdir("new_directory", 0777) == -1)
    {
        perror("Directory creation failed");
    }
    printf("Empty directory created: new_directory\n");
}

void deleteFile(const char *file_path)
{
    struct stat path_stat;
    if (stat(file_path, &path_stat) == 0)
    {
        if (S_ISREG(path_stat.st_mode))
        {
            FILE *file = fopen(file_path, "r");
            if (file)
            {
                fclose(file);

                if (remove(file_path) == 0)
                {
                    printf("File deleted: %s\n", file_path);
                }
                else
                {
                    perror("File deletion failed");
                }
            }
            else
            {
                printf("Failed to open file: %s\n", file_path);
            }
        }
        else if (S_ISDIR(path_stat.st_mode))
        {
            printf("%s is a directory. Not deleting.\n", file_path);
        }
        else
        {
            printf("%s is not a regular file or directory.\n", file_path);
        }
    }
    else
    {
        perror("Error getting file/directory information");
    }
}

void deleteDirectory(const char *dir_path)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(dir_path);
    if (!dir)
    {
        perror("Unable to open directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char entry_path[PATH_MAX];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", dir_path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            deleteDirectory(entry_path);
        }
        else if (entry->d_type == DT_REG)
        {
            deleteFile(entry_path);
        }
    }

    closedir(dir);
    if (rmdir(dir_path) == 0)
    {
        printf("Directory deleted: %s\n", dir_path);
    }
    else
    {
        perror("Directory deletion failed");
    }
}
void readfile(struct readclient *CM)
{   printf("%s",CM->clientpath);
     int i=CM->clienti;
    char path[100];
    strcpy(path,CM->clientpath);
    printf("%s",path);
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror("Unable to open file");
        return;
    }

    char buffer[8];
    // Read and send file in lines
    
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // Send the buffer over the network
        if (send(clientarr[i], buffer, strlen(buffer), 0) == -1) {
            perror("Sending file content failed");
            fclose(file);
            close(clientarr[i]);
            return;
        }
    }

    // Send a completion message
    const char *completionMessage = "STOP";
    if (send(clientarr[i], completionMessage, strlen(completionMessage), 0) == -1) {
        perror("Sending completion message failed");
    }

    // Close the file
    fclose(file);
    close(clientarr[i]);
}
void removestrings(char *str, char *sub)
{
    size_t len = strlen(sub);

    while ((str = strstr(str, sub)) != NULL)
    {
        memmove(str, str + len, strlen(str + len) + 1);
    }
}

void trim(char *str)
{
    while (isspace((unsigned char)*str))
    {
        str++;
    }
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
    {
        len--;
    }
    str[len] = '\0';
}
void writefile(char *content, char *path)
{
    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Unable to open file: %s\n", path);
        return;
    }
    fprintf(file, "%s\n", content);

    fclose(file);
}
void getper(int i, char *path)
{
    struct stat fileInfo;
    if (stat(path, &fileInfo) != 0)
    {
        perror("Error in stat");
        return;
    }
    char fileSizeString[100];
    sprintf(fileSizeString, "File Size: %lld bytes\n", (long long)fileInfo.st_size);
    char filePermissionsString[100];
    sprintf(filePermissionsString, "File Permissions: %s%s%s%s%s%s%s%s%s%s\n",
            (S_ISDIR(fileInfo.st_mode)) ? "d" : "-",
            (fileInfo.st_mode & S_IRUSR) ? "r" : "-",
            (fileInfo.st_mode & S_IWUSR) ? "w" : "-",
            (fileInfo.st_mode & S_IXUSR) ? "x" : "-",
            (fileInfo.st_mode & S_IRGRP) ? "r" : "-",
            (fileInfo.st_mode & S_IWGRP) ? "w" : "-",
            (fileInfo.st_mode & S_IXGRP) ? "x" : "-",
            (fileInfo.st_mode & S_IROTH) ? "r" : "-",
            (fileInfo.st_mode & S_IWOTH) ? "w" : "-",
            (fileInfo.st_mode & S_IXOTH) ? "x" : "-");

    if (send(clientarr[i], fileSizeString, strlen(fileSizeString), 0) == -1)
    {
        perror("Sending completion message failed");
    }
    if (send(clientarr[i], filePermissionsString, strlen(filePermissionsString), 0) == -1)
    {
        perror("Sending completion message failed");
    }
}
void sendStorageServerInfoToNamingServer(int ns_socket, const struct StorageServerInfo *ss_info)
{
    char request_type = 'I';
    if (send(ns_socket, &request_type, sizeof(request_type), 0) == -1)
    {
        perror("Sending request type to naming server failed");
        close(ns_socket);
        exit(1);
    }
    sleep(1);
    if (send(ns_socket, ss_info, sizeof(struct StorageServerInfo), 0) == -1)
    {
        perror("Sending storage server info to naming server failed");
        close(ns_socket);
        exit(1);
    }
}

void *sendInfoToNamingServer(void *arg)
{
    int ns_socket = *((int *)arg);
    char accessible_paths[4096];
    int current_pos = 0;
    collectAccessiblePaths(".", accessible_paths, &current_pos, sizeof(accessible_paths));
    accessible_paths[current_pos] = '\0';
    struct StorageServerInfo ss_info;
    strcpy(ss_info.ip_address, "127.0.0.1");
    ss_info.nm_port = NAMING_SERVER_PORT;
    ss_info.client_port = STORAGE_SERVER_PORT;
    strcpy(ss_info.accessible_paths, accessible_paths);
    if (getcwd(ss_info.absolute_address, sizeof(ss_info.absolute_address)) == NULL)
    {
        perror("Getting current absolute address failed");
        exit(1);
    }
    printf("%s\n", ss_info.absolute_address);
    char request_type = 'I';
    if (send(ns_socket, &request_type, sizeof(request_type), 0) == -1)
    {
        perror("Error sending request type to naming server");
        close(ns_socket);
        exit(1);
    }

    printf("Request type sent to naming server\n");

    if (send(ns_socket, &ss_info, sizeof(ss_info), 0) == -1)
    {
        perror("Sending storage server info to naming server failed");
        close(ns_socket);
        exit(1);
    }

    printf("Storage server information sent to naming server\n");

    pthread_exit(NULL);
}

void *receiveCommandsFromNamingServer(void *arg)
{
    int ns_socket = *((int *)arg);
struct readclient CR;
    char command[MAX_COMMAND_SIZE];
    while (1)
    {
        int i;
        if (recv(ns_socket, &i, sizeof(i), 0) == -1)
        {
            continue;
        }
        if (recv(ns_socket, command, sizeof(command), 0) == -1)
        {
            continue;
        }
        if (strcmp(command, "") == 0)
            continue;
        printf("Received command: %s\n", command);

        char a[1024];
        strcpy(a, command);

        char *token = strtok(a, " ");
        char command1[50]; 
        char path[100];
        if (token != NULL)
        {
            strcpy(command1, token);
          

            while ((token = strtok(NULL, " ")) != NULL)
            {
                strcpy(path, token);
              
            }
        }
        CR.clienti=i;
        strcpy(CR.clientpath,path);
        
        if (strcmp(command1, "CREATEFILE") == 0)
        {
            createFile(path);
        }
        else if (strcmp(command1, "CREATEDIRECTORY") == 0)
        {
            createDirectory();
        }
        else if (strncmp(command1, "READ", 4) == 0)
        {
           if (pthread_create(&clientread[i], NULL, readfile, &CR) != 0)
    {
        perror("Failed to create send thread");
        close(ns_socket);
        exit(1);
    }
        }
        else if (strncmp(command1, "DELETEDIR", 9) == 0)
        {
            deleteDirectory(path);
        }
        else if (strncmp(command1, "DELETEFILE", 10) == 0)
        {
            deleteFile(path);
        }

        else if (strcmp(command1, "WRITE") == 0)
        {
            removestrings(command, command1);
            removestrings(command, path);
            trim(command);
            printf("%s\n", command);
            writefile(command, path);
        }
        else if (strcmp(command1, "GET") == 0)
        {
            getper(i, path);
        }
        else
        {
            printf("Invalid command\n");
        }
        fflush(stdout);
    }
    pthread_exit(NULL);
}



int main()
{
    int ns_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ns_socket == -1)
    {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in ns_address;
    ns_address.sin_family = AF_INET;
    ns_address.sin_port = htons(NAMING_SERVER_PORT);
    ns_address.sin_addr.s_addr = inet_addr(NAMING_SERVER_IP);

    if (connect(ns_socket, (struct sockaddr *)&ns_address, sizeof(ns_address)) == -1)
    {
        perror("Connection to the naming server failed");
        close(ns_socket);
        exit(1);
    }

    pthread_t send_thread, receive_thread, client_thread;
    if (pthread_create(&send_thread, NULL, sendInfoToNamingServer, &ns_socket) != 0)
    {
        perror("Failed to create send thread");
        close(ns_socket);
        exit(1);
    }

    if (pthread_create(&receive_thread, NULL, receiveCommandsFromNamingServer, &ns_socket) != 0)
    {
        perror("Failed to create receive thread");
        close(ns_socket);
        exit(1);
    }

    int ss_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_socket == -1)
    {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in ss_address;
    ss_address.sin_family = AF_INET;
    ss_address.sin_port = htons(STORAGE_SERVER_PORT);
    ss_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(ss_socket, (struct sockaddr *)&ss_address, sizeof(ss_address)) == -1)
    {
        perror("Bind failed");
        close(ss_socket);
        exit(1);
    }

    if (listen(ss_socket, 5) == -1)
    {
        perror("Listen failed");
        close(ss_socket);
        exit(1);
    }

    while (1)
    {
        int client_socket = accept(ss_socket, NULL, NULL);
        clientarr[num_clients++] = client_socket;
        if (client_socket == -1)
        {
            perror("Accepting client connection failed");
            continue; // Continue to the next iteration to keep listening
        }
    }

    // Close the naming server socket
    close(ns_socket);

    // Close the storage server socket
    close(ss_socket);

    return 0;
}