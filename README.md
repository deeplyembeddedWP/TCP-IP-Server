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
SRCDIR = /home/vinay_divakar/tcp-ip-server/src
```
2. Run **make** from within the project's root i.e.*Server* to build your project. This will generate an executable called *server_app*.
   
## Running the application
1. Run the *server_app* executable and the below shows the server application is running & has started listening for connections!
```
vinay_divakar@vinay-divakar-Linux:~/Server$ ./server_app 
server listening on port 12345 using socket fd 3
```
2. You could use the [client program](https://github.com/deeplyembeddedWP/tcp-ip-client) to test the server OR tools such as telnet.
3. For debug purposes or visiblity, you can enable/uncomment the below line in *file_transfer.c* within the function *file_transfer()*. This prints what's being sent over the socket.
```
// enable to see whats being sent out
// printf("content: %.*s\r\n", err, (char *)buffer);
```

## How it works
To ensure the server supports concurrent and services concurrent connections, there are a few ways to about it. The most common one's are using the [*select()*](https://man7.org/linux/man-pages/man2/select.2.html) or [*poll()*](https://man7.org/linux/man-pages/man2/poll.2.html) functions. For this application, I decided to use *poll()* cosnidering a few advantages it has over *select()* and also simplifies the software design. There also seems to [*epoll*](https://man7.org/linux/man-pages/man7/epoll.7.html) which is believed to offer much better performace, however, for this case, poll should be good enough.

The application has five states(shown below) to manage all the operations and present in the *server_state_machine.h*.
```
enum server_state_t {
  SERVER_LISTEN_BEGIN,
  SERVER_POLL_FOR_EVENTS,
  SERVER_POLL_INCOMING_CONNECTIONS,
  SERVER_PROCESS_CONNECTION_EVENTS,
  SERVER_FATAL_ERROR
};
```
1. SERVER_LISTEN_BEGIN: Initializes the variables, sets up a socket and starts listening for incoming connections.
2. SERVER_POLL_FOR_EVENTS: Polls for events on all the active sockets i.e. listening and any active connections.
3. SERVER_POLL_INCOMING_CONNECTIONS: Handles incoming connection request events from clients and adds them for poll to monitor for events.
4. SERVER_PROCESS_CONNECTION_EVENTS: Processes events generated on all active connection such as socket read, socket write or socket errors.
5. SERVER_FATAL_ERROR: Handles unexpected errors and exits on a failure.  
