# Assumptions

- Accessible paths in the storage server are predefined, taken from the paths present in the directory where the storage server code is run.
- All storage servers have the same IP address. Changing the port number and running it in another directory connects that storage server to the naming server.
- The order of client commands is as follows:
  - **READ command:** `[READ][path/of/file]` and command is `[GET]` for getting permissions.
  - **WRITE command:** `[WRITE][content][path/of/file]`
  - **COPY command:** `[Command][SOURCE][DESTINATION]` command is `COPY` for a file and `COPYDIR` for a directory.
  - **CREATE command:** `[CREATEFILE][Path]` for a file and `[CREATEDIR][Path]` for a directory.
  - **DELETE command:** Similar to CREATE where the command is `DELETE`.
     The path given here is an absolute path.
- Logs can be checked using the command: `grep "NamingServer" /var/log/syslog`
# Implementation

After running the naming server, it listens on a socket, receives a request type, and handles the storage or client accordingly. The request type sent from the storage server is 'I,' and that from the client is 'P.'

There are two threads, one for handling the storage server and the other for handling the client.

When the request type 'I' is received, it gets into the handling storage thread, where the information about the storage server sent by the storage server is received and stored.

When the request type 'P' is received, it gets into the handle client thread, in which the message from the client is received and, after tokenizing, the command is taken, and an appropriate task is performed.

### Efficient Algorithm

In the information stored about the storage server, there is also an entry that stores the absolute address of the directory in which the storage server is running. When a client provides a message and after retrieving the path by the naming server, the algorithm compares the path string with the address string in the storage server to the length of the storage server. If it is a match, then the relative path is extracted from the obtained path and searched in the accessible paths. If the path is found, 1 is returned.

**Time Complexity:** O(n+m); where n is the number of storage servers and m is the length of the max accessible paths string.

### LRU Cache

The maximum size of the cache is taken as 5. When a 1 is returned, it is updated in the cache if it is not present already. If it is present, then it is pushed to the front of the queue. If the cache size is full, then the LRU storage server info is removed.

So, the naming server first checks in the cache. If it is a hit, then it returns after pushing it to the front. If it is a miss, then it is searched in the table of storage servers.
