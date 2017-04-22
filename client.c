#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"


struct rfclist * head;

int main(int argc, char ** argv) {
    int sock;
    int port;
        
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE];
    char hostname[MAX_NAME_LEN];

    int myport, numrfc;
    fprintf(stdout, "Enter port number to listen for peer connections:");
    scanf("%d", &myport);
    fprintf(stdout, "Enter the number of RFC this host has:");
    scanf("%d", &numrfc);
    int i;
    for(i = 1; i<=numrfc; i++) {
        struct rfclist * tmp = (struct rfclist*)calloc(1, sizeof(struct rfclist));
        fprintf(stdout, "Enter RFC number:");
        scanf("%d",&tmp->rfcnum);
        fprintf(stdout, "Enter RFC title:");
        scanf("%*[^\n]");
        scanf("%*c");
        fgets(tmp->title, MAX_NAME_LEN, stdin);
        if (head == NULL) {
            head = tmp;
        } else {
            tmp->next = head;
            head = tmp;
        }
    }
    
    int rc = 0;

    gethostname(hostname, sizeof(hostname));

    if (argc < 3) {
        fprintf(stderr, "./client <server_ip> <port_no>");
        exit(1);
    }
    port = atoi(argv[2]);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "socket() error: %s", strerror(errno));
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <=0) {
        fprintf(stderr, "inet_pton() error: %s\n", strerror(errno));
        exit(1);
    }

    rc = connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rc < 0)  {
        fprintf(stderr, "connect failed: %s\n", strerror(errno));
        exit(1);
    }
    int ch;
    while (1) {
        fprintf(stdout, "\n\n Menu\n");
        fprintf(stdout, "1. Add RFC's\n");
        fprintf(stdout, "2. Lookup\n");
        fprintf(stdout, "3. List\n");
        fprintf(stdout, "4. Exit\n");
        fprintf(stdout, "Enter choice: ");
        scanf("%d",&ch);
        switch(ch) {
            case 1: {
                    struct rfclist * temp = head;
                    for (i=1; i<=numrfc; i++) {
                         memset(buffer, 0, MAX_BUFFER_SIZE);
                         snprintf(buffer, MAX_BUFFER_SIZE, 
                                  "ADD RFC %d P2P-CI/1.0\nHost: %s\nPort: %d\nTitle: %s\n", 
                                  temp->rfcnum,hostname,myport,temp->title);
                         int len = write(sock, buffer, MAX_BUFFER_SIZE);
                         if (len < 0) {
                             fprintf(stderr, "write error: %s\n", strerror(errno));
                         }
                         memset(buffer, 0, MAX_BUFFER_SIZE);
                         len = read(sock,  buffer, MAX_BUFFER_SIZE);
                         if (len < 0) {
                             fprintf(stderr, "read error: %s\n", strerror(errno));
                         }
                         printf("Server Replied: %s\n", buffer);
                         temp = temp->next;
                    }
            }
            break;

            case 2: {
                memset(buffer, 0, MAX_BUFFER_SIZE);
                int rfcnum;
                fprintf(stdout, "Enter number of the RFC you wish to search");
                scanf("%d",&rfcnum);
                snprintf(buffer, MAX_BUFFER_SIZE,"LOOKUP RFC %d P2P-CI/1.0\nHost: %s\nPort: %d\n", 
                          rfcnum,hostname,myport);
                int len = write(sock, buffer, MAX_BUFFER_SIZE);
                if (len < 0) {
                    fprintf(stderr, "write error: %s\n", strerror(errno));
                }
                memset(buffer, 0, MAX_BUFFER_SIZE);
                len = read(sock,  buffer, MAX_BUFFER_SIZE);
                if (len < 0) {
                    fprintf(stderr, "read error: %s\n", strerror(errno));
                }
                printf("Server Replied: %s\n", buffer);

            }
            break;

            case 3: {

                        memset(buffer, 0, MAX_BUFFER_SIZE);
                        snprintf(buffer, MAX_BUFFER_SIZE, 
                                 "LIST ALL P2P-CI/1.0\nHost: %s\nPort: %d\n", 
                                 hostname,myport);
                         int len = write(sock, buffer, MAX_BUFFER_SIZE);
                         if (len < 0) {
                             fprintf(stderr, "write error: %s\n", strerror(errno));
                         }
                         char recvbuffer[LARGE_BUFFER_SIZE];
                         len = read(sock,  recvbuffer, LARGE_BUFFER_SIZE);
                         if (len < 0) {
                             fprintf(stderr, "read error: %s\n", strerror(errno));
                         }
                         printf("Server Replied: %s\n",recvbuffer);

            }break;

            case 4:{
                strcpy(buffer, "EXIT");
                int len = write(sock, buffer, MAX_BUFFER_SIZE);
                if (len < 0) {
                    fprintf(stderr, "write error: %s\n", strerror(errno));
                }
                close(sock);
                exit(0);
            }
            default: 
                fprintf(stderr, "Invalid choice\n");

        }

    }
    return 0;
}
