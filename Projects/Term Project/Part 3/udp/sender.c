#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>


int main(int argc, char **argv) {

    if (argc != 5) {
        perror("Usage: ./sender sender_port receiver_port receiver_ip message\n");
        exit(1);
    }
    int myport = atoi(argv[1]);
    int receiver_port = atoi(argv[2]);
    const char* receiver_ip = argv[3];
    const char* message = argv[4];

    // create UDP socket.
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // use IPPROTO_UDP
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    // below is the same as TCP socket.
    struct sockaddr_in myaddr;
    memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    // bind to a specific port
    myaddr.sin_port = htons(myport);
    bind(sockfd, (struct sockaddr *) & myaddr, sizeof(myaddr));

    struct sockaddr_in receiver_addr;
    struct hostent *host = gethostbyname(receiver_ip);
    memcpy(&(receiver_addr.sin_addr), host->h_addr, host->h_length);
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(receiver_port);

    // UDP is connectionless, you just send, even if the receiver is not ready for receiving.
    // You don't really know whether this message arrives at receiver without error.
    sendto(sockfd, (void*) message, strlen(message), 0, (const struct sockaddr *) &receiver_addr, sizeof(struct sockaddr_in));


    return 0;
}
