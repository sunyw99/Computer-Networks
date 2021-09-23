#include <stdlib.h>		// atoi()
#include <unistd.h>		// close()
#include <arpa/inet.h>		// htons(), ntohs()
#include <netdb.h>		// gethostbyname(), struct hostent
#include <netinet/in.h>		// struct sockaddr_in
#include <stdio.h>		// perror(), fprintf()
#include <string.h>		// memcpy()
#include <sys/socket.h>		// getsockname()
#include <unistd.h>		// stderr

static const int MAX_MESSAGE_SIZE = 256;

/**
 *	hostname: 	Remote hostname of the server.
 *	port: 		Remote port of the server.
 * 	message: 	The message to send, as a C-string.
 *  client_port: The port that client binds to.
 */
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


int main(int argc, const char **argv) {
	// Parse command line arguments
	if (argc != 5) {
		printf("Usage: ./client hostname server_port client_port message\n");
		return 1;
	}
	const char *hostname = argv[1];
	int port = atoi(argv[2]);
	int client_port = atoi(argv[3]);
	const char *filename = argv[4];

	printf("Sending filename %s to %s:%d\n", filename, hostname, port);
	if (client(hostname, port, filename, client_port) == -1) {
		return 1;
	}

	return 0;
}
