#include <arpa/inet.h>		// htons()
#include <stdio.h>		// printf(), perror()
#include <stdlib.h>		// atoi()
#include <sys/socket.h>		// socket(), bind(), listen(), accept(), send(), recv()
#include <unistd.h>		// close()
#include <string.h>		// memcpy()
#include <sys/stat.h> 
using namespace std;

static const size_t MAX_MESSAGE_SIZE = 256;

/**
 * Receives a string message from the client and prints it to stdout.
 *
 * Parameters:
 * 		connectionfd: 	File descriptor for a socket connection
 * 				(e.g. the one returned by accept())
 * Returns:
 *		0 on success, -1 on failure.
 */
int handle_connection(int connectionfd) {

	printf("New connection %d\n", connectionfd);

	// (1) Receive message from client.

	char msg[MAX_MESSAGE_SIZE + 1];
	memset(msg, 0, sizeof(msg));

	// Call recv() enough times to consume all the data the client sends.
	size_t recvd = 0;
	ssize_t rval;
	do {
		// Receive as many additional bytes as we can in one call to recv()
		// (while not exceeding MAX_MESSAGE_SIZE bytes in total).
		rval = recv(connectionfd, msg + recvd, MAX_MESSAGE_SIZE - recvd, 0);
		if (rval == -1) {
			perror("Error reading stream message");
			return -1;
		}
		recvd += rval;
	} while (rval > 0);  // recv() returns 0 when client closes, in project, you should a different logic.

	// (2) Print out the message
	printf("Client %d says '%s'\n", connectionfd, msg);

	// (4) Close connection
	close(connectionfd);

	return 0;
}

/**
 * Endlessly runs a server that listens for connections and serves
 * them _synchronously_.
 *
 * Parameters:
 *		port: 		The port on which to listen for incoming connections.
 *		queue_size: 	Size of the listen() queue
 * Returns:
 *		-1 on failure, does not return on success.
 */
int server(int port, int queue_size) {

	// (1) Create socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		perror("Error opening stream socket");
		return -1;
	}

	// (2) Set the "reuse port" socket option
	int yesval = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		perror("Error setting socket options");
		return -1;
	}

	// (3) Create a sockaddr_in struct for the proper port and bind() to it.
	struct sockaddr_in server_addr;

	// 3(a) Make sockaddr_in
	server_addr.sin_family = AF_INET;
	// Let the OS map it to the correct address.
	server_addr.sin_addr.s_addr = INADDR_ANY;
	// If port is 0, the OS will choose the port for us. Here we specify a port.
	// Use htons to convert from local byte order to network byte order.
	server_addr.sin_port = htons(port);
	// Bind to the port.
	if (bind(server_socket, (sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		perror("Error binding stream socket");
		return -1;
	}

	// (3b)Detect which port was chosen.
	// port = get_port_number(server_socket);
	printf("Server listening on port %d...\n", port);

	// (4) Begin listening for incoming connections.
	if (listen(server_socket, queue_size))
    {
        printf("Server Listen Failed!\n");
        exit(1);
    }
    
	// (5) Serve incoming connections one by one forever.
	while(1)
    {
        // 定义客户端的socket地址结构client_addr，当收到来自客户端的请求后，调用accept
        // 接受此请求，同时将client端的地址和端口等信息写入client_addr中
        struct sockaddr_in client_addr;
        socklen_t          length = sizeof(client_addr);

        // 接受一个从client端到达server端的连接请求,将客户端的信息保存在client_addr中
        // 如果没有连接请求，则一直等待直到有连接请求为止，这是accept函数的特性，可以
        // 用select()来实现超时检测
        // accpet返回一个新的socket,这个socket用来与此次连接到server的client进行通信
        // 这里的new_server_socket代表了这个通信通道
        int new_server_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
        if (new_server_socket < 0)
        {
            printf("Server Accept Failed!\n");
            break;
        }

        char buffer[MAX_MESSAGE_SIZE];
        bzero(buffer, sizeof(buffer));
        length = recv(new_server_socket, buffer, MAX_MESSAGE_SIZE, 0);
        if (length < 0)
        {
            printf("Server Recieve Data Failed!\n");
            break;
        }

        char file_name[MAX_MESSAGE_SIZE + 1];
        bzero(file_name, sizeof(file_name));
        strncpy(file_name, buffer,
                strlen(buffer) > MAX_MESSAGE_SIZE ? MAX_MESSAGE_SIZE : strlen(buffer));
        
        struct stat statbuf;  
        stat(file_name,&statbuf);  
        int file_length = statbuf.st_size;
        char file_size[MAX_MESSAGE_SIZE];
        sprintf(file_size, "%d", file_length);
        if (send(new_server_socket, file_size, MAX_MESSAGE_SIZE, 0) < 0)
        {
            printf("Send File Length: Failed!\n");
            break;
        }
        
        FILE *fp = fopen(file_name, "r");
        if (fp == NULL)
        {
            printf("File:\t%s Not Found!\n", file_name);
        }
        else
        {
            bzero(buffer, MAX_MESSAGE_SIZE);
            int file_block_length = 0;
            while( (file_block_length = fread(buffer, sizeof(char), MAX_MESSAGE_SIZE, fp)) > 0)
            {
                printf("file_block_length = %d\n", file_block_length);

                // 发送buffer中的字符串到new_server_socket,实际上就是发送给客户端
                if (send(new_server_socket, buffer, file_block_length, 0) < 0)
                {
                    printf("Send File:\t%s Failed!\n", file_name);
                    break;
                }

                bzero(buffer, sizeof(buffer));
            }
            fclose(fp);
            printf("File:\t%s Transfer Finished!\n", file_name);
        }

        close(new_server_socket);
    }

    close(server_socket);

    return 0;
}

int main(int argc, const char **argv) {
	// Parse command line arguments
	if (argc != 2) {
		printf("Usage: ./server port_num\n");
		return 1;
	}
	int port = atoi(argv[1]);

	if (server(port, 10) == -1) {
		return 1;
	}
	return 0;
}
