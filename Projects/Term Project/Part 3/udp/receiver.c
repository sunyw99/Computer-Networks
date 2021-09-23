#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>

// In porject, the size of your packet would be len(header) + len(payload)
// here we ASSUME sender would not send message longer than 2000B
#define MAX_MESSAGE_SIZE 2000

int main(int argc, char **argv) {

    if (argc != 2) {
        perror("Usage: ./receiver receiver_port\n");
        exit(1);
    }

    int myport = atoi(argv[1]);
    struct sockaddr_in si_me;
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) {
        fputs("socket creation failed!", stderr);
        exit(2);
    }
    memset((char *) &si_me, 0, sizeof(struct sockaddr_in));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd , (struct sockaddr*) &si_me, sizeof(struct sockaddr_in));

    char *buf = malloc(MAX_MESSAGE_SIZE);
    socklen_t slen = sizeof(struct sockaddr_in);
    struct sockaddr_in si_other;

    struct timeval start, stop;
    gettimeofday(&start, NULL);
    gettimeofday(&stop, NULL);

    while (1) {
        // Important: MSG_DONTWAIT specifies that recvfrom() is non-blocking, by default it's blocking
        // In your project, you may want to use a non-blocking recvfrom(), since packet may get lost and you don't want to block your program.
        // Non-blocking recvfrom() returns -1 when there is no available packet at the socket. Otherwise it returns number of bytes received.
        // Blocking recvfrom() blocks until there is an available packet at the socket, and returns number of bytes received.

        // Here we are using a non-blocking recvfrom() ------|
        //                                                   |
        int n = recvfrom(sockfd, buf, MAX_MESSAGE_SIZE, MSG_DONTWAIT, (struct sockaddr *) &si_other, &slen);
        printf("n = %d\n", n);
        if (n > 0) {
            printf("Receive message: %s\n", buf);
            break;  // our sender only sends one message, no need to loop
        }
        gettimeofday(&stop, NULL);
        if ((double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000 > 4000) {
            printf("I do not receive anything within 4 seconds. Give up\n" );
            break;
        }
    }


    return 0;
}
