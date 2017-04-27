#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"

struct peer {
	unsigned int ipaddr;
	int client_fd;
	unsigned int port;
	char peer_name[MAX_NAME_LEN];
	int active;
	struct rfclist * rfchead;
	struct peer * next;
};

struct dummy_obj {
	struct sockaddr_in cliaddr;
	int client_fd;
};

pthread_mutex_t dblock;

struct peer * head;

void * process_cmd(void * obj) {

    int ch;
    while (1) {
        fprintf(stdout, "\n\n Menu\n");
        fprintf(stdout, "1. Active Registered Peers List\n");
        fprintf(stdout, "2. Inactive Peers List\n");
        fprintf(stdout, "3. Exit\n");
        fprintf(stdout, "Enter choice: ");
        scanf("%d",&ch);
        switch(ch) {
            case 1: {
            	LOCK(dblock);
	    	    struct peer * temp = head;
	    	    int i = 1;
	            while (temp!= NULL) {
	            	if (temp->active) {
	        	        fprintf(stdout, "%d: Hostname %s port %d\n", i, temp->peer_name, temp->port);
	        	        i++;
	        	    }
	        	    temp = temp->next;
	            }
	            UNLOCK(dblock);
            } break;
            
            case 2: {
            	LOCK(dblock);
	    	    struct peer * temp = head;
	    	    int i = 1;
	            while (temp!= NULL) {
	            	if (!temp->active) {
	        	        fprintf(stdout, "%d: Hostname %s port %d\n", i, temp->peer_name, temp->port);
	        	        i++;
	        	    }
	        	    temp = temp->next;
	            }
	            UNLOCK(dblock);

            } break;

            case 3: {
                exit(0);
            } break;

            default: 
                  fprintf(stderr, "Invalid choice\n");
        }  
    }
}

void free_rfclist (struct rfclist * rfcdb) {
	struct rfclist * tmp = rfcdb;
	while (tmp != NULL) {
		struct rfclist * element = tmp;
		tmp = tmp->next;
		free(element);
	}
}

void db_remove_client(int client_fd) {

   	LOCK(dblock);
	    	
	struct peer * temp = head;
	while (temp != NULL) {
	   if (client_fd == temp->client_fd && temp->active == true) {
	       temp->active = false;
	       free(temp->rfchead);
	       temp->rfchead = NULL;
	   }
	   temp = temp->next;
	}
	UNLOCK(dblock);	
}

void * process_thread(void *obj) {
    
    int client_fd = ((struct dummy_obj *)obj)->client_fd;
    unsigned int ip_addr = ((struct dummy_obj *)obj)->cliaddr.sin_addr.s_addr;

	while (1) {
	    char buffer[MAX_BUFFER_SIZE];
        char * saveptr = NULL;
	    
	    memset(buffer, 0, MAX_BUFFER_SIZE);
	    int len = read(client_fd, buffer, MAX_BUFFER_SIZE);

	    if (len <= 0) {
	    	db_remove_client(client_fd);
	    	close(client_fd);
	    	return (NULL);
	    }

	    if (buffer[0] != '\0') {
	        fprintf(stdout, "Client Mesg: %s\n", buffer);
	    }

	    const char *token = strtok_r(buffer, " \n", &saveptr);
        if (token == NULL) {
            continue;
        }
	    if (strcmp(token, "EXIT") == 0 ) {
	    
	    	db_remove_client(client_fd);
	    	char eok[MAX_BUFFER_SIZE];
	    	strncpy(eok, "P2P-CI/1.0 200 OK\n", MAX_BUFFER_SIZE);
	    	len = write(client_fd, eok, strlen(eok)+1);
	    	if (len < 0) {
	        	fprintf(stderr, "write error: %s\n", strerror(errno));
	        }
	    	close(client_fd);
	        return NULL;

	    } else if (strcmp(token, "ADD") == 0) {


	    	struct peer * temp = (struct peer *)calloc(1,sizeof(struct peer));
	        temp->active = true;
	    	temp->client_fd = client_fd;
	    	temp->ipaddr = ip_addr;

	    	token = strtok_r(NULL, " \n", &saveptr);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	int rfcnum = atoi(token);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	strncpy(temp->peer_name,token, MAX_NAME_LEN);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	temp->port = atoi(token);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	token = strtok_r(NULL, "\n", &saveptr); 

	    	struct rfclist * newrfc = (struct rfclist *) calloc(1, sizeof(struct rfclist));
	    	newrfc->rfcnum = rfcnum;
	    	strncpy(newrfc->title,token,MAX_TITLE);

	    	LOCK(dblock);
	    	if (head == NULL) {
	    		head = temp;
	    		head->rfchead = newrfc;
	    	} else {
	    		struct peer * iter = head;
	    		while (iter != NULL) {
	    			if (iter->port == temp->port && strcmp(iter->peer_name,temp->peer_name)==0) {
	    				iter->active = true;
	    				free(temp);
	    				int rc = insertrfc(&iter->rfchead, newrfc);
	    				if (rc < 0) {
	    					free(newrfc);
	    				}
	    				break;
	    			}
                    iter=iter->next;
	    		}
	    		if (iter == NULL) {
	    		    temp->next = head;
	    		    head = temp;
	    		    (void)insertrfc(&head->rfchead, newrfc);
	    		}
	    	}
	    	UNLOCK(dblock);

	    	char replybuffer[MAX_BUFFER_SIZE] = "";
	    	snprintf(replybuffer, MAX_BUFFER_SIZE, "P2P-CI/1.0 200 OK\nRFC number %d\n", rfcnum);

	    	len = write(client_fd, replybuffer, strlen(replybuffer) +1);
	    	if (len < 0) {
	        	fprintf(stderr, "write error: %s\n", strerror(errno));
	        }

        } else if (strcmp(token, "LIST") == 0) {
        	LOCK(dblock);
	    	
	    	struct peer * temp = head;
	    	char sendbuffer[LARGE_BUFFER_SIZE] = "";
	        char tmpbuf[MAX_BUFFER_SIZE] = "";

	        snprintf(sendbuffer, LARGE_BUFFER_SIZE, "P2P-CI/1.0 200 OK\n");
	        
	        while (temp!= NULL) {
	        	struct rfclist * iter = temp->rfchead;
	        	while (iter != NULL) {
	        	       snprintf(tmpbuf, MAX_BUFFER_SIZE, "RFC number %d hostname %s port %d title %s\n", 
	        		            iter->rfcnum, temp->peer_name, temp->port, iter->title);
	        	       strncat(sendbuffer, tmpbuf, LARGE_BUFFER_SIZE - strlen(sendbuffer));
	        	       //fprintf(stderr, "%s\n", sendbuffer);
	        	       iter= iter->next;
	        	}
	        	temp = temp->next;
	        }
	        UNLOCK(dblock);
	        
	        len = write(client_fd, sendbuffer, strlen(sendbuffer) + 1);

	        if (len < 0) {
	        	fprintf(stderr, "write error: %s\n", strerror(errno));
	        }
	    
	    } else if(strcmp(token, "LOOKUP") == 0) {
	    	
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	token = strtok_r(NULL, " \n", &saveptr);
	    	
	    	int rfcnum = atoi(token);

	    	LOCK(dblock);
	    	
	    	struct peer * temp = head;
	    	char sendbuffer[LARGE_BUFFER_SIZE] = "";
	        char tmpbuf[MAX_BUFFER_SIZE] = "";
	        
	        while (temp!= NULL) {
	        	struct rfclist * iter = temp->rfchead;
	        	while (iter != NULL) {
	        	       if (iter->rfcnum == rfcnum) {
	        	       	   if (sendbuffer[0] == '\0') {
	        	       	       snprintf(sendbuffer, LARGE_BUFFER_SIZE, "P2P-CI/1.0 200 OK\n");
	        	       	   }
	        	           snprintf(tmpbuf, MAX_BUFFER_SIZE, "RFC %d hostname %s port %d title %s\n", 
	        		                rfcnum, temp->peer_name, temp->port, iter->title);
	        	           strncat(sendbuffer, tmpbuf, LARGE_BUFFER_SIZE - strlen(sendbuffer));
	        	           //fprintf(stderr, "%s\n", sendbuffer);
	        	       }
	        	       iter = iter->next;
	        	}
	        	temp = temp->next;
	        }

	        UNLOCK(dblock);
	        if (sendbuffer[0] == '\0') {
	        	snprintf(sendbuffer, LARGE_BUFFER_SIZE, "P2P-CI/1.0 404 Not Found\n");
	        }
	        len = write(client_fd, sendbuffer, strlen(sendbuffer) + 1);
	        if (len < 0) {
	        	fprintf(stderr, "write error: %s\n", strerror(errno));
	        }
	    } 
	}
}

int main(int argc, char ** argv) {

	int sock;
	int port;
	int client_fd;
	struct sockaddr_in server_addr, client_addr;
	int client_len = sizeof(client_addr);
        struct dummy_obj x;

	int rc = 0;

	if (argc < 2) {
		fprintf(stderr, "./server <port no>");
		exit(1);
	}
    
    if (pthread_mutex_init(&dblock, NULL) != 0) {
        fprintf(stderr, "mutex init failed: %s", strerror(errno));
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

	pthread_t command_thread;

	if (pthread_create(&command_thread, NULL, &process_cmd, NULL) != 0) {
        fprintf(stderr, "Unable to start command thread\n");
    }  

	listen(sock, 10);
	while (1) {
		client_fd = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *)&client_len);
		if (client_fd < 0)  {
			fprintf(stderr, "accept failed: %s\n", strerror(errno));
			continue;
		}
	    x.cliaddr = client_addr;
	    x.client_fd = client_fd;
	    
		pthread_t peer_thread;
        if (pthread_create(&peer_thread, NULL, &process_thread, &x) != 0) {
            fprintf(stderr, "Unable to start peer thread\n"); 
        }  
	}
	pthread_mutex_destroy(&dblock);
	return 0;
}
