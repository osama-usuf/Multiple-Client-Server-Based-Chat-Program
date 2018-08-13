#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *recvmg(void *sock)
{
	int their_sock = *((int *)sock);
	char msg[500];
	int len;
	while ((len = recv(their_sock, msg, 500, 0)) > 0) {
		msg[len] = '\0';
		
		//If duplicate client id, server sends shutdown cmd, terminate client
		if (strncmp(msg, "/shutdown", 9) == 0)
		{
			printf("\nDuplicate Client ID, Exiting...\n");
			exit(0);
		}
        printf("\n%s\n\n",msg);
		memset(msg, '\0', sizeof(msg));
	}
}

int main(int argc, char *argv[])
{
	struct sockaddr_in their_addr;
	int my_sock;
	int their_sock;
	int their_addr_size;
	int portno;
	pthread_t sendt, recvt;
	char msg[500];
	char username[100];
	username[0] = '*'; //for establishing uniqueness
	char res[600];
	char ip[INET_ADDRSTRLEN];
	int len;

	if (argc != 4) {
		printf("Usage: ip port client-username\n");
		exit(1);
	}

	portno = atoi(argv[2]);

	//Concatenating username with '*' character
	for (int i = 0; i < strlen(argv[3]); i++)
	{
		username[i + 1] = (argv[3][i]);
	}
	username[strlen(argv[3]) + 1] = '\0';

	my_sock = socket(AF_INET, SOCK_STREAM, 0);
	memset(their_addr.sin_zero, '\0', sizeof(their_addr.sin_zero));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(portno);

	//Initiliazing socket communication system
	if (inet_pton(AF_INET, argv[1], &their_addr.sin_addr) < 0)
	{
		perror("Client Error: IP not initialized succesfully");
		exit(1);
	}


	if (connect(my_sock, (struct sockaddr *)&their_addr, sizeof(their_addr)) < 0) {
		perror("connection not esatablished");
		exit(1);
	}
	inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
	printf("Connected to server at %s, type /cmds to view commands.\n", ip);
	pthread_create(&recvt, NULL, recvmg, &my_sock);

	//transmitting username to server
	if ( write(my_sock, username, strlen(username)) < 0 )
	{
		perror("message not sent");
		exit(1);
	}

	memset(msg, '\0', sizeof(msg));
	memset(res, '\0', sizeof(res));

	//block for sending messages to server
	while (fgets(msg, 500, stdin) > 0)
	{
		strcpy(res, msg);
		len = write(my_sock, res, strlen(res));
		if (len < 0) {
			perror("message not sent");
			exit(1);
		}
		if (strncmp(res, "/quit", 5) == 0)
		{
			close(my_sock);
			return 0;
		}
		memset(msg, '\0', sizeof(msg));
		memset(res, '\0', sizeof(res));
	}
	pthread_join(recvt, NULL);
	close(my_sock);


}
