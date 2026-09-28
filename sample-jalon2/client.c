#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

#include "common.h"
#include "msg_struct.h"

int valider_pseudo(const char *pseudo) {
    if (pseudo[0] == '\0') {
        return 0; // Pseudo vide
    }

    for (int i = 0; pseudo[i] != '\0'; i++) {
        if (!isalnum((unsigned char)pseudo[i])) {
            return 0; // Contient un espace ou un caractère spécial
        }
    }

    return 1;
}

int echo_client(int sockfd, char* pseudo) {
	struct message msgstruct;
	char buff[MSG_LEN];
	int n;
	while (1) {
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		printf("Message: ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
		// Filling structure
		msgstruct.pld_len = strlen(buff) - sizeof(char);
		strncpy(msgstruct.nick_sender, pseudo, MAX_PSEUDO);
		msgstruct.type = NICKNAME_NEW;
		switch (msgstruct.type) {
			case NICKNAME_NEW:
				snprintf(msgstruct.infos, sizeof(msgstruct.infos), "Nouveau pseudo %s est vide avant l'attribution d'un pseudo, sinon il contient le pseudo actuel.", pseudo);
				break;

			case NICKNAME_LIST:
				strncpy(msgstruct.infos, "Chaîne vide.", sizeof(msgstruct.infos)-1);
				break;

			case NICKNAME_INFOS:
				strncpy(msgstruct.infos, "Pseudo de l'utilisateur recherché.", sizeof(msgstruct.infos)-1);
				break;

			case ECHO_SEND:
				strncpy(msgstruct.infos, "Chaîne vide.", sizeof(msgstruct.infos)-1);
				break;

			case UNICAST_SEND:
				strncpy(msgstruct.infos, "Pseudo du destinataire.", sizeof(msgstruct.infos)-1);
				break;

			case BROADCAST_SEND:
				strncpy(msgstruct.infos, "Chaîne vide.", sizeof(msgstruct.infos)-1);
				break;

			default:
				printf("Il y a un problème");
				return EXIT_FAILURE;

		}
		// Sending structure
		if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			break;
		}
		// Sending message (ECHO)
		if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Receiving structure
		if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
			break;
		}
		// Receiving message
		if (recv(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
			break;
		}
		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		printf("Received: %s\n", buff);
	}
	return EXIT_SUCCESS;
}

int handle_connect() {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(SERV_ADDR, SERV_PORT, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char* argv[]) {
	if (argc != 2){
		printf("veuillez rentrer votre pseudo en argument!\n");
		return EXIT_FAILURE;
	}
    if (valider_pseudo(argv[1])==0) {
		printf("Veuillez rentrer seulement des lettres et des chiffres pour votre pseudo\n");
		return 0;
	}
	int sfd;
	sfd = handle_connect();
	echo_client(sfd, argv[1]);
	close(sfd);
	return EXIT_SUCCESS;
}

