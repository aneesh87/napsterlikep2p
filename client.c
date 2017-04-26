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
#include <sys/stat.h>
#include <pthread.h>

struct rfclist * head;
int server_sock;

pthread_mutex_t rlock;

int check_if_rfc_exists(int rfcnum) {

  LOCK(rlock);
  struct rfclist * temp = head;
  while (temp) {
    if (temp->rfcnum == rfcnum) {
      UNLOCK(rlock);
      return true;
    }
    temp = temp->next;
  }
  UNLOCK(rlock);
  return false;
}

void * process_thread(void * obj) {

  int client_fd = *((int *)obj);
  char buffer[MAX_BUFFER_SIZE];
  char * saveptr = NULL; 
  memset(buffer, 0, MAX_BUFFER_SIZE);
  int len = read(client_fd, buffer, MAX_BUFFER_SIZE);
  fprintf(stdout, "Client Mesg: %s\n", buffer);
  const char *token = strtok_r(buffer, " \n", &saveptr);

  if (token == NULL) {
      close(client_fd);
      return(NULL);
  }
  char tmpbuf[MAX_BUFFER_SIZE] = "";
  if (strcmp(token,"GET") == 0) {
      token = strtok_r(NULL, " \n", &saveptr);
      token = strtok_r(NULL, " \n", &saveptr);
      int rfcnum = atoi(token);

      LOCK(rlock);
      struct rfclist *temp = head;
      while (temp != NULL) {

          if (temp->rfcnum == rfcnum) {
              char rfcname[MAX_NAME_LEN] ="";
              snprintf(rfcname, MAX_NAME_LEN, "rfc%d", rfcnum);
                  
              FILE * fp = fopen(rfcname, "r");
              if (fp == NULL) {
                  fprintf(stderr, "File not found\n");
                  temp = NULL;
                  break;
              }

              struct stat sb;
              stat(rfcname, &sb);
              snprintf(tmpbuf, MAX_BUFFER_SIZE, "P2P-CI/1.0 200 OK\nContent-Length %lld\nTitle %s\nContent-Type: text/text\n", 
                           (long long) sb.st_size, temp->title);
              fprintf(stdout, "Sending RFC to peer\n");
              int len = write(client_fd, tmpbuf, strlen(tmpbuf) + 1);
              if (len < 0) {
                  fprintf(stderr, "write error: %s\n", strerror(errno));
              }
              int i = 0;
              char ch;
              memset(tmpbuf, 0, MAX_BUFFER_SIZE);

                  /* Unlock for this long loop */

              UNLOCK(rlock);

              while ((ch=fgetc(fp))!=EOF) {
                    if(i==1024) {
                       sleep(1);
                       write(client_fd, (void*) tmpbuf, strlen(tmpbuf)+1);
                       memset(tmpbuf, 0, MAX_BUFFER_SIZE);
                       i=0;
                    }
                    tmpbuf[i]=ch;
                    i++;
              }
              LOCK(rlock);
              //fprintf(stderr,"buffer has %s\n",tmpbuf);
              write(client_fd, (void*) tmpbuf, strlen(tmpbuf)+1);
              fclose(fp);
              break;
          }
          temp = temp->next;
      }
      UNLOCK(rlock);

      if (temp == NULL) {
          snprintf(tmpbuf, MAX_BUFFER_SIZE, "P2P-CI/1.0 404 Not Found");
          int len = write(client_fd, tmpbuf, strlen(tmpbuf) + 1);
          if (len < 0) {
              fprintf(stderr, "write error: %s\n", strerror(errno));
          }
      }

  } else {
      snprintf(tmpbuf, MAX_BUFFER_SIZE, "P2P-CI/1.0 400 Bad Request");
      int len = write(client_fd, tmpbuf, strlen(tmpbuf) + 1);
      if (len < 0) {
          fprintf(stderr, "write error: %s\n", strerror(errno));
      }
  }
  close(client_fd);
  return(NULL);

}
void * process_client(void *myport) {

  int client_fd;
  struct sockaddr_in client_addr;
  listen(server_sock, 10);
  int client_len;
  pthread_t peer_thread;

  while (1) {

     client_fd = accept(server_sock, (struct sockaddr *) &client_addr, (socklen_t *)&client_len);
     if (client_fd < 0)  {
         fprintf(stderr, "accept failed: %s\n", strerror(errno));
         continue;
     }
     
     if (pthread_create(&peer_thread, NULL, &process_thread, &client_fd) != 0) {
         fprintf(stderr, "Unable to start a thread\n"); 
     }  
  }
}

int main(int argc, char ** argv) {
    int sock;
    int port;
        
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE];
    char hostname[MAX_NAME_LEN];

    int myport, numrfc;
    fprintf(stdout, "Enter port number to listen for peer connections:");
    scanf("%d", &myport);

    {   
        // start server thread
        pthread_t stid;
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0) {
            fprintf(stderr, "socket() failed: %s", strerror(errno));
            exit(1);
        }
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(myport);

        int rc = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

        if (rc != 0) {
            fprintf(stderr, "Not able to bind to socket: %s\n", strerror(errno));
            exit(1);
        }

        if ((pthread_create(&stid, NULL, &process_client, (void *) &myport))!=0) {
            fprintf(stderr, "Server thread create failed: %s\n", strerror(errno));
            exit(1);
        }
    }
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
        (void)insertrfc(&head, tmp);
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
                    LOCK(rlock);
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
                    UNLOCK(rlock);
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


                if (check_if_rfc_exists(rfcnum) == true) {
                  fprintf(stderr, "RFC already in local database\n");
                  continue;
                }
                
                char * saveptr = NULL;
                char * token = strtok_r(buffer, " \n", &saveptr);
                
                if (token == NULL) {
                    fprintf(stderr, "Unexpected error\n");
                    continue;
                }

                token = strtok_r(NULL, " \n", &saveptr);
                int status = atoi(token);
                if (status!= 200) {
                    continue;
                }
                // Take the first host
                token = strtok_r(NULL, " \n", &saveptr); //OK
                token = strtok_r(NULL, " \n", &saveptr); //RFC
                token = strtok_r(NULL, " \n", &saveptr); //Number
                token = strtok_r(NULL, " \n", &saveptr); //Hostname
                token = strtok_r(NULL, " \n", &saveptr); //hostname
                char peerhostname[MAX_NAME_LEN];
                strncpy(peerhostname, token, MAX_NAME_LEN);
                token = strtok_r(NULL, " \n", &saveptr); //port
                token = strtok_r(NULL, " \n", &saveptr);
                int cport = atoi(token);

                int peersock = socket(AF_INET, SOCK_STREAM, 0);
                if (peersock < 0) {
                    fprintf(stderr, "socket() error: %s", strerror(errno));
                    continue;
                }
                struct hostent *peer;
                struct sockaddr_in peer_addr;

                memset(&peer_addr, 0, sizeof(server_addr));
                peer_addr.sin_family = AF_INET;
                peer_addr.sin_port = htons(cport);

                peer=gethostbyname(peerhostname);

                if (peer == NULL) {
                  fprintf(stderr, "gethostbyname error: %s", strerror(errno));
                  continue;
                }
                bcopy((char *)peer->h_addr,(char *)&peer_addr.sin_addr.s_addr,peer->h_length);

                rc = connect(peersock, (struct sockaddr *) &peer_addr, sizeof(peer_addr));
                if (rc < 0)  {
                    fprintf(stderr, "connect failed: %s\n", strerror(errno));
                    continue;
                }
                memset(buffer, 0, MAX_BUFFER_SIZE);
                snprintf(buffer, MAX_BUFFER_SIZE,  "GET RFC %d\n", rfcnum);
                len = write(peersock, buffer, MAX_BUFFER_SIZE);
                if (len < 0) {
                    fprintf(stderr, "write error: %s\n", strerror(errno));
                }
                memset(buffer, 0, MAX_BUFFER_SIZE);
                len = read(peersock,  buffer, MAX_BUFFER_SIZE);
                if (len <= 0){
                  fprintf(stderr, "Peer closed connection\n");
                  continue;
                }
                fprintf(stderr, "Peer Replied: %s\n", buffer);


                // check for status code from peer

                char * c = buffer;
                while (*c !=' ') c++;
                c++;
                int status_code = atoi(c);

                if (status_code != 200) {
                    fprintf(stderr, "Peer error: %s\n",c);
                    continue;
                }

                char rfc_title[MAX_NAME_LEN];
                char * t = strstr(buffer, "Title");
                if (t!= NULL) {
                  while (*t != ' ') t++;
                  t++;
                  strncpy(rfc_title, t, MAX_NAME_LEN);
                } else {
                  fprintf(stderr, "Unexpected error\n");
                  continue;
                }

                char rfcfile[MAX_NAME_LEN] = "";
                snprintf(rfcfile, MAX_NAME_LEN, "rfc%d", rfcnum);
                FILE * fp = fopen(rfcfile, "w");
                if (fp == NULL) {
                  fprintf(stderr, "Unable to open file %s\n", rfcfile);
                  continue;
                }
                fprintf(stdout, "Downloading RFC from peer\n");
                memset(buffer, 0, MAX_BUFFER_SIZE);
                while (read(peersock,  buffer, 1024 + 1) > 0) {
                       if (len < 0) {
                           fprintf(stderr, "read error: %s\n", strerror(errno));
                       }
                       fprintf(fp,"%s", buffer);
                       memset(buffer, 0, MAX_BUFFER_SIZE);
                }
                fclose(fp);
                LOCK(rlock);
                
                struct rfclist *rfctemp = (struct rfclist *)calloc(1, sizeof(struct rfclist));
                rfctemp->rfcnum = rfcnum;
                strncpy(rfctemp->title, rfc_title, MAX_TITLE);
                (void)insertrfc(&head, rfctemp);
                
                UNLOCK(rlock);

                memset(buffer, 0, MAX_BUFFER_SIZE);
                snprintf(buffer, MAX_BUFFER_SIZE, 
                         "ADD RFC %d P2P-CI/1.0\nHost: %s\nPort: %d\nTitle: %s\n", 
                          rfcnum,hostname,myport,rfc_title);
                len = write(sock, buffer, MAX_BUFFER_SIZE);
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

            } break;

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
