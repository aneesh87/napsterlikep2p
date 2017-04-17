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


int main(int argc, char ** argv) {
    int sock;
    int port;
        
    struct sockaddr_in server_addr;
    char buffer[2048];
    int rc = 0;

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
    memset(buffer, 0, 2048);
    fgets(buffer, 2048, stdin);
    int len = write(sock, buffer, 2048);
    if (len < 0) {
        fprintf(stderr, "write error: %s\n", strerror(errno));
    }
    memset(buffer, 0, 2048);
    len = read(sock,  buffer, 2048);
    if (len < 0) {
        fprintf(stderr, "read error: %s\n", strerror(errno));
    }
    printf("Server Replied: %s\n", buffer);
    close(sock);
    return 0;
}
