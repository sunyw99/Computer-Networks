// TODO: implement ftrans here 
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/stat.h> 
using namespace std;

static const int MAX_MESSAGE_SIZE = 1024;

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
        
        //Send the file length to client.
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

int client(const char *hostname, int port, const char *file_name, int client_port) {
	if (strlen(file_name) > MAX_MESSAGE_SIZE) {
		perror("Error: Message exceeds maximum length\n");
		return -1;
	}

    // 设置一个socket地址结构client_addr, 代表客户机的internet地址和端口  
    struct sockaddr_in client_addr;  
    bzero(&client_addr, sizeof(client_addr));  
    client_addr.sin_family = AF_INET; // internet协议族  
    client_addr.sin_addr.s_addr = htons(INADDR_ANY); // INADDR_ANY表示自动获取本机地址  
    client_addr.sin_port = htons(client_port); // auto allocated, 让系统自动分配一个空闲端口  

    // 创建用于internet的流协议(TCP)类型socket，用client_socket代表客户端socket  
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);  
    if (client_socket < 0)  
    {  
        printf("Create Socket Failed!\n");  
        exit(1);  
    }  

    // 把客户端的socket和客户端的socket地址结构绑定   
    if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))  
    {  
        printf("Client Bind Port Failed!\n");  
        exit(1);  
    }  

    // 设置一个socket地址结构server_addr,代表服务器的internet地址和端口  
    struct sockaddr_in  server_addr;  
    bzero(&server_addr, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;  

    struct hostent *host = gethostbyname(hostname);
	if (host == nullptr) {
		fprintf(stderr, "%s: unknown host\n", hostname);
		return -1;
	}
	memcpy(&(server_addr.sin_addr), host->h_addr, host->h_length);

    server_addr.sin_port = htons(port);  


	// (3) Connect to remote server
	if (connect(client_socket, (sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		perror("Error connecting stream socket");
		return -1;
	}

    char buffer[MAX_MESSAGE_SIZE];  
    bzero(buffer, sizeof(buffer));  
    strncpy(buffer, file_name, strlen(file_name) > MAX_MESSAGE_SIZE ? MAX_MESSAGE_SIZE : strlen(file_name));  
    // 向服务器发送buffer中的数据，此时buffer中存放的是客户端需要接收的文件的名字  
    send(client_socket, buffer, MAX_MESSAGE_SIZE, 0);  
    
    //Receive file length from Server.
    if (recv(client_socket, buffer, MAX_MESSAGE_SIZE, 0) < 0)
    {
        printf("Recieve File Length From Server Failed!\n");
        return -1;
    }
    
    FILE *fp = fopen(file_name, "w");  
    if (fp == NULL)  
    {  
        printf("File:\t%s Can Not Open To Write!\n", file_name);  
        exit(1);  
    }  

    // 从服务器端接收数据到buffer中   
    bzero(buffer, sizeof(buffer));  
    int length = 0;  
    while((length = recv(client_socket, buffer, MAX_MESSAGE_SIZE, 0)))
    {  
        if (length < 0)  
        {  
            printf("Recieve Data From Server Failed!\n");  
            break;  
        }  

        int write_length = fwrite(buffer, sizeof(char), length, fp);  
        if (write_length < length)  
        {  
            printf("File:\t%s Write Failed!\n", file_name);  
            break;  
        }  
        bzero(buffer, MAX_MESSAGE_SIZE);  
    }  

    printf("Recieve File:\t %s From Server Finished!\n", file_name);  

    // 传输完毕，关闭socket   
    fclose(fp);  
    close(client_socket);  
    return 0;  
}

int main(int argc, char* argv[]) {
	if (strcmp(argv[1], "-s") == 0) {
        int port = atoi(argv[3]);

        if (server(port, 10) == -1) {
            return 1;
        }
    }
    else if (strcmp(argv[1], "-c") == 0) {
        const char *hostname = argv[3];
        int port = atoi(argv[5]);
        int client_port = atoi(argv[9]);
        const char *filename = argv[7];

        printf("Sending filename %s to %s:%d\n", filename, hostname, port);
        if (client(hostname, port, filename, client_port) == -1) {
            return 1;
        }
    }
    return 0;
}