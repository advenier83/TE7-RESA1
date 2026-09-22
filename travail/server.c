#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

void echo_server(struct pollfd fds[], int i) {
	char buff[MSG_LEN];
	// Cleaning memory
	memset(buff, 0, MSG_LEN);
	// Receiving message
    if(recv(fds[i].fd, buff, MSG_LEN, 0) <=0 ){
        close(fds[i].fd);
        fds[i].fd = -1;
        fds[i].revents = 0;
        fds[i].events = 0;
    }

    else{
        printf("Received: %s", buff);
        // Sending message (ECHO)
        send(fds[i].fd, buff, strlen(buff), 0);
        printf("Message sent!\n");
    }
}

void accept_client(int j, struct pollfd fds[], int sfd, int nbfds){
    struct sockaddr_storage cli;
    socklen_t len = sizeof(cli);
    int new_client = accept(sfd, (struct sockaddr*) &cli, &len);
    fds[j].fd= new_client;
    fds[j].events= POLLIN;
    fds[j].revents= 0;
    printf("Nombre de fd connecté : %i\n",nbfds);
}

int handle_bind(char* port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char* argv[]) {

	int sfd;

	sfd = handle_bind(argv[1]);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}

	struct pollfd fds[FD_TAB_SIZE];
    fds[0].fd= sfd;
    fds[0].events= POLLIN;
    fds[0].revents= 0;

    for(int i=1; i<FD_TAB_SIZE; i++){
        fds[i].fd= -1;
        fds[i].events= 0;
        fds[i].revents= 0;
    }

    while(1){
        int nbfds = poll(fds, FD_TAB_SIZE, -1);
        for (int i=0; i<FD_TAB_SIZE; i++){
            if(i==0 && (fds[i].revents == POLLIN)){
                for(int j=1; j<FD_TAB_SIZE; j++){
                    if(fds[j].fd == -1){
                        accept_client(j,fds,sfd,nbfds);
                        break;
                    }
                }
            }
            if ( i != 0 && (fds[i].revents == POLLIN)){
                echo_server(fds, i);
            }
        }
    }
	close(sfd);
	return EXIT_SUCCESS;
}

