#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define MAX_CLIENTS 100 //The max clients the server can entertain

struct client_info
{
	int sockno;
	char ip[INET_ADDRSTRLEN];
};


//GLOBAL Variables
int client_sockets[MAX_CLIENTS];//stores client sockets
char client_ids[MAX_CLIENTS][MAX_CLIENTS];//for storing names of clients
int n = 0;//stores number of active clients to the server
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//Mutex lock for pthreads


int find_client_by_id(char* name) //returns -1 if specified client not connected, index in id/sock array otherwise
{
	for (int i = 0; i < n; i++)
	{
		if (strcmp(client_ids[i], name) == 0)
		{
			return i;
		}
	}
	return -1;
}

int find_client_by_sock(int socket) //returns -1 if specified client not connected, index in id/sock array otherwise
{
	for (int i = 0; i < n; i++)
	{
		if (client_sockets[i] == socket)
		{
			return i;
		}
	}
	return -1;
}

//helper for sending and receiving messages between threads

void sendtoallorone(char *msg_complete, int curr, int self) //self=0 means will send to all,1 means will send only to passed socket at curr
{
	int i;
	pthread_mutex_lock(&mutex);
	for (i = 0; i < n; i++)
	{
		if (self == 0)
		{
			if (client_sockets[i] != curr)
			{
				if (send(client_sockets[i], msg_complete, strlen(msg_complete), 0) < 0) {
					perror("sending failure");
					continue;
				}
			}
		}
		else
		{
			if (client_sockets[i] == curr)
			{
				if (send(client_sockets[i], msg_complete, strlen(msg_complete), 0) < 0)
				{
					perror("sending failure");
					continue;
				}
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}


//helper for receiving messages, handles actions as well
void *recvmg(void *sock)
{

	struct client_info cl = *((struct client_info *)sock);
	char msg_complete[500];
	char msg_text[500];
	char broadcast_text[500];
	int len;
	int i;
	int j;
	int is_id_set = 0;


	while ((len = recv(cl.sockno, msg_complete, 500, 0)) > 0)
	{
		msg_complete[len] = '\0';
		broadcast_text[len] = '\0';

		printf("\nServer transmitting message(s)...\n");

		if (strncmp(msg_complete, "*", 1) == 0 && is_id_set == 0) //Set the id of the lcient in the array to the username sent by the client, identified by the * character
		{
			is_id_set = 1;
			strcpy(client_ids[n - 1], &(msg_complete[1]));
			int index = find_client_by_id(&(msg_complete[1]));
			if (index != n - 1) //indicates that a duplicate named client is already connected to the server
			{
				//Duplicate client id, shutdown Client
				sendtoallorone("/shutdown", cl.sockno, 1); //Tell client to shut down
				break;
			}
		}

		if (strncmp(msg_complete, "/list", 5) == 0)//sends list of connected clients to the client requesting
		{
			char temp[100];
			snprintf(temp, 100, "\nCONNECTED CLIENTS:\n");
			strcat(broadcast_text, temp);
			for (int i = 0; i < n; i++)
			{
				snprintf(temp, 100, "%d%s%s\n", i + 1, ":", client_ids[i]);
				strcat(broadcast_text, temp);
			}
			sendtoallorone(broadcast_text, cl.sockno, 1);
		}

		if (strncmp(msg_complete, "/quit", 5) == 0)//breaks the loop, closes the socket and thread exits.
		{
			printf("Dropping from server\n");
			break;
		}

		if (strncmp(msg_complete, "/cmds", 5) == 0)//shows available cmds to the requesting client
		{
			strcat(broadcast_text, "\nAVAILABLE CMDS:\n/list - Display active client list\n/msg clientid message - Send message to user at client-id\n/all message - Send message to all active clients\n/cmds - View commands\n/quit - Disconnect and exit from the server");
			sendtoallorone(broadcast_text, cl.sockno, 1);
		}

		if (strncmp(msg_complete, "/msg", 4) == 0)
		{
			//temporary strings
			char temp[100];
			char id[100];
			char message[100];


			if (strlen(msg_complete) >= 6)
			{

				//slicing received msg to id + msg
				strcpy(temp, &(msg_complete[5]));

				strcat(broadcast_text, temp);

				//splitting input to the two id and message strings
				sscanf(broadcast_text, "%s %[^\r\n]", id, message);

				if (strlen(id) == 0 || strlen(message) == 0)
				{
					snprintf(temp, 100, "\nIncorrect Arguments to /msg\n");
					strcpy(broadcast_text, temp);
					sendtoallorone(broadcast_text, cl.sockno, 1);
				}
				else//PROPER FORMATTED ID AND MESSAGE RECEIVED
				{
					int socket = find_client_by_id(id);
					if (socket == -1)//INVALID CLIENT
					{
						snprintf(temp, 100, "\nSpecified user not connected to server\n");
						strcat(broadcast_text, temp);
						sendtoallorone(broadcast_text, cl.sockno, 1);
					}
					else//VALID CLIENT + MESSAGE, SEND
					{
						socket = client_sockets[socket];

						//add sender id at start of message, then deliver
						int index = find_client_by_sock(cl.sockno);
						char tmp[500];
						strcpy(tmp, client_ids[index]);
						strcat(tmp, ": ");
						strcat(tmp, message);

						sendtoallorone(tmp, socket, 1);
					}
				}

				strcpy(message, "");
				strcpy(id, "");

			}

			else
			{
				snprintf(temp, 100, "\nIncorrect Arguments to /msg\n");
				strcat(broadcast_text, temp);
			}

		}

		if (strncmp(msg_complete, "/all", 4) == 0)//EXTRA TEST CMD, send a message to all connected clients
		{
			char tmp[500];
			if (strlen(msg_complete) >= 6)
			{
				int index = find_client_by_sock(cl.sockno);
				strcpy(tmp, client_ids[index]);
				strcat(tmp, ": ");
				strcat(tmp, &msg_complete[5]);
				sendtoallorone(tmp, cl.sockno, 0);
			}
			else
			{
				snprintf(tmp, 100, "\nInvalid arguments to /all\n");
				sendtoallorone(tmp, cl.sockno, 1);
			}
		}
		//clean buffer
		memset(msg_complete, '\0', sizeof(msg_complete));
		memset(broadcast_text, '\0', sizeof(broadcast_text));
	}

	pthread_mutex_lock(&mutex);

	//A client has disconnected, update the two arrays
	printf("%s disconnected\n", cl.ip);

	//A client has dropped, we remove its entries from the two main arrays (socket and id)
	for (i = 0; i < n; i++)
	{
		if (client_sockets[i] == cl.sockno)
		{
			j = i;
			while (j < n - 1)
			{
				client_sockets[j] = client_sockets[j + 1];
				strcpy(client_ids[j], client_ids[j + 1]);
				j++;
			}
		}
	}
	n--;
	pthread_mutex_unlock(&mutex);

}

int main(int argc, char *argv[])
{
	for (int i = 0; i < 100; i++)
		strcpy(client_ids[i], "");//clean the array


	//Initializing socket communication system
	struct sockaddr_in my_addr, their_addr;
	int my_sock;
	int their_sock;
	socklen_t their_addr_size;
	int portno;
	pthread_t sendt, recvt;
	char msg_complete[500];
	int len;
	struct client_info cl;
	char ip[INET_ADDRSTRLEN];

	if (argc != 2)
	{
		printf("Usage: ./server port\n");
		exit(1);
	}

	portno = atoi(argv[1]);
	my_sock = socket(AF_INET, SOCK_STREAM, 0);
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(portno);
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	their_addr_size = sizeof(their_addr);

	//Binding and starting to listen to the passed port
	if (bind(my_sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) != 0)
	{
		perror("binding unsuccessful");
		exit(1);
	}

	if (listen(my_sock, 5) != 0) {
		perror("listening unsuccessful");
		exit(1);
	}


	printf("\nServer up and ready to transmit...\n");

	while (1)
	{
		//If a client connects to our socket, we create a server thread for it for managing ipc and update server information
		if ((their_sock = accept(my_sock, (struct sockaddr *)&their_addr, &their_addr_size)) < 0)
		{
			perror("accept unsuccessful");
			exit(1);
		}
		pthread_mutex_lock(&mutex);
		inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
		printf("%s connected\n", ip);
		cl.sockno = their_sock;
		strcpy(cl.ip, ip);
		client_sockets[n] = their_sock;
		n++;
		pthread_create(&recvt, NULL, recvmg, &cl);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

