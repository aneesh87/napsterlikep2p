#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char ** argv) {
	int sock;
	int port;
	int client_fd;
	struct sockaddr_in server_addr, client_addr;
	char buffer[2048];
	int client_len = sizeof(client_addr);

	int rc = 0;

	if (argc < 2) {
		fprintf(stderr, "./server <port no>");
		exit(1);
	}
	port = atoi(argv[1]);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		fprintf(stderr, "socket() failed: %s", strerror(errno));
		exit(1);
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	rc = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

	if (rc != 0) {
		fprintf(stderr, "Not able to bind to socket: %s\n",
			    strerror(errno));
		exit(1);
	}
	listen(sock, 10);
	client_fd = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *)&client_len);
	if (client_fd < 0)  {
		fprintf(stderr, "accept failed: %s\n", strerror(errno));
	}
	memset(buffer, 0, 2048);
	int len = read(client_fd, buffer, 2048);
	fprintf(stdout, "Client Mesg: %s\n", buffer);
	len = write(client_fd, "Got your message", 16);
	close(client_fd);
	close(sock);
	return 0;
}
