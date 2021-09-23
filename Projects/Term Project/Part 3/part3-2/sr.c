// TODO: copy your code from part 3.1 and implement part 3.2

#include "./crc32.h"
#include "./SRHeader.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdint.h>

typedef struct packet{
    char payload[MAX_PAYLOAD_SIZE];
    struct SRHeader *header;
}PKT;

void writeLog(struct SRHeader *header, const char *log_file) {
    FILE *fp_log = fopen(log_file, "a");
    if (fp_log == NULL) {  
        printf("File:\t%s Can Not Open To Write!\n", log_file);  
        exit(1);  
    }
    
    char log[100];
    bzero(log, sizeof(log));
    if (header->flag == 0) strcpy(log, "SYN ");
    else if (header->flag == 1) strcpy(log, "FIN ");
    else if (header->flag == 2) strcpy(log, "DATA ");
    else if (header->flag == 3) strcpy(log, "ACK ");
    
    char log_seq[20];
    bzero(log_seq, sizeof(log_seq));
    sprintf(log_seq, "%d", header->seq);
    strcat(log, log_seq);
    strcat(log, " ");
            
    char log_len[4];
    bzero(log_len, sizeof(log_len));
    sprintf(log_len, "%d", header->len);
    strcat(log, log_len);
    strcat(log, " ");
            
    char log_crc[20];
    bzero(log_crc, sizeof(log_crc));
    sprintf(log_crc, "%d", header->crc);
    strcat(log, log_crc);
            
    strcat(log, "\n");
    int write_length = fwrite(log, sizeof(char), strlen(log), fp_log);
    if (write_length < strlen(log)) {  
        printf("File:\t%s Write Failed!\n", log_file);
    }
    
    fclose(fp_log);
}


int receiver(int receiver_port, int window_size, const char *recv_dir, const char *log_file) {
    struct sockaddr_in receiverAddr;
    int receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == -1) {
        fputs("socket creation failed!", stderr);
        exit(2);
    }
    memset((char *) &receiverAddr, 0, sizeof(struct sockaddr_in));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(receiver_port);
    receiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(receiverSocket , (struct sockaddr*) &receiverAddr, sizeof(struct sockaddr_in));
    
    struct sockaddr_in senderAddr;
    
    PKT window[window_size];
    PKT rcv_pkt, ack_pkt;
    
    char *packet = malloc(sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE);
    char *ack_packet = malloc(sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE);
    
    int i = 0, j = 0, k = 0, delta_Z = 0;
    int newFileNum = 1;
    int win_start_sequence = 0;
    socklen_t slen = sizeof(struct sockaddr_in);
    char file_name[100];
    
    while (1) {
        // Important: MSG_DONTWAIT specifies that recvfrom() is non-blocking, by default it's blocking
        // In your project, you may want to use a non-blocking recvfrom(), since packet may get lost and you don't want to block your program.
        // Non-blocking recvfrom() returns -1 when there is no available packet at the socket. Otherwise it returns number of bytes received.
        // Blocking recvfrom() blocks until there is an available packet at the socket, and returns number of bytes received.

        // Here we are using a non-blocking recvfrom() ------|
        //                                                   |
        
        
        //bool Flag_writetofile = true;  
        
        int recv_len = recvfrom(receiverSocket, packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, MSG_DONTWAIT, (struct sockaddr *) &senderAddr, &slen);
        if (recv_len > 0) {
            printf("Receiving file.\n");
            
            struct SRHeader *rcv_header = malloc(sizeof(struct SRHeader));
            memcpy(rcv_header, packet, sizeof(struct SRHeader));
            memcpy(rcv_pkt.payload, packet + sizeof(struct SRHeader), MAX_PAYLOAD_SIZE);
            rcv_pkt.header = rcv_header;
            
            //Write to log file.
            writeLog(rcv_pkt.header, log_file);
            
            struct SRHeader *ack_header = malloc(sizeof(struct SRHeader));
            if (crc32(rcv_pkt.payload, rcv_pkt.header->len) == rcv_pkt.header->crc) {
                if (rcv_pkt.header->flag == 0) {
                    //newFileNum++;
                    char file_count[10];
                    sprintf(file_count, "%d", newFileNum);
                    //strcat(file_name, "/");
                    bzero(file_name, sizeof(file_name));
                    strcat(file_name, recv_dir);
                    strcat(file_name, "file_");
                    strcat(file_name, file_count);
                    strcat(file_name, ".txt");
    
                    ack_header->flag = 3;
                    ack_header->seq = rcv_pkt.header->seq;
                    ack_header->len = 0;
                    ack_header->crc = crc32(NULL, 0);
                    ack_pkt.header = ack_header;
                    bzero(ack_pkt.payload, sizeof(ack_pkt.payload));
                    memcpy(ack_packet, ack_pkt.header, sizeof(struct SRHeader));
                    memcpy(ack_packet + sizeof(struct SRHeader), ack_pkt.payload, MAX_PAYLOAD_SIZE);
                    if (sendto(receiverSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &senderAddr, sizeof(struct sockaddr_in)) < 0) {
                        printf("Send ACK Failed!\n");
                        break;
                    }
                    writeLog(ack_pkt.header, log_file);
                }
                else if (rcv_pkt.header->flag == 1) {
                    //struct SRHeader *header2 = malloc(sizeof(struct SRHeader));
                    ack_header->flag = 3;
                    ack_header->seq = rcv_pkt.header->seq;
                    ack_header->len = 0;
                    ack_header->crc = crc32(NULL, 0);
                    ack_pkt.header = ack_header;
                    bzero(ack_pkt.payload, sizeof(ack_pkt.payload));
                    memcpy(ack_packet, ack_pkt.header, sizeof(struct SRHeader));
                    memcpy(ack_packet + sizeof(struct SRHeader), ack_pkt.payload, MAX_PAYLOAD_SIZE);
                    if (sendto(receiverSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &senderAddr, sizeof(struct sockaddr_in)) < 0) {
                        printf("Send ACK Failed!\n");
                        break;
                    }
                    writeLog(ack_pkt.header, log_file);
                    win_start_sequence = 0;
                    newFileNum++;
                }
                else if (rcv_pkt.header->seq < win_start_sequence) {
                    //struct SRHeader *header3 = malloc(sizeof(struct SRHeader));
                    ack_header->flag = 3;
                    ack_header->seq = win_start_sequence;
                    ack_header->len = 0;
                    ack_header->crc = crc32(NULL, 0);
                    ack_pkt.header = ack_header;
                    bzero(ack_pkt.payload, sizeof(ack_pkt.payload));
                    memcpy(ack_packet, ack_pkt.header, sizeof(struct SRHeader));
                    memcpy(ack_packet + sizeof(struct SRHeader), ack_pkt.payload, MAX_PAYLOAD_SIZE);
                    if (sendto(receiverSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &senderAddr, sizeof(struct sockaddr_in)) < 0) {
                        printf("Send ACK Failed!\n");
                        break;
                    }
                    writeLog(ack_pkt.header, log_file);
                    /*int write_length = fwrite(ack_pkt.payload, sizeof(char), ack_pkt.header->len, fp);  
                    if (write_length < ack_pkt.header->len) {  
                        printf("File:\t%s Write Failed!\n", file_name);  
                        break;  
                    }*/
                }
                else if (win_start_sequence < rcv_pkt.header->seq && rcv_pkt.header->seq < win_start_sequence + window_size) {
                    //struct SRHeader *header4 = malloc(sizeof(struct SRHeader));
                    ack_header->flag = 3;
                    ack_header->seq = win_start_sequence;
                    ack_header->len = 0;
                    ack_header->crc = crc32(NULL, 0);
                    ack_pkt.header = ack_header;
                    bzero(ack_pkt.payload, sizeof(ack_pkt.payload));
                    memcpy(ack_packet, ack_pkt.header, sizeof(struct SRHeader));
                    memcpy(ack_packet + sizeof(struct SRHeader), ack_pkt.payload, MAX_PAYLOAD_SIZE);
                    if (sendto(receiverSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &senderAddr, sizeof(struct sockaddr_in)) < 0) {
                        printf("Send ACK Failed!\n");
                        break;
                    }
                    writeLog(ack_pkt.header, log_file);
                    window[rcv_pkt.header->seq - win_start_sequence] = rcv_pkt;
                    //Flag_writetofile = false;
                }
                else if (win_start_sequence == rcv_pkt.header->seq) {
                    window[0] = rcv_pkt;
                    i = 0;
                    delta_Z = -1;
                    while(i < window_size && window[i].header != NULL) {
                        delta_Z++;
                        i++;
                    }
                    //struct SRHeader *header5 = malloc(sizeof(struct SRHeader));
                    ack_header->flag = 3;
                    ack_header->seq = win_start_sequence + delta_Z + 1;
                    ack_header->len = 0;
                    ack_header->crc = crc32(NULL, 0);
                    ack_pkt.header = ack_header;
                    bzero(ack_pkt.payload, sizeof(ack_pkt.payload));
                    memcpy(ack_packet, ack_pkt.header, sizeof(struct SRHeader));
                    memcpy(ack_packet + sizeof(struct SRHeader), ack_pkt.payload, MAX_PAYLOAD_SIZE);
                    if (sendto(receiverSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &senderAddr, sizeof(struct sockaddr_in)) < 0) {
                        printf("Send ACK Failed!\n");
                        break;
                    }
                    writeLog(ack_pkt.header, log_file);
                    FILE *fp = fopen(file_name, "a");  
                    if (fp == NULL) {  
                        printf("File:\t%s Can Not Open To Write!\n", file_name);  
                        exit(1);  
                    }
                    for (j = 0; j <= delta_Z; j++) {
                        int write_length = fwrite(window[j].payload, sizeof(char), window[j].header->len, fp);
                        if (write_length < window[j].header->len) {  
                            printf("File:\t%s Write Failed!\n", file_name);  
                            break;  
                        }
                    }
                    fclose(fp);
                    for (k = 0; k < window_size; k++) {
                        if (k < window_size - delta_Z - 1) {
                            window[k] = window[k + delta_Z + 1];
                        }
                        else {
                            window[k].header = NULL;
                        }
                    }
                    win_start_sequence = win_start_sequence + delta_Z + 1;
                }
            }
        }
    }
    free(packet);
    free(ack_packet);
    return 0;
}


int sender(const char *receiver_ip, int receiver_port, int sender_port, int window_size, const char *file_to_send, const char *log_file) {
    // create UDP socket.
    int senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // use IPPROTO_UDP
    if (senderSocket < 0) {
        perror("socket creation failed");
        exit(1);
    }
    
    socklen_t slen = sizeof(struct sockaddr_in);
    
    struct sockaddr_in senderAddr;
    memset(&senderAddr, 0, sizeof(senderAddr));
    senderAddr.sin_family = AF_INET;
    senderAddr.sin_addr.s_addr = INADDR_ANY;
    // bind to a specific port
    senderAddr.sin_port = htons(sender_port);
    bind(senderSocket, (struct sockaddr *) & senderAddr, sizeof(senderAddr));
    
    struct sockaddr_in receiverAddr;
    struct hostent *host = gethostbyname(receiver_ip);
    memcpy(&(receiverAddr.sin_addr), host->h_addr, host->h_length);
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(receiver_port);
    
    struct stat statbuf;  
    stat(file_to_send, &statbuf);  
    int file_length = statbuf.st_size;
    
    FILE *fp = fopen(file_to_send, "r");
    if (fp == NULL) {
    	printf("file to send not found\n");
    	return -1;
    }
    
    
    //PKT window[window_size];
    PKT send_pkt, ack_pkt;
    char *send_packet = malloc(sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE);
    char *ack_packet = malloc(sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE);
    
    int file_block_length = 0;
    
    srand((unsigned int)time(0));
    unsigned int rand_sequence = rand()%1000000;
    
    int FLAG_resend = 1;
    struct timeval start, stop;
    while (FLAG_resend == 1) {
        
        gettimeofday(&start, NULL);
        gettimeofday(&stop, NULL);
        
        struct SRHeader *send_header = malloc(sizeof(struct SRHeader));
        send_header->flag = 0;
        send_header->seq = rand_sequence;
        send_header->len = 0;
        send_header->crc = crc32(NULL, 0);
        send_pkt.header = send_header;
        bzero(send_pkt.payload, sizeof(send_pkt.payload));
        memcpy(send_packet, send_pkt.header, sizeof(struct SRHeader));
        memcpy(send_packet + sizeof(struct SRHeader), send_pkt.payload, MAX_PAYLOAD_SIZE);
        if (sendto(senderSocket, send_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &receiverAddr, sizeof(struct sockaddr_in)) < 0) {
            printf("Send File:\t%s Failed!\n", file_to_send);
            break;
        }
        writeLog(send_pkt.header, log_file);
        

        while (1) {
            int ack_len = recvfrom(senderSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, MSG_DONTWAIT, (struct sockaddr *) &receiverAddr, &slen);
            if (ack_len > 0) {
                struct SRHeader *header = malloc(sizeof(struct SRHeader));
                memcpy(header, ack_packet, sizeof(struct SRHeader));
                memcpy(ack_pkt.payload, ack_packet + sizeof(struct SRHeader), MAX_PAYLOAD_SIZE);
                ack_pkt.header = header;
                writeLog(ack_pkt.header, log_file);
                FLAG_resend = 0;
                break;
            }
            gettimeofday(&stop, NULL);
            if ((double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000 > 400) {
                printf("Time out. No START ACK from receiver. Resend.\n" );
                break;
            }
        }
    }
    
    int sequence = 0;
    int win_start_sequence = 0;
    int duplicate_seq = 0;
    int count_duplicateACK = 0;
    int FLAG_duplicateACK = 0;
    int last_fileblock = 0;
    gettimeofday(&start, NULL);
    gettimeofday(&stop, NULL);
    while (1) {
        while(sequence < win_start_sequence + window_size && (file_block_length = fread(send_pkt.payload, sizeof(char), MAX_PAYLOAD_SIZE, fp)) > 0) {
            struct SRHeader *send_header = malloc(sizeof(struct SRHeader));
            send_header->flag = 2;
            send_header->seq = sequence;
            send_header->len = file_block_length;
            send_header->crc = crc32(send_pkt.payload, file_block_length);
            send_pkt.header = send_header;
            memcpy(send_packet, send_pkt.header, sizeof(struct SRHeader));
            memcpy(send_packet + sizeof(struct SRHeader), send_pkt.payload, MAX_PAYLOAD_SIZE);
            printf("file_block_length = %d\n", file_block_length);
            if (sendto(senderSocket, send_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &receiverAddr, sizeof(struct sockaddr_in)) < 0) {
                printf("Send File:\t%s Failed!\n", file_to_send);
                break;
            }
            writeLog(send_pkt.header, log_file);
            if (sequence == win_start_sequence + window_size - 1) last_fileblock = file_block_length;
            if (FLAG_duplicateACK == 1) {
                fseek(fp, MAX_PAYLOAD_SIZE * (window_size - 2) + last_fileblock, SEEK_CUR);
                FLAG_duplicateACK = 0;
                sequence = win_start_sequence + window_size;
            }
            else sequence++;
        }
        while (1) {
            int ack_len = recvfrom(senderSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, MSG_DONTWAIT, (struct sockaddr *) &receiverAddr, &slen);
            if (ack_len > 0) {
                struct SRHeader *header = malloc(sizeof(struct SRHeader));
                memcpy(header, ack_packet, sizeof(struct SRHeader));
                memcpy(ack_pkt.payload, ack_packet + sizeof(struct SRHeader), MAX_PAYLOAD_SIZE);
                ack_pkt.header = header;
                writeLog(ack_pkt.header, log_file);
                if (ack_pkt.header->seq == duplicate_seq) {
                    count_duplicateACK++;
                    if (count_duplicateACK == 3) {
                        printf("Duplicate ACKs. Resend.\n");
                        fseek(fp, - MAX_PAYLOAD_SIZE * (sequence - win_start_sequence - 1) - last_fileblock, SEEK_CUR);
                        sequence = win_start_sequence;
                        FLAG_duplicateACK = 1;
                        count_duplicateACK = 0;
                        gettimeofday(&start, NULL);
                        gettimeofday(&stop, NULL);
                        break;
                    }
                }
                if (win_start_sequence < ack_pkt.header->seq) {
                    win_start_sequence = ack_pkt.header->seq;
                    duplicate_seq = win_start_sequence;
                    count_duplicateACK = 1;
                    gettimeofday(&start, NULL);
                    gettimeofday(&stop, NULL);
                    break;
                }
            }
            gettimeofday(&stop, NULL);
            if ((double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000 > 400) {
                printf("Time out. No DATA ACK from receiver. Resend.\n");
                fseek(fp, - MAX_PAYLOAD_SIZE * (sequence - win_start_sequence - 1) - last_fileblock, SEEK_CUR);
                sequence = win_start_sequence;
                gettimeofday(&start, NULL);
                gettimeofday(&stop, NULL);
                break;
            }
        }
        if (win_start_sequence == file_length / MAX_PAYLOAD_SIZE + 1) {
            fclose(fp);
            break;
        }
    }
    
    FLAG_resend = 1;
    while (FLAG_resend == 1) {
        gettimeofday(&start, NULL);
        gettimeofday(&stop, NULL);
        
        struct SRHeader *send_header = malloc(sizeof(struct SRHeader));
        send_header->flag = 1;
        send_header->seq = rand_sequence;
        send_header->len = 0;
        send_header->crc = crc32(NULL, 0);     
        send_pkt.header = send_header;
        bzero(send_pkt.payload, sizeof(send_pkt.payload));        
        memcpy(send_packet, send_pkt.header, sizeof(struct SRHeader));
        memcpy(send_packet + sizeof(struct SRHeader), send_pkt.payload, MAX_PAYLOAD_SIZE);
        if (sendto(senderSocket, send_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, 0, (const struct sockaddr *) &receiverAddr, sizeof(struct sockaddr_in)) < 0) {
            printf("Send File:\t%s Failed!\n", file_to_send);
            break;
        }
        writeLog(send_pkt.header, log_file);
        
        while (1) {
            int ack_len = recvfrom(senderSocket, ack_packet, sizeof(struct SRHeader) + MAX_PAYLOAD_SIZE, MSG_DONTWAIT, (struct sockaddr *) &receiverAddr, &slen);
            if (ack_len > 0) {
                struct SRHeader *header = malloc(sizeof(struct SRHeader));
                memcpy(header, ack_packet, sizeof(struct SRHeader));
                memcpy(ack_pkt.payload, ack_packet + sizeof(struct SRHeader), MAX_PAYLOAD_SIZE);
                ack_pkt.header = header;
                writeLog(ack_pkt.header, log_file);
                FLAG_resend = 0;
                break;
            }
            gettimeofday(&stop, NULL);
            if ((double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000 > 400) {
                printf("Time out. No END ACK from receiver. Resend.\n" );
                break;
            }
        }
    }
   
    
    printf("File:\t%s Transfer Finished!\n", file_to_send);
    free(send_packet);
    free(ack_packet);
    return 0;
}


int main(int argc, char* argv[]) {
	if (strcmp(argv[1], "-s") == 0) {
        const char *receiver_ip = argv[2];
        int receiver_port = atoi(argv[3]);
        int sender_port = atoi(argv[4]);
        int window_size = atoi(argv[5]);
        const char *file_to_send = argv[6];
        const char *log_file = argv[7];
        
        printf("Sending filename %s to %s:%d\n", file_to_send, receiver_ip, receiver_port);
        if (sender(receiver_ip, receiver_port, sender_port, window_size, file_to_send, log_file) == -1) {
            return 1;
        }
    }
    else if (strcmp(argv[1], "-r") == 0) {
        int receiver_port = atoi(argv[2]);
        int window_size = atoi(argv[3]);
        const char *recv_dir = argv[4];
        const char *log_file = argv[5];

        if (receiver(receiver_port, window_size, recv_dir, log_file) == -1) {
            return 1;
        }
    }
    return 0;
}
