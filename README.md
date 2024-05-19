# Server
A tcp/ip server application that supports concurrent connections from clients enabling them to request file downloads.

## Project structure
At present, all the source files are within a single directory called **Src** to keep things simple. Though at some point it'd be better to split the header and source files into two seperate folders e.g. *include* and *src*.

## Server Configuration
1. In **file_transfer.h**
```
#define FILE_TRANSFER_NAME_SIZE_MAX 32 + 1                      // Maximum size supported for the requested filename
#define FILE_TRANSFER_TABLE "/home/vinay_divakar/file_storage"  // files storage path
#define FILE_TRANSFER_PATH_NAME_SIZE_MAX 64                     // Maximum size supported for the file path
#define FILE_TRANSFER_BUFF_READ_SIZE 32                         //  Maximum size supported for buffer reads
```
2. In **server.h**
```
#define SERVER_SOCKET_LISTEN_INDEX 0        // listening socket index in poll fds descriptor set
#define SERVER_SOCKET_LISTEN_PORT_NUM 12345 // listening socket port number to which clients request connection
#define SERVER_SOCKET_POLL_TIMEOUT -1       // poll timeout set to block indefinetly if not events occur
#define SERVER_CONNECTIONS_BACKLOG 5        // maximum connections to be queued to be serviced
#define SERVER_STATE_MACHINE_FDS_MAX 3 + 1  // maximum number of connections supported 
```
## Building the project
1. In the Makefile, update the *SRCDIR* variable to point the path project is located on your local machine.
```
SRCDIR = /home/vinay_divakar/Server/src
```
2. Run **make** from within the project's root i.e.*Server* to build your project. This will generate an executable called *server_app*.
   
## Running the application
1. Run the *server_app* executable and the below shows the server application is running & has started listening for connections!
```
vinay_divakar@vinay-divakar-Linux:~/Server$ ./server_app 
server listening on port 12345 using socket fd 3
```
2. You could use the [client program](https://github.com/deeplyembeddedWP/tcp-ip-client) to test the server OR tools such as telnet.

## How it works
TBD

